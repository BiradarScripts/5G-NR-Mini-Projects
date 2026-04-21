#include "header.h"

// ---------------------------------------------------------------------------
// Helper: compute smallest T such that T*(T+1)/2 >= E
// Fully unrolled over 32 comparisons — negligible LUT cost.
// ---------------------------------------------------------------------------
static int compute_triangle_side(const int E) {
#pragma HLS INLINE
    static const int tri_num[CI_MAX_T] = {
          1,   3,   6,  10,  15,  21,  28,  36,
         45,  55,  66,  78,  91, 105, 120, 136,
        153, 171, 190, 210, 231, 253, 276, 300,
        325, 351, 378, 406, 435, 465, 496, 528
    };
    int T = 1;
    for (int t = 0; t < CI_MAX_T; ++t) {
#pragma HLS UNROLL
        if (E > tri_num[t]) T = t + 2;
    }
    if (T > CI_MAX_T) T = CI_MAX_T;
    return T;
}

// ---------------------------------------------------------------------------
// Top-level function
// ---------------------------------------------------------------------------
void channel_interleaver(
    hls::stream<axis_word_t> &inData,
    hls::stream<axis_word_t> &outData,
    ap_uint<64>               cnData
) {
#pragma HLS INTERFACE axis      port=inData
#pragma HLS INTERFACE axis      port=outData
#pragma HLS INTERFACE ap_none   port=cnData
#pragma HLS INTERFACE ap_ctrl_hs port=return

    // -----------------------------------------------------------------------
    // Decode configuration
    // -----------------------------------------------------------------------
    const int E_cfg   = (int)cnData.range(11, 0);
    const int E       = (E_cfg < 0) ? 0 : ((E_cfg > CI_MAX_E) ? CI_MAX_E : E_cfg);
    const int n_words = (E + CI_AXIS_DW - 1) / CI_AXIS_DW;
    const int T       = (E == 0) ? 1 : compute_triangle_side(E);

    // -----------------------------------------------------------------------
    // AXI-word receive buffer (4 x 128-bit = 512 bits)
    // Only CI_MAX_CHUNKS = 4 elements — complete partition is cheap (4 regs).
    // -----------------------------------------------------------------------
    ap_uint<CI_AXIS_DW> in_buf[CI_MAX_CHUNKS];
#pragma HLS ARRAY_PARTITION variable=in_buf complete dim=1

    // -----------------------------------------------------------------------
    // Individual 1-bit input/output arrays, fully partitioned.
    //   - in_bits[512]  : each element is one separate FF → no mux on WRITE
    //   - out_bits[512] : each element is one separate FF → no mux on WRITE
    // All unpack/pack loops use STATIC indices (w*128+b), so HLS uses direct
    // wire connections instead of mux trees — eliminating the LUT explosion.
    // -----------------------------------------------------------------------
    ap_uint<1> in_bits[CI_MAX_E];
    ap_uint<1> out_bits[CI_MAX_E];
#pragma HLS ARRAY_PARTITION variable=in_bits  complete dim=1
#pragma HLS ARRAY_PARTITION variable=out_bits complete dim=1

    // Triangular storage: each ap_uint<32> holds one column of 32 row-bits.
    ap_uint<CI_MAX_T> tri_data[CI_MAX_T];
    ap_uint<CI_MAX_T> tri_valid[CI_MAX_T];
#pragma HLS ARRAY_PARTITION variable=tri_data  complete dim=1
#pragma HLS ARRAY_PARTITION variable=tri_valid complete dim=1

    // Zero-initialise output bits (required for correct partial-word output)
    INIT_OUT:
    for (int k = 0; k < CI_MAX_E; ++k) {
#pragma HLS UNROLL
        out_bits[k] = 0;
    }

    // =======================================================================
    // Stage 1  READ_WORDS
    // Read n_words AXI packets; store each 128-bit word directly in in_buf.
    // Pipelining the word-level loop at II=1 — no inner bit loop needed.
    // =======================================================================
    READ_WORDS:
    for (int w = 0; w < CI_MAX_WORDS; ++w) {
#pragma HLS PIPELINE II=1
        if (w < n_words) {
            axis_word_t pkt = inData.read();
            in_buf[w] = pkt.data;
        } else {
            in_buf[w] = 0;
        }
    }

    // =======================================================================
    // Stage 1b  UNPACK
    // Unpack 4 x 128-bit chunks into 512 individual 1-bit registers.
    // STATIC indices: w and b are both loop variables; b is unrolled, so
    // every unrolled copy accesses in_buf[w][b_const] — a direct wire, no mux.
    // =======================================================================
    UNPACK_INPUT:
    for (int w = 0; w < CI_MAX_CHUNKS; ++w) {
#pragma HLS PIPELINE II=1
        for (int b = 0; b < CI_AXIS_DW; ++b) {
#pragma HLS UNROLL
            const int idx = w * CI_AXIS_DW + b;
            // idx is fully determined at unroll time → static wire connection
            in_bits[idx] = in_buf[w][b];
        }
    }

    // =======================================================================
    // Stage 2  WRITE_TRIANGLE  (3GPP row-major write)
    // Pipeline the INNER (col) loop at II=1.
    // The outer (row) loop is NOT pipelined — HLS will NOT unroll the inner
    // loop, avoiding the 32-way parallel mux chain that caused 154ns timing.
    // src_k increments by 0 or 1 each inner cycle — loop-carried but 1-cycle.
    // in_bits[src_k] with dynamic src_k creates a 512:1 MUX, but this is a
    // single-reader balanced tree (~9 LUT levels), not a 32-deep chain.
    // =======================================================================
    {
        int src_k = 0;
        WRITE_TRIANGLE_ROW:
        for (int i = 0; i < T; ++i) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=32
            WRITE_TRIANGLE_COL:
            for (int j = 0; j < (T - i); ++j) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=32
                const bool is_valid = (src_k < E);
                tri_valid[j][i] = is_valid ? ap_uint<1>(1) : ap_uint<1>(0);
                tri_data[j][i]  = is_valid ? in_bits[src_k] : ap_uint<1>(0);
                if (is_valid) ++src_k;
            }
        }
    }

    // =======================================================================
    // Stage 3  READ_TRIANGLE  (3GPP column-major read)
    // Pipeline the INNER (row) loop at II=1.
    // out_bits[dst_k] = val  with dynamic dst_k: each of the 512 out_bits
    // registers has its own enable line (one-hot decoder ~54 LUTs), not a
    // 32-deep chained update. No read-modify-write on a wide register.
    // =======================================================================
    {
        int dst_k = 0;
        READ_TRIANGLE_COL:
        for (int j = 0; j < T; ++j) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=32
            READ_TRIANGLE_ROW:
            for (int i = 0; i < (T - j); ++i) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=32
                if (tri_valid[j][i]) {
                    out_bits[dst_k] = tri_data[j][i];
                    ++dst_k;
                }
            }
        }
    }

    // =======================================================================
    // Stage 4  PACK_WORDS + WRITE_WORDS
    // Repack out_bits into 128-bit output packets.
    // STATIC INDICES: b is unrolled → pkt.data[b] = out_bits[w*128+b] is a
    // compile-time constant subscript — direct wire, zero mux cost.
    // Entire stage is a simple 4-iteration loop at II=1, Depth=1.
    // =======================================================================
    WRITE_WORDS:
    for (int w = 0; w < CI_MAX_WORDS; ++w) {
#pragma HLS PIPELINE II=1
        if (w < n_words) {
            axis_word_t pkt;
            pkt.data = 0;
            pkt.keep = (ap_uint<CI_AXIS_DW / 8>)(-1);
            pkt.strb = (ap_uint<CI_AXIS_DW / 8>)(-1);
            pkt.last = (w == (n_words - 1)) ? ap_uint<1>(1) : ap_uint<1>(0);

            for (int b = 0; b < CI_AXIS_DW; ++b) {
#pragma HLS UNROLL
                // w and b are both statically known at unroll time:
                // HLS maps this to a direct wire from out_bits[static_idx].
                const int idx = w * CI_AXIS_DW + b;
                if (idx < CI_MAX_E) pkt.data[b] = out_bits[idx];
            }
            outData.write(pkt);
        }
    }
}

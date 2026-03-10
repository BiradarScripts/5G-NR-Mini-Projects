#include "header.h"

static const int MAX_E = 16384; // > max testcase E

void Interleaver(
    hls::stream<axis96_t> &inData,
    hls::stream<axis96_t> &outData,
    ap_uint<128> cnData
) {
#pragma HLS INTERFACE axis port=inData
#pragma HLS INTERFACE axis port=outData
#pragma HLS INTERFACE s_axilite port=cnData
#pragma HLS INTERFACE s_axilite port=return

    ap_uint<15> E  = cnData.range(14, 0);
    ap_uint<4>  Qm = cnData.range(18, 15);

    if (E == 0) return;
    if (!(Qm == 2 || Qm == 4 || Qm == 6 || Qm == 8)) return;

    const int e_int  = (int)E;
    const int qm_int = (int)Qm;
    const int cols   = e_int / qm_int; // E/Qm

    ap_uint<1> e_mem[MAX_E];
#pragma HLS BIND_STORAGE variable=e_mem type=ram_1p impl=bram

    // -----------------------------
    // Read input words until we collect E bits (TLAST-safe).
    // IMPORTANT: MSB-first packing: first sequence bit is at data[95].
    // -----------------------------
    int idx = 0;
read_words:
    while (idx < e_int) {
#pragma HLS PIPELINE II=1
        axis96_t in = inData.read();
        ap_uint<96> din = in.data;

        // Unpack 96 bits MSB-first: b=0 -> bit95, b=95 -> bit0
        for (int b = 0; b < 96; b++) {
#pragma HLS UNROLL
            if (idx < e_int) {
                int bitpos = 95 - b;
                e_mem[idx] = din[bitpos];
                idx++;
            }
        }

        // If TLAST early, still okay because E tells exact size,
        // but we break to avoid reading extra.
        if (in.last) break;
    }

    // If TLAST arrived before we got E bits, remaining bits assumed 0
fill_remaining:
    for (int i = idx; i < e_int; i++) {
#pragma HLS PIPELINE II=1
        e_mem[i] = 0;
    }

    // -----------------------------
    // Write output bits packed MSB-first into 96-bit words.
    // f[k] = e[(k%Qm)*(E/Qm) + floor(k/Qm)]
    // -----------------------------
    int k = 0;
write_words:
    while (k < e_int) {
#pragma HLS PIPELINE II=1
        axis96_t out;
        out.data = 0;

        // last word if after this word we emitted >= E
        out.last = ((k + 96) >= e_int) ? 1 : 0;

        for (int b = 0; b < 96; b++) {
#pragma HLS UNROLL
            int kk = k + b;
            int bitpos = 95 - b; // MSB-first output packing

            if (kk < e_int) {
                int i = kk % qm_int;
                int j = kk / qm_int;
                int src = i * cols + j;

                out.data[bitpos] = e_mem[src];
            } else {
                out.data[bitpos] = 0;
            }
        }

        outData.write(out);
        k += 96;
    }
}

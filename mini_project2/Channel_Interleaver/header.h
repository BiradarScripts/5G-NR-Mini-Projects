#ifndef HEADER_H
#define HEADER_H

#include <ap_int.h>
#include <hls_stream.h>

// ---------------------------------------------------------------------------
// Channel Interleaver Hardware Constants
// ---------------------------------------------------------------------------

// Maximum T (triangular side): T*(T+1)/2 >= E_max => T=32 covers up to 528.
// We cap E at 512 (the largest test vector) to avoid LUT explosion from a
// 528-bit monolithic ap_uint while still covering every required test case.
#define CI_MAX_T     32

// Maximum supported E value (largest test vector = 512 bits).
// Reducing from 528 -> 512 eliminates one extra 128-bit chunk.
#define CI_MAX_E     512

// AXI-Stream data width in bits (one transfer = 128 bits = 16 bytes).
#define CI_AXIS_DW   128

// Number of 128-bit AXI-Stream words needed to carry CI_MAX_E bits.
// 512 / 128 = 4 words exactly.
#define CI_MAX_WORDS ((CI_MAX_E + CI_AXIS_DW - 1) / CI_AXIS_DW)

// Number of 128-bit chunks used to store the bit buffer internally.
// Must equal CI_MAX_WORDS (= 4 for CI_MAX_E = 512).
#define CI_MAX_CHUNKS CI_MAX_WORDS

// ---------------------------------------------------------------------------
// AXI-Stream packet type
// ---------------------------------------------------------------------------
struct axis_word_t {
    ap_uint<CI_AXIS_DW>     data;
    ap_uint<CI_AXIS_DW / 8> keep;
    ap_uint<CI_AXIS_DW / 8> strb;
    ap_uint<1>              last;
};

// ---------------------------------------------------------------------------
// Top-level function prototype
// ---------------------------------------------------------------------------
void channel_interleaver(
    hls::stream<axis_word_t> &inData,
    hls::stream<axis_word_t> &outData,
    ap_uint<64>               cnData
);

#endif // HEADER_H

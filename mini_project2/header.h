#ifndef HEADER_H
#define HEADER_H

#include <ap_int.h>
#include <hls_stream.h>

// Macros for Channel Interleaver
#define CI_MAX_T 32
#define CI_MAX_E 528
#define CI_AXIS_DW 128
#define CI_MAX_WORDS ((CI_MAX_E + CI_AXIS_DW - 1) / CI_AXIS_DW)

// AXI-Stream Struct
struct axis_word_t {
    ap_uint<CI_AXIS_DW> data;
    ap_uint<CI_AXIS_DW/8> keep;
    ap_uint<CI_AXIS_DW/8> strb;
    ap_uint<1> last;
};

// Function Prototype
void channel_interleaver(
    hls::stream<axis_word_t> &inData,
    hls::stream<axis_word_t> &outData,
    ap_uint<64> cnData
);

#endif // HEADER_H

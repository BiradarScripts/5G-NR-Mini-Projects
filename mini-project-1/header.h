#ifndef INTERLEAVER_H
#define INTERLEAVER_H

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

typedef ap_axiu<96, 0, 0, 0> axis96_t;

void Interleaver(
    hls::stream<axis96_t> &inData,
    hls::stream<axis96_t> &outData,
    ap_uint<128> cnData
);

#endif

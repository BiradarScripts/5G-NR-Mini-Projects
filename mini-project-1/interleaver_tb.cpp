#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>

#include "header.h"

static std::string strip_spaces(const std::string& s) {
    std::string out;
    for (char c : s) if (!std::isspace((unsigned char)c)) out.push_back(c);
    return out;
}

static bool is_binary_line(const std::string& s) {
    bool seen01 = false;
    for (char c : s) {
        if (c=='0' || c=='1') { seen01 = true; continue; }
        if (std::isspace((unsigned char)c)) continue;
        return false;
    }
    return seen01;
}

// Parses either:
//  - binary string (length <= 96 or >=96) interpreted MSB->LSB
//  - hex string optionally with 0x prefix
static ap_uint<96> parse_word96(const std::string& line_raw) {
    std::string line = strip_spaces(line_raw);
    if (line.empty()) return 0;

    // binary
    if (is_binary_line(line)) {
        ap_uint<96> v = 0;

        // If longer than 96, take last 96 bits
        int start = (line.size() > 96) ? (int)line.size() - 96 : 0;
        int len = (int)line.size() - start;

        for (int i = 0; i < len; i++) {
            char c = line[start + i];
            v <<= 1;
            v |= (c == '1') ? 1 : 0;
        }
        if (len < 96) v <<= (96 - len);
        return v;
    }

    // hex
    if (line.rfind("0x", 0) == 0 || line.rfind("0X", 0) == 0) line = line.substr(2);

    ap_uint<96> v = 0;
    for (char c : line) {
        int nib = -1;
        if (c >= '0' && c <= '9') nib = c - '0';
        else if (c >= 'a' && c <= 'f') nib = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') nib = 10 + (c - 'A');
        else continue;

        v = (v << 4) | (ap_uint<96>)nib;
    }
    return v;
}

static bool read_vector_words(const std::string& path, std::vector<ap_uint<96>>& words) {
    std::ifstream fin(path.c_str());
    if (!fin) return false;

    std::string line;
    while (std::getline(fin, line)) {
        std::string s = strip_spaces(line);
        if (s.empty()) continue;
        words.push_back(parse_word96(s));
    }
    return true;
}

static void run_one_case(int tc, int E, int Qm) {
    std::string inFile  = "test_case_" + std::to_string(tc) + "_in";
    std::string outFile = "test_case_" + std::to_string(tc) + "_out";

    std::vector<ap_uint<96>> vin, vexp;
    if (!read_vector_words(inFile, vin)) {
        std::cerr << "ERROR: cannot open " << inFile << "\n";
        return;
    }
    if (!read_vector_words(outFile, vexp)) {
        std::cerr << "ERROR: cannot open " << outFile << "\n";
        return;
    }

    // Build cnData
    ap_uint<128> cnData = 0;
    cnData.range(14,0)  = (ap_uint<15>)E;
    cnData.range(18,15) = (ap_uint<4>)Qm;

    hls::stream<axis96_t> s_in, s_out;

    int words = (E + 95) / 96;

    // push inputs
    for (int w = 0; w < words; w++) {
        axis96_t a;
        a.data = (w < (int)vin.size()) ? vin[w] : (ap_uint<96>)0;
        a.last = (w == words - 1) ? 1 : 0;
        s_in.write(a);
    }

    // call DUT
    Interleaver(s_in, s_out, cnData);

    // check outputs
    bool pass = true;
    for (int w = 0; w < words; w++) {
        axis96_t got = s_out.read();
        ap_uint<96> exp = (w < (int)vexp.size()) ? vexp[w] : (ap_uint<96>)0;

        if (got.data != exp) {
            pass = false;
            std::cout << "TC" << tc << " MISMATCH word " << w << "\n";
            std::cout << "  got: 0x" << got.data.to_string(16) << "\n";
            std::cout << "  exp: 0x" << exp.to_string(16) << "\n";
            break;
        }

        if ((w == words - 1) && (got.last != 1)) {
            pass = false;
            std::cout << "TC" << tc << " MISMATCH TLAST on final word\n";
            break;
        }
    }

    // Make sure DUT didn’t produce extra unexpected words
    if (pass && !s_out.empty()) {
        pass = false;
        std::cout << "TC" << tc << " produced EXTRA output words (stream not empty)\n";
    }

    std::cout << "Test case " << tc << " (E=" << E << ", Qm=" << Qm << "): "
              << (pass ? "PASS" : "FAIL") << "\n";
}

int main() {
    const int Qm_list[14] = {2,2,2,2,4,4,4,4,6,6,6,6,8,8};
    const int E_list [14] = {40,672,1360,7200,8400,9840,12888,13120,9258,9336,9582,10800,9496,10000};

    for (int tc = 1; tc <= 14; tc++) {
        run_one_case(tc, E_list[tc-1], Qm_list[tc-1]);
    }
    return 0;
}

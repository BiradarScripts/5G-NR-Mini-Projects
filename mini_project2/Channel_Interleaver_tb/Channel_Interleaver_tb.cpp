#include "header.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::string join_path(const std::string &dir, const std::string &name) {
  if (dir.empty() || dir == ".") {
    return name;
  }
  const char last = dir[dir.size() - 1];
  if (last == '/' || last == '\\') {
    return dir + name;
  }
  return dir + "/" + name;
}

static bool file_exists(const std::string &path) {
  std::ifstream f(path.c_str(), std::ios::binary);
  return f.good();
}

static std::string resolve_vector_path(const std::string &dir,
                                       const std::string &base_name) {
  static const char *suffixes[] = {"", ".txt", ".dat", ".bin"};
  for (unsigned i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
    const std::string path = join_path(dir, base_name + suffixes[i]);
    if (file_exists(path)) {
      return path;
    }
  }
  throw std::runtime_error("Could not find vector file for base name: " +
                           base_name);
}

static int hex_value(const char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static bool is_binary_token(const std::string &tok) {
  if (tok.empty())
    return false;
  for (size_t i = 0; i < tok.size(); ++i) {
    if (tok[i] != '0' && tok[i] != '1') {
      return false;
    }
  }
  return true;
}

static bool is_explicit_hex_token(const std::string &tok) {
  if (tok.size() <= 2)
    return false;
  if (!(tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')))
    return false;
  for (size_t i = 2; i < tok.size(); ++i) {
    if (hex_value(tok[i]) < 0) {
      return false;
    }
  }
  return true;
}

static bool is_long_hex_token(const std::string &tok) {
  if (tok.size() < 8)
    return false;
  bool has_hex_letter = false;
  for (size_t i = 0; i < tok.size(); ++i) {
    if (hex_value(tok[i]) < 0) {
      return false;
    }
    if ((tok[i] >= 'a' && tok[i] <= 'f') || (tok[i] >= 'A' && tok[i] <= 'F')) {
      has_hex_letter = true;
    }
  }
  return has_hex_letter;
}

static void append_binary_token(const std::string &tok,
                                std::vector<int> &bits) {
  for (size_t i = 0; i < tok.size(); ++i) {
    bits.push_back(tok[i] - '0');
  }
}

static void append_hex_token(std::string tok, std::vector<int> &bits) {
  if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
    tok = tok.substr(2);
  }
  for (size_t i = 0; i < tok.size(); ++i) {
    const int v = hex_value(tok[i]);
    for (int b = 3; b >= 0; --b) {
      bits.push_back((v >> b) & 1);
    }
  }
}

static std::vector<std::string> extract_tokens(const std::string &text) {
  std::vector<std::string> tokens;
  std::string tok;
  for (size_t i = 0; i < text.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (std::isalnum(ch) || text[i] == 'x' || text[i] == 'X') {
      tok.push_back(text[i]);
    } else if (!tok.empty()) {
      tokens.push_back(tok);
      tok.clear();
    }
  }
  if (!tok.empty()) {
    tokens.push_back(tok);
  }
  return tokens;
}

static std::vector<int> load_bits_from_file(const std::string &path,
                                            const int expected_E) {
  std::ifstream fin(path.c_str(), std::ios::binary);
  if (!fin) {
    throw std::runtime_error("Unable to open file: " + path);
  }

  std::string content((std::istreambuf_iterator<char>(fin)),
                      std::istreambuf_iterator<char>());
  const std::vector<std::string> tokens = extract_tokens(content);

  std::vector<int> bits;

  // Pass 1: binary tokens and explicit 0x-prefixed hex tokens.
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (is_binary_token(tokens[i])) {
      append_binary_token(tokens[i], bits);
    } else if (is_explicit_hex_token(tokens[i])) {
      append_hex_token(tokens[i], bits);
    }
  }

  // Pass 2 fallback: some files may store long hex strings without 0x.
  if ((int)bits.size() < expected_E) {
    bits.clear();
    for (size_t i = 0; i < tokens.size(); ++i) {
      if (is_binary_token(tokens[i])) {
        append_binary_token(tokens[i], bits);
      } else if (is_explicit_hex_token(tokens[i]) ||
                 is_long_hex_token(tokens[i])) {
        append_hex_token(tokens[i], bits);
      }
    }
  }

  // Final fallback: raw scan for literal 0/1 characters only.
  if ((int)bits.size() < expected_E) {
    bits.clear();
    for (size_t i = 0; i < content.size(); ++i) {
      if (content[i] == '0' || content[i] == '1') {
        bits.push_back(content[i] - '0');
      }
    }
  }

  if ((int)bits.size() < expected_E) {
    std::ostringstream oss;
    oss << "File " << path
        << " does not contain enough bit data. Expected at least " << expected_E
        << " bits, found " << bits.size();
    throw std::runtime_error(oss.str());
  }

  bits.resize(expected_E);
  return bits;
}

static std::vector<int>
sw_channel_interleaver(const std::vector<int> &in_bits) {
  const int E = (int)in_bits.size();
  int T = 0;
  while ((T * (T + 1)) / 2 < E) {
    ++T;
  }

  std::vector<std::vector<int>> value(T, std::vector<int>(T, 0));
  std::vector<std::vector<int>> valid(T, std::vector<int>(T, 0));

  int k = 0;
  for (int i = 0; i < T; ++i) {
    for (int j = 0; j < T - i; ++j) {
      if (k < E) {
        value[j][i] = in_bits[k];
        valid[j][i] = 1;
      }
      ++k;
    }
  }

  std::vector<int> out_bits;
  out_bits.reserve(E);
  for (int j = 0; j < T; ++j) {
    for (int i = 0; i < T - j; ++i) {
      if (valid[j][i]) {
        out_bits.push_back(value[j][i]);
      }
    }
  }
  return out_bits;
}

static void push_input_stream(const std::vector<int> &bits,
                              hls::stream<axis_word_t> &stream_in) {
  const int E = (int)bits.size();
  const int n_words = (E + CI_AXIS_DW - 1) / CI_AXIS_DW;

  for (int w = 0; w < n_words; ++w) {
    axis_word_t pkt;
    pkt.data = 0;
    pkt.keep = -1;
    pkt.strb = -1;
    pkt.last = (w == n_words - 1) ? ap_uint<1>(1) : ap_uint<1>(0);

    for (int b = 0; b < CI_AXIS_DW; ++b) {
      const int idx = w * CI_AXIS_DW + b;
      if (idx < E) {
        pkt.data[b] = bits[idx];
      }
    }
    stream_in.write(pkt);
  }
}

static std::vector<int> pop_output_stream(hls::stream<axis_word_t> &stream_out,
                                          const int E) {
  const int n_words = (E + CI_AXIS_DW - 1) / CI_AXIS_DW;
  std::vector<int> bits(E, 0);

  for (int w = 0; w < n_words; ++w) {
    if (stream_out.empty()) {
      throw std::runtime_error(
          "DUT produced fewer output words than expected.");
    }

    axis_word_t pkt = stream_out.read();
    const bool expected_last = (w == n_words - 1);
    if ((bool)pkt.last != expected_last) {
      std::ostringstream oss;
      oss << "Unexpected TLAST on output word " << w
          << ". expected_last=" << expected_last << ", got=" << (bool)pkt.last;
      throw std::runtime_error(oss.str());
    }

    for (int b = 0; b < CI_AXIS_DW; ++b) {
      const int idx = w * CI_AXIS_DW + b;
      if (idx < E) {
        bits[idx] = (int)pkt.data[b];
      }
    }
  }

  if (!stream_out.empty()) {
    throw std::runtime_error("DUT produced more output words than expected.");
  }

  return bits;
}

static bool compare_bits(const std::vector<int> &got,
                         const std::vector<int> &exp, const std::string &tag,
                         const int E) {
  if (got.size() != exp.size()) {
    std::cout << "[FAIL] " << tag << " size mismatch. got=" << got.size()
              << ", exp=" << exp.size() << std::endl;
    return false;
  }

  for (size_t i = 0; i < got.size(); ++i) {
    if (got[i] != exp[i]) {
      std::cout << "[FAIL] " << tag << " mismatch for E=" << E << " at bit "
                << i << ": got=" << got[i] << ", exp=" << exp[i] << std::endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char **argv) {
  const std::string vector_dir = (argc > 1) ? argv[1] : ".";

  const int tests[] = {32,  48,  64,  80,  96,  112, 128, 144,
                       176, 208, 288, 320, 352, 384, 480, 512};

  const int num_tests = (int)(sizeof(tests) / sizeof(tests[0]));
  int pass_count = 0;

  for (int t = 0; t < num_tests; ++t) {
    const int E = tests[t];

    try {
      const std::string in_base = "E_" + std::to_string(E) + "_in";
      const std::string out_base = "E_" + std::to_string(E) + "_out";

      const std::string in_path = resolve_vector_path(vector_dir, in_base);
      const std::string out_path = resolve_vector_path(vector_dir, out_base);

      const std::vector<int> in_bits = load_bits_from_file(in_path, E);
      const std::vector<int> golden_bits = load_bits_from_file(out_path, E);
      const std::vector<int> sw_bits = sw_channel_interleaver(in_bits);

      if (!compare_bits(sw_bits, golden_bits, "Software reference vs golden",
                        E)) {
        std::cout << "Stopping because the provided golden-vector "
                     "interpretation failed for E="
                  << E << std::endl;
        return 1;
      }

      hls::stream<axis_word_t> in_stream;
      hls::stream<axis_word_t> out_stream;

      push_input_stream(in_bits, in_stream);
      channel_interleaver(in_stream, out_stream, (ap_uint<64>)E);
      const std::vector<int> dut_bits = pop_output_stream(out_stream, E);

      const bool ok_dut_vs_gold =
          compare_bits(dut_bits, golden_bits, "DUT vs golden", E);
      const bool ok_dut_vs_sw =
          compare_bits(dut_bits, sw_bits, "DUT vs software", E);

      if (!ok_dut_vs_gold || !ok_dut_vs_sw) {
        return 1;
      }

      std::cout << "[PASS] E=" << E << std::endl;
      ++pass_count;
    } catch (const std::exception &e) {
      std::cout << "[FAIL] E=" << E << " : " << e.what() << std::endl;
      return 1;
    }
  }

  std::cout << "\nAll test cases passed: " << pass_count << " / " << num_tests
            << std::endl;
  return 0;
}

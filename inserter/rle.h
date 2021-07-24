#pragma once

#include <vector>

std::vector<uint8_t> compress_gba_rl(const std::vector<uint8_t> &input);
std::vector<uint8_t> decompress_gba_rl(const std::vector<uint8_t> &input);

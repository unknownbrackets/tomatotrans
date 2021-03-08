#pragma once

enum LZ77Flags {
	LZ77_NORMAL = 0x0000,
	LZ77_FAST = 0x0001,
	LZ77_VRAM_SAFE = 0x0002,
	LZ77_REVERSE = 0x0004,
};

std::vector<uint8_t> compress_gba_lz77(const std::vector<uint8_t> &input, int flags);
std::vector<uint8_t> decompress_gba_lz77(const std::vector<uint8_t> &input);

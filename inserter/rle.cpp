#include <algorithm>
#include "rle.h"

std::vector<uint8_t> compress_gba_rl(const std::vector<uint8_t> &input) {
	std::vector<uint8_t> data;

	data.reserve(input.size());
	data.push_back(0x30);
	data.push_back((input.size() >> 0) & 0xFF);
	data.push_back((input.size() >> 8) & 0xFF);
	data.push_back((input.size() >> 16) & 0xFF);

	size_t inpos = 0;
	while (inpos < input.size()) {
		size_t left = input.size() - inpos;
		if (left >= 3 && input[inpos] == input[inpos + 1] && input[inpos] == input[inpos + 2]) {
			// This is the best case, we correct down.
			int len = 0x82;
			for (int i = 3; i < left && i < 0x82; ++i) {
				if (input[inpos] != input[inpos + i]) {
					len = i;
					break;
				}
			}

			data.push_back((len - 3) | 0x80);
			data.push_back(input[inpos]);
			inpos += len;
			continue;
		}

		int rawlen = (int)std::min(left, (size_t)0x80);
		data.push_back(rawlen - 1);
		for (int i = 0; i < rawlen; ++i) {
			data.push_back(input[inpos++]);
		}
	}

	return data;
}

std::vector<uint8_t> decompress_gba_rl(const std::vector<uint8_t> &input) {
	std::vector<uint8_t> data;
	if (input.size() < 4 || input[0] != 0x30) {
		fprintf(stderr, "Input data too small to decompress via RLE.\n");
		return data;
	}

	uint32_t sz = input[1] | (input[2] << 8) | (input[3] < 16);
	data.resize(sz);

	uint32_t outpos = 0;
	size_t inpos = 4;
	while (outpos < sz && inpos < input.size()) {
		uint8_t flags = input[inpos++];
		bool run = (flags & 0x80) != 0;
		int len = (flags & 0x7F) + (run ? 3 : 1);

		if (run) {
			uint8_t value = input[inpos++];
			for (int i = 0; i < len && outpos < sz; ++i) {
				data[outpos++] = value;
			}
		} else {
			for (int i = 0; i < len && outpos < sz; ++i) {
				data[outpos++] = input[inpos++];
			}
		}
	}

	return data;
}

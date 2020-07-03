#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>
#include "lz77.h"

class LZ77GBACompressor {
public:
	LZ77GBACompressor(const std::vector<uint8_t> &input) : input_(input) {
		BuildIndex();
	}

	std::vector<uint8_t> Compress(int flags, int max_scenarios);

private:
	constexpr static int MAX_BACKREF_OFFSET = 0xFFF;
	constexpr static int MIN_BACKREF_LEN = 3;
	constexpr static int MAX_BACKREF_LEN = 3 + 0xF;

	struct Scenario {
		int waste = 1;
		int backref_offset = 0;
		int backref_len = 0;
		int saved_bytes = 0;
	};

	void CompressReverse(int max_scenarios);
	void CompressForward(int max_scenarios);

	void BuildIndex();
	Scenario TryReverseWaste(size_t src, int waste);
	Scenario TryForwardWaste(size_t src, int waste);

	const std::vector<uint8_t> &input_;
	std::vector<uint8_t> output_;
	std::multimap<uint32_t, size_t> index_;
	int flags_ = 0;
};

std::vector<uint8_t> LZ77GBACompressor::Compress(int flags, int max_scenarios) {
	output_.clear();

	// Before any compression, validate the size can be supported.
	if (input_.size() > 0x00FFFFFF) {
		fprintf(stderr, "Input data too big to compress via LZ77.\n");
		return output_;
	}

	flags_ = flags;
	if (flags & LZ77_REVERSE) {
		CompressReverse(max_scenarios);
	} else {
		CompressForward(max_scenarios);
	}

	return output_;
}

void LZ77GBACompressor::BuildIndex() {
	if (input_.size() < 3) {
		return;
	}
	index_.clear();

	uint32_t hash = (input_[0] << 16) | (input_[1] << 8);
	for (size_t i = 2; i < input_.size(); ++i) {
		hash = (hash | input_[i]) << 8;
		index_.emplace(hash, i);
	}
}

LZ77GBACompressor::Scenario LZ77GBACompressor::TryReverseWaste(size_t src, int waste) {
	Scenario plan{ waste };
	// Start out in debt, our match has to save more.
	plan.saved_bytes = -2 - waste;
	if ((size_t)waste + MIN_BACKREF_LEN >= src) {
		return plan;
	}

	// According to this plan, our buffer will END at this position.
	src -= waste;

	// We have at most this many bytes to encode.
	int left = src - MIN_BACKREF_LEN > (size_t)MAX_BACKREF_LEN ? MAX_BACKREF_LEN : (int)src - MIN_BACKREF_LEN;
	if (left < MIN_BACKREF_LEN) {
		return plan;
	}

	int best_offset = 0;
	int best_len = 0;

	uint32_t hash = (input_[src - 3] << 24) | (input_[src - 2] << 16) | (input_[src - 1] << 8);
	const bool checkVRAM = (flags_ & LZ77_VRAM_SAFE) != 0;
	auto matches = index_.equal_range(hash);
	for (auto it = matches.first; it != matches.second; ++it) {
		const size_t &found_pos = it->second;
		if (found_pos >= src - 1) {
			break;
		}
		if (found_pos + MAX_BACKREF_OFFSET < src - 1) {
			continue;
		}

		int found_off = (int)(src - 1 - found_pos);
		if (checkVRAM && found_off <= 1) {
			continue;
		}

		// How long is this match?  We only looked for three bytes so far.
		int found_len = 3;
		int max_len = found_pos >= (size_t)left ? left : (int)found_pos;
		for (int j = found_len; j < max_len; ++j) {
			if (input_[found_pos - j] != input_[src - j - 1]) {
				break;
			}
			found_len++;
		}

		if (found_len > best_len) {
			// Maybe this is the one?
			best_len = found_len;
			best_offset = found_off;
			if (found_len == MAX_BACKREF_LEN) {
				break;
			}
		}
	}

	if (best_len >= MIN_BACKREF_LEN) {
		// Alright, this is our best shot.
		plan.backref_offset = best_offset - 1;
		plan.backref_len = best_len;
		plan.saved_bytes += best_len;
	}
	return plan;
}

void LZ77GBACompressor::CompressReverse(int max_scenarios) {
	std::vector<std::vector<uint8_t>> chunks;
	// This is the worst case.
	chunks.reserve(input_.size());

	// Start at the END, and build the file backwards.
	// This gives us the smartest backreference decisions.
	ptrdiff_t total_saved = 0;
	size_t compressed_size = 0;
	for (size_t src = input_.size(); src > 0; ) {
		if (src == 0 || src > input_.size()) {
			break;
		}

		// Default plan is no backref.
		Scenario plan;

		// Play Chess.  Is it smarter to waste a byte now to save one later?
		for (int s = 0; s < max_scenarios; ++s) {
			Scenario option = TryReverseWaste(src, s);
			if (option.saved_bytes > plan.saved_bytes) {
				plan = option;
				// If any plan wasting a byte is better, go with it.
				// We don't need to examine future plans, since we'll do this next byte.
				if (s != 0) {
					break;
				}
				// If this is already ideal, we're done.
				if (plan.backref_len == MAX_BACKREF_LEN) {
					break;
				}
			} else if (s == 0) {
				// Won't save, no sense checking other scenarios.
				break;
			}
		}

		if (plan.waste != 0) {
			for (int i = 0; i < plan.waste; ++i) {
				chunks.push_back({ input_[--src] });
				compressed_size++;
			}
		} else {
			uint8_t byte1 = ((plan.backref_len - 3) << 4) | (plan.backref_offset >> 8);
			uint8_t byte2 = plan.backref_offset & 0xFF;
			chunks.push_back({ byte1, byte2 });
			src -= plan.backref_len;
			total_saved += plan.saved_bytes;
			compressed_size += 2;
		}
	}

	compressed_size += (chunks.size() + 7) / 8;
	compressed_size += chunks.size() & 7;

	output_.resize((compressed_size + 4 + 3) & ~3);
	output_[0] = 0x10;
	output_[1] = (input_.size() >> 0) & 0xFF;
	output_[2] = (input_.size() >> 8) & 0xFF;
	output_[3] = (input_.size() >> 16) & 0xFF;

	// Time to reverse the chunks and insert the flags.
	size_t pos = 4;
	for (size_t i = 0; i < chunks.size(); i += 8) {
		uint8_t *quadflags = &output_[pos++];
		for (int j = 0; j < 8 && i + j < chunks.size(); ++j) {
			const auto &chunk = chunks[chunks.size() - i - j - 1];
			if (chunk.size() == 1) {
				output_[pos++] = chunk[0];
			} else {
				*quadflags |= 0x80 >> j;
				output_[pos++] = chunk[0];
				output_[pos++] = chunk[1];
			}
		}
	}
}

LZ77GBACompressor::Scenario LZ77GBACompressor::TryForwardWaste(size_t src, int waste) {
	Scenario plan{ waste };
	// Start out in debt, our match has to save more.
	plan.saved_bytes = -2 - waste;
	// According to this plan, we'll have this many src bytes to match.
	src += waste;

	if (src == 0 || src + waste >= input_.size()) {
		return plan;
	}

	// We have at most this many bytes to encode.
	int left = input_.size() - src > (size_t)MAX_BACKREF_LEN ? MAX_BACKREF_LEN : (int)(input_.size() - src);
	if (left < MIN_BACKREF_LEN) {
		return plan;
	}

	int best_offset = 0;
	int best_len = 0;

	uint32_t hash = (input_[src] << 24) | (input_[src + 1] << 16) | (input_[src + 2] << 8);
	const bool checkVRAM = (flags_ & LZ77_VRAM_SAFE) != 0;
	auto matches = index_.equal_range(hash);
	for (auto it = matches.first; it != matches.second; ++it) {
		const size_t &found_pos = it->second;
		if (found_pos >= src) {
			break;
		}
		if (found_pos + MAX_BACKREF_OFFSET < src) {
			continue;
		}

		int found_off = (int)(src - found_pos);
		if (checkVRAM && found_off <= 1) {
			continue;
		}

		// How long is this match?  We only looked for three bytes so far.
		int found_len = 3;
		for (int j = found_len; j < left; ++j) {
			if (input_[found_pos + j] != input_[src + j]) {
				break;
			}
			found_len++;
		}

		if (found_len > best_len) {
			// Maybe this is the one?
			best_len = found_len;
			best_offset = found_off;
			if (found_len == MAX_BACKREF_LEN) {
				break;
			}
		}
	}

	if (best_len >= MIN_BACKREF_LEN) {
		// Alright, this is our best shot.
		plan.backref_offset = best_offset - 1;
		plan.backref_len = best_len;
		plan.saved_bytes += best_len;
	}
	return plan;
}

void LZ77GBACompressor::CompressForward(int max_scenarios) {
	// Worst case: we add one extra byte for each 8, plus 4 byte header.
	output_.resize(((input_.size() + 7) & ~7) + (input_.size() + 7) / 8 + 4);
	uint8_t *pos = output_.data();

	// Now the header, method 0x10 + size << 4, in LE.
	*pos++ = 0x10;
	*pos++ = (input_.size() >> 0) & 0xFF;
	*pos++ = (input_.size() >> 8) & 0xFF;
	*pos++ = (input_.size() >> 16) & 0xFF;

	ptrdiff_t total_saved = 0;
	for (size_t src = 0; src < input_.size(); ) {
		uint8_t *quadflags = pos++;
		uint8_t nowflags = 0;

		for (int i = 0; i < 8; ++i) {
			if (src >= input_.size()) {
				*pos++ = 0;
				continue;
			}

			// Default plan is no backref.
			Scenario plan;

			// Play Chess.  Is it smarter to waste a byte now to save one later?
			for (int s = 0; s < max_scenarios; ++s) {
				Scenario option = TryForwardWaste(src, s);
				if (option.saved_bytes >= plan.saved_bytes) {
					plan = option;
					// If any plan wasting a byte is better, go with it.
					// We don't need to examine future plans, since we'll do this next byte.
					if (s != 0) {
						break;
					}
					// This is good enough, let's not keep looking.  Might be worse.
					if (plan.backref_len >= 13) {
						break;
					}
				} else if (s == 0) {
					// Won't save, no sense checking other scenarios.
					break;
				}
			}

			if (plan.waste != 0) {
				for (int j = 0; j < plan.waste && i < 8; ++j) {
					*pos++ = input_[src++];
					i++;
				}
				// We increased this one extra.
				i--;
			} else {
				*pos++ = ((plan.backref_len - 3) << 4) | (plan.backref_offset >> 8);
				*pos++ = plan.backref_offset & 0xFF;
				src += plan.backref_len;
				nowflags |= 0x80 >> i;
				total_saved += plan.saved_bytes;
			}
		}

		*quadflags = nowflags;
		total_saved--;
	}

	size_t len = pos - output_.data();
	output_.resize((len + 3) & ~3);
}

std::vector<uint8_t> compress_gba_lz77(const std::vector<uint8_t> &input, int flags) {
	LZ77GBACompressor engine(input);

	if (flags & LZ77_FAST) {
		return engine.Compress(flags, 1);
	}

	// Try to compress as well as possible to fit more in the same space.
	std::vector<uint8_t> best;
	for (int s = 0; s < 4; ++s) {
		std::vector<uint8_t> trial = engine.Compress(flags & ~LZ77_REVERSE, s);
		if (trial.size() < best.size() || best.size() == 0) {
			best = std::move(trial);
		}

		trial = engine.Compress(flags | LZ77_REVERSE, s);
		if (trial.size() < best.size() || best.size() == 0) {
			best = std::move(trial);
		}
	}

	return best;
}

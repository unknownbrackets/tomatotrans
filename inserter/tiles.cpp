#include <cstdio>
#include <cstring>
#include "tiles.h"

void Palette::Load(uint8_t base, uint8_t count, uint16_t *data) {
	if (base != 0) {
		memset(colors_, 0, base * 16 * sizeof(uint16_t));
	}
	for (int i = 0; i < count * 16; ++i) {
		colors_[base * 16 + i] = data[i] & 0x7FFF;
	}
	validBase_ = base;
	validEnd_ = base + count;
	if (validEnd_ != 16) {
		memset(colors_ + validEnd_ * 16, 0, sizeof(colors_) - validEnd_ * 16 * sizeof(uint16_t));
	}
}

uint16_t Palette::FindPaletteMask16(const uint8_t *color4, uint16_t lastMask) const {
	uint16_t c = ColorTo555(color4);
	if (c & 0x8000) {
		// Transparent, any palette will do.
		return lastMask;
	}

	uint16_t valid = 0;
	for (uint8_t p = validBase_; p < validEnd_; ++p) {
		if ((lastMask & (1 << p)) == 0) {
			continue;
		}
		for (int i = p * 16; i < p * 16 + 16; ++i) {
			if (colors_[i] == c) {
				valid |= 1 << p;
			}
		}
	}
	return valid;
}

int Palette::FindIndex16(const uint8_t *color4, uint16_t paletteMask, uint8_t *palette) const {
	uint16_t c = ColorTo555(color4);

	if (paletteMask == 0) {
		return -1;
	}

	for (uint8_t i = 0; i < 16; ++i) {
		if (paletteMask & (1 << i)) {
			*palette = i;
			break;
		}
	}

	if (c & 0x8000) {
		// Transparent, always index 0.  Pick the first palette in the mask.
		return 0;
	}

	for (int i = *palette * 16; i < *palette * 16 + 16; ++i) {
		if (colors_[i] == c) {
			return i & 15;
		}
	}
	// Shouldn't happen, if used properly...
	return -1;
}

int Palette::FindIndex256(const uint8_t *color4) const {
	uint16_t c = ColorTo555(color4);
	if (c & 0x8000) {
		// Transparent, always index 0.
		return 0;
	}

	for (int i = validBase_ * 16; i < validEnd_ * 16; ++i) {
		if (colors_[i] == c) {
			return i;
		}
	}
	// Shouldn't happen?
	/*
	printf("Could not find %04x = %02x %02x %02x %02x\n", c, color4[0], color4[1], color4[2], color4[3]);
	for (int i = validBase_ * 16; i < validEnd_ * 16; ++i) {
		printf("  c%03d: %04x\n", i, colors_[i]);
	}
	*/
	return -1;
}

Tile Tile::Load16(const uint8_t *src) {
	Tile tile;
	for (int i = 0; i < 32; ++i) {
		tile.pixels_[i * 2 + 0] = src[i] & 0x0F;
		tile.pixels_[i * 2 + 1] = src[i] >> 4;
	}
	return tile;
}

Tile Tile::Load256(const uint8_t *src) {
	Tile tile;
	memcpy(tile.pixels_, src, 64);
	return tile;
}

void Tile::Encode16(uint8_t *dest) const {
	for (int i = 0; i < 32; ++i) {
		*dest++ = pixels_[i * 2] | (pixels_[i * 2 + 1] << 4);
	}
}

void Tile::Encode256(uint8_t *dest) const {
	memcpy(dest, pixels_, 64);
}

bool Tile::From16(const uint8_t *image, const Palette &pal, uint8_t *palette, int pixelStride) {
	// First, find a common ground palette.
	uint16_t common = 0xFFFF;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			common = pal.FindPaletteMask16(src, common);
			if (common == 0) {
				return false;
			}
		}
	}

	// Okay, great.  Now we can get the indexes for the first matching palette.
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			int c = pal.FindIndex16(src, common, palette);
			if (c == -1) {
				return false;
			}
			pixels_[y * 8 + x] = c;
		}
	}

	return true;
}

bool Tile::From256(const uint8_t *image, const Palette &pal, int pixelStride) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			int c = pal.FindIndex256(src);
			if (c == -1) {
				fprintf(stderr, "Could not find color at subpos %d,%d\n", x, y);
				return false;
			}
			pixels_[y * 8 + x] = c;
		}
	}
	return true;
}

bool Tile::Match(const Tile &other, bool *hflip, bool *vflip) const {
	if (memcmp(pixels_, other.pixels_, 64) == 0) {
		*hflip = false;
		*vflip = false;
		return true;
	}

	// Well, let's try, maybe flipped vertically...?
	bool vmatch = true;
	for (int y = 0; y < 8; ++y) {
		if (memcmp(pixels_ + y * 8, other.pixels_ + (7 - y) * 8, 8) != 0) {
			vmatch = false;
			break;
		}
	}
	if (vmatch) {
		*hflip = false;
		*vflip = true;
		return true;
	}

	// Last chance, not looking good.
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			if (pixels_[y * 8 + x] != other.pixels_[y * 8 + (7 - x)]) {
				return false;
			}
		}
	}

	*hflip = true;
	*vflip = false;
	return true;
}

void Tileset::Load16(uint8_t *data, int count) {
	tiles_.resize(count);
	free_.resize(count, false);
	for (size_t i = 0; i < tiles_.size(); ++i) {
		tiles_[i] = Tile::Load16(data);
		data += 32;
	}
}

int Tileset::FindOrAdd(const Tile &tile, uint8_t palette) {
	for (size_t i = 0; i < tiles_.size(); ++i) {
		bool hflip = false, vflip = false;
		if (tiles_[i].Match(tile, &hflip, &vflip)) {
			if (allowFlip_ || (!hflip && !vflip)) {
				free_[i] = false;
				return (uint16_t)i | (hflip ? 0x0400 : 0) | (vflip ? 0x0800 : 0) | (palette << 12);
			}
		}
	}

	return Add(tile) | (palette << 12);
}

int Tileset::Add(const Tile &tile) {
	size_t i = tiles_.size();
	// Try to find a free slot.
	for (size_t j = 0; j < free_.size(); ++j) {
		if (free_[j]) {
			i = j;
			break;
		}
	}

	if (i >= 0x03FF) {
		// Not good, out of tiles.
		return -1;
	}
	if (i == tiles_.size()) {
		tiles_.push_back(tile);
		free_.push_back(false);
	} else {
		tiles_[i] = tile;
		free_[i] = false;
	}

	return (uint16_t)i;
}

void Tileset::Free(int i) {
	free_[i] = true;
	size_t newSize = free_.size();
	for (size_t j = free_.size(); j > 0; --j) {
		if (!free_[j - 1]) {
			break;
		}
		newSize = j - 1;
	}
	if (newSize != free_.size()) {
		tiles_.resize(newSize);
		free_.resize(newSize);
	}
}

bool Tilemap::FromImage(const uint8_t *image, int width, int height, const Palette &pal, bool is256) {
	if ((width & 7) != 0 || (height & 7) != 0) {
		fprintf(stderr, "Image is not a clean multiple of 8x8 tiles\n");
		return false;
	}

	width_ = width / 8;
	height_ = height / 8;
	is256_ = is256;
	tiles_.resize(width_ * height_);

	auto compareTile = [width](const uint8_t *tile1, const uint8_t *tile2) {
		for (int y = 0; y < 8; ++y) {
			if (memcmp(tile1 + y * width * 4, tile2 + y * width * 4, 8 * 4) != 0) {
				return false;
			}
		}
		return true;
	};

	const uint8_t *lastSrc = nullptr;
	uint16_t lastIndex = 0;
	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
			const uint8_t *src = image + (width * y * 8 + x * 8) * 4;

			// If a tile repeats, we can reuse everything - tileset, index, palette, etc.
			if (lastSrc != nullptr && compareTile(lastSrc, src)) {
				tiles_[y * width_ + x] = lastIndex;
				continue;
			}

			Tile tile;
			uint8_t palette = 0;
			if (is256) {
				if (!tile.From256(src, pal, width)) {
					fprintf(stderr, "Could not palette tile at %d,%d\n", x, y);
					return false;
				}
			} else {
				if (!tile.From16(src, pal, &palette, width)) {
					fprintf(stderr, "Could not palette tile at %d,%d\n", x, y);
					return false;
				}
			}
			int index = tileset_.FindOrAdd(tile, palette);
			if (index == -1) {
				fprintf(stderr, "Could not allocate tile\n");
				return false;
			}
			lastSrc = src;
			lastIndex = (uint16_t)index;
			tiles_[y * width_ + x] = lastIndex;
		}
	}

	return true;
}

void Tilemap::EncodeMap(uint8_t *dest) const {
	memcpy(dest, tiles_.data(), ByteSizeMap());
}

Chip Chip::FromTilemap(const Tilemap &tilemap, int x, int y) {
	Chip chip;
	chip.tiles_[0] = tilemap.At(x, y);
	chip.tiles_[1] = tilemap.At(x + 1, y);
	chip.tiles_[2] = tilemap.At(x, y + 1);
	chip.tiles_[3] = tilemap.At(x + 1, y + 1);
	return chip;
}

void Chip::Encode(uint8_t *dest) const {
	memcpy(dest, tiles_, sizeof(tiles_));
}

bool Chip::Match(const Chip &other) const {
	return memcmp(tiles_, other.tiles_, sizeof(tiles_)) == 0;
}

int Chipset::FindOrAdd(const Chip &chip) {
	for (size_t i = 0; i < chips_.size(); ++i) {
		if (chips_[i].Match(chip)) {
			return (uint16_t)i;
		}
	}

	if (chips_.size() >= 0xFFFF) {
		// Not good, out of chips.
		return -1;
	}

	size_t i = chips_.size();
	chips_.push_back(chip);
	return (uint16_t)i;
}

bool Chipmap::FromTilemap(const Tilemap &tilemap) {
	if ((tilemap.Width() & 1) != 0 || (tilemap.Height() & 1) != 0) {
		return false;
	}

	width_ = tilemap.Width() / 2;
	height_ = tilemap.Height() / 2;
	chips_.resize(width_ * height_);

	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
			int index = chipset_.FindOrAdd(Chip::FromTilemap(tilemap, x * 2, y * 2));
			if (index == -1) {
				return false;
			}
			chips_[y * width_ + x] = (uint16_t)index;
		}
	}

	return true;
}

void Chipmap::EncodeMap(uint8_t *dest) const {
	memcpy(dest, chips_.data(), ByteSizeMap());
}

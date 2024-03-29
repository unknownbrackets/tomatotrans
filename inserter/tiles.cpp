#include <cstdio>
#include <cstring>
#include <set>
#include "tiles.h"

Palette::Palette(uint8_t base, uint8_t count) {
	validBase_ = base;
	validEnd_ = base + count;
	memset(colors_, 0, sizeof(colors_));
}

void Palette::Load(uint8_t base, uint8_t count, uint16_t *data) {
	if (base != 0) {
		memset(colors_, 0, base * 16 * sizeof(uint16_t));
	}
	for (int i = 0; i < count * 16; ++i) {
		int c = base * 16 + i;
		colors_[c] = data[i] & 0x7FFF;
		used_[c] = true;
	}
}

bool Palette::FromImage16(const uint8_t *image, int width, int height) {
	if ((width & 7) != 0 || (height & 7) != 0) {
		fprintf(stderr, "Image is not a clean multiple of 8x8 tiles\n");
		return false;
	}

	// Mark all transparent colors as used.
	for (int i = validBase_; i < validEnd_; ++i) {
		used_[i * 16] = true;
	}

	int h8 = height / 8;
	int w8 = width / 8;
	for (int y = 0; y < h8; ++y) {
		for (int x = 0; x < w8; ++x) {
			const uint8_t *src = image + (width * y * 8 + x * 8) * 4;
			if (!FromTile16(src, width)) {
				return false;
			}
		}
	}

	return true;
}

bool Palette::FromTile16(const uint8_t *image, int pixelStride) {
	// Get all colors used in this tile to start with.
	std::set<uint16_t> uniqueColors;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			uint16_t c = ColorTo555(src);
			// We always have room for transparent.
			if (c & 0x8000) {
				continue;
			}
			uniqueColors.insert(c);
		}
	}

	// If there's more than 15 (not counting transparent), it's over.
	if (uniqueColors.size() > 15) {
		return false;
	}

	int bestPalette = 16;
	int bestPaletteNeeded = 16;
	int bestPaletteFree = 16;
	std::set<uint16_t> bestExisting;
	for (uint8_t p = validBase_; p < validEnd_; ++p) {
		int needed = 0;
		std::set<uint16_t> availColors;
		for (int i = p * 16 + 1; i < p * 16 + 16; ++i) {
			if (used_[i]) {
				availColors.insert(colors_[i]);
			}
		}

		for (uint16_t c : uniqueColors) {
			if (availColors.count(c) == 0) {
				needed++;
			}
		}

		// We need more than it has, not even an option.
		int freeColors = 15 - (int)availColors.size();
		if (needed > freeColors) {
			continue;
		}
		// Try to find the palette with the least free entries that we can use.
		if (bestPaletteFree < freeColors || (bestPaletteFree == freeColors && bestPaletteNeeded < needed)) {
			continue;
		}

		// This one is better.
		bestPalette = p;
		bestPaletteNeeded = needed;
		bestPaletteFree = freeColors;
		bestExisting = availColors;
	}

	// We couldn't find one with enough free colors.
	if (bestPalette >= 16) {
		return false;
	}

	int nextFree = 16;
	for (int i = bestPalette * 16 + 1; i < bestPalette * 16 + 16; ++i) {
		if (!used_[i]) {
			nextFree = i;
			break;
		}
	}

	for (uint16_t c : uniqueColors) {
		if (bestExisting.count(c) == 0) {
			colors_[nextFree] = c;
			used_[nextFree] = true;

			for (int i = nextFree + 1; i < bestPalette * 16 + 16; ++i) {
				if (!used_[i]) {
					nextFree = i;
					break;
				}
			}
		}
	}

	return true;
}

bool Palette::FromImage256(const uint8_t *image, int width, int height) {
	std::set<uint16_t> existing;

	used_[0] = true;

	int end = validEnd_ * 16;
	int nextFree = 256;
	for (int i = validBase_ * 16; i < end; ++i) {
		if (used_[i]) {
			existing.insert(colors_[i]);
		} else if (nextFree == 256) {
			nextFree = i;
		}
	}

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const uint8_t *src = image + (width * y + x) * 4;
			uint16_t c = ColorTo555(src);
			if ((c & 0x8000) != 0 || existing.count(c) != 0) {
				// Transparent is always index 0, or existing.
				continue;
			}

			if (nextFree == 256) {
				return false;
			}

			colors_[nextFree] = c;
			used_[nextFree] = true;
			existing.insert(c);
			for (int i = nextFree + 1; i < end; ++i) {
				if (!used_[i]) {
					nextFree = i;
					break;
				}
			}
		}
	}

	return true;
}

void Palette::Resize(uint8_t base, uint8_t count) {
	validBase_ = base;
	validEnd_ = base + count;
}

int Palette::TotalUsage() const {
	int c = 0;
	int start = validBase_ == 0 ? 1 : validBase_ * 16;
	for (int i = start; i < validEnd_ * 16; ++i) {
		if (used_[i]) {
			c++;
		}
	}
	return c;
}

void Palette::Encode(uint16_t *dst, uint8_t base, uint8_t count) const {
	memcpy(dst, colors_ + base * 16, count * 16 * sizeof(uint16_t));
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
		for (int i = p * 16 + 1; i < p * 16 + 16; ++i) {
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

	for (int i = *palette * 16 + 1; i < *palette * 16 + 16; ++i) {
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

	int start = validBase_ == 0 ? 1 : validBase_ * 16;
	for (int i = start; i < validEnd_ * 16; ++i) {
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

bool Palette::To555Tile16(const uint8_t *input, uint16_t *output, uint8_t palette) const {
	if (palette < validBase_ || palette >= validEnd_) {
		return false;
	}

	uint16_t base = palette * 16;
	for (int i = 0; i < 64; ++i) {
		if (input[i] == 0) {
			output[i] = 0x8000;
		} else {
			output[i] = colors_[base + input[i]];
		}
	}
	return true;
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
	uint16_t common = FindPalettes(image, pal, pixelStride);

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

uint16_t Tile::FindPalettes(const uint8_t *image, const Palette &pal, int pixelStride) {
	uint16_t common = 0xFFFF;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			common = pal.FindPaletteMask16(src, common);
			if (common == 0) {
				return 0;
			}
		}
	}
	return common;
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

	if (MatchHFlip(other, false)) {
		*hflip = true;
		*vflip = false;
		return true;
	}
	// Last chance is horizontal and vertical flip.
	if (MatchHFlip(other, true)) {
		*hflip = true;
		*vflip = true;
		return true;
	}

	return false;
}

bool Tile::MatchHFlip(const Tile &other, bool vflip) const {
	for (int y = 0; y < 8; ++y) {
		int yy = vflip ? 7 - y : y;
		for (int x = 0; x < 8; ++x) {
			if (pixels_[y * 8 + x] != other.pixels_[yy * 8 + (7 - x)]) {
				return false;
			}
		}
	}
	return true;
}

void TilesetLookupCache::Add(int index, const Tile &tile) {
	uint8_t pixels[64];
	tile.Encode256(pixels);

	Entry entry;
	for (int p = 0; p < 16; ++p) {
		if (!pal_.To555Tile16(pixels, entry.pixels, p)) {
			continue;
		}

		entry.index = (index & 0x0FFF) | (p << 12);
		entries_.push_back(entry);

		if (allowFlip_) {
			FlipV(entry);
			entries_.push_back(entry);

			FlipH(entry);
			entries_.push_back(entry);

			// Only horizontal.
			FlipV(entry);
			entries_.push_back(entry);
		}
	}
}

int TilesetLookupCache::Find16(const uint8_t *image, int pixelStride) {
	Entry match;
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			const uint8_t *src = image + (y * pixelStride + x) * 4;
			uint16_t c = pal_.ColorTo555(src);
			if (c & 0x8000) {
				c = 0x8000;
			}
			match.pixels[y * 8 + x] = c;
		}
	}

	// Okay, now find that entry.
	for (const Entry &entry : entries_) {
		if (memcmp(match.pixels, entry.pixels, sizeof(match.pixels)) == 0) {
			return entry.index;
		}
	}

	return -1;
}

void TilesetLookupCache::FlipV(Entry &entry) {
	entry.index ^= 0x0800;
	uint16_t buf[16];
	// Only need half, because we swap.
	for (int y = 0; y < 4; ++y) {
		// Swap the top - y row with bottom + y.
		memcpy(buf, &entry.pixels[y * 8], sizeof(uint16_t) * 8);
		memcpy(&entry.pixels[y * 8], &entry.pixels[(7 - y) * 8], sizeof(uint16_t) * 8);
		memcpy(&entry.pixels[(7 - y) * 8], buf, sizeof(uint16_t) * 8);
	}
}

void TilesetLookupCache::FlipH(Entry &entry) {
	entry.index ^= 0x0400;
	for (int y = 0; y < 8; ++y) {
		// Only need to go half to mirror, since we swap.
		for (int x = 0; x < 4; ++x) {
			uint16_t buf = entry.pixels[y * 8 + x];
			entry.pixels[y * 8 + x] = entry.pixels[y * 8 + 7 - x];
			entry.pixels[y * 8 + 7 - x] = buf;
		}
	}
}

void Tileset::Load16(uint8_t *data, int count) {
	if (sizeLocked_ && count != (int)tiles_.size()) {
		return;
	}
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
		if (sizeLocked_) {
			return -1;
		}
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
	if (sizeLocked_) {
		return;
	}

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

void Tileset::LockSize() {
	sizeLocked_ = true;
}

void Tileset::Clear() {
	tiles_.clear();
	free_.clear();
	sizeLocked_ = false;
}

void Tileset::PopulateCache(TilesetLookupCache &cache) const {
	for (size_t i = 0; i < tiles_.size(); ++i) {
		if (free_[i]) {
			continue;
		}

		cache.Add((int)i, tiles_[i]);
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

	// Palettes can have duplicate entries, so we compare the expanded colors.
	TilesetLookupCache cache(pal, tileset_.AllowFlip());
	if (!is256_) {
		tileset_.PopulateCache(cache);
	}

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

			int index = -1;
			if (is256) {
				Tile tile;
				if (!tile.From256(src, pal, width)) {
					fprintf(stderr, "Could not palette tile at %d,%d\n", x, y);
					return false;
				}
				index = tileset_.FindOrAdd(tile, 0);
			} else {
				index = cache.Find16(src, width);
				if (index == -1) {
					Tile tile;
					uint8_t palette = 0;
					if (!tile.From16(src, pal, &palette, width)) {
						fprintf(stderr, "Could not palette tile at %d,%d\n", x, y);
						return false;
					}
					index = tileset_.FindOrAdd(tile, palette);
					if (index != -1) {
						cache.Add(index, tile);
					}
				}
			}
			if (index == -1) {
				fprintf(stderr, "Could not allocate tile at %d,%d\n", x, y);
				return false;
			}
			lastSrc = src;
			lastIndex = (uint16_t)index;
			tiles_[y * width_ + x] = lastIndex;
		}
	}

	return true;
}

void Tilemap::Override(int x, int y, uint16_t value) {
	tiles_[y * width_ + x] = value;
}

void Tilemap::EncodeMap(uint8_t *dest) const {
	memcpy(dest, tiles_.data(), ByteSizeMap());
}

Chip::Chip(uint16_t a1, uint16_t b1, uint16_t a2, uint16_t b2) {
	tiles_[0] = a1;
	tiles_[1] = b1;
	tiles_[2] = a2;
	tiles_[3] = b2;
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

void Chipset::InsertAt(uint16_t index, const Chip &chip) {
	if (index >= chips_.size()) {
		chips_.resize(index + 1);
		free_.resize(index + 1, true);
	}

	chips_[index] = chip;
	free_[index] = false;
}

int Chipset::FindOrAdd(const Chip &chip) {
	size_t nextFree = chips_.size();

	for (size_t i = 0; i < chips_.size(); ++i) {
		if (chips_[i].Match(chip)) {
			free_[i] = false;
			return (uint16_t)i;
		} else if (free_[i] && nextFree == chips_.size()) {
			nextFree = i;
		}
	}

	if (nextFree >= 0xFFFF) {
		// Not good, out of chips.
		return -1;
	}

	if (nextFree == chips_.size()) {
		chips_.push_back(chip);
		free_.push_back(false);
	} else {
		chips_[nextFree] = chip;
		free_[nextFree] = false;
	}
	return (uint16_t)nextFree;
}

void Chipmap::InsertChipAt(uint16_t index, const Chip &chip) {
	chipset_.InsertAt(index, chip);
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

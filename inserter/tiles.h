#include <cstdint>
#include <vector>

class Palette {
public:
	Palette(uint8_t base, uint8_t count);
	void Load(uint8_t base, uint8_t count, uint16_t *data);
	bool FromImage16(const uint8_t *image, int width, int height);
	bool FromImage256(const uint8_t *image, int width, int height);

	void Resize(uint8_t base, uint8_t count);
	int TotalUsage() const;

	void Encode(uint16_t *dst, uint8_t base, uint8_t count) const;

	// Finds palettes that contain a color, so you can make all of a tile use the same palette.
	uint16_t FindPaletteMask16(const uint8_t *color4, uint16_t lastMask = 0xFFFF) const;
	int FindIndex16(const uint8_t *color4, uint16_t paletteMask, uint8_t *palette) const;
	int FindIndex256(const uint8_t *color4) const;

private:
	bool FromTile16(const uint8_t *image, int pixelStride);
	uint16_t ColorTo555(const uint8_t *color4) const {
		uint8_t r = color4[0] >> 3;
		uint8_t g = color4[1] >> 3;
		uint8_t b = color4[2] >> 3;
		// We reverse the A flag to handle transparent.
		uint8_t a = color4[3] < 128;
		return (r << 0) | (g << 5) | (b << 10) | (a << 15);
	}

	uint16_t colors_[256]{};
	bool used_[256]{};
	uint8_t validBase_ = 16;
	uint8_t validEnd_ = 0;
};

class Tile {
public:
	static Tile Load16(const uint8_t *src);
	static Tile Load256(const uint8_t *src);
	void Encode16(uint8_t *dest) const;
	void Encode256(uint8_t *dest) const;

	bool From16(const uint8_t *image, const Palette &pal, uint8_t *palette, int pixelStride);
	bool From256(const uint8_t *image, const Palette &pal, int pixelStride);

	bool Match(const Tile &other, bool *hflip, bool *vflip) const;

private:
	uint8_t pixels_[64];
};

class Tileset {
public:
	Tileset() {
	}
	explicit Tileset(bool allowFlip) : allowFlip_(allowFlip) {
	}

	void Load16(uint8_t *data, int count);

	int FindOrAdd(const Tile &tile, uint8_t palette);
	int Add(const Tile &tile);
	void Free(int i);
	void Clear();

	size_t ByteSize16() const {
		return tiles_.size() * 32;
	}

	size_t ByteSize256() const {
		return tiles_.size() * 64;
	}

	void Encode16(uint8_t *dest) const {
		for (const Tile &tile : tiles_) {
			tile.Encode16(dest);
			dest += 32;
		}
	}

	void Encode256(uint8_t *dest) const {
		for (const Tile &tile : tiles_) {
			tile.Encode256(dest);
			dest += 64;
		}
	}

	const Tile &At(int i) const {
		return tiles_[i];
	}

private:
	std::vector<Tile> tiles_;
	std::vector<bool> free_;
	bool allowFlip_ = true;
};

class Tilemap {
public:
	Tilemap(Tileset &tileset) : tileset_(tileset) {
	}

	bool FromImage(const uint8_t *image, int width, int height, const Palette &pal, bool is256);
	void Override(int x, int y, uint16_t value);

	uint16_t At(int x, int y) const {
		return tiles_[y * width_ + x];
	}

	int Width() const {
		return width_;
	}

	int Height() const {
		return height_;
	}

	size_t ByteSizeMap() const {
		return tiles_.size() * sizeof(uint16_t);
	}

	void EncodeMap(uint8_t *dest) const;

	size_t ByteSizeSet() const {
		return is256_ ? tileset_.ByteSize256() : tileset_.ByteSize16();
	}

	void EncodeSet(uint8_t *dest) const {
		if (is256_) {
			tileset_.Encode256(dest);
		} else {
			tileset_.Encode16(dest);
		}
	}

private:
	std::vector<uint16_t> tiles_;
	Tileset &tileset_;
	int width_ = 0;
	int height_ = 0;
	bool is256_ = false;
};

class Chip {
public:
	Chip() {
	}

	explicit Chip(uint16_t a1, uint16_t b1, uint16_t a2, uint16_t b2);
	static Chip FromTilemap(const Tilemap &tilemap, int x, int y);

	void Encode(uint8_t *dest) const;

	bool Match(const Chip &other) const;

private:
	uint16_t tiles_[4];
};

class Chipset {
public:
	void InsertAt(uint16_t index, const Chip &chip);
	int FindOrAdd(const Chip &chip);

	size_t ByteSize() const {
		return chips_.size() * 8;
	}

	void Encode(uint8_t *dest) const {
		for (const Chip &chip : chips_) {
			chip.Encode(dest);
			dest += 8;
		}
	}

private:
	std::vector<Chip> chips_;
	std::vector<bool> free_;
};

class Chipmap {
public:
	void InsertChipAt(uint16_t index, const Chip &chip);
	bool FromTilemap(const Tilemap &tilemap);

	int Width() const {
		return width_;
	}

	int Height() const {
		return height_;
	}

	size_t ByteSizeMap() const {
		return chips_.size() * sizeof(uint16_t);
	}

	void EncodeMap(uint8_t *dest) const;

	size_t ByteSizeSet() const {
		return chipset_.ByteSize();
	}

	void EncodeSet(uint8_t *dest) const {
		chipset_.Encode(dest);
	}

private:
	std::vector<uint16_t> chips_;
	Chipset chipset_;
	int width_ = 0;
	int height_ = 0;
};

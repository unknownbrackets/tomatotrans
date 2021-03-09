#include <cstdint>
#include <cstdio>
#include <vector>
#include "lz77.h"
#include "tiles.h"
#include "util.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static Palette LoadPaletteAt(FILE *ta, uint32_t pos, uint8_t base, uint8_t count) {
	Palette pal(base, count);
	fseek(ta, pos, SEEK_SET);
	std::vector<uint16_t> buf;
	buf.resize(count * 16);
	fread(buf.data(), 2, count * 16, ta);
	pal.Load(base, count, buf.data());
	return pal;
}

static bool TilemapFromPNG(Tilemap &tilemap, const Palette &pal, const char *filename, bool is256) {
	int width, height, channels;
	uint8_t *image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
	if (!image) {
		fprintf(stderr, "Failed to load png: %s\n", filename);
		return false;
	}

	bool success = tilemap.FromImage(image, width, height, pal, is256);
	if (!success) {
		fprintf(stderr, "Failed to convert png: %s\n", filename);
	}
	stbi_image_free(image);
	return success;
}

static bool TilemapAndPaletteFromPNG(Tilemap &tilemap, Palette &pal, const char *filename, bool is256) {
	int width, height, channels;
	uint8_t *image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
	if (!image) {
		fprintf(stderr, "Failed to load png: %s\n", filename);
		return false;
	}

	bool success = true;
	if (is256) {
		success = pal.FromImage256(image, width, height);
	} else {
		success = pal.FromImage16(image, width, height);
	}
	if (!success) {
		fprintf(stderr, "Failed to convert png, too many colors: %s\n", filename);
	} else {
		success = tilemap.FromImage(image, width, height, pal, is256);
		if (!success) {
			fprintf(stderr, "Failed to convert png to tilemap: %s\n", filename);
		}
	}

	stbi_image_free(image);
	return success;
}

struct TilemapLayers {
	TilemapLayers(Tileset &tileset) : bg1(tileset), bg2(tileset), bg3(tileset) {
	}

	Tilemap bg1;
	Tilemap bg2;
	Tilemap bg3;
};

static bool TilemapsAndPaletteFromPNG(TilemapLayers &tilemaps, Palette &pal, const char *filename, bool is256) {
	char bgfilename[512];
	snprintf(bgfilename, sizeof(bgfilename), filename, 1);
	if (!TilemapAndPaletteFromPNG(tilemaps.bg1, pal, bgfilename, is256)) {
		return false;
	}
	snprintf(bgfilename, sizeof(bgfilename), filename, 2);
	if (!TilemapAndPaletteFromPNG(tilemaps.bg2, pal, bgfilename, is256)) {
		return false;
	}
	snprintf(bgfilename, sizeof(bgfilename), filename, 3);
	if (!TilemapAndPaletteFromPNG(tilemaps.bg3, pal, bgfilename, is256)) {
		return false;
	}
	return true;
}

static void WriteRelocs(FILE *ta, const uint32_t relocs[], size_t c, uint32_t ptr) {
	for (size_t i = 0; i < c; ++i) {
		fseek(ta, relocs[i], SEEK_SET);
		WriteLE32(ta, ptr | 0x08000000);
	}
}

static bool SaveTilemapAsChips(FILE *ta, const Tilemap &tilemap, const std::vector<uint32_t> &mapIDs, uint32_t bg, uint32_t oldSize, uint32_t chipmapPos, uint32_t chipsetPos, uint32_t &nextPos) {
	Chipmap chipmap;
	if (!chipmap.FromTilemap(tilemap)) {
		return false;
	}

	std::vector<uint8_t> buf;
	fseek(ta, chipmapPos, SEEK_SET);
	buf.resize(chipmap.ByteSizeMap());
	chipmap.EncodeMap(buf.data());
	fwrite(buf.data(), 1, buf.size(), ta);

	buf.resize(chipmap.ByteSizeSet());
	chipmap.EncodeSet(buf.data());

	if (buf.size() <= oldSize) {
		fseek(ta, chipsetPos, SEEK_SET);
	} else {
		nextPos = (nextPos + 3) & ~3;

		std::vector<uint32_t> relocations;
		for (uint32_t mapID : mapIDs) {
			relocations.push_back(0x0031E7B4 + 12 * mapID + (bg - 1) * 4);
		}
		WriteRelocs(ta, relocations.data(), relocations.size(), nextPos);

		fseek(ta, nextPos, SEEK_SET);
		nextPos += (uint32_t)buf.size();
	}
	return fwrite(buf.data(), 1, buf.size(), ta) == buf.size();
}

static bool SaveTilemap(FILE *ta, const Tilemap &tilemap, uint32_t oldSize, uint32_t oldPos, uint32_t &nextPos) {
	if (oldSize < tilemap.ByteSizeMap()) {
		return false;
	}

	std::vector<uint8_t> buf;
	fseek(ta, oldPos, SEEK_SET);
	buf.resize(tilemap.ByteSizeMap());
	tilemap.EncodeMap(buf.data());
	return fwrite(buf.data(), 1, buf.size(), ta) == buf.size();
}

static std::vector<uint8_t> CompressTileset(const Tileset &tileset, int depth) {
	std::vector<uint8_t> buf;
	if (depth == 16) {
		buf.resize(tileset.ByteSize16());
		tileset.Encode16(buf.data());
	} else {
		buf.resize(tileset.ByteSize256());
		tileset.Encode256(buf.data());
	}
	return compress_gba_lz77(buf, LZ77_VRAM_SAFE);
}

struct SaveTilesetParams {
	int depth;
	uint32_t oldSize;
	uint32_t oldPos;
	const uint32_t *relocs;
	size_t relocCount;
};

static bool SaveCompressedTileset(FILE *ta, const Tileset &tileset, const SaveTilesetParams &params, uint32_t &nextPos) {
	std::vector<uint8_t> compressed = CompressTileset(tileset, params.depth);
	if (compressed.size() > params.oldSize) {
		nextPos = (nextPos + 3) & ~3;

		// Relocate the pointers...
		WriteRelocs(ta, params.relocs, params.relocCount, nextPos);

		fseek(ta, nextPos, SEEK_SET);
		nextPos += (uint32_t)compressed.size();
	} else {
		fseek(ta, params.oldPos, SEEK_SET);
	}
	return fwrite(compressed.data(), 1, compressed.size(), ta) == compressed.size();
}

static bool SavePalette(FILE *ta, const Palette &pal, uint8_t base, uint8_t count, uint32_t pos) {
	std::vector<uint16_t> palbuf;
	fseek(ta, pos, SEEK_SET);
	palbuf.resize(count * 16);
	pal.Encode(palbuf.data(), base, count);
	return fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta) == palbuf.size();
}

static bool SaveNewPalette(FILE *ta, const Palette &pal, uint8_t base, uint8_t count, uint32_t &nextPos) {
	nextPos = (nextPos + 3) & ~3;
	if (SavePalette(ta, pal, base, count, nextPos)) {
		nextPos += count * 16 * (uint32_t)sizeof(uint16_t);
		return true;
	}
	return false;
}

static bool LoadCompressedTileset(FILE *ta, Tileset &tileset, uint32_t ptr, uint32_t sz, int count, const int *keep = nullptr) {
	std::vector<uint8_t> compressed;
	compressed.resize(sz);
	fseek(ta, ptr, SEEK_SET);
	if (fread(compressed.data(), 1, compressed.size(), ta) != compressed.size()) {
		return false;
	}
	std::vector<uint8_t> data = decompress_gba_lz77(compressed);
	if (data.empty()) {
		return false;
	}
	tileset.Load16(data.data(), count);

	if (keep != nullptr) {
		int nextKeep = 0;
		for (int i = 0; i < count; ++i) {
			if (i == keep[nextKeep]) {
				nextKeep++;
				continue;
			}
			tileset.Free(i);
		}
	}

	return true;
}

static bool InsertIntroMaps(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.
	Palette pal = LoadPaletteAt(ta, 0x00259D18, 0, 15);

	// One common tileset for each of the maps.
	Tileset tileset;

	Tilemap tilemap0000(tileset);
	if (!TilemapFromPNG(tilemap0000, pal, "images/map0000_bg3_eng.png", false)) {
		return false;
	}

	Tilemap tilemap0070(tileset);
	if (!TilemapFromPNG(tilemap0070, pal, "images/map0070_bg3_eng.png", false)) {
		return false;
	}

	Tilemap tilemap01CC(tileset);
	if (!TilemapFromPNG(tilemap01CC, pal, "images/map01CC_bg3_eng.png", false)) {
		return false;
	}

	// The tileset is now ready, let's compress.
	static const uint32_t relocations[]{
		0x0015BA3C + 4 * 0x0000,
		0x0015BA3C + 4 * 0x0070,
		0x0015BA3C + 4 * 0x01CC,
	};
	SaveTilesetParams params{ 16, 2816, 0x00186D30, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}
	// We could also update table at 0x0015B84C, which specifies the tileset size clearing.
	// But it's not important.

	if (!SaveTilemapAsChips(ta, tilemap0000, { 0x0000 }, 3, 544, 0x0027B864, 0x0032DF2C, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemap0070, { 0x0070 }, 3, 432, 0x0027B288, 0x0032DC5C, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemap01CC, { 0x0070 }, 3, 288, 0x0027B648, 0x0032DE0C, nextPos)) {
		return false;
	}
	return true;
}

static bool InsertSpillMaps(FILE *ta, uint32_t &nextPos) {
	// One common palette and tileset for each of the layers.
	// Only 9 and A are referenced by animations, but let's just load the whole thing.
	Palette pal = LoadPaletteAt(ta, 0x002566b8, 0, 15);
	Tileset tileset;

	// This is the lid above the entrance to Selemo's place.
	static const int keep[]{
		0x20B, 0x20C, 0x20D, 0x20E, 0x20F, 0x210, 0x211, 0x212, 0x213, 0x214, 0x215, 0x216,
		0x217, 0x218, 0x219, 0x21A, 0x21B, 0x21C, 0x21D, 0x21E, 0x21F, 0x220, 0x221, 0x222,
		0x223, 0x224, 0x225, 0x226, 0x227, 0x228, 0x229, 0x22A, 0x22B, 0x22C, 0x22D, 0x22E,
		// Is this even valid?  It looks like it points to tilemap data...
		0x251,
		0xFFF,
	};
	// Some of the tiles have overrides, we have to keep those.
	LoadCompressedTileset(ta, tileset, 0x0015c1f4, 12173, 0x240, keep);

	// Also used by 012C and 0158.
	TilemapLayers tilemaps0001(tileset);
	if (!TilemapsAndPaletteFromPNG(tilemaps0001, pal, "images/map0001_bg%d_eng.png", false)) {
		return false;
	}
	TilemapLayers tilemaps0002(tileset);
	if (!TilemapsAndPaletteFromPNG(tilemaps0002, pal, "images/map0002_bg%d_eng.png", false)) {
		return false;
	}

	// Add the animated tiles (not actually in our tileset.)
	// This is the cat that bobs its head.
	uint16_t tile = 0xA300;
	for (int y = 14; y <= 15; ++y) {
		for (int x = 15; x <= 17; ++x) {
			tilemaps0001.bg1.Override(x, y, tile++);
		}
	}

	static const uint32_t relocations[]{
		0x0015BA3C + 4 * 0x0001,
		0x0015BA3C + 4 * 0x0002,
		0x0015BA3C + 4 * 0x012C,
		0x0015BA3C + 4 * 0x0158,
	};
	SaveTilesetParams params{ 16, 12173, 0x0015c1f4, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}
	// We could also update table at 0x0015B84C, which specifies the tileset size clearing.
	// But it's not important.

	if (!SaveTilemapAsChips(ta, tilemaps0001.bg1, { 0x0001, 0x012C, 0x0158 }, 1, 168, 0x0026fa80, 0x0031fedc, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps0001.bg2, { 0x0001, 0x012C, 0x0158 }, 2, 664, 0x0026fc60, 0x0031ff84, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps0001.bg3, { 0x0001, 0x012C, 0x0158 }, 3, 984, 0x0026fe40, 0x0032021c, nextPos)) {
		return false;
	}

	if (!SaveTilemapAsChips(ta, tilemaps0002.bg1, { 0x0002 }, 1, 40, 0x00270020, 0x003205f4, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps0002.bg2, { 0x0002 }, 2, 528, 0x002701ec, 0x0032061c, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps0002.bg3, { 0x0002 }, 3, 1024, 0x002703b8, 0x0032082c, nextPos)) {
		return false;
	}

	if (!SavePalette(ta, pal, 0, 15, 0x002566b8)) {
		return false;
	}

	return true;
}

static bool InsertRockIsleMaps(FILE *ta, uint32_t &nextPos) {
	// One common palette and tileset for each of the layers.
	// Only the first 6 palettes have any data.
	Palette pal = LoadPaletteAt(ta, 0x0025B578, 0, 6);
	pal.Resize(0, 15);
	Tileset tileset;

	// This is the elevator door that uses overrides.
	static const int keep[]{
		0x081, 0x082, 0x0A0, 0x0A1, 0x0C0, 0x0C1,
		0x1FB, 0x1FC, 0x1FD, 0x1FE, 0x1FF, 0x200,
		0x201, 0x202, 0x203, 0x204, 0x205, 0x206,
		0x207, 0x208, 0x209, 0x20A, 0x20B, 0x20C,
		0xFFF,
	};
	// Some of the tiles have overrides, we have to keep those.
	LoadCompressedTileset(ta, tileset, 0x0019272C, 12064, 0x20C, keep);

	TilemapLayers tilemaps005F(tileset);
	if (!TilemapsAndPaletteFromPNG(tilemaps005F, pal, "images/map005F_bg%d_eng.png", false)) {
		return false;
	}

	// Plot some animation tile numbers, not actually in the tileset.
	// This part is for the sea in the background.
	for (int x = 50; x <= 65; x += 2) {
		tilemaps005F.bg3.Override(x + 0, 6, 0x4302);
		tilemaps005F.bg3.Override(x + 1, 6, 0x4303);
		tilemaps005F.bg3.Override(x + 0, 7, 0x4300);
		tilemaps005F.bg3.Override(x + 1, 7, 0x4301);
	}
	// Add bottom left corner, might not be important.
	tilemaps005F.bg3.Override(0, 18, 0x03FF);
	tilemaps005F.bg3.Override(1, 18, 0x03FF);
	tilemaps005F.bg3.Override(0, 19, 0x03FF);
	tilemaps005F.bg3.Override(1, 19, 0x03FF);

	// The tileset is now ready, let's compress.
	static const uint32_t relocations[]{
		0x0015BA3C + 4 * 0x005F,
	};
	SaveTilesetParams params{ 16, 12064, 0x0019272C, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}
	// We could also update table at 0x0015B84C, which specifies the tileset size clearing.
	// But it's not important.

	if (!SaveTilemapAsChips(ta, tilemaps005F.bg1, { 0x005F }, 1, 280, 0x0027d9d0, 0x003311d4, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps005F.bg2, { 0x005F }, 2, 616, 0x0027dc64, 0x003312ec, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemaps005F.bg3, { 0x005F }, 3, 744, 0x0027def8, 0x00331554, nextPos)) {
		return false;
	}

	if (!SavePalette(ta, pal, 0, 15, 0x0025b578)) {
		return false;
	}

	return true;
}

static bool InsertDefeatScreen(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.  Should we?
	Palette pal = LoadPaletteAt(ta, 0x0063FF98, 0, 16);

	// One common tileset for each version of the screen and each BG.
	Tileset tileset;
	std::vector<uint8_t> buf;
	fseek(ta, 0x0063B8D0, SEEK_SET);
	buf.resize(0x3400);
	fread(buf.data(), 1, buf.size(), ta);
	tileset.Load16(buf.data(), 0xE2);

	// These aren't used by the animation, clear 'em out.
	for (int i = 0x3e; i < 0x4b; i++) {
		tileset.Free(i);
	}
	for (int i = 0x4f; i < 0x60; i++) {
		tileset.Free(i);
	}
	for (int i = 0x64; i < 0x74; i++) {
		tileset.Free(i);
	}
	for (int i = 0x7A; i < 0x85; i++) {
		tileset.Free(i);
	}
	for (int i = 0x88; i < 0x93; i++) {
		tileset.Free(i);
	}
	for (int i = 0x96; i < 0xBD; i++) {
		tileset.Free(i);
	}
	for (int i = 0xC0; i < 0xCE; i++) {
		tileset.Free(i);
	}

	Tilemap regularBG(tileset);
	if (!TilemapFromPNG(regularBG, pal, "images/defeat_bg_eng.png", false)) {
		return false;
	}

	Tilemap regularOptions(tileset);
	if (!TilemapFromPNG(regularOptions, pal, "images/defeat_options_eng.png", false)) {
		return false;
	}

	Tilemap nosaveBG(tileset);
	if (!TilemapFromPNG(nosaveBG, pal, "images/defeat_nosave_bg_eng.png", false)) {
		return false;
	}

	Tilemap nosaveOptions(tileset);
	if (!TilemapFromPNG(nosaveOptions, pal, "images/defeat_nosave_options_eng.png", false)) {
		return false;
	}

	// The tileset is now ready, leave it uncompressed?
	buf.resize(tileset.ByteSize16());
	tileset.Encode16(buf.data());
	if (buf.size() > 0x3400) {
		nextPos = (nextPos + 3) & ~3;

		static const uint32_t relocations[]{ 0x0002A684 };
		WriteRelocs(ta, relocations, std::size(relocations), nextPos);

		fseek(ta, nextPos, SEEK_SET);
		nextPos += (uint32_t)buf.size();
	} else {
		fseek(ta, 0x0063B8D0, SEEK_SET);
	}
	fwrite(buf.data(), 1, buf.size(), ta);

	if (!SaveTilemap(ta, regularBG, 0x04B0, 0x0063ECD2, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, regularOptions, 0x04B0, 0x0063F184, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, nosaveBG, 0x04B0, 0x0063F636, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, nosaveOptions, 0x04B0, 0x0063FAE8, nextPos)) {
		return false;
	}
	return true;
}

static bool LoadTitleScreenTilemapsAndPalette(Palette &pal, Tilemap &logo, Tilemap &logoShadow, Tilemap &logoBG, Tilemap &logoLoop) {
	if (!TilemapAndPaletteFromPNG(logo, pal, "images/title_logo_eng.png", true)) {
		return false;
	}
	if (!TilemapAndPaletteFromPNG(logoShadow, pal, "images/title_logo_shadow_eng.png", true)) {
		return false;
	}
	if (!TilemapAndPaletteFromPNG(logoBG, pal, "images/title_logo_bg_eng.png", true)) {
		return false;
	}
	// Used at the end of the animation loop.  Should just use the same tiles...
	if (!TilemapAndPaletteFromPNG(logoLoop, pal, "images/title_loop_logo_eng.png", true)) {
		return false;
	}
	return true;
}

static bool InsertTitleScreen(FILE *ta, uint32_t &nextPos) {
	Palette pal(8, 8);

	// One common tileset for each.
	Tileset tileset;
	Tilemap logo(tileset);
	Tilemap logoShadow(tileset);
	Tilemap logoBG(tileset);
	Tilemap logoLoop(tileset);
	if (!LoadTitleScreenTilemapsAndPalette(pal, logo, logoShadow, logoBG, logoLoop)) {
		pal.Resize(7, 9);
		tileset.Clear();

		// Try again with the larger palette.
		if (!LoadTitleScreenTilemapsAndPalette(pal, logo, logoShadow, logoBG, logoLoop)) {
			return false;
		}
	}

	// The tileset is now ready, let's compress.
	static const uint32_t relocations[]{ 0x00082A18, 0x000863E8 };
	SaveTilesetParams params{ 256, 9460, 0x0049FAB8, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}

	if (pal.TotalUsage() <= 128) {
		if (!SavePalette(ta, pal, 8, 8, 0x0045BB64)) {
			return false;
		}
	} else {
		static const uint32_t relocationPtrs[]{ 0x00082A20, 0x000863D0 };
		static const uint32_t relocationDests[]{ 0x00082A24, 0x000863D4 };
		static const uint32_t relocationSizes[]{ 0x00082A28, 0x000863D8 };
		nextPos = (nextPos + 3) & ~3;
		WriteRelocs(ta, relocationPtrs, std::size(relocationPtrs), nextPos);
		for (uint32_t p : relocationDests) {
			fseek(ta, p, SEEK_SET);
			WriteLE32(ta, 0x050000E0);
		}
		for (uint32_t p : relocationSizes) {
			fseek(ta, p, SEEK_SET);
			WriteLE32(ta, 0x80000090);
		}
		if (!SaveNewPalette(ta, pal, 7, 9, nextPos)) {
			return false;
		}
	}

	if (!SaveTilemap(ta, logo, 0x0500, 0x0045C384, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, logoShadow, 0x0500, 0x0045C884, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, logoBG, 0x0500, 0x0045CD84, nextPos)) {
		return false;
	}
	if (!SaveTilemap(ta, logoLoop, 0x0500, 0x0045F94C, nextPos)) {
		return false;
	}
	return true;
}

static bool InsertTitleButtons(FILE *ta, uint32_t &nextPos) {
	Palette pal(0, 1);

	// One common tileset for each button.
	Tileset unorderedTileset(false);

	Tilemap newButton(unorderedTileset);
	if (!TilemapAndPaletteFromPNG(newButton, pal, "images/title_new_eng.png", true)) {
		return false;
	}

	Tilemap loadButton(unorderedTileset);
	if (!TilemapAndPaletteFromPNG(loadButton, pal, "images/title_load_eng.png", true)) {
		return false;
	}

	Tilemap deleteButton(unorderedTileset);
	if (!TilemapAndPaletteFromPNG(deleteButton, pal, "images/title_delete_eng.png", true)) {
		return false;
	}

	// The tiles need to be in a specific order for the sprite defs in title_screen.asm.
	Tileset tileset(false);
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 8; ++x) {
			tileset.Add(unorderedTileset.At(newButton.At(x, y) & 0x3FF));
		}
	}
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 8; ++x) {
			tileset.Add(unorderedTileset.At(loadButton.At(x, y) & 0x3FF));
		}
	}
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 8; ++x) {
			tileset.Add(unorderedTileset.At(deleteButton.At(x, y) & 0x3FF));
		}
	}

	// The tileset is now ready, let's compress.
	static const uint32_t relocations[]{ 0x00082F44 };
	SaveTilesetParams params{ 16, 1056, 0x0049F698, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}

	// And also insert the updated palette.
	if (!SavePalette(ta, pal, 0, 1, 0x0045BB04)) {
		return false;
	}

	return true;
}

static bool InsertGimmickMenuIcons(FILE *ta, uint32_t &nextPos) {
	Palette pal(7, 1);

	Tileset tileset;
	Tilemap tilemap(tileset);
	if (!TilemapAndPaletteFromPNG(tilemap, pal, "images/gimmick_types_eng.png", false)) {
		return false;
	}

	// The tileset is now ready, leave it uncompressed.
	std::vector<uint8_t> buf;
	buf.resize(tileset.ByteSize16());
	tileset.Encode16(buf.data());
	if (buf.size() > 0x0280) {
		// Okay, need to relocate.  Not supported yet.
		fprintf(stderr, "Relocate?");
		return false;
	} else {
		fseek(ta, 0x00488208, SEEK_SET);
		fwrite(buf.data(), 1, buf.size(), ta);
	}

	if (0x0028 < tilemap.ByteSizeMap()) {
		return false;
	}

	fseek(ta, 0x0048D710, SEEK_SET);
	buf.resize(tilemap.ByteSizeMap());
	tilemap.EncodeMap(buf.data());
	// Before we write it, offset each tile by 0x7C.  It can't be more than 20 tiles.
	// The game loads the tileset at an offset.
	for (size_t i = 0; i < buf.size(); i += 2) {
		buf[i] += 0x007C;
	}
	fwrite(buf.data(), 1, buf.size(), ta);

	// And also insert the updated palette.
	if (!SavePalette(ta, pal, 7, 1, 0x00489308)) {
		return false;
	}

	return true;
}

static bool InsertGimmickBattleIcons(FILE *ta, uint32_t &nextPos) {
	// This palette is shared by several other gimmicks.
	Palette pal = LoadPaletteAt(ta, 0x0051F5D4, 0, 1);

	Tileset unorderedCraneTileset(false);
	Tileset unorderedCatTileset(false);

	Tilemap craneTilemap(unorderedCraneTileset);
	if (!TilemapAndPaletteFromPNG(craneTilemap, pal, "images/crane_panels_eng.png", true)) {
		return false;
	}

	Tilemap catTilemap(unorderedCatTileset);
	if (!TilemapAndPaletteFromPNG(catTilemap, pal, "images/luckycat_panels_eng.png", true)) {
		return false;
	}

	// The tiles need to be in a specific order for the gimmick battle UI.
	Tileset craneTileset(false);
	Tileset catTileset(false);
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 2; ++x) {
			craneTileset.Add(unorderedCraneTileset.At(craneTilemap.At(x, y) & 0x3FF));
		}
	}
	for (int y = 0; y < 6; ++y) {
		for (int x = 0; x < 16; ++x) {
			catTileset.Add(unorderedCatTileset.At(catTilemap.At(x, y) & 0x3FF));
		}
	}

	// The tileset is now ready, let's compress.
	static const uint32_t craneRelocs[]{ 0x00599058 };
	SaveTilesetParams craneParams{ 16, 164, 0x00599058, craneRelocs, std::size(craneRelocs) };
	if (!SaveCompressedTileset(ta, craneTileset, craneParams, nextPos)) {
		return false;
	}

	static const uint32_t catRelocs[]{ 0x0005CFE0, 0x0005D0F8 };
	SaveTilesetParams catParams{ 16, 1011, 0x0059B990, catRelocs, std::size(catRelocs) };
	if (!SaveCompressedTileset(ta, catTileset, catParams, nextPos)) {
		return false;
	}

	return true;
}

static bool InsertGimicaCard(FILE *ta, uint32_t &nextPos) {
	Palette pal = LoadPaletteAt(ta, 0x00485BD0, 0, 1);

	Tileset unorderedTileset(false);
	Tilemap card(unorderedTileset);
	if (!TilemapFromPNG(card, pal, "images/gimica_card_eng.png", false)) {
		return false;
	}

	// The tiles need to be in a specific order for the sprite defs used by the game at 0x00480B68.
	Tileset tileset(false);
	// First, the top left 4x4.
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			tileset.Add(unorderedTileset.At(card.At(x, y) & 0x3FF));
		}
	}
	// Then top right 2x4.
	for (int y = 0; y < 4; ++y) {
		for (int x = 4; x < 6; ++x) {
			tileset.Add(unorderedTileset.At(card.At(x, y) & 0x3FF));
		}
	}
	// The next row is 4x1, then 2x1, essentially linear.
	for (int y = 4; y < 5; ++y) {
		for (int x = 0; x < 6; ++x) {
			tileset.Add(unorderedTileset.At(card.At(x, y) & 0x3FF));
		}
	}
	// Almost done, 4x2 in the bottom left corner.
	for (int y = 5; y < 7; ++y) {
		for (int x = 0; x < 4; ++x) {
			tileset.Add(unorderedTileset.At(card.At(x, y) & 0x3FF));
		}
	}
	// Finally, 2x2 bottom right corner.
	for (int y = 5; y < 7; ++y) {
		for (int x = 4; x < 6; ++x) {
			tileset.Add(unorderedTileset.At(card.At(x, y) & 0x3FF));
		}
	}

	// The tileset is now ready, let's compress.
	static const uint32_t relocations[]{ 0x0007E804, 0x000801C8, 0x000819A0, 0x0008BC14 };
	SaveTilesetParams params{ 16, 801, 0x004858B4, relocations, std::size(relocations) };
	if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
		return false;
	}

	return true;
}

static bool InsertShopBanners(FILE *ta, uint32_t &nextPos) {
	static const uint32_t originalSizes[]{ 529, 438, 625, 532, 441, 519, 411, 337, 286, 587 };
	static const uint32_t tilesetPtrBase = 0x0064A7AC;
	static const uint32_t palettePtrBase = 0x0064A7D4;

	for (int i = 0; i < 10; ++i) {
		uint32_t tilesetPtr = 0;
		fseek(ta, tilesetPtrBase + i * 4, SEEK_SET);
		ReadLE32(ta, tilesetPtr);
		uint32_t palettePtr = 0;
		fseek(ta, palettePtrBase + i * 4, SEEK_SET);
		ReadLE32(ta, palettePtr);

		Palette pal(0, 1);

		char filename[64];
		snprintf(filename, sizeof(filename), "images/shop_%02X_eng.png", i);

		Tileset unorderedTileset(false);
		Tilemap banner(unorderedTileset);
		if (!TilemapAndPaletteFromPNG(banner, pal, filename, false)) {
			return false;
		}

		// The tiles are in a specific order, we duplicate the blank in the bottom left.
		Tileset tileset(false);
		for (int x = 0; x < 11; ++x) {
			tileset.Add(unorderedTileset.At(banner.At(x, 0) & 0x3FF));
		}
		// Just skip that one tile.
		for (int x = 1; x < 11; ++x) {
			tileset.Add(unorderedTileset.At(banner.At(x, 1) & 0x3FF));
		}

		// The tileset is now ready, let's compress.
		const uint32_t relocations[]{ tilesetPtrBase + i * 4 };
		SaveTilesetParams params{ 16, originalSizes[i], tilesetPtr & ~0x08000000, relocations, std::size(relocations) };
		if (!SaveCompressedTileset(ta, tileset, params, nextPos)) {
			return false;
		}

		// And also insert the updated palette.
		if (!SavePalette(ta, pal, 0, 1, palettePtr & ~0x08000000)) {
			return false;
		}
	}

	return true;
}

bool InsertImages(FILE *ta, uint32_t &nextPos) {
	struct Inserter {
		bool (* f)(FILE *ta, uint32_t &nextPos);
		const char *desc;
	};
	constexpr static Inserter inserters[] = {
		{ InsertIntroMaps, "intro maps" },
		{ InsertSpillMaps, "Spill maps" },
		{ InsertRockIsleMaps, "Rock Isle maps" },
		{ InsertDefeatScreen, "defeat screen" },
		{ InsertTitleScreen, "title screen" },
		{ InsertTitleButtons, "title buttons" },
		{ InsertGimmickMenuIcons, "gimmick menu icons" },
		{ InsertGimmickBattleIcons, "gimmick battle icons" },
		{ InsertGimicaCard, "gimica card" },
		{ InsertShopBanners, "shop banners" },
	};

	bool failed = false;
	for (const Inserter &inserter : inserters) {
		if (!inserter.f(ta, nextPos)) {
			printf("Failed to insert %s\n", inserter.desc);
			failed = true;
		}
	}

	return !failed;
}

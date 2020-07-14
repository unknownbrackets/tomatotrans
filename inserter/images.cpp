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
	Palette pal;
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

static bool SaveTilemapAsChips(FILE *ta, const Tilemap &tilemap, int mapID, uint32_t oldSize, uint32_t chipmapPos, uint32_t chipsetPos, uint32_t &nextPos) {
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
		fwrite(buf.data(), 1, buf.size(), ta);
	} else {
		nextPos = (nextPos + 3) & ~3;
		fseek(ta, nextPos, SEEK_SET);
		fwrite(buf.data(), 1, buf.size(), ta);

		fseek(ta, 0x0031E7B4 + 12 * mapID + 8, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);

		nextPos += (uint32_t)buf.size();
	}

	return true;
}

static bool SaveTilemap(FILE *ta, const Tilemap &tilemap, uint32_t oldSize, uint32_t oldPos, uint32_t &nextPos) {
	if (oldSize < tilemap.ByteSizeMap()) {
		return false;
	}

	std::vector<uint8_t> buf;
	fseek(ta, oldPos, SEEK_SET);
	buf.resize(tilemap.ByteSizeMap());
	tilemap.EncodeMap(buf.data());
	fwrite(buf.data(), 1, buf.size(), ta);

	return true;
}

bool InsertIntroMaps(FILE *ta, uint32_t &nextPos) {
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
	std::vector<uint8_t> buf;
	buf.resize(tileset.ByteSize16());
	tileset.Encode16(buf.data());
	std::vector<uint8_t> compressed = compress_gba_lz77(buf, LZ77_VRAM_SAFE);
	if (compressed.size() > 2816) {
		nextPos = (nextPos + 3) & ~3;
		// Relocate the pointers...
		fseek(ta, 0x0015BA3C + 4 * 0x0000, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);
		fseek(ta, 0x0015BA3C + 4 * 0x0070, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);
		fseek(ta, 0x0015BA3C + 4 * 0x01CC, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);

		fseek(ta, nextPos, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
		nextPos += (uint32_t)compressed.size();
	} else {
		fseek(ta, 0x00186D30, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
	}

	if (!SaveTilemapAsChips(ta, tilemap0000, 0x0000, 544, 0x0027B864, 0x0032DF2C, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemap0070, 0x0070, 432, 0x0027B288, 0x0032DC5C, nextPos)) {
		return false;
	}
	if (!SaveTilemapAsChips(ta, tilemap01CC, 0x0070, 288, 0x0027B648, 0x0032DE0C, nextPos)) {
		return false;
	}
	return true;
}

bool InsertDefeatScreen(FILE *ta, uint32_t &nextPos) {
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
		// Okay, need to relocate.
		nextPos = (nextPos + 3) & ~3;
		fseek(ta, 0x0002A684, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);

		fseek(ta, nextPos, SEEK_SET);
		fwrite(buf.data(), 1, buf.size(), ta);
		nextPos += (uint32_t)buf.size();
	} else {
		fseek(ta, 0x0063B8D0, SEEK_SET);
		fwrite(buf.data(), 1, buf.size(), ta);
	}

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

bool InsertTitleScreen(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.  Should we?
	Palette pal = LoadPaletteAt(ta, 0x0045BB64, 8, 8);

	// One common tileset for each.
	Tileset tileset;

	Tilemap logo(tileset);
	if (!TilemapFromPNG(logo, pal, "images/title_logo_eng.png", true)) {
		return false;
	}

	Tilemap logoShadow(tileset);
	if (!TilemapFromPNG(logoShadow, pal, "images/title_logo_shadow_eng.png", true)) {
		return false;
	}

	Tilemap logoBG(tileset);
	if (!TilemapFromPNG(logoBG, pal, "images/title_logo_bg_eng.png", true)) {
		return false;
	}

	// Used at the end of the animation loop.  Should just use the same tiles...
	Tilemap logoNoCopyrightBG(tileset);
	if (!TilemapFromPNG(logoNoCopyrightBG, pal, "images/title_loop_logo_eng.png", true)) {
		return false;
	}

	// The tileset is now ready, let's compress.
	std::vector<uint8_t> buf;
	buf.resize(tileset.ByteSize256());
	tileset.Encode256(buf.data());
	std::vector<uint8_t> compressed = compress_gba_lz77(buf, LZ77_VRAM_SAFE);
	if (compressed.size() > 9460) {
		// Okay, need to relocate.
		nextPos = (nextPos + 3) & ~3;
		fseek(ta, 0x00082A18, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);
		fseek(ta, 0x000863E8, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);

		fseek(ta, nextPos, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
		nextPos += (uint32_t)compressed.size();
	} else {
		fseek(ta, 0x0049FAB8, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
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
	if (!SaveTilemap(ta, logoNoCopyrightBG, 0x0500, 0x0045F94C, nextPos)) {
		return false;
	}
	return true;
}

bool InsertTitleButtons(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.  Should we?
	Palette pal = LoadPaletteAt(ta, 0x0045BB04, 0, 1);

	// One common tileset for each button.
	Tileset unorderedTileset;

	Tilemap newButton(unorderedTileset);
	if (!TilemapFromPNG(newButton, pal, "images/title_new_eng.png", true)) {
		return false;
	}

	Tilemap loadButton(unorderedTileset);
	if (!TilemapFromPNG(loadButton, pal, "images/title_load_eng.png", true)) {
		return false;
	}

	Tilemap deleteButton(unorderedTileset);
	if (!TilemapFromPNG(deleteButton, pal, "images/title_delete_eng.png", true)) {
		return false;
	}

	// The tiles need to be in a specific order for the sprite defs in title_screen.asm.
	Tileset tileset;
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
	std::vector<uint8_t> buf;
	buf.resize(tileset.ByteSize16());
	tileset.Encode16(buf.data());
	std::vector<uint8_t> compressed = compress_gba_lz77(buf, LZ77_VRAM_SAFE);
	if (compressed.size() > 1056) {
		// Okay, need to relocate.
		nextPos = (nextPos + 3) & ~3;
		fseek(ta, 0x00082F44, SEEK_SET);
		WriteLE32(ta, nextPos | 0x08000000);

		fseek(ta, nextPos, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
		nextPos += (uint32_t)compressed.size();
	} else {
		fseek(ta, 0x0049F698, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
	}

	return true;
}

bool InsertGimmickMenuIcons(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.  Should we?
	Palette pal = LoadPaletteAt(ta, 0x00489308, 7, 1);

	Tileset tileset;
	Tilemap tilemap(tileset);
	if (!TilemapFromPNG(tilemap, pal, "images/gimmick_types_eng.png", false)) {
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

	return true;
}

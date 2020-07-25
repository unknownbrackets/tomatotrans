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

	if (pal.TotalUsage() <= 128) {
		std::vector<uint16_t> palbuf;
		fseek(ta, 0x0045BB64, SEEK_SET);
		palbuf.resize(128);
		pal.Encode(palbuf.data(), 8, 8);
		fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta);
	} else {
		static const uint32_t relocationPtrs[]{ 0x00082A20, 0x000863D0 };
		static const uint32_t relocationDests[]{ 0x00082A24, 0x000863D4 };
		static const uint32_t relocationSizes[]{ 0x00082A28, 0x000863D8 };
		nextPos = (nextPos + 3) & ~3;
		for (uint32_t p : relocationPtrs) {
			fseek(ta, p, SEEK_SET);
			WriteLE32(ta, nextPos | 0x08000000);
		}
		for (uint32_t p : relocationDests) {
			fseek(ta, p, SEEK_SET);
			WriteLE32(ta, 0x050000E0);
		}
		for (uint32_t p : relocationSizes) {
			fseek(ta, p, SEEK_SET);
			WriteLE32(ta, 0x80000090);
		}
		std::vector<uint16_t> palbuf;
		fseek(ta, nextPos, SEEK_SET);
		palbuf.resize(9 * 16);
		pal.Encode(palbuf.data(), 7, 9);
		fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta);
		nextPos += palbuf.size() * sizeof(uint16_t);
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

	// And also insert the updated palette.
	std::vector<uint16_t> palbuf;
	palbuf.resize(16);
	pal.Encode(palbuf.data(), 0, 1);
	fseek(ta, 0x0045BB04, SEEK_SET);
	fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta);

	return true;
}

static bool InsertGimmickMenuIcons(FILE *ta, uint32_t &nextPos) {
	// We don't change the palette, just reuse.  Should we?
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
	std::vector<uint16_t> palbuf;
	palbuf.resize(16);
	pal.Encode(palbuf.data(), 7, 1);
	fseek(ta, 0x00489308, SEEK_SET);
	fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta);

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
	std::vector<uint8_t> buf;
	buf.resize(tileset.ByteSize16());
	tileset.Encode16(buf.data());
	std::vector<uint8_t> compressed = compress_gba_lz77(buf, LZ77_VRAM_SAFE);
	if (compressed.size() > 801) {
		// Okay, need to relocate.
		static const uint32_t relocations[]{ 0x0007E804, 0x000801C8, 0x000819A0, 0x0008BC14 };

		nextPos = (nextPos + 3) & ~3;
		for (uint32_t reloc : relocations)
		{
			fseek(ta, reloc, SEEK_SET);
			WriteLE32(ta, nextPos | 0x08000000);
		}

		fseek(ta, nextPos, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
		nextPos += (uint32_t)compressed.size();
	} else {
		fseek(ta, 0x004858B4, SEEK_SET);
		fwrite(compressed.data(), 1, compressed.size(), ta);
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
		std::vector<uint8_t> buf;
		buf.resize(tileset.ByteSize16());
		tileset.Encode16(buf.data());
		std::vector<uint8_t> compressed = compress_gba_lz77(buf, LZ77_VRAM_SAFE);
		if (compressed.size() > originalSizes[i]) {
			// Okay, need to relocate.
			nextPos = (nextPos + 3) & ~3;
			fseek(ta, tilesetPtrBase + i * 4, SEEK_SET);
			WriteLE32(ta, nextPos | 0x08000000);

			fseek(ta, nextPos, SEEK_SET);
			fwrite(compressed.data(), 1, compressed.size(), ta);
			nextPos += (uint32_t)compressed.size();
		} else {
			fseek(ta, tilesetPtr & ~0x08000000, SEEK_SET);
			fwrite(compressed.data(), 1, compressed.size(), ta);
		}

		// And also insert the updated palette.
		std::vector<uint16_t> palbuf;
		palbuf.resize(16);
		pal.Encode(palbuf.data(), 0, 1);
		fseek(ta, palettePtr & ~0x08000000, SEEK_SET);
		fwrite(palbuf.data(), sizeof(uint16_t), palbuf.size(), ta);
	}

	return true;
}

bool InsertImages(FILE *ta, uint32_t &nextPos) {
	bool failed = false;
	if (!InsertIntroMaps(ta, nextPos)) {
		printf("Failed to insert intro maps\n");
		failed = true;
	}
	if (!InsertDefeatScreen(ta, nextPos)) {
		printf("Failed to insert defeat screen\n");
		failed = true;
	}
	if (!InsertTitleScreen(ta, nextPos)) {
		printf("Failed to insert title screen\n");
		failed = true;
	}
	if (!InsertTitleButtons(ta, nextPos)) {
		printf("Failed to insert title buttons\n");
		failed = true;
	}
	if (!InsertGimmickMenuIcons(ta, nextPos)) {
		printf("Failed to insert gimmick menu icons\n");
		failed = true;
	}
	if (!InsertGimicaCard(ta, nextPos)) {
		printf("Failed to insert gimica card\n");
		failed = true;
	}
	if (!InsertShopBanners(ta, nextPos)) {
		printf("Failed to insert shop banners\n");
		failed = true;
	}
	return !failed;
}

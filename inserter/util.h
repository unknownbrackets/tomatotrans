#include <cstdint>
#include <cstdio>

inline bool WriteLE32(FILE *fout, uint32_t value) {
	uint8_t parts[4];
	parts[0] = (value & 0x000000FF) >> 0;
	parts[1] = (value & 0x0000FF00) >> 8;
	parts[2] = (value & 0x00FF0000) >> 16;
	parts[3] = (value & 0xFF000000) >> 24;

	return fwrite(parts, 1, 4, fout) == 4;
}

inline bool ReadLE32(FILE *fin, uint32_t &value) {
	uint8_t parts[4];
	if (fread(parts, 1, 4, fin) != 4)
		return false;

	value = parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
	return true;
}

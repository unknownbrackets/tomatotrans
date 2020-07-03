#include <cstdint>
#include <cstdio>
#include <cstring>
#include "images.h"
#include "text.h"

static const uint32_t TOMATO_END_POS = 0x0064B4EC;
static const uint32_t TOMATO_SIZE = 0x007F0000;

int main(int argc, char **argv)
{
	if (argc == 2 && !strcmp(argv[1], "--force"))
		forceAll = true;

	LoadTable();

	FILE *fout = fopen("test.gba", "rb+");
	if (!fout)
	{
		printf("Couldn't open test.gba, make sure you ran armips\n");
		return 1;
	}

	uint32_t totalInsertions = 0;
	uint32_t afterTextPos = TOMATO_END_POS;
	totalInsertions += InsertMainScript(fout, afterTextPos);
	totalInsertions += InsertMenuStuff2(fout, afterTextPos);
	totalInsertions += InsertEnemies(fout, afterTextPos);
	totalInsertions += InsertBattleText(fout, afterTextPos);

	if (InsertIntroMaps(fout, afterTextPos)) {
		printf("Inserted updated intro maps\n");
	}
	if (InsertDefeatScreen(fout, afterTextPos)) {
		printf("Inserted updated defeat screen\n");
	}
	if (InsertTitleScreen(fout, afterTextPos)) {
		printf("Inserted updated title screen\n");
	}
	if (InsertTitleButtons(fout, afterTextPos)) {
		printf("Inserted updated title buttons\n");
	}

	uint32_t usedBytes = afterTextPos - TOMATO_END_POS;
	uint32_t usedPercent = 100 * usedBytes / (TOMATO_SIZE - TOMATO_END_POS);
	printf("Inserted %u strings, using %u extra bytes (%03u%%)\n", totalInsertions, usedBytes, usedPercent);

	fclose(fout);
	return 0;
}

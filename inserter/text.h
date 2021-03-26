#include <cstdint>
#include <cstdio>

extern bool forceAll;

void LoadTable();

uint32_t InsertMainScript(FILE *, uint32_t &afterTextPos);
uint32_t InsertEnemies(FILE *, uint32_t &afterTextPos);
uint32_t InsertMenuStuff2(FILE *, uint32_t &afterTextPos);
uint32_t InsertBattleText(FILE *, uint32_t &afterTextPos);
uint32_t InsertGimicaTutorialText(FILE *, uint32_t &afterTextPos);
uint32_t InsertCreditsText(FILE *, uint32_t &afterTextPos);

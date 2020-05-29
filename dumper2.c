#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct tableEntry
{
	int  hexVal;
	char str[500];
};

tableEntry table[500];
int        tableLen = 0;
tableEntry kanjiTable[500];
int        kanjiTableLen = 0;
tableEntry smallTable[500];
int        smallTableLen = 0;
int        line = 0;
FILE*      fptrs;

void 		 ConvChar(unsigned char, char[], FILE*, FILE*, bool);
void         ConvCharSimple(unsigned char, char[]);
void         ConvCharSimple2(FILE*, unsigned char, char[], int&);
void 		 LoadTable(void);
void 		 LoadKanjiTable(void);
void         LoadSmallTable(void);
void         FindPointers(int, FILE*, FILE*);
unsigned int hstrtoi(char* string);
int          CharToHex(char);
void         DumpStuff1(void);
void         DumpEnemyNames(void);
void         DumpMenuStuff1(void);
void         DumpStuff(char[], char[], int, int, int);
void         DumpStuff2(char[], char[], int, int, int);


//=================================================================================================

int main(void)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	char          letter[1000] = "";
	int           loc;
	int           i;
	int           len;
	int           temp;

	LoadTable();
	LoadKanjiTable();
	LoadSmallTable();

	fin = fopen("ta.gba", "rb");

	//fptrs = fopen("pointers1.dat", "w");

    /*loc = 0x5DC;
	fseek (fin, loc, SEEK_SET);
    fout = fopen("ta_script_jpn.txt", "w");
	fprintf(fout, "LINE: %03X  ADDR: %08X\n", line, loc);
	//FindPointers(loc, fin, fout);
    while (ftell(fin) < 0x209D5)
	{
		ch = fgetc(fin);
		ConvChar(ch, letter, fin, fout, false);
	}
	fclose(fout);
	//fclose(fptrs);

    //fptrs = fopen("pointers2.dat", "w");
    loc = 0xF9600;
    line = 0;
	fseek (fin, loc, SEEK_SET);
    fout = fopen("ta_misc_jpn.txt", "w");
	fprintf(fout, "LINE: %03X  ADDR: %08X\n", line, loc);
	//FindPointers(loc, fin, fout);
    while (ftell(fin) < 0xF992B)
	{
		ch = fgetc(fin);
		ConvChar(ch, letter, fin, fout, false);
	}
	fclose(fout);*/
	//fclose(fptrs);

/*        fout = fopen("ta_script_eng.txt", "w");
	for (i = 0; i < 0xBD7; i++)
	   fprintf(fout, "%03X-E:\n\n", i);
	fclose(fout);

	fout = fopen("ta_misc_eng.txt", "w");
	for (i = 0; i < 0x24; i++)
	   fprintf(fout, "%03X-E:\n\n", i);
        fclose(fout);*/

    //DumpStuff1();
    //DumpEnemyNames();
    //DumpMenuStuff1();
    //DumpMenuStuff2();
    /*DumpStuff("ta_jpn_stuff1.txt", "ta_eng_stuff1.txt", 0x457226, 0x6, 0x7);
    DumpStuff("ta_jpn_stuff2.txt", "ta_eng_stuff2.txt", 0x457220, 0x6, 0x1);
	DumpStuff("ta_jpn_stuff3.txt", "ta_eng_stuff3.txt", 0x457350, 0xC, 0x1);
	DumpStuff("ta_jpn_ta_menus_eng.txt", "ta_eng_ta_menus_eng.txt", 0x459F30, 0x8, 0x100);*/

	/*for (i = 0; i < 95; i++)
	{
		fseek(fin, 0x4903C0 + i * 8, SEEK_SET);
		printf("%02X %08X\n", i, 0x4903C0 + i * 8);

		temp = fgetc(fin);
		temp |= (fgetc(fin) << 8);
		temp |= (fgetc(fin) << 16);
		temp |= (fgetc(fin) << 24);
		temp -= 0x8000000;

		len = fgetc(fin);
		ch = fgetc(fin);

        DumpStuff("crazydump_jpn.txt", "boo.txt", temp, len, ch);
	}*/

    fout=fopen("ta_menus_jpn.txt", "w");
    fclose(fout);
    //fout=fopen("ta_menus_eng.txt", "w");
    //fclose(fout);


	//DumpStuff("ta_items_jpn.txt", "ta_items_eng.txt", 0x4573F2, 8, 0xB7);



	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x457226, 0x6, 0x17);  // good, various names
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x457220, 0x6, 0x1);   // good, but no change needed, just "??????"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x457350, 0xC, 0x1);   // "you have no items"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45735C, 0x8, 0x5);   // good, status stuff?
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x457384, 0x8, 0xA);   // good, status effect text
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4573D4, 0x5, 0x6);   // good, machine parts/tools
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4573F2, 0x8, 0xB7);  // good, item names (menu)
    //DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4573FA, 0x8, 0xB6);  // don't need, item names (menu) but one slot up
    //DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x457702, 0x8, 0x1);   // don't need, points to (nothing) in items
    //DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45770A, 0x8, 0x1);   // don't need, points to blank above jackets
    //457712 - points to jackets
    //457812 - points to blank above technique/weapon names
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4579AA, 0x24, 0x1);  // good, "press buttons to select difficulty"
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4579CE, 0x1A, 0x64); // good, explanations of different gimmicks, etc.
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4583F6, 0x8, 0x12);  // good, location names
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x458486, 0x12, 0x3);  // good, "delete old data, yes/no"
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4584BC, 0x1C, 0xC2); // good, item descriptions (# of entries might be off?)
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459CAC, 0x12, 0x10); // good, menu option descriptions
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459C64, 0x12, 0x1);  // good, first menu option (separate for some reason)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459DCC, 0x16, 0x2);  // good, don't really need though, just "????????"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459DF8, 0x16, 0xC);  // good, key item descriptions
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F00, 0x6, 0x8);   // don't need due to hack, menu option text
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F30, 0x8, 0x1);   // good, "tokubetsu item"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F40, 0x12, 0x3);  // good, "go back yes/no", might be less entries though
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F76, 0x4, 0x1);   // good, "gimmick" (where used?)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F7A, 0x7, 0x1);   // good, "equipped gimmicks"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F81, 0x4, 0x1);   // good, "times"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F85, 0x4, 0x1);   // good, "attack"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F89, 0x4, 0x1);   // good, "battery"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F8D, 0x4, 0x1);   // good, "difficulty"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459F91, 0x1B, 0x1);  // good, "you can adjust blank's gimmick"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459FAC, 0x1B, 0x3);  // good, gimmick menu instructions (3 lines for some reason)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x459FFD, 0x1B, 0x2);  // good, gimmick menu instructions (2 lines, this seems to be the proper one)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A033, 0x4, 0x1);   // good, "speed"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A037, 0x4, 0x1);   // good, "defense"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A03B, 0x1C, 0x3);  // good, "change into these clothes? yes/no"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A08F, 0x9, 0x2);   // good, "press L/R to switch difficulty"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0A1, 0x3, 0x1);   // good, "save"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0A4, 0x3, 0x1);   // good, "load"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0A7, 0x5, 0x1);   // good, "shoukyo"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0AC, 0x5, 0x6);   // good, "basho wo", etc. strange formatting for so many lines though
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0CA, 0x10, 0x3);  // good, various saving messages
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0FD, 0x12, 0x4);  // good, save/overwrite confirmation text
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A0FA, 0x3, 0x1);   // good, "level"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A145, 0x8, 0x1);   // good, "tokubetsu item"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A14D, 0x5, 0x1);   // good, "toy parts"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A152, 0x7, 0x1);   // good, "gimmick card"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A159, 0x3, 0x1);   // good, "effect"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A15C, 0x4, 0x1);   // good, "maisuu"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A160, 0x3, 0x1);   // good, "level"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A163, 0x5, 0x1);   // good, "tairyoku"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A168, 0x5, 0x1);   // good, "exp"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A16D, 0x4, 0x1);   // good, "speed"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A171, 0x4, 0x1);   // good, "defense"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A175, 0x8, 0x1);   // good, "to next level"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A182, 0x7, 0x1);   // good, "equipped gimmick"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A189, 0x4, 0x1);   // good, "sugoi no"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A18D, 0xF, 0x1);   // good, "select friend to switch to"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A2E9, 0xF, 0x3);   // good, shop greetings
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A316, 0x6, 0x1);   // good, "buy what?"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A31C, 0x6, 0x1);   // good, "buy what?"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A322, 0x6, 0x1);   // good, "sell what?"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A328, 0x6, 0x1);   // good, "sell what?"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A32E, 0x6, 0x1);   // good, "number on hand"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A334, 0x9, 0x1);   // good, "welcome!"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A33D, 0xF, 0x6);   // good, various shop messages
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A397, 0x4, 0x1);   // good, "senyou"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A39B, 0x4, 0x1);   // good, "defense"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A39F, 0x4, 0x1);   // good, "all"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A19C, 0x3, 0x1);   // good, "effect"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A19F, 0x4, 0x1);   // good, "maisuu"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A1A3, 0x4, 0x1);   // good, "naiyou"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A1A7, 0x6, 0x2);   // good, "max hp" and "speed"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A1B3, 0x1A, 0x2);  // good, gimika rank stuff
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A1E7, 0x8, 0x4);   // good, gimmick card link battle text
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A207, 0x8, 0x1);   // good, "winner gimmicker"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A20F, 0x1, 0xA);   // good, but change not needed, it goes from 0-9
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A219, 0x1C, 0x1);   // good, "you have it equipped"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A235, 0x1C, 0x3);  // good, "press start button to test stuff, etc."
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A289, 0x1C, 0x2);  // good, stuff about whose clothes to change
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A2C1, 0x9, 0x2);   // good, "see deck" "arrange cards"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A2D3, 0x3, 0x1);   // good, "deck"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A2D6, 0x7, 0x1);   // good, "cards left"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A3A3, 0x3, 0x1);   // good, "card" (note: 45A3A6-4543F1 has text that doesn't seem to be referenced)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4599F4, 0x0D, 0x30); // good, a lot of card text, might not be correct # of entries (too many if anything)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4572B0, 0x08, 0x14); // good, various names, probably card people?
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x480607, 0x0A, 0x0B); // good, gimmicker ranks
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x480675, 0x03, 0xB);  // good, but no changes needed, these are #s that corresponde with the above "50" "100",
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x480696, 0x12, 0x10); // good, gimmicker menu messages (maybe too many entries?)
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x4807B6, 0x12, 0x30); // good, a bunch more card menu descriptions
    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x480B1D, 0x7, 0xA);   // good, misc card text
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x480B16, 0x07, 0x1);  // good, but don't need to change, is just "???????"
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639BA8, 0x4, 0x4);   // good, battle menu text

	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C20, 0x10, 0x3);   // good, "run!" and misc. text

	//DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x470472, 0xA, 0x32);   // good, battle menu text, there's stuff before it though

    // dumps battle weapon/skill names and descriptions
	for (i = 0; i < 0x32; i++)
	{
		loc = 0x62C250 + i * 0x58;
		DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", loc, 0x8, 0x1);
		DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", loc + 8, 0x1A, 0x1);
	}

	for (i = 0; i < 0x81; i++)
		DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x630E5C + 0x10 * i, 0x8, 0x1);   //battle items


	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639558, 0x1A, 0x11); // battle item descs

	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C80, 0xB, 0x1);   // you won!
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C8B, 0x5, 0x1);   // exp
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C90, 0x3, 0x1);   // topa
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C93, 0xB, 0x1);   // level up
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639C9E, 0x6, 0x1);   // tairyoku ga
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639CA4, 0x5, 0x1);   // subayasa ga
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639CA9, 0x4, 0x1);   // agatta
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639CAD, 0x4, 0x1);   // item
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639CB1, 0x6, 0x1);   // item
	DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x639CB7, 0x8, 0x1);   // item

    for (i = 0; i < 0x100; i++)
		DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x638558 + i * 0x10, 0x8, 0x1);   // enemy attack names

    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x46FB68, 0x8, 0x11);   // overworld map names

    DumpStuff("ta_menus_jpn.txt", "ta_menus_eng.txt", 0x45A17D, 0x5, 0x1);   // "equipped clothes"


   	fseek (fin, 0x649748, SEEK_SET);
	fout = fopen("ta_maps_jpn.txt", "w");
	fprintf(fout, "LINE: %03X  ADDR: %08X\n", line, ftell(fin));
    while (ftell(fin) < 0x649A07)
	{
		ch = fgetc(fin);
		ConvChar(ch, letter, fin, fout, false);
	}
	fclose(fout);


	fclose(fin);
}

//=================================================================================================

void ConvChar(unsigned char ch, char letter[1000], FILE* fin, FILE* fout, bool isMenu)
{
	FILE*         tableFile;
	unsigned char ch2;
	int           loc;
	int           i;

    for (i = 0; i < 1000; i++)
       letter[i] = '\0';

    if (ch == 0x00 && isMenu == true)
    {
	   ch2 = fgetc(fin);
	   if (ch2 == 00)
	      fprintf(fout, "[END]\n\nLINE: %03X  ADDR: %08X\n", line, ftell(fin));
	   else
	      fseek(fin, ftell(fin) - 1, SEEK_SET);
	}

	if (ch == 0xFF)
	{
		ch2 = fgetc(fin);
		switch (ch2)
		{
			case 0x00:
			    line++;
			    loc = ftell(fin);
				fprintf(fout, "[END]\n\nLINE: %03X  ADDR: %08X\n", line, loc);
				//FindPointers(loc, fin, fout);
				//printf("Line %03X / BD7ish\n", line);
				break;

		    case 0x01:
		        fprintf(fout, "[WAIT]\n");
		        break;

		    case 0x02:
		        fprintf(fout, "[NEWWIN]\n");
		        break;

		    case 0x05:
		        fprintf(fout, "[BREAK]\n");
		        break;

		    case 0x06:
		        fprintf(fout, "[FF 06]\n");
		        break;

		    case 0x11:
		        fprintf(fout, "[PAUSE %02X]", fgetc(fin));
		        break;

		    case 0x12:
		        fprintf(fout, "[SETPAUSE]");
		        break;

		    case 0x1C:
		        fprintf(fout, "[SIZE_WIDE]");
		        break;

		    case 0x1D:
		        fprintf(fout, "[SIZE_TALL]");
		        break;

		    case 0x1E:
		        fprintf(fout, "[SIZE_BIG]");
		        break;

		    case 0x1F:
		        fprintf(fout, "[SIZE_NORMAL]");
		        break;

		    case 0x24:
		        fprintf(fout, "[CENTER_H]");
		        break;

		    case 0x25:
		        fprintf(fout, "[CENTER_V]");
		        break;

		    case 0x26:
		        fprintf(fout, "[CENTER_HV]");
		        break;

		    case 0x30:
		        fprintf(fout, "[COLOR_OFF]");
		        break;

		    case 0x32:
		        fprintf(fout, "[COLOR_ON]");
		        break;

		    case 0x40:
		        fprintf(fout, "[SIMPLEWAIT]");
		        break;

		    case 0x41:
		        fprintf(fout, "[MENU1]");
		        break;

		    case 0x42:
		        fprintf(fout, "[MENU2]");
		        break;

		    case 0x43:
		        fprintf(fout, "[MENU_END]");
		        break;

		    case 0x80:
		        fprintf(fout, "[FF 80]");
		        break;

		    case 0x81:
		        fprintf(fout, "[NAME1]");
		        break;

		    case 0x82:
	        fprintf(fout, "[NAME2]");
		        break;

		    case 0x83:
	        fprintf(fout, "[NAME3]");
		        break;

		    case 0x84:
	        fprintf(fout, "[NAME4]");
		        break;

		    case 0x85:
	        fprintf(fout, "[FF 85]");
		        break;

		    default:
		    	fprintf(fout, "[%02X %02X]", ch, ch2);
		    	break;
		}

	}
	else if (ch == 0xFE)
	{
		ch2 = fgetc(fin);
		fprintf(fout, "%s", kanjiTable[ch2].str);
	}
    else
		fprintf(fout, "%s", table[ch].str);
}

//=================================================================================================

void LoadTable(void)
{
   FILE* fin;
   char  tempStr[500] = "";
   int i;

   fin = fopen("ta_table_jpn.txt", "r");
   if (fin == NULL)
   {
	   printf("Can't open ta_table_jpn.txt!\n");
	   return;
   }

   i = fgetc(fin);
   i = fgetc(fin);
   i = fgetc(fin);


   fscanf(fin, "%s", tempStr);
   table[tableLen].hexVal = hstrtoi(tempStr);
   fscanf(fin, "%s", table[tableLen].str);
   while (!feof(fin))
   {
	   tableLen++;

   	   fscanf(fin, "%s", tempStr);
   	   //printf("%s ", tempStr);
   	   table[tableLen].hexVal = hstrtoi(tempStr);
       fscanf(fin, "%s", table[tableLen].str);
       //printf("%s\n", table[tableLen].str);
   }

   table[0x00].str[0] = ' ';
   table[0x00].str[1] = '\0';

   fclose(fin);
}

//=================================================================================================

void LoadKanjiTable(void)
{
   FILE* fin;
   char  tempStr[500] = "";
   int i;

   fin = fopen("ta_kanji.txt", "r");
   if (fin == NULL)
   {
	   printf("Can't open ta_kanji.txt!\n");
	   return;
   }

   i = fgetc(fin);
   i = fgetc(fin);
   i = fgetc(fin);


   fscanf(fin, "%s", tempStr);
   kanjiTable[kanjiTableLen].hexVal = hstrtoi(tempStr);
   fscanf(fin, "%s", kanjiTable[kanjiTableLen].str);
   while (!feof(fin))
   {
	   kanjiTableLen++;

   	   fscanf(fin, "%s", tempStr);
   	   //printf("%s ", tempStr);
   	   kanjiTable[kanjiTableLen].hexVal = hstrtoi(tempStr);
       fscanf(fin, "%s", kanjiTable[kanjiTableLen].str);
       //printf("%s\n", table[tableLen].str);
   }

   fclose(fin);
}

//=================================================================================================

void LoadSmallTable(void)
{
   FILE* fin;
   char  tempStr[500] = "";
   int i;

   fin = fopen("ta_table_jpn2.txt", "r");
   if (fin == NULL)
   {
	   printf("Can't open ta_table_jpn2.txt!\n");
	   return;
   }

   i = fgetc(fin);
   i = fgetc(fin);
   i = fgetc(fin);


   fscanf(fin, "%s", tempStr);
   smallTable[smallTableLen].hexVal = hstrtoi(tempStr);
   fscanf(fin, "%s", smallTable[smallTableLen].str);
   while (!feof(fin))
   {
	   smallTableLen++;

   	   fscanf(fin, "%s", tempStr);
   	   smallTable[smallTableLen].hexVal = hstrtoi(tempStr);
       fscanf(fin, "%s", smallTable[smallTableLen].str);
   }

   fclose(fin);
}

//=================================================================================================

void FindPointers(int loc, FILE* fin, FILE* fout)
{
   int           gbaLoc = loc + 0x8000000;
   int           tempLoc;
   unsigned char ch;
   int           temp;

   fseek(fin, 0, SEEK_SET);
   ch = fgetc(fin);

   while (!feof(fin))
   {
      if (ch == 0x08)
      {
 		 tempLoc = ftell(fin);

		  fseek(fin, tempLoc - 4, SEEK_SET);
		  temp = fgetc(fin);
		  temp |= (fgetc(fin) << 8);
		  temp |= (fgetc(fin) << 16);
		  temp |= (fgetc(fin) << 24);


		  if (temp == gbaLoc)
		     fprintf(fptrs, "%08X ", tempLoc - 4);
	  }

	  ch = fgetc(fin);
   }

   fprintf(fptrs, "\n");

   fseek(fin, loc, SEEK_SET);
}

//=================================================================================================

unsigned int hstrtoi(char* string)
{
   unsigned int retval = 0;

   for (int i = 0; i < strlen(string); i++)
   {
      retval <<= 4;
      retval += CharToHex(string[i]);
   }

   return retval;
}

//=================================================================================================

int CharToHex(char ch)
{
   // Converts a single hex character to an integer.

   int retVal = 0;

   ch = toupper(ch);

   switch (ch)
   {
      case '0':
      {
         retVal = 0;
         break;
      }
      case '1':
      {
         retVal = 1;
         break;
      }
      case '2':
      {
         retVal = 2;
         break;
      }
      case '3':
      {
         retVal = 3;
         break;
      }
      case '4':
      {
         retVal = 4;
         break;
      }
      case '5':
      {
         retVal = 5;
         break;
      }
      case '6':
      {
         retVal = 6;
         break;
      }
      case '7':
      {
         retVal = 7;
         break;
      }
      case '8':
      {
         retVal = 8;
         break;
      }
      case '9':
      {
         retVal = 9;
         break;
      }
      case 'A':
      {
         retVal = 10;
         break;
      }
      case 'B':
      {
         retVal = 11;
         break;
      }
      case 'C':
      {
         retVal = 12;
         break;
      }
      case 'D':
      {
         retVal = 13;
         break;
      }
      case 'E':
      {
         retVal = 14;
         break;
      }
      case 'F':
      {
         retVal = 15;
         break;
      }
   }

   return retVal;
}

//=================================================================================================

void ConvCharSimple(unsigned char ch, char conv[])
{
   sprintf(conv, "%s", smallTable[ch].str);
}

//=================================================================================================

void DumpStuff1(void)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	int           loc;
	int           line;
	char          letter[1000];

	fin = fopen("test.gba", "rb");

    loc = 0x634000;
    line = 0;
	fseek (fin, loc, SEEK_SET);
    fout = fopen("ta_menus1_jpn.txt", "w");
	fprintf(fout, "LINE: %03X  ADDR: %08X\n", line, loc);
	//FindPointers(loc, fin, fout);
    while (ftell(fin) < 0x64B4EC)
	{
		ch = fgetc(fin);
		ConvChar(ch, letter, fin, fout, true);
	}
	fclose(fout);
	fclose(fin);
}

//=================================================================================================

void DumpEnemyNames(void)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	unsigned char ch2;
	char          conv[100];
	int           i;
	int           j;

	fin = fopen("test.gba", "rb");
	if (fin == NULL)
	{
		printf("couldn't open test.gba\n");
		return;
	}

	fout = fopen("ta_enemies_jpn.txt", "w");

	for (i = 0; i < 0xB6; i++)
	{
	   fprintf(fout, "%02X-J:", i);
	   fseek(fin, 0x634F9C + 0x4C * i, SEEK_SET);
       for (j = 0; j < 8; j++)
       {
		   ch = fgetc(fin);
		   if (ch != 0)
		   {
			   if (ch == 0xFE)
			   {
                  ch2 = fgetc(fin);
                  sprintf(conv, "%s", smallTable[ch2].str);
			   }
			   else if (ch == 0xFF)
			   {
				  ch2 = fgetc(fin);
				  sprintf(conv, "[%02X %02X]", ch, ch2);
			   }
			   else
			      ConvCharSimple(ch, conv);

			   fprintf(fout, "%s", conv);
		   }
	   }
	   fprintf(fout, "\n");
	}


	fclose(fout);
	fclose(fin);

	/*fout = fopen("ta_enemies_eng.txt", "w");
	for (i = 0; i < 0xB6; i++)
       fprintf(fout, "%02X-E:\n", i);
    fclose(fout);*/
}

//=================================================================================================

void DumpMenuStuff1(void)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	char          conv[100];
	int           i;
	int           j;

	fin = fopen("test.gba", "rb");
	if (fin == NULL)
	{
		printf("couldn't open test.gba\n");
		return;
	}

	fout = fopen("ta_menus1_jpn.txt", "w");

	for (i = 0; i < 0x8; i++)
	{
	   fprintf(fout, "%02X-J:", i);
	   fseek(fin, 0x459F00 + 0x6 * i, SEEK_SET);
       for (j = 0; j < 6; j++)
       {
		   ch = fgetc(fin);
		   if (ch != 0)
		   {
			   ConvCharSimple(ch, conv);
			   fprintf(fout, "%s", conv);
		   }
	   }
	   fprintf(fout, "\n");
	}


	fclose(fout);
	fclose(fin);

/*	fout = fopen("ta_menus1_eng.txt", "w");
	for (i = 0; i < 0x8; i++)
       fprintf(fout, "%02X-E:\n", i);
    fclose(fout);*/
}

//=================================================================================================

void DumpStuff(char jpnFile[], char engFile[], int address, int len, int total)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	unsigned char ch2;
	char          conv[100];
	int           i;
	int           j;

	fin = fopen("test.gba", "rb");
	if (fin == NULL)
	{
		printf("couldn't open test.gba\n");
		return;
	}

	fout = fopen(jpnFile, "a");

    fprintf(fout, "BLOCKSTART %06X %02X %02X\n", address, len, total);
	for (i = 0; i < total; i++)
	{
	   fprintf(fout, "%02X-J:", i);
	   fseek(fin, address + len * i, SEEK_SET);
       for (j = 0; j < len; j++)
       {
		   ch = fgetc(fin);
		   if (ch != 0)
		   {
		       ConvCharSimple(ch, conv);

			   fprintf(fout, "%s", conv);
		   }
	   }
	   fprintf(fout, "\n");
	}

    fprintf(fout, "\n");
//    fprintf(fout, "%X:\n\n", ftell(fin));

	fclose(fout);
	fclose(fin);

/*	fout = fopen(engFile, "a");
	fprintf(fout, "BLOCKSTART %06X %02X %02X\n", address, len, total);
	for (i = 0; i < total; i++)
       fprintf(fout, "%02X-E:\n", i);
    fprintf(fout, "\n");
    fclose(fout);*/
}

//=================================================================================================

void DumpStuff2(char jpnFile[], char engFile[], int address, int len, int total)
{
	FILE*         fin;
	FILE*         fout;
	unsigned char ch;
	unsigned char ch2;
	char          conv[100];
	int           i;
	int           j;

	fin = fopen("test.gba", "rb");
	if (fin == NULL)
	{
		printf("couldn't open test.gba\n");
		return;
	}

	fout = fopen(jpnFile, "a");

    fprintf(fout, "BLOCKSTART %06X %02X %02X\n", address, len, total);
	for (i = 0; i < total; i++)
	{
	   fprintf(fout, "%02X-J:", i);
	   fseek(fin, address + len * i, SEEK_SET);
       for (j = 0; j < len; j++)
       {
		   ch = fgetc(fin);
		   if (ch != 0)
		   {
		       ConvCharSimple2(fin, ch, conv, j);

			   fprintf(fout, "%s", conv);
		   }
	   }
	   fprintf(fout, "\n");
	}

    fprintf(fout, "\n");
//    fprintf(fout, "%X:\n\n", ftell(fin));

	fclose(fout);
	fclose(fin);

/*	fout = fopen(engFile, "a");
	fprintf(fout, "BLOCKSTART %06X %02X %02X\n", address, len, total);
	for (i = 0; i < total; i++)
       fprintf(fout, "%02X-E:\n", i);
    fprintf(fout, "\n");
    fclose(fout);*/
}

void ConvCharSimple2(FILE* fin, unsigned char ch, char conv[], int& len)
{
   unsigned char ch2;


   if (ch == 0xFF)
   {
	   ch2 = fgetc(fin);
	   if (ch2 == 0x11)
	   {
		   ch2 = fgetc(fin);
		   sprintf(conv, "[%02X 11 %02X]", ch, ch2);
		   len += 2;
	   }
	   else
	   {
	   	  sprintf(conv, "[%02X %02X]", ch, ch2);
	   	  len++;
   	   }
   }
   else if (ch == 0xFE)
   {
	   ch2 = fgetc(fin);
	   sprintf(conv, "%s", kanjiTable[ch2].str);
	   len++;
   }
   else
   	   sprintf(conv, "%s", smallTable[ch].str);
}

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
#include "util.h"

static std::map<std::vector<uint8_t>, uint32_t> previousInsertions;
bool forceAll = false;

struct tableEntry
{
	int  hexVal;
	char str[500];
};

static tableEntry table[500];
static int tableLen = 0;

static void PrepString(char[], char[], int);
static unsigned char ConvChar(unsigned char);
static int ConvComplexString(char *s, size_t availLen, bool stripNewline);
static int CompileCC(const char *str, int len, int &pos, unsigned char *dest);
static int DetectScriptLen(const char *str, int maxLen);
// Modifies stringID.
static void ForceTranslateComplex(char *str, int len, char *stringID);
static int CharToHex(char);
static unsigned int hstrtoi(char*);
static uint32_t UpdatePointers(const std::vector<uint32_t> &list, int loc, FILE *fout);
static std::vector<std::vector<uint32_t>> BuildPointerMap(const char *pointersFile);

//=================================================================================================

static uint32_t InsertStringAt(FILE *fout, const char *str, uint32_t len, uint32_t fullLen, uint32_t pos)
{
	std::vector<uint8_t> full;
	full.resize(fullLen, 0);
	memcpy(full.data(), str, len);

	fseek(fout, pos, SEEK_SET);
	fwrite(full.data(), 1, fullLen, fout);

	previousInsertions[full] = pos;
	return pos;
}

static uint32_t InsertString(FILE *fout, const char *str, uint32_t len, uint32_t fullLen, uint32_t &nextFree)
{
	std::vector<uint8_t> full;
	full.resize(fullLen, 0);
	memcpy(full.data(), str, len);

	auto already = previousInsertions.find(full);
	// Forcing is meant to stress, let's disable this optimization.
	if (already != previousInsertions.end() && !forceAll)
		return already->second;

	uint32_t pos = nextFree;
	nextFree += fullLen;

	fseek(fout, pos, SEEK_SET);
	fwrite(full.data(), 1, fullLen, fout);

	previousInsertions[full] = pos;
	return pos;
}

//=================================================================================================

uint32_t InsertMainScript(FILE *fout, uint32_t &afterTextPos)
{
	FILE *fin = fopen("ta_script_eng.txt", "r");
	if (fin == NULL)
	{
		printf("Couldn't open ta_script_eng.txt!\n");
		return 0;
	}

	std::vector<std::vector<uint32_t>> pointers = BuildPointerMap("pointers1.dat");

	uint32_t insertions = 0;
	int i = 0;
	int state = 0;
	char pendingString[5000];
	int pendingPos = 0;

	auto flushString = [&]()
	{
		if (pendingPos == 0)
		{
			if (forceAll)
			{
				// We need to load the old string to force it.
				if (pointers[i].empty())
					return;

				uint32_t oldPtrAddress = pointers[i][0];
				fseek(fout, oldPtrAddress, SEEK_SET);
				uint32_t oldStrAddress = 0;
				ReadLE32(fout, oldStrAddress);
				fseek(fout, oldStrAddress & ~0x08000000, SEEK_SET);
				fread(pendingString, 1, 4999, fout);

				int len = DetectScriptLen(pendingString, 4999);
				char stringID[16];
				sprintf(stringID, "S%03X", i);
				ForceTranslateComplex(pendingString, len, stringID);

				// So we can test insertion works, insert as a "new" string.
				uint32_t forceLoc = InsertString(fout, pendingString, len, len, afterTextPos);
				insertions += UpdatePointers(pointers[i], forceLoc, fout);
				return;
			}
			else
				return;
		}

		pendingString[pendingPos] = '\0';
		int len = ConvComplexString(pendingString, sizeof(pendingString), false);
		// Ensure it has an END at the END, they should always.
		if (len < 2 || pendingString[len - 2] != (char)0xFF || pendingString[len - 1] != 0x00)
		{
			pendingString[len++] = (char)0xFF;
			pendingString[len++] = (char)0x00;
		}
		uint32_t loc = InsertString(fout, pendingString, len, len, afterTextPos);
		insertions += UpdatePointers(pointers[i], loc, fout);
		pendingPos = 0;
	};

	// Assume a BOM may exist.
	if ((uint8_t)fgetc(fin) == 0xEF)
		fseek(fin, 3, SEEK_SET);
	else
		fseek(fin, 0, SEEK_SET);

	while (!feof(fin))
	{
		char str[5000];
		if (!fgets(str, 5000, fin))
			continue;

		bool isStartLine = strstr(str, "-E:") == str + 3;
		if (!isStartLine && state == 0)
		{
			printf("Unexpected line: %s", str);
			continue;
		}

		if (isStartLine && state != 0)
		{
			flushString();
			i++;
			state = 0;
		}

		if (state == 0)
		{
			// Brand new string.  We'll write this later (see above.)
			state = 1;
			int toCopy = (int)strlen(str + 6) - 1;
			if (toCopy > 0)
				memcpy(pendingString + pendingPos, str + 6, toCopy);
			pendingPos += toCopy;
		}
		else if (state == 1)
		{
			// Continuation of an existing line.
			int toCopy = (int)strlen(str) - 1;
			if (toCopy > 0)
				memcpy(pendingString + pendingPos, str, toCopy);
			pendingPos += toCopy;
		}
	}
	// Flush any last string.
	flushString();
	fclose(fin);

	return insertions;
}

//=================================================================================================

static int ConvComplexString(char *str, size_t availLen, bool stripNewline)
{
	unsigned char newStr[5000];
	int len = (int)strlen(str) - (stripNewline ? 1 : 0);
	int newLen = 0;
	for (int counter = 0; counter < len; ++counter)
	{
		if (str[counter] == '[')
		{
			// Updates counter one less than full string.
			int streamLen = CompileCC(str, len, counter, newStr + newLen);
			newLen += streamLen;
		}
		else if (str[counter] == ' ')
			newStr[newLen++] = '\0';
		else
			newStr[newLen++] = ConvChar(str[counter]);
	}

	memcpy(str, newStr, newLen);
	memset(str + newLen, '\0', availLen - newLen);

	return newLen;
}

//=================================================================================================

static int CompileCC(const char *str, int totalLength, int &strLoc, unsigned char *newStream)
{
	char str2[5000] = "";
	char *ptr[5000];
	int ptrCount = 0;

	// we're gonna mess with the string, so do it in a copy
	strcpy(str2 + strLoc, str + strLoc);

	// first we gotta parse the codes, what a pain
	ptr[ptrCount++] = &str2[strLoc + 1];
	while (str2[strLoc] != ']' && strLoc < totalLength)
	{
		if (str2[strLoc] == ' ')
		{
			ptr[ptrCount++] = &str2[strLoc + 1];
			str2[strLoc] = 0;
		}

		strLoc++;
	}

	if (str2[strLoc] == ']')
		str2[strLoc] = 0;

	// Capitalize all the arguments for ease of use
	for (int i = 0; i < ptrCount; i++)
	{
		for (int j = 0; j < (int)strlen(ptr[i]); j++)
			ptr[i][j] = toupper(ptr[i][j]);
	}

	// now the actual compiling into the data stream
	int streamLen = 0;
   if (strcmp(ptr[0], "END") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x00;
   }
   else if (strcmp(ptr[0], "WAIT") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x01;
   }
   else if (strcmp(ptr[0], "NEWWIN") == 0 || strcmp(ptr[0], "CONTINUE") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x02;
   }
   else if (strcmp(ptr[0], "PAUSEBREAK") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x03;
	   newStream[streamLen++] = hstrtoi(ptr[1]);
   }
   else if (strcmp(ptr[0], "CLEAR") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x04;
   }
   else if (strcmp(ptr[0], "BREAK") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x05;
   }
   else if (strcmp(ptr[0], "TICKEROFF") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x06;
   }
   else if (strcmp(ptr[0], "PAUSE") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x11;
	   newStream[streamLen++] = hstrtoi(ptr[1]);
   }
   else if (strcmp(ptr[0], "SETPAUSE") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x12;
   }
   else if (strcmp(ptr[0], "PAD_H") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x18;
	   newStream[streamLen++] = hstrtoi(ptr[1]);
   }
   else if (strcmp(ptr[0], "PAD_V") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x19;
	   newStream[streamLen++] = hstrtoi(ptr[1]);
   }
   else if (strcmp(ptr[0], "SIZE_WIDE") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x1C;
   }
   else if (strcmp(ptr[0], "SIZE_TALL") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x1D;
   }
   else if (strcmp(ptr[0], "SIZE_BIG") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x1E;
   }
   else if (strcmp(ptr[0], "SIZE_NORMAL") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x1F;
   }
   else if (strcmp(ptr[0], "CENTER_H") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x24;
   }
   else if (strcmp(ptr[0], "CENTER_V") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x25;
   }
   else if (strcmp(ptr[0], "CENTER_HV") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x26;
   }
   else if (strcmp(ptr[0], "TIME") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x27;
   }
   else if (strcmp(ptr[0], "COLOR_OFF") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x30;
   }
   else if (strcmp(ptr[0], "COLOR_ON") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x32;
   }
   else if (strcmp(ptr[0], "COLOR2_ON") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x39;
   }
   else if (strcmp(ptr[0], "SIMPLEWAIT") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x40;
   }
   else if (strcmp(ptr[0], "MENU1") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x41;
   }
   else if (strcmp(ptr[0], "MENU2") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x42;
   }
   else if (strcmp(ptr[0], "MENU_END") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x43;
   }
   else if (strcmp(ptr[0], "NUMBER") == 0 || strcmp(ptr[0], "MONEY") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x80;
   }
   else if (strcmp(ptr[0], "NAME1") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x81;
   }
   else if (strcmp(ptr[0], "NAME2") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x82;
   }
   else if (strcmp(ptr[0], "NAME3") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x83;
   }
   else if (strcmp(ptr[0], "NAME4") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x84;
   }
   else if (strcmp(ptr[0], "ITEM") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x85;
   }
   else if (strcmp(ptr[0], "UP") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF0;
   }
   else if (strcmp(ptr[0], "RIGHT") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF1;
   }
   else if (strcmp(ptr[0], "DOWN") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF2;
   }
   else if (strcmp(ptr[0], "LEFT") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF3;
   }
   else if (strcmp(ptr[0], "L_BUTTON") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF4;
   }
   else if (strcmp(ptr[0], "R_BUTTON") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF5;
   }
   else if (strcmp(ptr[0], "A_BUTTON") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF6;
   }
   else if (strcmp(ptr[0], "B_BUTTON") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF7;
   }
   else if (strcmp(ptr[0], "HEART") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF8;
   }
   else if (strcmp(ptr[0], "NOTE") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xF9;
   }
   else if (strcmp(ptr[0], "STAR") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xFA;
   }
   else if (strcmp(ptr[0], "DOTS") == 0)
   {
       newStream[streamLen++] = 0xFE;
       newStream[streamLen++] = 0xFB;
   }

	else if (isalpha((int)ptr[0][0]) && strlen(ptr[0]) != 2)
	{
		int retVal = 0;
		for (int i = 0; i < tableLen; ++i)
		{
			if (strcmp(ptr[0], table[i].str) == 0)
			{
				retVal = table[i].hexVal;
				break;
			}
		}

		newStream[streamLen++] = retVal;
		newStream[streamLen++] = 0x00;
		if (retVal == 0)
			printf("Couldn't convert control code: %s\n", ptr[0]);
	}

	// going to assume raw codes now, in 2-char hex things, like [FA 1A 2C EE]
	else if (strlen(ptr[0]) == 2)
	{
		for (int i = 0; i < ptrCount; i++)
			newStream[streamLen++] = hstrtoi(ptr[i]);
	}

	else
		printf("UNKNOWN CONTROL CODE: %s\n", ptr[0]);

	return streamLen;
}

//=================================================================================================

static unsigned char ConvChar(unsigned char ch)
{
	unsigned char retVal = 0;
	char          origChar[100] = "";
	int           i = 0;

	while ((i < tableLen) && (retVal == 0))
	{
		sprintf(origChar, "%c", ch);

		if (strcmp(origChar, table[i].str) == 0)
		   retVal = table[i].hexVal;
		else
		   i++;
	}

	if (retVal == 0)
		printf("UNABLE TO CONVERT CHARACTER: %c %02X\n", ch, ch);

	return retVal;
}

//=================================================================================================

void LoadTable()
{
   FILE* fin;
   char  tempStr[500] = "";
   int i;

   fin = fopen("ta_table_eng.txt", "r");
   if (fin == NULL)
   {
	   printf("Can't open ta_table_eng.txt!\n");
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
   	   table[tableLen].hexVal = hstrtoi(tempStr);
       fscanf(fin, "%s", table[tableLen].str);
       //printf("%s\n", table[tableLen].str);
   }

   table[0xEF].str[0] = ' ';
   table[0xEF].str[1] = '\0';

   fclose(fin);
}

//=================================================================================================

static unsigned int hstrtoi(char* string)
{
   unsigned int retval = 0;

   for (int i = 0; i < (int)strlen(string); i++)
   {
      retval <<= 4;
      retval += CharToHex(string[i]);
   }

   return retval;
}

//=================================================================================================

static int CharToHex(char ch)
{
	// Converts a single hex character to an integer.
	std::string possible = "0123456789ABCDEF";
	size_t pos = possible.find(ch);
	if (pos != possible.npos)
		return (int)pos;

	return 0;
}

//=================================================================================================

static void PrepString(char str[5000], char str2[5000], int startPoint)
{
	int j;
	int ctr;

    for (j = 0; j < 5000; j++)
	    str2[j] = '\0';

    ctr = 0;
	for (j = startPoint; j < (int)strlen(str); j++)
	{
	   str2[ctr] = str[j];
	   ctr++;
	}

}

//=================================================================================================

static std::vector<std::vector<uint32_t>> BuildPointerMap(const char *pointersFile)
{
	std::vector<std::vector<uint32_t>> pointers;
	FILE *fptrs = fopen(pointersFile, "r");
	if (!fptrs)
	{
		printf("Couldn't open %s!\n", pointersFile);
		return pointers;
	}

	int i = 0;
	while (!feof(fptrs))
	{
		char str[5000]{};
		fgets(str, sizeof(str), fptrs);
		pointers.emplace_back();

		uint32_t address;
		int pos = 0;
		int oldpos = 0;
		while (sscanf(str + oldpos + pos, "%X%n", &address, &pos) > 0)
		{
			if (address != 0xFFFFFFFF)
				pointers[i].push_back(address);
			oldpos += pos + 1;
			pos = 0;
		}
		i++;
	}
	fclose(fptrs);

	return pointers;
}

static uint32_t UpdatePointers(const std::vector<uint32_t> &list, int loc, FILE *fout)
{
	uint32_t insertions = 0;

	loc |= 0x08000000;
	for (uint32_t address : list)
	{
		//printf("loc:%08X    %08X", loc, address);
		fseek(fout, address, SEEK_SET);
		WriteLE32(fout, loc);
		insertions++;
	}

	return insertions;
}

//=================================================================================================

static int DetectFixedLen(const char *str, int maxLen)
{
	for (int i = maxLen; i > 0; --i)
	{
		if (str[i - 1] != 0)
			return i;
	}
	return 0;
}

static uint32_t EnemyInfoAddress(int i, int state)
{
	// State 1 = enemy names, state 2 = attack names.
	if (state == 2)
		return 0x00638558 + i * 16;
	return 0x00634F50 + i * 76;
}

uint32_t InsertEnemies(FILE *fout, uint32_t &afterTextPos)
{
	char str[5000];
	char str2[5000];

	FILE *fin = fopen("ta_enemies_eng.txt", "r");
	if (fin == NULL)
	{
		printf("Couldn't open ta_enemies_eng.txt!\n");
		return 0;
	}

	uint32_t insertions = 0;
	int state = 0;
	int states = 0;
	int i = 0;

	// Enemies have names and attack names.  We use this file for both.
	// Detect the old format and skip the first name.
	while (!feof(fin))
	{
		if (!fgets(str, 5000, fin))
			continue;
		if (str[0] == '#')
			continue;
		if (strstr(str, "NAMES:") != nullptr)
		{
			state = 1;
			i = 0;
			continue;
		}
		if (strstr(str, "ATTACKS:") != nullptr)
		{
			state = 2;
			i = 0;
			continue;
		}
		if (strstr(str, "-E:") == NULL)
			continue;

		// Okay, this is a string.  In state 0, we skipped the first enemy.
		// This is for old ta_enemies_eng.txt compatibility.
		if (state == 0)
		{
			state = 1;
			printf("Old ta_enemies_eng.txt file, inserting old Celemo...\n");

			// Just do the first one now.
			uint32_t nameAddress = EnemyInfoAddress(i, state);
			fseek(fout, nameAddress, SEEK_SET);
			fread(str2, 1, 8, fout);
			int len = DetectFixedLen(str2, 8);
			if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%c:%06X", state == 2 ? 'A' : 'E', i);
				ForceTranslateComplex(str2, 8, stringID);
			}

			uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);

			fseek(fout, nameAddress, SEEK_SET);
			WriteLE32(fout, pos | 0x08000000);
			fputc((uint8_t)len, fout);
			fputc(0x00, fout);
			fputc(0x00, fout);
			fputc(0x00, fout);

			insertions++;
			i++;
		}

		states |= state;

		PrepString(str, str2, 5);
		int len = ConvComplexString(str2, sizeof(str2), true);
		if (len > 24)
		{
			printf("Enemy name too long: %s\n", str);
			len = 24;
		}

		uint32_t nameAddress = EnemyInfoAddress(i, state);

		if (len == 0)
		{
			// We have to read in the old string and still insert.
			fseek(fout, nameAddress, SEEK_SET);
			fread(str2, 1, 8, fout);
			len = DetectFixedLen(str2, 8);
			if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%c:%06X", state == 2 ? 'A' : 'E', i);
				ForceTranslateComplex(str2, 8, stringID);
			}
		}

		uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);
		fseek(fout, nameAddress, SEEK_SET);
		WriteLE32(fout, pos | 0x08000000);
		fputc((uint8_t)len, fout);
		fputc(0x00, fout);
		fputc(0x00, fout);
		fputc(0x00, fout);

		insertions++;
		i++;
	}

	if ((states & 2) == 0)
	{
		// We never did attack names, but the game will crash now if we don't.
		// Copy the existing names over.
		state = 2;
		for (i = 0; i < 256; ++i)
		{
			uint32_t nameAddress = EnemyInfoAddress(i, state);
			fseek(fout, nameAddress, SEEK_SET);
			fread(str2, 1, 8, fout);
			int len = DetectFixedLen(str2, 8);
			if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%c:%06X", state == 2 ? 'A' : 'E', i);
				ForceTranslateComplex(str2, 8, stringID);
			}

			uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);
			fseek(fout, nameAddress, SEEK_SET);
			WriteLE32(fout, pos | 0x08000000);
			fputc((uint8_t)len, fout);
			fputc(0x00, fout);
			fputc(0x00, fout);
			fputc(0x00, fout);

			insertions++;
		}
	}

	fclose(fin);
	return insertions;
}

//=================================================================================================

uint32_t InsertBattleText(FILE *fout, uint32_t &afterTextPos)
{
	char str[5000];
	char str2[5000];

	FILE *fin = fopen("ta_battle_eng.txt", "r");
	if (fin == NULL)
	{
		printf("Couldn't open ta_enemies_eng.txt!\n");
		return 0;
	}

	uint32_t insertions = 0;

	// Enemies have names and attack names.  We use this file for both.
	// Detect the old format and skip the first name.
	while (!feof(fin))
	{
		if (!fgets(str, 5000, fin))
			continue;
		if (str[0] == '#')
			continue;

		uint32_t loc = 0;
		int offset = 0;
		if (sscanf(str, "%X-E:%n", &loc, &offset) < 1) {
			continue;
		}

		uint8_t align = 0;
		if (!strncmp(str + offset, "[CENTER]", strlen("[CENTER]")))
		{
			align = 1;
			offset += (int)strlen("[CENTER]");
		}
		else if (!strncmp(str + offset, "[CENTER_H]", strlen("[CENTER_H]")))
		{
			align = 1;
			offset += (int)strlen("[CENTER_H]");
		}
		else if (!strncmp(str + offset, "[RIGHT]", strlen("[RIGHT]")))
		{
			align = 2;
			offset += (int)strlen("[RIGHT]");
		}
		else if (!strncmp(str + offset, "[LEFT]", strlen("[LEFT]")))
		{
			// No change, just for completeness.
			offset += (int)strlen("[LEFT]");
		}

		PrepString(str, str2, offset);
		int len = ConvComplexString(str2, sizeof(str2), true);

		uint32_t oldAddress = 0;
		fseek(fout, loc, SEEK_SET);
		ReadLE32(fout, oldAddress);

		if (len == 0)
		{
			fseek(fout, oldAddress & ~0x08000000, SEEK_SET);
			fread(str2, 1, 16, fout);
			len = 16;

			if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%c:%06X", 'B', loc);
				ForceTranslateComplex(str2, len, stringID);
			}
		}

		// Let's just insert in place if we can.
		if (align == 0 && len <= 16 && str2[0] != (uint8_t)0xFF && !forceAll)
			InsertStringAt(fout, str2, len, 16, oldAddress & ~0x08000000);
		else if (len >= 0 && len <= 0x1A)
		{
			// If <= 0x1A, we embed the string directly.  FF, copyLength, align, length, string.
			memmove(str2 + 4, str2, len);
			str2[0] = (char)0xFF;
			str2[1] = len + 2;
			str2[2] = align;
			str2[3] = len;
			uint32_t pos = InsertString(fout, str2, len + 4, len + 4, afterTextPos);
			fseek(fout, loc, SEEK_SET);
			WriteLE32(fout, pos | 0x08000000);
		}
		else
		{
			// For a longer string, we use a pointer instead - see battle_message.asm.
			uint32_t textPos = InsertString(fout, str2, len, len, afterTextPos);
			uint32_t packetPos = afterTextPos;

			// Format: FF, copyLength, align, length, 00, 00, ptr32.
			uint8_t packet[]{ 0xFF, 0x08, align, (uint8_t)len, 0, 0 };
			fseek(fout, packetPos, SEEK_SET);
			fwrite(packet, 1, sizeof(packet), fout);
			WriteLE32(fout, textPos | 0x08000000);
			afterTextPos += 10;

			fseek(fout, loc, SEEK_SET);
			WriteLE32(fout, packetPos | 0x08000000);
		}

		insertions++;
	}

	fclose(fin);
	return insertions;
}

//=================================================================================================

static int DetectScriptLen(const char *str, int maxLen)
{
	ptrdiff_t pos = 0;
	while (pos < maxLen)
	{
		// Did we find FF 00 (END)?
		if (pos != 0 && str[pos] == 0)
			return (int)pos + 1;
		const char *next = (const char *)memchr(str + pos, 0xFF, maxLen - pos);
		if (!next)
			break;

		pos = next - str + 1;
	}

	return maxLen;
}

static void ForceTranslateComplex(char *s, int len, char *stringID)
{
	int stringIDLen = ConvComplexString(stringID, strlen(stringID) + 1, false);

	unsigned char *str = (unsigned char *)s;
	// Isn't "machine translation" great?
	for (int i = 0; i < len; ++i)
	{
		// Already English or a space/symbol?
		if (str[i] <= 30 || (str[i] >= 129 && str[i] <= 154))
			continue;
		// Symbols and numbers should be kept too...
		if ((str[i] >= 107 && str[i] <= 128) || (str[i] >= 155 && str[i] <= 159))
			continue;

		if (str[i] == 0xFF)
		{
			int code = str[++i];
			// Skip args for commands that have them.
			if (code == 17 || code == 3 || code == 16 || code == 24 || code == 25 || code == 26 || code == 34)
				i++;
			continue;
		}

		if (str[i] == 0xFE)
		{
			// For Kanji, we keep the symbols.
			int code = str[i + 1];
			if (code >= 240)
			{
				i++;
				continue;
			}

			// Okay, it's just kanji, remove the next char, we won't need it.
			memmove(str + i, str + i + 1, len - i - 1);
			// Keep going, though.
		}

		// And finally, our amazing translation algorithm.
		if (stringIDLen > 0)
		{
			str[i] = *stringID++;
			stringIDLen--;
		}
		else
			str[i] = 1;
	}
}

uint32_t InsertMenuStuff2(FILE *fout, uint32_t &afterTextPos)
{
	char  str[5000];
	char  str2[5000];
	int   address;
	int   len;

	FILE *fin = fopen("ta_menus_eng.txt", "r");
	if (!fin)
	{
		printf("Couldn't open ta_menus_eng.txt!\n");
		return 0;
	}

	uint32_t insertions = 0;

	fgets(str, 5000, fin);
	while (!feof(fin))
	{
		// It's a comment if it starts with #.
		bool comment = str[0] == '#';

		// New format: BLOCKFIXED PointerLoc NewLen Count [ClearLen]
		if (!comment && strstr(str, "BLOCKFIXED") != NULL)
		{
			int maxLen = 0;
			int lines = 0;
			int clearLen = 0;
			sscanf(str, "%*s %X %X %X %X", &address, &maxLen, &lines, &clearLen);
			if (maxLen <= 0 || maxLen > 255)
			{
				printf("Bad size for BLOCKFIXED %08x: %02x\n", address, maxLen);
				maxLen = 1;
			}

			uint32_t origAddress = 0;
			uint8_t origLen = 0;
			// The block has the length at + 4.  Can it fit?
			fseek(fout, address, SEEK_SET);
			ReadLE32(fout, origAddress);
			origAddress &= ~0x08000000;
			fread(&origLen, 1, 1, fout);

			uint32_t blockPos = afterTextPos;
			bool updateNext = true;
			if (origLen == maxLen)
			{
				blockPos = origAddress;
				updateNext = false;
			}
			else
			{
				uint8_t lenByte = (uint8_t)maxLen;
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, blockPos | 0x08000000);
				fwrite(&lenByte, 1, 1, fout);
				// Default to clearing the original width, less surprises.
				if (clearLen == 0)
					clearLen = origLen;
			}

			if (clearLen != 0)
			{
				// Let's also write the original width - hack for our VWF.
				fseek(fout, address + 6, SEEK_SET);
				fputc((uint8_t)clearLen, fout);
			}

			for (int i = 0; i < lines; ++i)
			{
				fgets(str, 5000, fin);
				PrepString(str, str2, 5);

				len = ConvComplexString(str2, sizeof(str2), true);
				if (len > maxLen)
				{
					printf("too long: %s\n", str);
					len = maxLen;
				}

				// Let's read it in from the original position if blank (use a space to prevent.)
				if (len == 0 && origLen <= maxLen)
				{
					fseek(fout, origAddress + i * origLen, SEEK_SET);
					len = (int)fread(str2, 1, origLen, fout);
					if (forceAll)
					{
						char stringID[16];
						sprintf(stringID, "%d:%06X", i, address);
						ForceTranslateComplex(str2, len, stringID);
					}
				}

				InsertStringAt(fout, str2, len, maxLen, blockPos);
				blockPos += maxLen;
				insertions++;
			}

			if (updateNext)
				afterTextPos = blockPos;
		}
		// Alternate for just a pointer: FIXEDPOINTER PointerLoc MaxLen
		else if (!comment && strstr(str, "FIXEDPOINTER") != NULL)
		{
			int maxLen = 0;
			sscanf(str, "%*s %X %X", &address, &maxLen);

			fgets(str, 5000, fin);
			PrepString(str, str2, 5);
			len = ConvComplexString(str2, sizeof(str2), true);

			if (maxLen != 0 && len > maxLen)
			{
				printf("too long: %s\n", str);
				len = maxLen;
			}

			if (len != 0)
			{
				uint32_t pos = InsertString(fout, str2, len, maxLen, afterTextPos);
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, pos | 0x08000000);
				insertions++;
			}
			else if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%06X", address);

				fseek(fout, address, SEEK_SET);
				uint32_t oldPos = 0;
				ReadLE32(fout, oldPos);
				fseek(fout, oldPos & ~0x08000000, SEEK_SET);
				fread(str2, 1, maxLen, fout);
				ForceTranslateComplex(str2, maxLen, stringID);

				// Simulate insertion.
				uint32_t pos = InsertString(fout, str2, maxLen, maxLen, afterTextPos);
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, pos | 0x08000000);
				insertions++;
			}
		}
		// Alternate for pointer with byte specifying length: DYNPOINTER PointerLoc MaxLen SizeLoc SizeLoc2
		else if (!comment && strstr(str, "DYNPOINTER") != NULL)
		{
			int maxLen = 0;
			int sizeAddress = 0;
			int sizeAddress2 = 0;
			sscanf(str, "%*s %X %X %X %X", &address, &maxLen, &sizeAddress, &sizeAddress2);

			fgets(str, 5000, fin);
			PrepString(str, str2, 5);
			len = ConvComplexString(str2, sizeof(str2), true);

			if (maxLen != 0 && len > maxLen)
			{
				printf("too long: %s\n", str);
				len = maxLen;
			}

			if (len != 0)
			{
				fseek(fout, sizeAddress, SEEK_SET);
				int oldLen = fgetc(fout);
				if (oldLen >= len)
				{
					// Let's just keep the old length and overwrite.
					uint32_t oldPtr = 0;
					fseek(fout, address, SEEK_SET);
					ReadLE32(fout, oldPtr);
					InsertStringAt(fout, str2, len, oldLen, oldPtr & ~0x08000000);
				}
				else
				{
					// Write the actual size.  No need for padding or terminators.
					fseek(fout, sizeAddress, SEEK_SET);
					fputc((uint8_t)len, fout);
					if (sizeAddress2 != 0)
					{
						fseek(fout, sizeAddress2, SEEK_SET);
						fputc((uint8_t)len, fout);
					}

					uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);
					fseek(fout, address, SEEK_SET);
					WriteLE32(fout, pos | 0x08000000);
				}
				insertions++;
			}
			else if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%06X", address);

				fseek(fout, sizeAddress, SEEK_SET);
				int oldLen = fgetc(fout);

				uint32_t oldPtr = 0;
				fseek(fout, address, SEEK_SET);
				ReadLE32(fout, oldPtr);

				fseek(fout, oldPtr & ~0x08000000, SEEK_SET);
				fread(str2, 1, maxLen, fout);
				ForceTranslateComplex(str2, maxLen, stringID);

				InsertStringAt(fout, str2, oldLen, oldLen, oldPtr & ~0x08000000);
				insertions++;
			}
		}
		// Rewrite a structure size: RESTRUCT OldAddress OldSize OldStride NewAddress NewSize NewStride Count
		else if (!comment && strstr(str, "RESTRUCT") != NULL)
		{
			int oldAddress = 0, oldLen = 0, oldStride = 0;
			int newAddress = 0, newLen = 0, newStride = 0;
			int lines = 0;
			sscanf(str, "%*s %X %X %X %X %X %X %X", &oldAddress, &oldLen, &oldStride, &newAddress, &newLen, &newStride, &lines);

			for (int i = 0; i < lines; ++i)
			{
				fgets(str, 5000, fin);
				PrepString(str, str2, 5);
				len = ConvComplexString(str2, sizeof(str2), true);

				if (len > newLen)
				{
					printf("too long: %s\n", str);
					len = newLen;
				}

				// Let's read it in from the original position if blank (use a space to prevent.)
				if (len == 0)
				{
					fseek(fout, oldAddress + i * oldStride, SEEK_SET);
					len = (int)fread(str2, 1, oldLen, fout);
					if (forceAll)
					{
						char stringID[16];
						sprintf(stringID, "%d:%06X", i, newAddress);
						ForceTranslateComplex(str2, len, stringID);
					}
				}

				InsertStringAt(fout, str2, len, newLen, newAddress + i * newStride);
				insertions++;
			}
		}
		// A list of pointers: POINTERLIST PointerLoc Stride MaxLen Count
		else if (!comment && strstr(str, "POINTERLIST") != NULL)
		{
			int stride = 0;
			int maxLen = 0;
			int lines = 0;
			sscanf(str, "%*s %X %X %X %X", &address, &stride, &maxLen, &lines);

			for (int i = 0; i < lines; ++i)
			{
				fgets(str, 5000, fin);
				PrepString(str, str2, 5);
				len = ConvComplexString(str2, sizeof(str2), true);

				if (len > maxLen)
				{
					printf("too long: %s\n", str);
					len = maxLen;
				}

				// Let's read it in from the original position if blank (use a space to prevent.)
				if (len == 0)
				{
					uint32_t oldPointer = 0;
					fseek(fout, address + i * stride, SEEK_SET);
					ReadLE32(fout, oldPointer);
					fseek(fout, oldPointer & ~0x08000000, SEEK_SET);
					len = (int)fread(str2, 1, maxLen, fout);
					len = DetectScriptLen(str2, len);
					if (forceAll)
					{
						char stringID[16];
						sprintf(stringID, "%d:%06X", i, address);
						ForceTranslateComplex(str2, len, stringID);
					}
				}

				uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);
				fseek(fout, address + i * stride, SEEK_SET);
				WriteLE32(fout, pos | 0x08000000);

				insertions++;
			}
		}
		// Dynamic format with terminator: TERMSTRING Loc TermChar MaxLen
		else if (!comment && strstr(str, "TERMSTRING") != NULL)
		{
			int termChar = 0;
			int maxLen = 0;
			sscanf(str, "%*s %X %X %X", &address, &termChar, &maxLen);

			fgets(str, 5000, fin);
			PrepString(str, str2, 5);
			len = ConvComplexString(str2, sizeof(str2), true);

			if (maxLen != 0 && len > maxLen)
			{
				printf("too long: %s\n", str);
				len = maxLen;
			}

			if (len != 0)
			{
				str2[len] = termChar;
				uint32_t pos = InsertString(fout, str2, len + 1, len + 1, afterTextPos);
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, pos | 0x08000000);
				insertions++;
			}
			else if (forceAll)
			{
				char stringID[16];
				sprintf(stringID, "%06X", address);

				fseek(fout, address, SEEK_SET);
				uint32_t oldPos = 0;
				ReadLE32(fout, oldPos);
				fseek(fout, oldPos & ~0x08000000, SEEK_SET);
				fread(str2, 1, maxLen, fout);
				ForceTranslateComplex(str2, maxLen, stringID);

				// Simulate insertion.
				uint32_t pos = InsertString(fout, str2, maxLen, maxLen, afterTextPos);
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, pos | 0x08000000);
				insertions++;
			}
		}
		// Old format: BLOCKSTART TextLoc Len Count
		else if (!comment && strstr(str, "BLOCKSTART") != NULL)
		{
			int maxLen = 0;
			int lines = 0;
			sscanf(str, "%*s %X %X %X", &address, &maxLen, &lines);
			//printf("block address:%X  line len:%02X  line count:%02X\n", address, maxLen, lines);

			for (int i = 0; i < lines; i++)
			{
				fgets(str, 5000, fin);
				PrepString(str, str2, 5);

				//for (int j = 0; j < strlen(str2); j++)
				//	printf("%c ", str2[j]);

				len = ConvComplexString(str2, sizeof(str2), true);
				if (len > maxLen)
				{
					printf("too long: %s\n", str);
					len = maxLen;
				}

				if (len != 0)
				{
					InsertStringAt(fout, str2, len, maxLen, address + maxLen * i);
					insertions++;
				}
				else if (forceAll)
				{
					char stringID[16];
					sprintf(stringID, "%d:%06X", i, address);

					fseek(fout, address + maxLen * i, SEEK_SET);
					fread(str2, 1, maxLen, fout);
					ForceTranslateComplex(str2, len, stringID);

					fseek(fout, address + maxLen * i, SEEK_SET);
					fwrite(str2, 1, maxLen, fout);
					insertions++;
				}
			}
		}

		fgets(str, 5000, fin);
	}

	fclose(fin);
	return insertions;
}

uint32_t InsertGimicaTutorialText(FILE *fout, uint32_t &afterTextPos)
{
	FILE *fin = fopen("ta_gimica_tutorial_eng.txt", "r");
	if (!fin)
	{
		printf("Couldn't open ta_gimica_tutorial_eng.txt!\n");
		return 0;
	}

	// Assume a BOM may exist.
	if ((uint8_t)fgetc(fin) == 0xEF)
		fseek(fin, 3, SEEK_SET);
	else
		fseek(fin, 0, SEEK_SET);

	// Includes the space.
	static const int maxLen = 48;
	uint32_t insertions = 0;
	int groupNum = 0;
	int groupCount = 0;
	int state = 0;
	char pendingString[5000];
	int pendingPos = 0;
	std::vector<uint32_t> stringPointers;

	// For reusing prev text.
	static const uint8_t oldGroupOffsets[] = { 0x00, 0x02, 0x03, 0x07, 0x0A, 0x0B, 0x10, 0x11, 0x12, 0x13, 0x1A, 0x1B, 0x1F, 0x23, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x35, 0x37 };

	auto flushGroup = [&]()
	{
		if (pendingPos == 0 && groupNum < 21)
		{
			// Copy over the original text if entirely blank.
			uint32_t pointerLoc = 0x0064B1F8 + oldGroupOffsets[groupNum] * 4;
			uint32_t oldPointer = 0;
			bool foundEnd = false;

			do
			{
				fseek(fout, pointerLoc, SEEK_SET);
				ReadLE32(fout, oldPointer);
				pointerLoc += 4;

				char tutorialLine[maxLen + 1]{};
				fseek(fout, oldPointer & ~0x08000000, SEEK_SET);
				fread(tutorialLine, 1, 20, fout);
				tutorialLine[maxLen] = (char)fgetc(fout);
				if (tutorialLine[maxLen] == 0x7E)
					foundEnd = true;

				if (forceAll)
				{
					char stringID[16];
					sprintf(stringID, "GT%02X", groupNum);
					ForceTranslateComplex(tutorialLine, maxLen, stringID);
				}

				stringPointers.push_back(InsertString(fout, tutorialLine, maxLen + 1, maxLen + 1, afterTextPos));
				insertions++;
			}
			while (!foundEnd);
			groupCount++;
			return;
		}

		pendingString[pendingPos] = '\0';
		int groupLen = ConvComplexString(pendingString, sizeof(pendingString), false);
		// Now we split it back out into separate strings.
		for (int i = 0; i < groupLen; ) {
			int end = i;
			int type = -1;

			do
			{
				char *nextFF = (char *)memchr(pendingString + end, 0xFF, groupLen - end);
				if (!nextFF)
					end = groupLen;
				else
					end = (int)(nextFF - pendingString);
				if (end <= groupLen - 2)
					type = *((uint8_t *)nextFF + 1);
			}
			while (end < groupLen && type != 0 && type != 5);

			// [BREAK] becomes 0x1C, [END] becomes 0x7E.
			if (type == 5)
				type = 0x1C;
			else
				type = 0x7E;
			// Also, if this is the end, force to 0x7E.
			if (end + 2 >= groupLen)
				type = 0x7E;

			// Now let's construct the string.
			char tutorialLine[maxLen + 1]{};
			tutorialLine[0] = (char)0xEF;
			tutorialLine[maxLen] = type;

			if (end - i <= maxLen - 1)
				memcpy(tutorialLine + 1, pendingString + i, end - i);
			else
			{
				pendingString[end] = '\0';
				printf("too long: %s\n", pendingString + i);
				memcpy(tutorialLine + 1, pendingString + i, maxLen - 1);
			}
			i = end + 2;

			stringPointers.push_back(InsertString(fout, tutorialLine, maxLen + 1, maxLen + 1, afterTextPos));
			insertions++;
		}
		pendingPos = 0;
		groupCount++;
	};

	while (!feof(fin))
	{
		char str[5000];
		if (!fgets(str, 5000, fin))
			continue;
		if (str[0] == '#' || str[0] == '\0')
			continue;

		uint32_t num = 0;
		int offset = 0;
		if (sscanf(str, "%X-E:%n", &num, &offset) >= 1)
		{
			// New group of strings.
			if (offset != 0)
			{
				if (state == 1)
					flushGroup();
				state = 0;
				groupNum = num;
			}
		}

		if (state == 0)
		{
			// Brand new string.  We'll write this later (see above.)
			state = 1;
			int toCopy = (int)strlen(str + 5) - 1;
			if (toCopy > 0 && str[5 + toCopy - 1] == '\r')
				toCopy--;
			if (toCopy > 0)
				memcpy(pendingString + pendingPos, str + 5, toCopy);
			pendingPos += toCopy;
		}
		else if (state == 1)
		{
			// Continuation of an existing line.
			int toCopy = (int)strlen(str) - 1;
			if (toCopy > 0 && str[toCopy - 1] == '\r')
				toCopy--;
			if (toCopy > 0)
				memcpy(pendingString + pendingPos, str, toCopy);
			pendingPos += toCopy;
		}
	}

	// Flush any last group.
	flushGroup();

	if (groupCount != 21)
		printf("Wrong number of [END]s in ta_gimica_tutorial_eng.txt: %d instead of 21\n", groupCount);

	if (stringPointers.size() <= 0x38)
		fseek(fout, 0x0064B1F8, SEEK_SET);
	else
	{
		// We'll put it in free space.  Overwrite the pointers it looks up.
		// But make sure we're aligned, first.
		afterTextPos = (afterTextPos + 3) & ~3;
		fseek(fout, 0x00091390, SEEK_SET);
		WriteLE32(fout, afterTextPos | 0x08000000);
		fseek(fout, 0x0009140C, SEEK_SET);
		WriteLE32(fout, afterTextPos | 0x08000000);

		fseek(fout, afterTextPos, SEEK_SET);
		afterTextPos += (int)stringPointers.size() * 4;
	}

	for (uint32_t p : stringPointers)
		WriteLE32(fout, p | 0x08000000);

	fclose(fin);
	return insertions;
}

uint32_t InsertCreditsText(FILE *fout, uint32_t &afterTextPos)
{
	FILE *fin = fopen("ta_credits_eng.txt", "r");
	if (!fin)
	{
		printf("Couldn't open ta_gimica_tutorial_eng.txt!\n");
		return 0;
	}

	std::vector<uint8_t> data;
	// Speed is the first byte, let's keep the original value.
	data.push_back(4);

	bool ended = false;
	while (!feof(fin))
	{
		char str[5000]{};
		if (!fgets(str, 5000, fin))
			continue;
		if (str[0] == '\0')
			continue;

		for (size_t pos = 0, n = strlen(str); pos < n; )
		{
			uint32_t param;
			if (strncmp(str + pos, "[SMALL]", strlen("[SMALL]")) == 0)
			{
				data.push_back(0xFD);
				pos += strlen("[SMALL]");
			}
			else if (strncmp(str + pos, "[SIGNAL]", strlen("[SIGNAL]")) == 0)
			{
				data.push_back(0xFB);
				pos += strlen("[SIGNAL]");
			}
			else if (strncmp(str + pos, "[ICON ", strlen("[ICON ")) == 0 && sscanf(str + pos, "[ICON %02X]", &param) > 0)
			{
				data.push_back(0xFA);
				data.push_back(param);
				pos += strlen("[ICON 00]");
			}
			else if (strncmp(str + pos, "[END]", strlen("[END]")) == 0)
			{
				data.push_back(0xFE);
				ended = true;
				break;
			}
			// Ignore a blank line (don't write 0 length.)
			else if (str[pos] == '\r' || str[pos] == '\n')
				pos++;
			else
			{
				char encoded[5000];
				PrepString(str, encoded, (int)pos);
				int len = ConvComplexString(encoded, 4999, true);
				if (len >= 0xFA) {
					printf("Credits line too long: %s\n", str);
					len = 0xF9;
				}

				data.push_back((uint8_t)len);
				size_t dest = data.size();
				data.resize(dest + len);
				memcpy(&data[dest], encoded, len);
				// We encoded to the end of the line.
				break;
			}
		}

		// Encode the line break.
		if (!ended)
			data.push_back(0xFF);
	}

	if (data.size() <= 2216) {
		// We can insert it in place.
		fseek(fout, 0x0062AAC8, SEEK_SET);
	} else {
		fseek(fout, 0x0002ACC4, SEEK_SET);
		WriteLE32(fout, afterTextPos | 0x08000000);

		fseek(fout, afterTextPos, SEEK_SET);
		afterTextPos += (int)data.size();
	}

	fwrite(&data[0], 1, data.size(), fout);

	fclose(fin);
	return 1;
}

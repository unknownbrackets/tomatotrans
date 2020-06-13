#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

static const uint32_t TOMATO_END_POS = 0x0064B4EC;
static const uint32_t TOMATO_SIZE = 0x007F0000;
std::map<std::vector<uint8_t>, uint32_t> previousInsertions;

struct tableEntry
{
	int  hexVal;
	char str[500];
};

tableEntry table[500];
int        tableLen = 0;

void 		  LoadTable(void);
void          PrepString(char[], char[], int);
unsigned char ConvChar(unsigned char);
void          ConvComplexString(char s[], int &l, bool spaceIsZero);
void          CompileCC(char[], int&, unsigned char[], int&);
int           CharToHex(char);
unsigned int  hstrtoi(char*);
uint32_t UpdatePointers(int, int, FILE *, const char *);
uint32_t InsertEnemies(FILE *, uint32_t &afterTextPos);
void          InsertMenuStuff1(FILE*);
void          InsertStuff(FILE*, char[], int, int, int);
uint32_t InsertMenuStuff2(FILE *, uint32_t &afterTextPos);

//=================================================================================================

bool WriteLE32(FILE *fout, uint32_t value)
{
	uint8_t parts[4];
	parts[0] = (value & 0x000000FF) >> 0;
	parts[1] = (value & 0x0000FF00) >> 8;
	parts[2] = (value & 0x00FF0000) >> 16;
	parts[3] = (value & 0xFF000000) >> 24;

	return fwrite(parts, 1, 4, fout) == 4;
}

bool ReadLE32(FILE *fin, uint32_t &value)
{
	uint8_t parts[4];
	if (fread(parts, 1, 4, fin) != 4)
		return false;

	value = parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
	return true;
}

uint32_t InsertStringAt(FILE *fout, const char *str, uint32_t len, uint32_t fullLen, uint32_t pos)
{
	std::vector<uint8_t> full;
	full.resize(fullLen, 0);
	memcpy(full.data(), str, len);

	fseek(fout, pos, SEEK_SET);
	fwrite(full.data(), 1, fullLen, fout);

	previousInsertions[full] = pos;
	return pos;
}

uint32_t InsertString(FILE *fout, const char *str, uint32_t len, uint32_t fullLen, uint32_t &nextFree)
{
	std::vector<uint8_t> full;
	full.resize(fullLen, 0);
	memcpy(full.data(), str, len);

	auto already = previousInsertions.find(full);
	if (already != previousInsertions.end())
		return already->second;

	uint32_t pos = nextFree;
	nextFree += fullLen;

	fseek(fout, pos, SEEK_SET);
	fwrite(full.data(), 1, fullLen, fout);

	previousInsertions[full] = pos;
	return pos;
}

//=================================================================================================

int main(void)
{
   FILE* fin;
   FILE* fout;
   char  str[5000];
   char  str2[5000];
   int   len;
   int   loc;
   int   i;
   int   j;

   LoadTable();

   fout = fopen("test.gba", "rb+");

   fin = fopen("ta_script_eng.txt", "r");
   if (fin == NULL)
   {
	   printf("Couldn't open ta_script_eng.txt!\n");
	   return -1;
   }

   int totalInsertions = 0;

   i = 0;
   fseek(fout, TOMATO_END_POS, SEEK_SET);
   fgets(str, 5000, fin);
   while(strstr(str, "-E:") == NULL)
   {
      fgets(str, 5000, fin);
   }
   while(!feof(fin))
   {
  	  PrepString(str, str2, 6);

  	  if (str2[0] != '\n')
  	  {
	  	ConvComplexString(str2, len, false);

      	loc = ftell(fout);
	  	for (j = 0; j < len; j++)
 	  	   fputc(str2[j], fout);

		totalInsertions += UpdatePointers(i, loc, fout, "pointers1.dat");
  	  }

      i++;
	  fgets(str, 5000, fin);
	  while(strstr(str, "-E:") == NULL && !feof(fin))
	  {
	   	 fgets(str, 5000, fin);
	  }
   }
   fclose(fin);

   uint32_t afterTextPos = (uint32_t)ftell(fout);

   totalInsertions += InsertEnemies(fout, afterTextPos);
   //InsertMenuStuff1(fout);
   //InsertStuff(fout, "ta_items_eng.txt", 0x4573F2, 5, 8);
   totalInsertions += InsertMenuStuff2(fout, afterTextPos);

   uint32_t usedBytes = afterTextPos - TOMATO_END_POS;
   uint32_t usedPercent = 100 * usedBytes / (TOMATO_SIZE - TOMATO_END_POS);
   printf("Inserted %u strings, using %u extra bytes (%03u%%)\n", totalInsertions, usedBytes, usedPercent);

   fclose(fout);
   return 0;
}

//=================================================================================================

void ConvComplexString(char str[5000], int &newLen, bool spaceIsZero)
{
	char          newStr[5000] = "";
	unsigned char newStream[100];
	int           streamLen =  0;
	int           len = strlen(str) - 1; // minus one to take out the newline
	int           counter = 0;
	int           i;

	newLen = 0;

    while (counter < len)
    {
		//printf("%c", str[counter]);
		if (str[counter] == '[')
		{
		   CompileCC(str, counter, newStream, streamLen);
		   for (i = 0; i < streamLen; i++)
		   {
			   newStr[newLen] = newStream[i];
			   newLen++;
		   }
		   counter++; // to skip past the ]
		}
		else if (str[counter] == ' ' && spaceIsZero)
		{
			newStr[newLen] = '\0';
			newLen++;
			counter++;
		}
		else
		{
		   newStr[newLen] = ConvChar(str[counter]);
		   newLen++;

		   counter++;
		}
	}

	memset(str, '\0', 5000);
	for (i = 0; i < newLen; i++)
	   str[i] = newStr[i];
}


//=================================================================================================

void CompileCC(char str[5000], int& strLoc, unsigned char newStream[100], int& streamLen)
{
   char  str2[5000] = "";
   char* ptr[5000];
   int   ptrCount = 0;
   int   totalLength = strlen(str);
   int   i;
   int   j;
   char  hexVal[100] = "";
   char  specialStr[100] = "";
   int   retVal = 0;


   // we're gonna mess with the original string, so make a backup for later
   strcpy(str2, str);

   // first we gotta parse the codes, what a pain
   ptr[ptrCount++] = &str[strLoc + 1];
   while (str[strLoc] != ']' && strLoc < totalLength)
   {
      if (str[strLoc] == ' ')
      {
         ptr[ptrCount++] = &str[strLoc + 1];
         str[strLoc] = 0;
      }

      strLoc++;
   }

   if (str[strLoc] == ']')
      str[strLoc] = 0;


   // Capitalize all the arguments for ease of use
   for (i = 0; i < ptrCount; i++)
   {
      for (j = 0; j < (int)strlen(ptr[i]); j++)
         ptr[i][j] = toupper(ptr[i][j]);
   }


   // now the actual compiling into the data stream
   streamLen = 0;
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
   else if (strcmp(ptr[0], "MONEY") == 0)
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
	    i = 0;

		while ((i < tableLen) && (retVal == 0))
		{
			if (strcmp(ptr[0], table[i].str) == 0)
			   retVal = table[i].hexVal;
			else
			   i++;
		}

		newStream[streamLen++] = retVal;
		newStream[streamLen++] = 0x00;
		if (retVal == 0)
		   printf("Couldn't convert control code: %s\n", ptr[0]);
   }

   // going to assume raw codes now, in 2-char hex things, like [FA 1A 2C EE]
   else if (strlen(ptr[0]) == 2)
   {
      for (i = 0; i < ptrCount; i++)
         newStream[streamLen++] = hstrtoi(ptr[i]);
   }

   else
      printf("UNKNOWN CONTROL CODE: %s\n", ptr[0]);

   // restore backup string
   strcpy(str, str2);
}

//=================================================================================================

unsigned char ConvChar(unsigned char ch)
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

void LoadTable(void)
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

unsigned int hstrtoi(char* string)
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

void PrepString(char str[5000], char str2[5000], int startPoint)
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

uint32_t UpdatePointers(int lineNum, int loc, FILE *fout, const char *pointersFile)
{
   char  str[5000];
   int   address;
   int   readOK;
   int   tempLoc = ftell(fout);

   FILE *fptrs = fopen(pointersFile, "r");
   if (fptrs == NULL)
   {
	   printf("Couldn't open %s!\n", pointersFile);
	   return 0;
   }

   for (int i = 0; i < lineNum; i++)
      fgets(str, 5000, fptrs);

   //printf("\nline %03X:", lineNum);

   uint32_t insertions = 0;
   readOK = fscanf(fptrs, "%X", &address);
   while ((readOK != 0) && (address != 0xFFFFFFFF))
   {
	  loc |= 0x08000000;
	  //printf("loc:%08X    %08X", loc, address);
	  fseek(fout, address, SEEK_SET);
	  WriteLE32(fout, loc);

      readOK = fscanf(fptrs, "%X", &address);
	  insertions++;
   }
   //printf("\n");

   fclose(fptrs);

   fseek(fout, tempLoc, SEEK_SET);
   return insertions;
}

//=================================================================================================

int DetectFixedLen(const char *str, int maxLen)
{
	for (int i = maxLen; i > 0; --i)
	{
		if (str[i - 1] != 0)
			return i;
	}
	return 0;
}

uint32_t EnemyInfoAddress(int i, int state)
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

		int len = 0;
		PrepString(str, str2, 5);
		ConvComplexString(str2, len, true);
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

void InsertMenuStuff1(FILE* fout)
{
	FILE* fin;
	char  str[5000];
	char  str2[5000];
	int   i = 0;
	int   j;
	int   len;

    fin = fopen("ta_menus1_eng.txt", "r");
    if (fin == NULL)
    {
		printf("Couldn't open ta_menus1_eng.txt!\n");
		return;
	}


   fgets(str, 5000, fin);
   while(strstr(str, "-E:") == NULL)
   {
      fgets(str, 5000, fin);
   }
   while(!feof(fin))
   {
  	  PrepString(str, str2, 5);

  	  if (str2[0] != '\n')
  	  {
		for (j = 0; j < (int)strlen(str2); j++)
		   printf("%c ", str2[j]);

	  	ConvComplexString(str2, len, false);

	  	if (len > 7)
	  	{
			printf("too long: %s\n", str);
			len = 7;
		}

 	    fseek(fout, 0x7F0000 + 0x7 * i, SEEK_SET);
 	    for (j = 0; j < 7; j++)
 	       fputc(0x00, fout);

      	fseek(fout, 0x7F0000 + 0x7 * i, SEEK_SET);
	  	for (j = 0; j < len; j++)
 	  	   fputc(str2[j], fout);
  	  }

      i++;
	  fgets(str, 5000, fin);
	  while(strstr(str, "-E:") == NULL && !feof(fin))
	  {
	   	 fgets(str, 5000, fin);
	  }
   }

   fclose(fin);
}

//=================================================================================================

void InsertStuff(FILE* fout, char inFile[], int address, int startPos, int maxLen)
{
	FILE* fin;
	char  str[5000];
	char  str2[5000];
	int   i = 0;
	int   j;
	int   len;

    fin = fopen(inFile, "r");
    if (fin == NULL)
    {
		printf("Couldn't open %s!\n", inFile);
		return;
	}


   fgets(str, 5000, fin);
   while(strstr(str, "-E:") == NULL)
   {
      fgets(str, 5000, fin);
   }
   while(!feof(fin))
   {
  	  PrepString(str, str2, startPos);

  	  if (str2[0] != '\n')
  	  {
		//for (j = 0; j < strlen(str2); j++)
		//   printf("%c ", str2[j]);

	  	ConvComplexString(str2, len, false);

	  	if (len > maxLen)
	  	{
			printf("too long: %s\n", str);
			len = maxLen;
		}

 	    fseek(fout, address + maxLen * i, SEEK_SET);
 	    for (j = 0; j < maxLen; j++)
 	       fputc(0x00, fout);

      	fseek(fout, address + maxLen * i, SEEK_SET);
	  	for (j = 0; j < len; j++)
 	  	   fputc(str2[j], fout);
  	  }

      i++;
	  fgets(str, 5000, fin);
	  while(strstr(str, "-E:") == NULL && !feof(fin))
	  {
	   	 fgets(str, 5000, fin);
	  }
   }

   fclose(fin);
}

//=================================================================================================

int DetectScriptLen(const char *str, int maxLen)
{
	int pos = 0;
	while (pos < maxLen)
	{
		// Did we find FF 00 (END)?
		if (pos != 0 && str[pos] == 0)
			return pos + 1;
		const char *next = (const char *)memchr(str + pos, 0xFF, maxLen - pos);
		if (!next)
			break;

		pos = next - str + 1;
	}

	return maxLen;
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

				ConvComplexString(str2, len, true);
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
			ConvComplexString(str2, len, true);

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
			ConvComplexString(str2, len, true);

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
				ConvComplexString(str2, len, true);

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
				ConvComplexString(str2, len, true);

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
					fseek(fout, oldPointer, SEEK_SET);
					len = (int)fread(str2, 1, maxLen, fout);
					len = DetectScriptLen(str2, len);
				}

				uint32_t pos = InsertString(fout, str2, len, len, afterTextPos);
				fseek(fout, address + i * stride, SEEK_SET);
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
		   	   //   printf("%c ", str2[j]);

	  		   ConvComplexString(str2, len, false);
	  		   if (len > maxLen)
	  		   {
			      printf("too long: %s\n", str);
			      len = maxLen;
			   }

			   if (str2[0] != 0x0)
			   {
					InsertStringAt(fout, str2, len, maxLen, address + maxLen * i);
					insertions++;
		   	   }
		   }
		}

		fgets(str, 5000, fin);
	}

	fclose(fin);
	return insertions;
}

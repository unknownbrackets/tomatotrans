#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>

static const uint32_t TOMATO_END_POS = 0x0064B4EC;
static const uint32_t TOMATO_SIZE = 0x007F0000;

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
uint32_t InsertEnemies(FILE*);
void          InsertMenuStuff1(FILE*);
void          InsertStuff(FILE*, char[], int, int, int);
uint32_t InsertMenuStuff2(FILE *, uint32_t &afterTextPos);

//=================================================================================================

bool WriteLE32(FILE *fout, uint32_t value) {
	uint8_t parts[4];
	parts[0] = (value & 0x000000FF) >> 0;
	parts[1] = (value & 0x0000FF00) >> 8;
	parts[2] = (value & 0x00FF0000) >> 16;
	parts[3] = (value & 0xFF000000) >> 24;

	return fwrite(parts, 1, 4, fout) == 4;
}

bool ReadLE32(FILE *fin, uint32_t &value) {
	uint8_t parts[4];
	if (fread(parts, 1, 4, fin) != 4)
		return false;

	value = parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
	return true;
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

   totalInsertions += InsertEnemies(fout);
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
   else if (strcmp(ptr[0], "NEWWIN") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x02;
   }
   else if (strcmp(ptr[0], "BREAK") == 0)
   {
       newStream[streamLen++] = 0xFF;
       newStream[streamLen++] = 0x05;
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

uint32_t InsertEnemies(FILE* fout)
{
	char  str[5000];
	char  str2[5000];
	int   i = 0;
	int   j;
	int   len;

    FILE *fin = fopen("ta_enemies_eng.txt", "r");
    if (fin == NULL)
    {
		printf("Couldn't open ta_enemies_eng.txt!\n");
		return 0;
	}

	uint32_t insertions = 0;

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
		//for (j = 0; j < strlen(str2); j++)
		//   printf("%c ", str2[j]);

	  	ConvComplexString(str2, len, false);

	  	if (len > 8)
	  	{
			printf("Enemy name too long: %s\n", str);
			len = 8;
		}

 	    fseek(fout, 0x634F9C + 0x4C * i, SEEK_SET);
 	    for (j = 0; j < 8; j++)
 	       fputc(0x00, fout);

      	fseek(fout, 0x634F9C + 0x4C * i, SEEK_SET);
	  	for (j = 0; j < len; j++)
 	  	   fputc(str2[j], fout);

		insertions++;
  	  }

      i++;
	  fgets(str, 5000, fin);
	  while(strstr(str, "-E:") == NULL && !feof(fin))
	  {
	   	 fgets(str, 5000, fin);
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

uint32_t InsertMenuStuff2(FILE *fout, uint32_t &afterTextPos)
{
	char  str[5000];
	char  str2[5000];
	int   address;
	int   len;
	int   maxLen;
	int   lines;

	FILE *fin = fopen("ta_menus_eng.txt", "r");
	if (!fin)
	{
		printf("Couldn't open ta_menus_eng.txt!\n");
		return 0;
	}

	// Just in case, let's make sure it's aligned.
	uint32_t nextPos = (afterTextPos + 1) & ~1;
	uint32_t insertions = 0;

	fgets(str, 5000, fin);
	while (!feof(fin))
	{
		// New format: BLOCKFIXED PointerLoc NewLen Count
		if (strstr(str, "BLOCKFIXED") != NULL)
		{
			sscanf(str, "%*s %X %X %X", &address, &maxLen, &lines);
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

			uint32_t blockPos = nextPos;
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
				if (len == 0 && origLen <= maxLen && updateNext)
				{
					fseek(fout, origAddress + i * origLen, SEEK_SET);
					len = (int)fread(str2, 1, origLen, fout);
				}

				fseek(fout, blockPos, SEEK_SET);
				for (int j = 0; j < maxLen; ++j)
				{
					char c = j < len ? str2[j] : '\0';
					fputc(c, fout);
				}
				blockPos += maxLen;
				insertions++;
			}

			if (updateNext)
				nextPos = (blockPos + 1) & ~1;
		}
		// Alternate for just a pointer: BLOCKPOINTER PointerLoc
		else if (strstr(str, "BLOCKPOINTER") != NULL)
		{
			sscanf(str, "%*s %X", &address);

			fgets(str, 5000, fin);
			PrepString(str, str2, 5);
			ConvComplexString(str2, len, true);

			if (len != 0)
			{
				fseek(fout, address, SEEK_SET);
				WriteLE32(fout, nextPos | 0x08000000);

				fseek(fout, nextPos, SEEK_SET);
				for (int j = 0; j < len + 1; ++j) {
					char c = j < len ? str2[j] : '\0';
					fputc(c, fout);
				}
				nextPos += len + 1;
				nextPos = (nextPos + 1) & ~1;
				insertions++;
			}
		}
		// Old format: BLOCKSTART TextLoc Len Count
		else if (strstr(str, "BLOCKSTART") != NULL)
		{
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
				   fseek(fout, address + maxLen * i, SEEK_SET);
			       for (int j = 0; j < maxLen; j++)
			          fputc(0x00, fout);

			   	   fseek(fout, address + maxLen * i, SEEK_SET);
			       for (int j = 0; j < len; j++)
			         fputc(str2[j], fout);
				   insertions++;
		   	   }
		   }
		}

		fgets(str, 5000, fin);
	}
	afterTextPos = (nextPos + 1) & ~1;

	fclose(fin);
	return insertions;
}

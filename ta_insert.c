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

void 		  LoadTable(void);
void          PrepString(char[], char[], int);
unsigned char ConvChar(unsigned char);
void          ConvComplexString(char[], int&);
void          CompileCC(char[], int&, unsigned char[], int&);
int           CharToHex(char);
unsigned int  hstrtoi(char*);
void          UpdatePointers(int, int, FILE*, char*);
void          InsertEnemies(FILE*);
void          InsertMenuStuff1(FILE*);
void          InsertStuff(FILE*, char[], int, int, int);
void          InsertMenuStuff2(FILE*);

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

   i = 0;
   fseek(fout, 0x64B4EC, SEEK_SET);
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
	  	ConvComplexString(str2, len);

      	loc = ftell(fout);
	  	for (j = 0; j < len; j++)
 	  	   fputc(str2[j], fout);

      	UpdatePointers(i, loc, fout, "pointers1.dat");
  	  }

      i++;
	  fgets(str, 5000, fin);
	  while(strstr(str, "-E:") == NULL && !feof(fin))
	  {
	   	 fgets(str, 5000, fin);
	  }
   }
   fclose(fin);


   InsertEnemies(fout);
   //InsertMenuStuff1(fout);
   //InsertStuff(fout, "ta_items_eng.txt", 0x4573F2, 5, 8);
   InsertMenuStuff2(fout);

   fclose(fout);
   return 0;
}

//=================================================================================================

void ConvComplexString(char str[5000], int& newLen)
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
		else
		{
		   newStr[newLen] = ConvChar(str[counter]);
		   newLen++;

		   counter++;
		}
	}

    for (i = 0; i < 5000; i++)
       str[i] = '\0';
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
   FILE* fin;
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
      for (j = 0; j < strlen(ptr[i]); j++)
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

   else if ((isalpha(ptr[0][0]) == true) && (strlen(ptr[0]) != 2))
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

void PrepString(char str[5000], char str2[5000], int startPoint)
{
	int j;
	int ctr;

    for (j = 0; j < 5000; j++)
	    str2[j] = '\0';

    ctr = 0;
	for (j = startPoint; j < strlen(str); j++)
	{
	   str2[ctr] = str[j];
	   ctr++;
	}

}

//=================================================================================================

void UpdatePointers(int lineNum, int loc, FILE* fout, char* pointersFile)
{
   FILE* fptrs;
   char  str[5000];
   int   i;
   int   address;
   int   readOK;
   int   tempLoc = ftell(fout);
   int   ch;

   fptrs = fopen(pointersFile, "r");
   if (fptrs == NULL)
   {
	   printf("Couldn't open %s!\n", pointersFile);
	   return;
   }

   for (i = 0; i < lineNum; i++)
      fgets(str, 5000, fptrs);

   printf("\nline %03X:", lineNum);

   readOK = fscanf(fptrs, "%X", &address);
   while ((readOK != 0) && (address != 0xFFFFFFFF))
   {
	  loc += 0x8000000;
	  printf("loc:%08X    %08X", loc, address);
	  fseek(fout, address, SEEK_SET);

	  fputc(loc & 0x000000FF, fout);
      fputc((loc & 0x0000FF00) >> 8, fout);
	  fputc((loc & 0x00FF0000) >> 16, fout);
      fputc(loc >> 24, fout);

      readOK = fscanf(fptrs, "%X", &address);
   }
   //printf("\n");

   fclose(fptrs);

   fseek(fout, tempLoc, SEEK_SET);
}

//=================================================================================================

void InsertEnemies(FILE* fout)
{
	FILE* fin;
	char  str[5000];
	char  str2[5000];
	int   i = 0;
	int   j;
	int   len;

    fin = fopen("ta_enemies_eng.txt", "r");
    if (fin == NULL)
    {
		printf("Couldn't open ta_enemies_eng.txt!\n");
		return;
	}


   fgets(str, 5000, fin);
   while(strstr(str, "-E:") == NULL)
   {
      fgets(str, 5000, fin);
   }
   while(!feof(fin))
   {
      //offset[i] = currOffset;
      // copy string minus the number and -E: stuff

  	  PrepString(str, str2, 5);

  	  if (str2[0] != '\n')
  	  {
		//for (j = 0; j < strlen(str2); j++)
		//   printf("%c ", str2[j]);

	  	ConvComplexString(str2, len);

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
		for (j = 0; j < strlen(str2); j++)
		   printf("%c ", str2[j]);

	  	ConvComplexString(str2, len);

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

	  	ConvComplexString(str2, len);

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

void InsertMenuStuff2(FILE* fout)
{
	FILE* fin;
	char  str[5000];
	char  str2[5000];
	int   address;
	int   len;
	int   maxLen;
	int   lines;
	int   i;
	int   j;

	fin = fopen("ta_menus_eng.txt", "r");
	if (fin == NULL)
	{
		printf("Couldn't open ta_menus_eng.txt!\n");
		return;
	}

	fgets(str, 5000, fin);
	while (!feof(fin))
	{
		//printf(str);
		if (strstr(str, "BLOCKSTART") != NULL)
		{
		   sscanf(str, "%*s %X %X %X", &address, &maxLen, &lines);
		   //printf("block address:%X  line len:%02X  line count:%02X\n", address, maxLen, lines);

		   for (i = 0; i < lines; i++)
		   {
			   fgets(str, 5000, fin);
  	  		   PrepString(str, str2, 5);

 		       //for (j = 0; j < strlen(str2); j++)
		   	   //   printf("%c ", str2[j]);

	  		   ConvComplexString(str2, len);
	  		   if (len > maxLen)
	  		   {
			      printf("too long: %s\n", str);
			      len = maxLen;
			   }

			   if (str2[0] != 0x0)
			   {
				   fseek(fout, address + maxLen * i, SEEK_SET);
			       for (j = 0; j < maxLen; j++)
			          fputc(0x00, fout);

			   	   fseek(fout, address + maxLen * i, SEEK_SET);
			       for (j = 0; j < len; j++)
			         fputc(str2[j], fout);
		   	   }
		   }
		}

		fgets(str, 5000, fin);
	}


	fclose(fin);
}

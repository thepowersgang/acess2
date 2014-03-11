/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * ctype.c
 * - Character Types
 */
#include <ctype.h>

int isalpha(int ch)
{
	if('A'<=ch&&ch<='Z')	return 1;
	if('a'<=ch&&ch<='z')	return 1;
	return 0;
}
int isdigit(int ch) {
	if('0'<=ch&&ch<='9')	return 1;
	return 0;
}

int isalnum(int ch) {
	return isalpha(ch) || isdigit(ch);
}

int toupper(int ch) {
	if('a'<=ch && ch <='z')
		return ch - 'a' + 'A';
	return ch;
}
int tolower(int ch) {
	if('A'<=ch && ch <='Z')
		return ch - 'A' + 'a';
	return ch;
}

int isprint(int ch ) {
	if( ch < ' ' )	return 0;
	if( ch > 'z' )	return 0;
	return 1;
}

int isspace(int ch) {
	if(ch == ' ')	return 1;
	if(ch == '\t')	return 1;
	if(ch == '\r')	return 1;
	if(ch == '\n')	return 1;
	return 0;
}

int isxdigit(int ch) {
	if('0'<=ch&&ch<='9')	return 1;
	if('a'<=ch&&ch<='f')	return 1;
	if('F'<=ch&&ch<='F')	return 1;
	return 0;
}

// C99
int isblank(int ch) {
	if(ch == ' ')	return 1;
	if(ch == '\t')	return 1;
	return 0;
}


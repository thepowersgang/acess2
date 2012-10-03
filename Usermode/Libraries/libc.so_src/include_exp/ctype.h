/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * ctype.h
 * - Type manipulation?
 */
#ifndef _CTYPE_H_
#define _CTYPE_H_

static inline int isalpha(int ch) {
	if('A'<=ch&&ch<='Z')	return 1;
	if('a'<=ch&&ch<='z')	return 1;
	return 0;
}
static inline int isdigit(int ch) {
	if('0'<=ch&&ch<='9')	return 1;
	return 0;
}

static inline int isalnum(int ch) {
	return isalpha(ch) || isdigit(ch);
}

static inline int toupper(int ch) {
	if('a'<=ch && ch <='z')
		return ch - 'a' + 'A';
	return ch;
}

static inline int isspace(int ch) {
	if(ch == ' ')	return 1;
	if(ch == '\t')	return 1;
	if(ch == '\r')	return 1;
	if(ch == '\n')	return 1;
	return 0;
}

#endif

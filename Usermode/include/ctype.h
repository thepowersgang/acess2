/*
 */
#ifndef _CTYPE_H_
#define _CTYPE_H_

static inline int isalpha(char ch) {
	if('A'<=ch&&ch<='Z')	return 1;
	if('a'<=ch&&ch<='z')	return 1;
	return 0;
}
static inline int isdigit(char ch) {
	if('0'<=ch&&ch<='9')	return 1;
	return 0;
}

#endif

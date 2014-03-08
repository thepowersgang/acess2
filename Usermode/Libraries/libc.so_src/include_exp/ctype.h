/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * ctype.h
 * - Type manipulation?
 */
#ifndef _CTYPE_H_
#define _CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int isalpha(int ch);
extern int isdigit(int ch);

extern int isalnum(int ch);

extern int toupper(int ch);
extern int tolower(int ch);

extern int isprint(int ch);

extern int isspace(int ch);

extern int isxdigit(int ch);

// C99
extern int isblank(int ch);

#ifdef __cplusplus
}
#endif

#endif

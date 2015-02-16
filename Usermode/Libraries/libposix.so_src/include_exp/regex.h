/*
 * Acess2 POSIX Emulation Library
 * - By John Hodge (thePowersGang)
 *
 * regex.h
 * - POSIX regular expression support
 */
#ifndef _LIBPOSIX_REGEX_H_
#define _LIBPOSIX_REGEX_H_

typedef struct {
	void *unused;
} regex_t;

typedef size_t	regoff_t;

typedef struct {
	regoff_t rm_so;
	regoff_t rm_eo;
} regmatch_t;

extern int regcomp(regex_t *preg, const char *regex, int cflags);
extern int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);
extern size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size);
extern void regfree(regex_t *preg);

enum {
	REG_BADBR = 1,
	REG_BADPAT,
	REG_BADRPT,
};

#define REG_EXTENDED	0x1
#define REG_ICASE	0x2
#define REG_NOSUB	0x4
#define REG_NEWLINE	0x8


#endif



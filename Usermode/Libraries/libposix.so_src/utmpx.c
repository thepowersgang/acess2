/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * utmpx.c
 * - User login records
 */
#include <stddef.h>
#include <utmpx.h>

/*
 * Rewind UTMPX file pointer
 */
void setutxent(void)
{
}

/*
 * Close UTMPX file
 */
void endutxent(void)
{
}

/*
 * Read from current position in UTMPX file
 */
struct utmpx *getutxent(void)
{
	return NULL;
}

/*
 * Locate an entry in the UTMPX file for a given id
 */
struct utmpx *getutxid(const struct utmpx *utmpx __attribute__((unused)))
{
	return NULL;
}

struct utmpx *getutxline(const struct utmpx *utmpx __attribute__((unused)))
{
	utmpx = NULL;
	return NULL;
}

struct utmpx *pututxline(const struct utmpx *utmpx __attribute__((unused)))
{
	utmpx = NULL;
	return NULL;
}


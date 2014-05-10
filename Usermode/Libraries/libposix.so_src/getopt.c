/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge (thePowersGang)
 *
 * getopt.c
 * - getopt() command line parsing code
 */
#include <getopt.h>

// === GLOBALS ===
char*	optarg;
 int	opterr;
 int	optind;
 int	optopt;

// === CODE ===
int getopt(int argc, char * const argv[], const char *optstring)
{
	return -1;
}


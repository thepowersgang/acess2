/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge (thePowersGang)
 *
 * getopt.h
 * - getopt() command line parsing code
 */
#ifndef _LIBPOSIX_GETOPT_H_
#define _LIBPOSIX_GETOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

extern char*	optarg;
extern int	opterr;
extern int	optind;
extern int	optopt;

extern int	getopt(int argc, char * const argv[], const char *optstring);

#ifdef __cplusplus
}
#endif

#endif


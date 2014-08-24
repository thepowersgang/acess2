/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * inttypes.h
 * - Fixed-size integer sizes
 *
 * Defined in IEEE Std 1003.1
 */
#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#include <stdint.h>

#if INT64MAX == LONG_MAX
# define _PRI64	"l"
#else
# define _PRI64	"ll"
#endif

#define PRId64	_PRI64"ld"
#define PRIdLEAST64	_PRI64"ld"
#define PRIdFAST64	_PRI64"ld"
#define PRIdMAX
#define PRIdPTR
#define PRIi64	_PRI64"i"
#define PRIiLEAST64
#define PRIiFAST64
#define PRIiMAX
#define PRIiPTR

#define PRIx64	_PRI64"i"

#endif

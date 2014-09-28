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
#include <limits.h>

#if INT64_MAX == LONG_MAX
# define _PRI64	"l"
#else
# define _PRI64	"ll"
#endif

#define PRId64	_PRI64"d"
#define PRIdLEAST64	_PRI64"d"
#define PRIdFAST64	_PRI64"d"
#define PRIdMAX
#define PRIdPTR
#define PRIi64	_PRI64"i"
#define PRIiLEAST64
#define PRIiFAST64
#define PRIiMAX
#define PRIiPTR

#define PRIx64	_PRI64"i"

#endif

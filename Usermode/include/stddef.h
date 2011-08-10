/*
 * AcessOS LibC
 * stddef.h
 */
#ifndef _STDDEF_H
#define _STDDEF_H

// === CONSTANTS ===
#ifndef NULL
# define NULL	((void*)0)
#endif

// === TYPES ===
#ifndef size_t
typedef unsigned int	size_t;
#endif

// === MACROS ===
#define offsetof(st, m) ((size_t)((uintptr_t)((char *)&((st *)(0))->m - (char *)0 )))

#endif

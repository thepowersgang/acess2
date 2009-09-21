/*
AcessOS LibC

stdlib.h
*/
#ifndef __STDLIB_H
#define __STDLIB_H

#include <stdarg.h>
#include <sys/types.h>

#ifndef NULL
# define NULL	((void*)0)
#endif

typedef unsigned int	size_t;

// --- Spinlock Macros ---
#define DEFLOCK(_name)	static int _spinlock_##_name=0;
//#define LOCK(_name)	__asm__ __volatile__("jmp ._tst;\n\t._lp:call yield;\n\t._tst:lock btsl $1,(%0);\n\tjc ._lp"::"D"(&_spinlock_##_name))
#define LOCK(_name)	do{int v=1;while(v){__asm__ __volatile__("lock cmpxchgl %%eax, (%1)":"=a"(v):"D"((&_spinlock_##_name)),"a"(1));yield();}}while(0)
#define UNLOCK(_name) __asm__ __volatile__("lock andl $0, (%0)"::"D"(&_spinlock_##_name))

// --- StdLib ---
extern int	atoi(const char *ptr);

extern void *memcpy(void *dest, void *src, size_t count);
extern void *memmove(void *dest, void *src, size_t count);

// --- Environment ---
extern char	*getenv(const char *name);

// --- Heap ---
extern void free(void *mem);
extern void *malloc(unsigned int bytes);
extern void *realloc(void *oldPos, unsigned int bytes);
extern int	IsHeap(void *ptr);

// --- Strings ---
extern int	strlen(const char *string);
extern int	strcmp(char *str1, char *str2);
extern int	strncmp(char *str1, char *str2, size_t len);
extern char	*strcpy(char *dst, const char *src);

#ifndef SEEK_CUR
# define SEEK_CUR	0
# define SEEK_SET	1
# define SEEK_END	(-1)
#endif

#endif

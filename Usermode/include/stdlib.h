/*
 * AcessOS LibC
 * stdlib.h
 */
#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0

// --- Spinlock Macros ---
#define DEFLOCK(_name)	static int _spinlock_##_name=0;
//#define LOCK(_name)	__asm__ __volatile__("jmp ._tst;\n\t._lp:call yield;\n\t._tst:lock btsl $1,(%0);\n\tjc ._lp"::"D"(&_spinlock_##_name))
#define LOCK(_name)	do{int v=1;while(v){__asm__ __volatile__("lock cmpxchgl %%eax, (%1)":"=a"(v):"D"((&_spinlock_##_name)),"a"(1));yield();}}while(0)
#define UNLOCK(_name) __asm__ __volatile__("lock andl $0, (%0)"::"D"(&_spinlock_##_name))

// --- StdLib ---
extern void	_exit(int code) __attribute__((noreturn));	//NOTE: Also defined in acess/sys.h
extern int	atoi(const char *ptr);
extern void	exit(int status) __attribute__((noreturn));
extern void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

// --- Environment ---
extern char	*getenv(const char *name);

// --- Heap ---
extern void free(void *mem);
extern void *malloc(size_t bytes);
extern void *calloc(size_t __nmemb, size_t __size);
extern void *realloc(void *__ptr, size_t __size);
extern int	IsHeap(void *ptr);

#ifndef SEEK_CUR
# define SEEK_CUR	0
# define SEEK_SET	1
# define SEEK_END	(-1)
#endif

#endif

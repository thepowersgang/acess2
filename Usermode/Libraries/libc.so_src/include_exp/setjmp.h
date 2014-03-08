/*
 * Acess2 LibC
 * - By John Hodge (thePowersGang)
 * 
 * setjmp.h
 * - setjmp/longjmp support
 */
#ifndef _LIBC_SETJMP_H_
#define _LIBC_SETJMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386__)
typedef void	*jmp_buf[8];
#elif defined(__amd64__)
typedef void	*jmp_buf[16];
#else
# error "Unknown Architecture"
#endif

extern int	setjmp(jmp_buf buf);
extern void	longjmp(jmp_buf buf, int val);

#ifdef __cplusplus
}
#endif

#endif


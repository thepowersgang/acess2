/*
 * Acess2 LibC
 * - By John Hodge (thePowersGang)
 * 
 * setjmp.h
 * - setjmp/longjmp support
 */
#ifndef _LIBC_SETJMP_H_
#define _LIBC_SETJMP_H_

#if ARCHDIR_is_x86
typedef void	*jmp_buf[8];
#elif ARCHDIR_is_x86_64
typedef void	*jmp_buf[16];
#else
# error "Unknown Architecture"
#endif

extern int	setjmp(jmp_buf buf);
extern void	longjmp(jmp_buf buf, int val);

#endif


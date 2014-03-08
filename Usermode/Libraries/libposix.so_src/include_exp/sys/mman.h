/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * sys/mman.h
 * - Memory Management (mmap and friends)
 */
#ifndef _LIBPOSIX__SYS_MMAN_H_
#define _LIBPOSIX__SYS_MMAN_H_

extern void	*mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
extern int	munmap(void *addr, size_t length);


#endif


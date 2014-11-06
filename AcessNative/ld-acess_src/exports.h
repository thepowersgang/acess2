/*
 * AcessNative Dymamic Linker
 * - By John Hodge (thePowersGang)
 *
 * exports.h
 * - Syscalls/Symbol definitions
 */
#ifndef _EXPORTS_H_
#define _EXPORTS_H_

#include <stddef.h>
#include <stdint.h>

// Syscall request (used by acess_*)
extern uint64_t	_Syscall(int SyscallID, const char *ArgTypes, ...);

extern int	acess__errno;

extern int	native_open(const char *Path, int Flags);
extern int	native_shm(const char *Tag, int Flags);
extern void	native_close(int FD);
extern size_t	native_read(int FD, void *Dest, size_t Bytes);
extern size_t	native_write(int FD, const void *Src, size_t Bytes);
extern int	native_seek(int FD, int64_t Offset, int Dir);
extern uint64_t	native_tell(int FD);

extern int	native_execve(const char *filename, const char *const argv[], const char *const envp[]);
extern int	native_spawn(const char *filename, const char *const argv[], const char *const envp[]);

// Syscalls used by the linker
extern int	acess__SysOpen(const char *Path, unsigned int Flags);
extern void	acess__SysClose(int FD);
extern size_t	acess__SysRead(int FD, void *Dest, size_t Bytes);
extern int	acess__SysSeek(int FD, int64_t Offset, int Dir);

// Symbol type
typedef struct {
	const char	*Name;
	void	*Value;
}	tSym;

#endif


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

// Syscall request (used by acess_*)
extern uint64_t	_Syscall(int SyscallID, const char *ArgTypes, ...);

extern int	native_open(const char *Path, int Flags);
extern void	native_close(int FD);
extern size_t	native_read(int FD, void *Dest, size_t Bytes);
extern size_t	native_write(int FD, const void *Src, size_t Bytes);
extern int	native_seek(int FD, int64_t Offset, int Dir);
extern uint64_t	native_tell(int FD);

extern int	native_execve(const char *filename, char *const argv[], char *const envp[]);

// Syscalls used by the linker
extern int	acess_open(const char *Path, int Flags);
extern void	acess_close(int FD);
extern size_t	acess_read(int FD, void *Dest, size_t Bytes);
extern int	acess_seek(int FD, int64_t Offset, int Dir);

// Symbol type
typedef struct {
	const char	*Name;
	void	*Value;
}	tSym;

#endif


/*
 * AcessNative Dymamic Linker
 * - By John Hodge (thePowersGang)
 *
 * exports.h
 * - Syscalls/Symbol definitions
 */
#ifndef _EXPORTS_H_
#define _EXPORTS_H_

// Syscall request (used by acess_*)
extern uint64_t	_Syscall(int SyscallID, const char *ArgTypes, ...);

extern int	native_open(const char *Path, int Flags);
extern void	native_close(int FD);
extern size_t	native_read(int FD, size_t Bytes, void *Dest);
extern size_t	native_write(int FD, size_t Bytes, const void *Src);
extern int	native_seek(int FD, int64_t Offset, int Dir);
extern uint64_t	native_tell(int FD);

// Syscalls used by the linker
extern int	acess_open(const char *Path, int Flags);
extern void	acess_close(int FD);
extern size_t	acess_read(int FD, size_t Bytes, void *Dest);
extern int	acess_seek(int FD, int64_t Offset, int Dir);

// Symbol type
typedef struct {
	const char	*Name;
	void	*Value;
}	tSym;

#endif


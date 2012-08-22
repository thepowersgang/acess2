/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 * 
 * include/disktool_common.h
 * - DiskTool internal API between native and kernel code
 */
#ifndef _INCLUDE__DISKTOOL_COMMON_H_
#define _INCLUDE__DISKTOOL_COMMON_H_

extern void	DiskTool_Cleanup(void);

extern int	DiskTool_RegisterLVM(const char *Identifier, const char *Path);
extern int	DiskTool_MountImage(const char *Identifier, const char *Path);
extern int	DiskTool_Copy(const char *Source, const char *Destination);
extern int	DiskTool_ListDirectory(const char *Directory);
extern int	DiskTool_Cat(const char *File);

extern size_t	DiskTool_int_TranslatePath(char *Buffer, const char *Path);

extern size_t	_fwrite_stdout(size_t bytes, const void *data);

#endif


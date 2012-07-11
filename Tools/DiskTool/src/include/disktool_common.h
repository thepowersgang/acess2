/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 * 
 * include/disktool_common.h
 * - DiskTool internal API between native and kernel code
 */
#ifndef _INCLUDE__DISKTOOL_COMMON_H_
#define _INCLUDE__DISKTOOL_COMMON_H_

extern int	DiskTool_MountImage(const char *Identifier, const char *Path);
extern int	DiskTool_Copy(const char *Source, const char *Destination);
extern int	DiskTool_ListDirectory(const char *Directory);

extern size_t	DiskTool_int_TranslatePath(char *Buffer, const char *Path);

#endif


/*
 * Acess2
 * - SysFS Export Header
 */
#ifndef _FS_SYSFS_H_
#define _FS_SYSFS_H_

extern int	SysFS_RegisterFile(char *Path, char *Data, int Length);
extern int	SysFS_UpdateFile(int ID, char *Data, int Length);
extern int	SysFS_RemoveFile(int ID);

#endif

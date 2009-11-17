/*
 * Acess 2
 * vfs_ext.h
 * - Exported Symbols from the VFS
 */
#ifndef _VFS_EXT_H
#define _VFS_EXT_H

// === CONSTANTS ===
#define	VFS_MEMPATH_SIZE	(3 + (BITS/8)*2)
#define VFS_OPENFLAG_EXEC	0x01
#define VFS_OPENFLAG_READ	0x02
#define VFS_OPENFLAG_WRITE	0x04
#define	VFS_OPENFLAG_NOLINK	0x40
#define	VFS_OPENFLAG_USER	0x80
#define VFS_KERNEL_FLAG	0x40000000

#define SEEK_SET	1
#define SEEK_CUR	0
#define SEEK_END	-1

// -- System Call Structures ---
/**
 * \brief ACL Defintion Structure
 */
typedef struct sVFS_ACL
{
	struct {
		unsigned Group:	1;	//!< Group (as opposed to user) flag
		unsigned ID:	31;	//!< ID of Group/User (-1 for nobody/world)
	};
	struct {
		unsigned Inv:	1;	//!< Invert Permissions
		unsigned Perms:	31;	//!< Permission Flags
	};
} tVFS_ACL;

struct s_sysFInfo {
	Uint	uid, gid;
	Uint	flags;
	Uint64	size;
	Sint64	atime;
	Sint64	mtime;
	Sint64	ctime;
	 int	numacls;
	tVFS_ACL	acls[];
};

// === FUNCTIONS ===
extern int	VFS_Init();

extern int	VFS_Open(char *Path, Uint Flags);
extern void	VFS_Close(int FD);

extern int	VFS_Seek(int FD, Sint64 Offset, int Direction);
extern Uint64	VFS_Tell(int FD);

extern Uint64	VFS_Read(int FD, Uint64 Length, void *Buffer);
extern Uint64	VFS_Write(int FD, Uint64 Length, void *Buffer);

extern Uint64	VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer);
extern Uint64	VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer);

extern int	VFS_IOCtl(int FD, int ID, void *Buffer);

extern void	VFS_GetMemPath(void *Base, Uint Length, char *Dest);
extern char	*VFS_GetTruePath(char *Path);

extern int	VFS_Mount(char *Device, char *MountPoint, char *Filesystem, char *Options);
extern int	VFS_MkDir(char *Path);
extern int	VFS_Symlink(char *Link, char *Dest);
extern int	VFS_ReadDir(int FD, char *Dest);

#endif

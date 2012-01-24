/**
 * \file vfs_ext.h
 * \brief Exported VFS Definitions
 * \author John Hodge (thePowersGang)
 */
#ifndef _VFS_EXT_H
#define _VFS_EXT_H

//! Inode number type
typedef Uint64	tInode;

//! Mountpoint identifier type
typedef Uint32	tMount;

// === CONSTANTS ===
//! Maximum size of a Memory Path generated by VFS_GetMemPath
#define	VFS_MEMPATH_SIZE	(3 + (BITS/4)*2)
/**
 * \name Flags for VFS_Open
 * \{
 */
//! Open for execution
#define VFS_OPENFLAG_EXEC	0x01
//! Open for reading
#define VFS_OPENFLAG_READ	0x02
//! Open for writing
#define VFS_OPENFLAG_WRITE	0x04
//! Do not resolve the final symbolic link
#define	VFS_OPENFLAG_NOLINK	0x40
//! Open as a user
#define	VFS_OPENFLAG_USER	0x80
/**
 * \}
 */
//! Marks a VFS handle as belonging to the kernel
#define VFS_KERNEL_FLAG	0x40000000

//! Architectual maximum number of file descriptors
#define MAX_FILE_DESCS	128

/**
 * \brief VFS_Seek directions
 */
enum eVFS_SeekDirs
{
	SEEK_SET = 1,	//!< Set the current file offset
	SEEK_CUR = 0,	//!< Seek relative to the current position
	SEEK_END = -1	//!< Seek from the end of the file backwards
};

/**
 * \name ACL Permissions
 * \{
 */
/**
 * \brief Readable
 */
#define VFS_PERM_READ	0x00000001
/**
 * \brief Writeable
 */
#define VFS_PERM_WRITE	0x00000002
/**
 * \brief Append allowed
 */
#define VFS_PERM_APPEND	0x00000004
/**
 * \brief Executable
 */
#define VFS_PERM_EXECUTE	0x00000008
/**
 * \brief All permissions granted
 */
#define VFS_PERM_ALL	0x7FFFFFFF	// Mask for permissions
/**
 * \brief Denies instead of granting permissions
 * \note Denials take precedence
 */
#define VFS_PERM_DENY	0x80000000	// Inverts permissions
/**
 * \}
 */

/**
 * \brief MMap protection flags
 * \{
 */
#define MMAP_PROT_READ	0x001	//!< Readable memory
#define MMAP_PROT_WRITE	0x002	//!< Writable memory
#define MMAP_PROT_EXEC	0x004	//!< Executable memory
/**
 * \}
 */

/**
 * \brief MMap mapping flags
 * \{
 */
#define MMAP_MAP_SHARED 	0x001	//!< Shared with all other users of the FD
#define MMAP_MAP_PRIVATE	0x002	//!< Local (COW) copy
#define MMAP_MAP_FIXED  	0x004	//!< Load to a fixed address
#define MMAP_MAP_ANONYMOUS	0x008	//!< Not associated with a FD
/**
 * \}
 */

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

/**
 * \brief SYS_FINFO structure
 */
typedef struct sFInfo
{
	tMount	mount;	//!< Mountpoint ID
	tInode	inode;	//!< Inode
	Uint32	uid;	//!< Owning User ID
	Uint32	gid;	//!< Owning Group ID
	Uint32	flags;	//!< File flags
	Uint64	size;	//!< File Size
	Sint64	atime;	//!< Last Accessed time
	Sint64	mtime;	//!< Last modified time
	Sint64	ctime;	//!< Creation time
	Sint32	numacls;	//!< Total number of ACL entries
	tVFS_ACL	acls[];	//!< ACL buffer (size is passed in the \a MaxACLs argument to VFS_FInfo)
} PACKED tFInfo;

/**
 * \brief fd_set for select()
 */
typedef struct
{
	//! Bitmap of set file descriptors
	Uint16	flags[MAX_FILE_DESCS/16];
}	fd_set;

/**
 * \brief Clear a descriptor flag in a fd_set
 * \param fd	File descriptor to clear
 * \param fdsetp	Set to modify
 */
#define FD_CLR(fd, fdsetp) ((fdsetp)->flags[(fd)/16]&=~(1<<((fd)%16)))
/**
 * \brief Set a descriptor flag in a fd_set
 * \param fd	File descriptor to set
 * \param fdsetp	Set to modify
 */
#define FD_SET(fd, fdsetp) ((fdsetp)->flags[(fd)/16]|=~(1<<((fd)%16)))
/**
 * \brief Test a descriptor flag in a fd_set
 * \param fd	File descriptor to test
 * \param fdsetp	Set to modify
 */
#define FD_ISSET(fd, fdsetp) ((fdsetp)->flags[(fd)/16]&(1<<((fd)%16)))

// === FUNCTIONS ===
/**
 * \brief Initialise the VFS (called by system.c)
 * \return Boolean Success
 */
extern int	VFS_Init(void);

/**
 * \brief Open a file
 * \param Path	Absolute or relative path to the file
 * \param Mode	Flags defining how to open the file
 * \return VFS Handle (an integer) or -1 if an error occured
 */
extern int	VFS_Open(const char *Path, Uint Mode);
/**
 * \brief Opens a file via an open directory
 * \note The file to open must be a direct child of the parent
 * \param FD	Parent Directory
 * \param Name	Child name
 * \param Mode	Open mode
 * \return File handle (same as returned from VFS_Open)
 */
extern int	VFS_OpenChild(int FD, const char *Name, Uint Mode);
/**
 * \brief Opens a file given a mount ID and an inode number
 * \param Mount	Mount ID returned by FInfo
 * \param Inode	Inode number from FInfo
 * \param Mode	Open mode (see VFS_Open)
 * \return File handle (same as VFS_Open)
 */
extern int	VFS_OpenInode(Uint32 Mount, Uint64 Inode, int Mode);

/**
 * \brief Close a currently open file
 * \param FD	Handle returned by ::VFS_Open
 */
extern void	VFS_Close(int FD);

/**
 * \brief Get file information from an open file
 * \param FD	File handle returned by ::VFS_Open
 * \param Dest	Destination for the read information
 * \param MaxACLs	Number of ACL slots allocated in the \a Dest structure
 * \return Boolean Success
 * 
 * If the \a NumACLs is smaller than the number of ACLs the node has, only
 * \a NumACLs will be copied into \a Dest, but the tFInfo.numacls field
 * will be set to the true ammount of ACLs. It is up to the user to do with
 * this information how they like.
 */
extern int	VFS_FInfo(int FD, tFInfo *Dest, int MaxACLs);
/**
 * \brief Gets the permissions appling to a user/group.
 * \param FD	File handle returned by ::VFS_Open
 * \param Dest	ACL information structure to edit
 * \return Boolean success
 * 
 * This function sets the tVFS_ACL.Inv and tVFS_ACL.Perms fields to what
 * permissions the user/group specied in tVFS_ACL.ID has on the file.
 */
extern int	VFS_GetACL(int FD, tVFS_ACL *Dest);
/**
 * \brief Changes the user's current working directory
 * \param Dest	New working directory (either absolute or relative to the current)
 * \return Boolean Success
 */
extern int	VFS_ChDir(const char *Dest);
/**
 * \brief Change the current virtual root for the user
 * \param New New virtual root (same as ::VFS_ChDir but cannot go
 *            above the current virtual root)
 * \return Boolean success
 */
extern int	VFS_ChRoot(const char *New);

/**
 * \brief Change the location of the current file pointer
 * \param FD	File handle returned by ::VFS_Open
 * \param Offset	Offset within the file to go to
 * \param Whence	A direction from ::eVFS_SeekDirs
 * \return Boolean success
 */
extern int	VFS_Seek(int FD, Sint64 Offset, int Whence);
/**
 * \brief Returns the current file pointer
 * \param FD	File handle returned by ::VFS_Open
 * \return Current file position
 */
extern Uint64	VFS_Tell(int FD);

/**
 * \brief Reads data from a file
 * \param FD	File handle returned by ::VFS_Open
 * \param Length	Number of bytes to read from the file
 * \param Buffer	Destination of read data
 * \return Number of read bytes
 */
extern Uint64	VFS_Read(int FD, Uint64 Length, void *Buffer);
/**
 * \brief Writes data to a file
 * \param FD	File handle returned by ::VFS_Open
 * \param Length	Number of bytes to write to the file
 * \param Buffer	Source of written data
 * \return Number of bytes written
 */
extern Uint64	VFS_Write(int FD, Uint64 Length, const void *Buffer);

/**
 * \brief Reads from a specific offset in the file
 * \param FD	File handle returned by ::VFS_Open
 * \param Offset	Byte offset in the file
 * \param Length	Number of bytes to read from the file
 * \param Buffer	Source of read data
 * \return Number of bytes read
 */
extern Uint64	VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer);
/**
 * \brief Writes to a specific offset in the file
 * \param FD	File handle returned by ::VFS_Open
 * \param Offset	Byte offset in the file
 * \param Length	Number of bytes to write to the file
 * \param Buffer	Source of written data
 * \return Number of bytes written
 */
extern Uint64	VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, const void *Buffer);

/**
 * \brief Sends an IOCtl request to the driver
 * \param FD	File handle returned by ::VFS_Open
 * \param ID	IOCtl call ID (driver specific)
 * \param Buffer	Data pointer to send to the driver
 * \return Driver specific response
 */
extern int	VFS_IOCtl(int FD, int ID, void *Buffer);

/**
 * \brief Creates a VFS Memory path from a pointer and size
 * \param Dest	Destination for the created path
 * \param Base	Base of the memory file
 * \param Length	Length of the memory buffer
 * \note A maximum of VFS_MEMPATH_SIZE bytes will be required in \a Dest
 */
extern void	VFS_GetMemPath(char *Dest, void *Base, Uint Length);
/**
 * \brief Gets the canoical (true) path of a file
 * \param Path	File path (may contain symlinks, relative elements etc.)
 * \return Absolute path with no symlinks
 */
extern char	*VFS_GetTruePath(const char *Path);

/**
 * \brief Mounts a filesystem
 * \param Device	Device to mount
 * \param MountPoint	Location to mount
 * \param Filesystem	Filesystem to use
 * \param Options	Options string to pass the driver
 * \return 1 on succes, -1 on error
 */
extern int	VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options);
/**
 * \brief Create a new directory
 * \param Path	Path to new directory (absolute or relative)
 * \return Boolean success
 * \note The parent of the directory must exist
 */
extern int	VFS_MkDir(const char *Path);
/**
 * \brief Create a symbolic link
 * \param Name	Name of the symbolic link
 * \param Link	File the symlink points to
 * \return Boolean success (\a Link is not tested for existence)
 */
extern int	VFS_Symlink(const char *Name, const char *Link);
/**
 * \brief Read from a directory
 * \param FD	File handle returned by ::VFS_Open
 * \param Dest	Destination array for the file name (max 255 bytes)
 * \return Boolean Success
 */
extern int	VFS_ReadDir(int FD, char *Dest);
/**
 * \brief Wait for an aciton on a file descriptor
 * \param MaxHandle	Maximum set handle in \a *Handles
 * \param ReadHandles	Handles to wait for data on
 * \param WriteHandles	Handles to wait to write to
 * \param ErrHandles	Handle to wait for errors on
 * \param Timeout	Timeout for select() (if null, there is no timeout), if zero select() is non blocking
 * \param ExtraEvents	Extra event set to wait on
 * \param IsKernel	Use kernel handles as opposed to user handles
 * \return Number of handles that actioned
 */
extern int VFS_Select(int MaxHandle, fd_set *ReadHandles, fd_set *WriteHandles, fd_set *ErrHandles, tTime *Timeout, Uint32 ExtraEvents, BOOL IsKernel);

/**
 * \brief Map a file into memory
 * \param DestHint	Suggested place for read data
 * \param Length	Size of area to map
 * \param Protection	Protection type (see `man mmap`)
 * \param Flags	Mapping flags
 * \param FD	File descriptor to load from
 * \param Offset	Start of region
 */
extern void	*VFS_MMap(void *DestHint, size_t Length, int Protection, int Flags, int FD, Uint64 Offset);

/**
 * \brief Unmap memory allocated by VFS_MMap
 * \param Addr	Address of data to unmap
 * \param Length	Length of data
 */
extern int	VFS_MUnmap(void *Addr, size_t Length);
#endif

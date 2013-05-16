/* 
 * Acess2
 * VFS Common Header
 */
/**
 * \file vfs.h
 * \brief Acess VFS Layer
 * 
 * The Acess Virtual File System (VFS) provides abstraction of multiple
 * physical filesystems, network storage and devices (both hardware and
 * virtual) to the user.
 * 
 * The core of the VFS is the concept of a \ref tVFS_Node "VFS Node".
 * A VFS Node represents a "file" in the VFS tree, this can be any sort
 * of file (an ordinary file, a directory, a symbolic link or a device)
 * depending on the bits set in the \ref tVFS_Node.Flags Flags field.
 * - For more information see "VFS Node Flags"
 */
#ifndef _VFS_H
#define _VFS_H

#include <acess.h>
#include <mutex.h>

/**
 * \brief Thread list datatype for VFS_Select
 */
typedef struct sVFS_SelectList	tVFS_SelectList;

typedef struct sVFS_NodeType	tVFS_NodeType;

/**
 * \name tVFS_Node Flags
 * \brief Flag values for tVFS_Node.Flags
 * \{
 */
//! \todo Is this still needed
#define VFS_FFLAG_READONLY	0x01	//!< Readonly File
/**
 * \brief Directory Flag
 * 
 * This flag marks the tVFS_Node as describing a directory, and says
 * that the tVFS_Node.FindDir, tVFS_Node.ReadDir, tVFS_Node.MkNod and
 * tVFS_Node.Relink function pointers are valid.
 * For a directory the tVFS_Node.Size field contains the number of files
 * within the directory, or -1 for undetermined.
 */
#define VFS_FFLAG_DIRECTORY	0x02
/**
 * \brief Symbolic Link Flag
 * 
 * Marks a file as a symbolic link
 */
#define VFS_FFLAG_SYMLINK	0x04
/**
 * \brief Set User ID Flag
 * 
 * Allows an executable file to change it's executing user to the file's
 * owner.
 * In the case of a directory, it means that all immediate children will
 * inherit the UID of the parent.
 */
#define VFS_FFLAG_SETUID	0x08
/**
 * \brief Set Group ID Flag
 * 
 * Allows an executable file to change it's executing group to the file's
 * owning group.
 * In the case of a directory, it means that all immediate children will
 * inherit the GID of the parent.
 */
#define VFS_FFLAG_SETGID	0x10

/**
 * \brief "Don't do Write-Back" Flag
 *
 * Stops the VFS from calling tVFS_Node.Write on dirty pages when a region
 * is unmapped. Nice for read-only files and memory-only files (or 
 * pseudo-readonly filesystems)
 */
#define VFS_FFLAG_NOWRITEBACK	0x1000

/**
 * \brief "Dirty Metadata" Flag
 *
 * Indicates that the node's metadata has been altered since the flag was last
 * cleared. Tells the driver that it should update the inode at the next flush.
 */
#define VFS_FFLAG_DIRTY 	0x8000
/**
 * \}
 */

/**
 * \brief Represents a node (file or directory) in the VFS tree
 * 
 * This structure provides the VFS with the functions required to read/write
 * the file (or directory) that it represents.
 */
typedef struct sVFS_Node
{
	/**
	 * \brief Functions associated with the node
	 */
	tVFS_NodeType	*Type;
	
	/**
	 * \name Identifiers
	 * \brief Fields used by the driver to identify what data this node
	 *        corresponds to.
	 * \{
	 */
	Uint64	Inode;	//!< Inode ID - Must identify the node uniquely if tVFS_Driver.GetNodeFromINode is non-NULL
	Uint	ImplInt;	//!< Implementation Usable Integer
	void	*ImplPtr;	//!< Implementation Usable Pointer
	/**
	 * \}
	 */
	
	/**
	 * \name Node State
	 * \brief Stores the misc information about the node
	 * \{
	 */
	 int	ReferenceCount;	//!< Number of times the node is used
	
	Uint64	Size;	//!< File Size
	
	Uint32	Flags;	//!< File Flags
	
	/**
	 * \brief Pointer to cached data (FS Specific)
	 * \note The Inode_* functions will free when the node is uncached
	 *       this if needed
	 */
	void	*Data;
	
	/**
	 * \brief Node mutex
	 * \note Provided for the Filesystem driver's use
	 */
	tMutex	Lock;
	
	/**
	 * \}
	 */
	
	/**
	 * \name Times
	 * \{
	 */
	tTime	ATime;	//!< Last Accessed Time
	tTime	MTime;	//!< Last Modified Time
	tTime	CTime;	//!< Creation Time
	/**
	 * \}
	 */
	
	/**
	 * \name Access control
	 * \{
	 */
	tUID	UID;	//!< ID of Owning User
	tGID	GID;	//!< ID of Owning Group
	
	 int	NumACLs;	//!< Number of ACL entries
	tVFS_ACL	*ACLs;	//!< Access Controll List pointer
	/**
	 * \}
	 */
	
	/**
	 * \name VFS_Select() fields
	 * \note Used by the VFS internals, drivers should use VFS_Mark*
	 * \{
	 */
	 int	DataAvaliable;
	tVFS_SelectList	*ReadThreads;	//!< Threads waiting to read
	 int	BufferFull;
	tVFS_SelectList	*WriteThreads;	//!< Threads waiting to write
	 int	ErrorOccurred;
	tVFS_SelectList	*ErrorThreads;	//!< Threads waiting for an error
	/**
	 * \}
	 */

	/**
	 * \name VFS_MMap() fields
	 * \{
	 */
	void	*MMapInfo;
	/**
	 * \}
	 */
} tVFS_Node;

/**
 * \name tVFS_NodeType.FindDir Flags
 * \brief 
 * \{
 */
//\! Attempt non-blocking IO
#define VFS_IOFLAG_NOBLOCK	0x001
/**
 * \}
 */

/**
 * \name tVFS_NodeType.FindDir Flags
 * \brief 
 * \{
 */
//\! Call was triggered by VFS_Stat (as opposed to open)
#define VFS_FDIRFLAG_STAT	0x001
/**
 * \}
 */

/**
 * \brief Functions for a specific node type
 */
struct sVFS_NodeType
{
	/**
	 * \brief Debug name for the type
	 */
	const char	*TypeName;

	/**
	 * \name Common Functions
	 * \brief Functions that are used no matter the value of .Flags
	 * \{
	 */
	/**
	 * \brief Reference the node
	 * \param Node Pointer to this node
	 */
	void	(*Reference)(struct sVFS_Node *Node);
	/**
	 * \brief Close (dereference) the node
	 * \param Node	Pointer to this node
	 * 
	 * Usually .Close is used to write any changes to the node back to
	 * the persistent storage.
	 */
	void	(*Close)(struct sVFS_Node *Node);
	
	/**
	 * \brief Send an IO Control
	 * \param Node	Pointer to this node
	 * \param Id	IOCtl call number
	 * \param Data	Pointer to data to pass to the driver
	 * \return Implementation defined
	 */
	 int	(*IOCtl)(struct sVFS_Node *Node, int Id, void *Data);
	
	/**
	 * \}
	 */
	
	/**
	 * \name Buffer Functions
	 * \brief Functions for accessing a buffer-type file (normal file or
	 *        symbolic link)
	 * \{
	 */
	
	/**
	 * \brief Read from the file
	 * \param Node	Pointer to this node
	 * \param Offset	Byte offset in the file
	 * \param Length	Number of bytes to read
	 * \param Buffer	Destination for read data
	 * \return Number of bytes read
	 */
	size_t	(*Read)(struct sVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
	/**
	 * \brief Write to the file
	 * \param Node	Pointer to this node
	 * \param Offset	Byte offser in the file
	 * \param Length	Number of bytes to write
	 * \param Buffer	Source of written data
	 * \return Number of bytes read
	 */
	size_t	(*Write)(struct sVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);

	/**
	 * \brief Map a region of a file into memory
	 * \param Node	Pointer to this node
	 * \param Offset	Start of the region (page aligned)
	 * \param Length	Length of the region (page aligned)
	 * \param Dest	Location to which to map
	 * \return Boolean Failure
	 * \note If NULL, the VFS implements it using .Read
	 */
	 int	(*MMap)(struct sVFS_Node *Node, off_t Offset, int Length, void *Dest);
	
	/**
	 * \}
	 */
	
	/**
	 * \name Directory Functions
	 * \{
	 */
	/**
	 * \brief Find an directory entry by name
	 * \param Node	Pointer to this node
	 * \param Name	Name of the file wanted
	 * \return Pointer to the requested node or NULL if it cannot be found
	 * \note The node returned must be accessable until tVFS_NodeType::Close
	 *       is called and ReferenceCount reaches zero.
	 */
	struct sVFS_Node	*(*FindDir)(struct sVFS_Node *Node, const char *Name, Uint Flags);
	
	/**
	 * \brief Read from a directory
	 * \param Node	Pointer to this node
	 * \param Pos	Offset in the directory
	 * \param Dest	Destination for filename
	 * \return Zero on success, negative on error or +ve for ignore entry
	 */
	 int	(*ReadDir)(struct sVFS_Node *Node, int Pos, char Dest[FILENAME_MAX]);
	
	/**
	 * \brief Create a node in a directory
	 * \param Node	Pointer to this node
	 * \param Name	Name of the new child
	 * \param Flags	Flags to apply to the new child (directory or symlink)
	 * \return Created node or NULL on error
	 */
	tVFS_Node	*(*MkNod)(struct sVFS_Node *Node, const char *Name, Uint Mode);
	
	/**
	 * \brief Relink (Rename/Remove) a file/directory
	 * \param Node	Pointer to this node
	 * \param OldName	Name of the item to move/delete
	 * \return Zero on Success, non-zero on error (see errno.h)
	 */
	 int	(*Unlink)(struct sVFS_Node *Node, const char *OldName);
	
	/**
	 * \brief Link a node to a name
	 * \param Node	Pointer to this node (directory)
	 * \param NewName	Name for the new link
	 * \param Child	Node to create a new link to
	 * \return Zero on success, non-zero on error (see errno.h)
	 */
	 int	(*Link)(struct sVFS_Node *Node, const char *NewName, struct sVFS_Node *Child);
	 
	 /**
	  * \}
	  */
};

/**
 * \brief VFS Driver (Filesystem) Definition
 */
typedef struct sVFS_Driver
{
	/**
	 * \brief Unique Identifier for this filesystem type
	 */
	const char	*Name;
	/**
	 * \brief Flags applying to this driver
	 */
	Uint	Flags;
	
	/**
	 * \brief Detect if a volume is accessible using this driver
	 * \return Boolean success (with higher numbers being higher priority)
	 *
	 * E.g. FAT would return 1 as it's the lowest common denominator while ext2 might return 2,
	 * because it can be embedded in a FAT volume (and is a more fully featured filesystem).
	 */
	 int	(*Detect)(int FD);

	/**
	 * \brief Callback to mount a device
	 */
	tVFS_Node	*(*InitDevice)(const char *Device, const char **Options);
	/**
	 * \brief Callback to unmount a device
	 */
	void	(*Unmount)(tVFS_Node *Node);
	/**
	 * \brief Retrieve a VFS node from an inode
	 */
	tVFS_Node	*(*GetNodeFromINode)(tVFS_Node *RootNode, Uint64 InodeValue);
	/**
	 * \brief Used internally (next driver in the chain)
	 */
	struct sVFS_Driver	*Next;
} tVFS_Driver;

// === GLOBALS ===
//! \brief Maximum number of elements that can be skipped in one return
#define	VFS_MAXSKIP	((void*)1024)
//! \brief Skip a single entry in readdir
#define	VFS_SKIP	((void*)1)
//! \brief Skip \a n entries in readdir
#define	VFS_SKIPN(n)	((void*)(n))

extern tVFS_Node	NULLNode;	//!< NULL VFS Node (Ignored/Skipped)
/**
 * \name Static ACLs
 * \brief Simple ACLs to aid writing drivers
 * \{
 */
extern tVFS_ACL	gVFS_ACL_EveryoneRWX;	//!< Everyone Read/Write/Execute
extern tVFS_ACL	gVFS_ACL_EveryoneRW;	//!< Everyone Read/Write
extern tVFS_ACL	gVFS_ACL_EveryoneRX;	//!< Everyone Read/Execute
extern tVFS_ACL	gVFS_ACL_EveryoneRO;	//!< Everyone Read only
/**
 * \}
 */

// === FUNCTIONS ===
/**
 * \fn int VFS_AddDriver(tVFS_Driver *Info)
 * \brief Registers the driver with the DevFS layer
 * \param Info	Driver information structure
 */
extern int	VFS_AddDriver(tVFS_Driver *Info);
/**
 * \brief Get the information structure of a driver given its name
 * \param Name	Name of filesystem driver to find
 */
extern tVFS_Driver	*VFS_GetFSByName(const char *Name);


/**
 * \brief Prepare a node for use
 */
extern void	VFS_InitNode(tVFS_Node *Node);

/**
 * \brief Clean up a node, ready for deletion
 * \note This should ALWAYS be called before a node is freed, as it
 *       cleans up VFS internal structures.
 */
extern void	VFS_CleanupNode(tVFS_Node *Node);

/**
 * \fn tVFS_ACL *VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group)
 * \brief Transforms Unix Permssions into Acess ACLs
 * \param Mode	Unix RWXrwxRWX mask
 * \param Owner	UID of the file's owner
 * \param Group	GID of the file's owning group
 * \return An array of 3 Acess ACLs
 */
extern tVFS_ACL	*VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group);

/**
 * \brief Flags fro \a TypeFlag of VFS_SelectNode
 * \{
 */
#define VFS_SELECT_READ	0x01
#define VFS_SELECT_WRITE	0x02
#define VFS_SELECT_ERROR	0x04
/**
 * \}
 */

/**
 * \brief Wait for an event on a node
 * \param Node	Node to wait on
 * \param Type	Type of wait
 * \param Timeout	Time to wait (NULL for infinite wait)
 * \param Name	Name to show in debug output
 * \return Number of nodes that actioned (0 or 1)
 */
extern int	VFS_SelectNode(tVFS_Node *Node, int Type, tTime *Timeout, const char *Name);

/**
 * \brief Change the full flag on a node
 */
extern int	VFS_MarkFull(tVFS_Node *Node, BOOL IsBufferFull);
/**
 * \brief Alter the space avaliable flag on a node
 */
extern int	VFS_MarkAvaliable(tVFS_Node *Node, BOOL IsDataAvaliable);
/**
 * \brief Alter the error flags on a node
 */
extern int	VFS_MarkError(tVFS_Node *Node, BOOL IsErrorState);

// --- Node Cache --
/**
 * \name Node Cache
 * \brief Functions to allow a node to be cached in memory by the VFS
 * 
 * These functions store a node for the driver, to prevent it from having
 * to re-generate the node on each call to FindDir. It also allows for
 * fast cleanup when a filesystem is unmounted.
 * \{
 */
/**
 * \fn int Inode_GetHandle(void)
 * \brief Gets a unique handle to the Node Cache
 * \return A unique handle for use for the rest of the Inode_* functions
 */
extern int	Inode_GetHandle(void);
/**
 * \fn tVFS_Node *Inode_GetCache(int Handle, Uint64 Inode)
 * \brief Gets an inode from the node cache
 * \param Handle	A handle returned by Inode_GetHandle()
 * \param Inode	Value of the Inode field of the ::tVFS_Node you want
 * \return A pointer to the cached node
 */
extern tVFS_Node	*Inode_GetCache(int Handle, Uint64 Inode);
/**
 * \fn tVFS_Node *Inode_CacheNode(int Handle, tVFS_Node *Node)
 * \brief Caches a node in the Node Cache
 * \param Handle	A handle returned by Inode_GetHandle()
 * \param Node	A pointer to the node to be cached (a copy is taken)
 * \return A pointer to the node in the node cache
 */
extern tVFS_Node	*Inode_CacheNode(int Handle, tVFS_Node *Node);
/**
 * \fn int Inode_UncacheNode(int Handle, Uint64 Inode)
 * \brief Dereferences (and removes if needed) a node from the cache
 * \param Handle	A handle returned by Inode_GetHandle()
 * \param Inode	Value of the Inode field of the ::tVFS_Node you want to remove
 * \return -1: Error (not present), 0: Not freed, 1: Freed
 */
extern int	Inode_UncacheNode(int Handle, Uint64 Inode);
/**
 * \fn void Inode_ClearCache(int Handle)
 * \brief Clears the cache for a handle
 * \param Handle	A handle returned by Inode_GetHandle()
 */
extern void	Inode_ClearCache(int Handle);

/**
 * \}
 */

#endif

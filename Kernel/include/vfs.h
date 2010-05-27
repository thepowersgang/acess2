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
	 * \name Identifiers
	 * \brief Fields used by the driver to identify what data this node
	 *        corresponds to.
	 * \{
	 */
	Uint64	Inode;	//!< Inode ID (Essentially another ImplInt)
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
	 * \}
	 */
	
	/**
	 * \name Times
	 * \{
	 */
	Sint64	ATime;	//!< Last Accessed Time
	Sint64	MTime;	//!< Last Modified Time
	Sint64	CTime;	//!< Creation Time
	/**
	 * \}
	 */
	
	/**
	 * \name Access controll
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
	Uint64	(*Read)(struct sVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
	/**
	 * \brief Write to the file
	 * \param Node	Pointer to this node
	 * \param Offset	Byte offser in the file
	 * \param Length	Number of bytes to write
	 * \param Buffer	Source of written data
	 * \return Number of bytes read
	 */
	Uint64	(*Write)(struct sVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
	
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
	 * \note The node returned must be accessable until ::tVFS_Node.Close
	 *       is called and ReferenceCount reaches zero.
	 */
	struct sVFS_Node	*(*FindDir)(struct sVFS_Node *Node, char *Name);
	
	/**
	 * \brief Read from a directory
	 * \param Node	Pointer to this node
	 * \param Pos	Offset in the directory
	 * \return Pointer to the name of the item on the heap (will be freed
	 *         by the caller). If the directory end has been reached, NULL
	 *         will be returned.
	 *         If an item is required to be skipped either &::NULLNode,
	 *         ::VFS_SKIP or ::VFS_SKIPN(0...1023) will be returned.
	 */
	char	*(*ReadDir)(struct sVFS_Node *Node, int Pos);
	
	/**
	 * \brief Create a node in a directory
	 * \param Node	Pointer to this node
	 * \param Name	Name of the new child
	 * \param Flags	Flags to apply to the new child (directory or symlink)
	 * \return Zero on Success, non-zero on error (see errno.h)
	 */
	 int	(*MkNod)(struct sVFS_Node *Node, char *Name, Uint Flags);
	
	/**
	 * \brief Relink (Rename/Remove) a file/directory
	 * \param Node	Pointer to this node
	 * \param OldName	Name of the item to move/delete
	 * \param NewName	New name (or NULL if unlinking is wanted)
	 * \return Zero on Success, non-zero on error (see errno.h)
	 */
	 int	(*Relink)(struct sVFS_Node *Node, char *OldName, char *NewName);
	
	/**
	 * \brief Link a node to a name
	 * \param Node	Pointer to this node (directory)
	 * \param Child	Node to create a new link to
	 * \param NewName	Name for the new link
	 * \return Zeron on success, non-zero on error (see errno.h)
	 */
	 int	(*Link)(struct sVFS_Node *Node, struct sVFS_Node *Child, char *NewName);
	 
	 /**
	  * \}
	  */
} tVFS_Node;

/**
 * \brief VFS Driver (Filesystem) Definition
 */
typedef struct sVFS_Driver
{
	//! \brief Unique Identifier for this filesystem type
	char	*Name;
	//! \brief Flags applying to this driver
	Uint	Flags;
	
	//! \brief Callback to mount a device
	tVFS_Node	*(*InitDevice)(char *Device, char **Options);
	//! \brief Callback to unmount a device
	void	(*Unmount)(tVFS_Node *Node);
	//! \brief Used internally (next driver in the chain)
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
 * \fn tVFS_Driver *VFS_GetFSByName(char *Name)
 * \brief Get the information structure of a driver given its name
 * \param Name	Name of filesystem driver to find
 */
extern tVFS_Driver	*VFS_GetFSByName(char *Name);
/**
 * \fn tVFS_ACL *VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group)
 * \brief Transforms Unix Permssions into Acess ACLs
 * \param Mode	Unix RWXrwxRWX mask
 * \param Owner	UID of the file's owner
 * \param Group	GID of the file's owning group
 * \return An array of 3 Acess ACLs
 */
extern tVFS_ACL	*VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group);

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
 * \fn int Inode_GetHandle()
 * \brief Gets a unique handle to the Node Cache
 * \return A unique handle for use for the rest of the Inode_* functions
 */
extern int	Inode_GetHandle();
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
 */
extern void	Inode_UncacheNode(int Handle, Uint64 Inode);
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

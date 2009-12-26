/* 
 * Acess2
 * VFS Common Header
 */
/**
 * \file vfs.h
 * \brief Acess VFS Layer
 */
#ifndef _VFS_H
#define _VFS_H

#include <common.h>

/**
 * \name VFS Node Flags
 * \{
 */
#define VFS_FFLAG_READONLY	0x01	//!< Read-only file
#define VFS_FFLAG_DIRECTORY	0x02	//!< Directory
#define VFS_FFLAG_SYMLINK	0x04	//!< Symbolic Link
/**
 * \}
 */

/**
 * \brief VFS Node
 * \todo Complete / Finalise
 */
typedef struct sVFS_Node {	
	Uint64	Inode;	//!< Inode ID
	Uint	ImplInt;	//!< Implementation Usable Integer
	void	*ImplPtr;	//!< Implementation Usable Pointer
	
	 int	ReferenceCount;	//!< Number of times the node is used
	
	Uint64	Size;	//!< File Size
	
	Uint32	Flags;	//!< File Flags
	
	Sint64	ATime;	//!< Last Accessed Time
	Sint64	MTime;	//!< Last Modified Time
	Sint64	CTime;	//!< Creation Time
	
	Uint	UID;	//!< Owning User
	Uint	GID;	//!< Owning Group
	
	 int	NumACLs;	//!< Number of ACL entries
	tVFS_ACL	*ACLs;	//!< ACL Entries
	
	/**
	 * \brief Reference the node
	 * \param Node Pointer to this node
	 */
	void	(*Reference)(struct sVFS_Node *Node);
	/**
	 * \brief Close (dereference) the node
	 * \param Node	Pointer to this node
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
	 * \return Boolean success
	 */
	 int	(*MkNod)(struct sVFS_Node *Node, char *Name, Uint Flags);
	
	/**
	 * \brief Relink (Rename/Remove) a file/directory
	 * \param Node	Pointer to this node
	 * \param OldName	Name of the item to move/delete
	 * \param NewName	New name (or NULL if unlinking is wanted)
	 * \return Boolean Success
	 */
	 int	(*Relink)(struct sVFS_Node *Node, char *OldName, char *NewName);
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
 * \name Simple ACLs to aid writing drivers
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
//! \name Node Cache
//! \{
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
 * \fn void Inode_UncacheNode(int Handle, Uint64 Inode)
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

//! \}

#endif

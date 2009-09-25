/* 
 * Acess Micro - VFS Server Ver 1
 */
#ifndef _VFS_H
#define _VFS_H

#include <common.h>

#define VFS_PERM_READ	0x00000001
#define VFS_PERM_WRITE	0x00000002
#define VFS_PERM_APPEND	0x00000004
#define VFS_PERM_EXECUTE	0x00000008
#define VFS_PERM_ALL	0x7FFFFFFF	// Mask for permissions
#define VFS_PERM_DENY	0x80000000	// Inverts permissions

typedef struct sVFS_ACL {
	struct {
		unsigned Group:	1;	// Group (as opposed to user) flag
		unsigned ID:	31;	// ID of Group/User (-1 for nobody/world)
	};
	struct {
		unsigned Inv:	1;	// Invert Permissions
		unsigned Perms:	31;	// Permission Flags
	};
} tVFS_ACL;

#define VFS_FFLAG_READONLY	0x01
#define VFS_FFLAG_DIRECTORY	0x02
#define VFS_FFLAG_SYMLINK	0x04

typedef struct sVFS_Node {
	//char	*Name;	//!< Node's Name (UTF-8)
	
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
	
	//! Reference the node
	void	(*Reference)(struct sVFS_Node *Node);
	//! Close (dereference) the node
	void	(*Close)(struct sVFS_Node *Node);
	//! Send an IO Control
	 int	(*IOCtl)(struct sVFS_Node *Node, int Id, void *Data);
	
	Uint64	(*Read)(struct sVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
	Uint64	(*Write)(struct sVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
	
	//! Find an directory entry by name
	struct sVFS_Node	*(*FindDir)(struct sVFS_Node *Node, char *Name);
	//! Read from a directory
	char	*(*ReadDir)(struct sVFS_Node *Node, int Pos);
	//! Create a node in a directory
	 int	(*MkNod)(struct sVFS_Node *Node, char *Name, Uint Flags);
	//! Relink (Rename/Remove) a file/directory
	 int	(*Relink)(struct sVFS_Node *Node, char *OldName, char *NewName);
	
	//!< \todo Complete
} tVFS_Node;

/**
 * \brief VFS Driver (Filesystem) Definition
 */
typedef struct sVFS_Driver {
	char	*Name;
	Uint	Flags;
	tVFS_Node	*(*InitDevice)(char *Device, char *Options);
	void	(*Unmount)(tVFS_Node *Node);
	struct sVFS_Driver	*Next;
} tVFS_Driver;

// === GLOBALS ===
#define	VFS_SKIP	((void*)1)
#define	VFS_SKIPN(n)	((void*)(n))
extern tVFS_Node	NULLNode;
extern tVFS_ACL	gVFS_ACL_EveryoneRWX;
extern tVFS_ACL	gVFS_ACL_EveryoneRW;
extern tVFS_ACL	gVFS_ACL_EveryoneRX;
extern tVFS_ACL	gVFS_ACL_EveryoneRO;

// === FUNCTIONS ===
extern int	VFS_AddDriver(tVFS_Driver *Info);
extern tVFS_Driver	*VFS_GetFSByName(char *Name);

// --- Node Cache --
extern int	Inode_GetHandle();
extern tVFS_Node	*Inode_GetCache(int Handle, Uint64 Inode);
extern tVFS_Node	*Inode_CacheNode(int Handle, tVFS_Node *Node);
extern void	Inode_UncacheNode(int Handle, Uint64 Inode);
extern void	Inode_ClearCache(int Handle);

#endif

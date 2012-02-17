/* 
 * Acess 2
 * Virtual File System
 * - Memory Pseudo Files
 */
#include <acess.h>
#include <vfs.h>

// === PROTOTYPES ===
tVFS_Node	*VFS_MemFile_Create(const char *Path);
void	VFS_MemFile_Close(tVFS_Node *Node);
size_t	VFS_MemFile_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	VFS_MemFile_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);

// === GLOBALS ===
tVFS_NodeType	gVFS_MemFileType = {
	.Close = VFS_MemFile_Close,
	.Read = VFS_MemFile_Read,
	.Write = VFS_MemFile_Write
	};

// === CODE ===
/**
 * \fn tVFS_Node *VFS_MemFile_Create(const char *Path)
 */
tVFS_Node *VFS_MemFile_Create(const char *Path)
{
	Uint	base, size;
	const char	*str = Path;
	tVFS_Node	*ret;
	
	str++;	// Eat '$'
	
	// Read Base address
	base = 0;
	for( ; ('0' <= *str && *str <= '9') || ('A' <= *str && *str <= 'F'); str++ )
	{
		base *= 16;
		if('A' <= *str && *str <= 'F')
			base += *str - 'A' + 10;
		else
			base += *str - '0';
	}
	
	// Check separator
	if(*str++ != ':')	return NULL;
	
	// Read buffer size
	size = 0;
	for( ; ('0' <= *str && *str <= '9') || ('A' <= *str && *str <= 'F'); str++ )
	{
		size *= 16;
		if('A' <= *str && *str <= 'F')
			size += *str - 'A' + 10;
		else
			size += *str - '0';
	}
	
	// Check for NULL byte
	if(*str != '\0') 	return NULL;
	
	// Allocate and fill node
	ret = malloc(sizeof(tVFS_Node));
	memset(ret, 0, sizeof(tVFS_Node));
	
	// State
	ret->ImplPtr = (void*)base;
	ret->Size = size;
	
	// ACLs
	ret->NumACLs = 1;
	ret->ACLs = &gVFS_ACL_EveryoneRWX;
	
	// Functions
	ret->Type = &gVFS_MemFileType;
	
	return ret;
}

/**
 * \fn void VFS_MemFile_Close(tVFS_Node *Node)
 * \brief Dereference and clean up a memory file
 */
void VFS_MemFile_Close(tVFS_Node *Node)
{
	Node->ReferenceCount --;
	if( Node->ReferenceCount == 0 ) {
		Node->ImplPtr = NULL;
		free(Node);
	}
}

/**
 * \brief Read from a memory file
 */
size_t VFS_MemFile_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	// Check for use of free'd file
	if(Node->ImplPtr == NULL)	return 0;
	
	// Check for out of bounds read
	if(Offset > Node->Size)	return 0;
	
	// Truncate data read if needed
	if(Length > Node->Size)
		Length = Node->Size;
	if(Offset + Length > Node->Size)
		Length = Node->Size - Offset;
	
	// Copy Data
	memcpy(Buffer, (Uint8*)Node->ImplPtr + Offset, Length);
	
	return Length;
}

/**
 * \brief Write to a memory file
 */
size_t VFS_MemFile_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	// Check for use of free'd file
	if(Node->ImplPtr == NULL)	return 0;
	
	// Check for out of bounds read
	if(Offset > Node->Size)	return 0;
	
	// Truncate data read if needed
	if(Length > Node->Size)
		Length = Node->Size;
	if(Offset + Length > Node->Size)
		Length = Node->Size - Offset;
	
	// Copy Data
	memcpy((Uint8*)Node->ImplPtr + Offset, Buffer, Length);
	
	return Length;
}

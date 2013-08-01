/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * vfs/io.c
 * - VFS IO Handling (Read/Write)
 *
 * TODO: VFS-level caching to support non-blocking IO?
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include <vfs_ext.h>
#include <vfs_int.h>

// === CODE ===
/**
 * \fn Uint64 VFS_Read(int FD, Uint64 Length, void *Buffer)
 * \brief Read data from a node (file)
 */
size_t VFS_Read(int FD, size_t Length, void *Buffer)
{
	tVFS_Handle	*h;
	size_t	ret;
	
	h = VFS_GetHandle(FD);
	if(!h) {
		LOG("FD%i is a bad Handle", FD);
		return -1;
	}
	
	if( !(h->Mode & VFS_OPENFLAG_READ) ) {
		LOG("FD%i not open for reading", FD);
		return -1;
	}
	if( (h->Node->Flags & VFS_FFLAG_DIRECTORY) ) {
		LOG("FD%i is a directory", FD);
		return -1;
	}

	if(!h->Node->Type || !h->Node->Type->Read) {
		LOG("FD%i has no read method", FD);
		return -1;
	}

	if( !MM_GetPhysAddr(h->Node->Type->Read) ) {
		Log_Error("VFS", "Node type %p(%s) read method is junk %p",
			h->Node->Type, h->Node, h->Node->Type->TypeName,
			h->Node->Type->Read);
		return -1;
	}
	
	Uint	flags = 0;
	flags |= (h->Mode & VFS_OPENFLAG_NONBLOCK) ? VFS_IOFLAG_NOBLOCK : 0;
	ret = h->Node->Type->Read(h->Node, h->Position, Length, Buffer, flags);
	if(ret != Length)	LOG("%i/%i read", ret, Length);
	if(ret == (size_t)-1)	return -1;
	
	h->Position += ret;
	return ret;
}

/**
 * \fn Uint64 VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read data from a given offset (atomic)
 */
size_t VFS_ReadAt(int FD, Uint64 Offset, size_t Length, void *Buffer)
{
	tVFS_Handle	*h;
	size_t	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_READ) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if( !h->Node->Type || !h->Node->Type->Read) {
		Warning("VFS_ReadAt - Node %p, does not have a read method", h->Node);
		return 0;
	}

	if( !MM_GetPhysAddr(h->Node->Type->Read) ) {
		Log_Error("VFS", "Node type %p(%s) read method is junk %p",
			h->Node->Type, h->Node->Type->TypeName,
			h->Node->Type->Read);
	}
	
	Uint	flags = 0;
	flags |= (h->Mode & VFS_OPENFLAG_NONBLOCK) ? VFS_IOFLAG_NOBLOCK : 0;
	ret = h->Node->Type->Read(h->Node, Offset, Length, Buffer, flags);
	if(ret != Length)	LOG("%i/%i read", ret, Length);
	if(ret == (size_t)-1)	return -1;
	return ret;
}

/**
 * \fn Uint64 VFS_Write(int FD, Uint64 Length, const void *Buffer)
 * \brief Read data from a node (file)
 */
size_t VFS_Write(int FD, size_t Length, const void *Buffer)
{
	tVFS_Handle	*h;
	size_t	ret;
	
	h = VFS_GetHandle(FD);
	if(!h) {
		LOG("FD%i is not open", FD);
		errno = EBADF;
		return -1;
	}
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) ) {
		LOG("FD%i not opened for writing", FD);
		errno = EBADF;
		return -1;
	}
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY ) {
		LOG("FD%i is a directory", FD);
		errno = EISDIR;
		return -1;
	}

	if( !h->Node->Type || !h->Node->Type->Write ) {
		LOG("FD%i has no write method", FD);
		errno = EINTERNAL;
		return 0;
	}

	if( !MM_GetPhysAddr(h->Node->Type->Write) ) {
		Log_Error("VFS", "Node type %p(%s) write method is junk %p",
			h->Node->Type, h->Node->Type->TypeName,
			h->Node->Type->Write);
		errno = EINTERNAL;
		return -1;
	}
	
	Uint flags = 0;
	flags |= (h->Mode & VFS_OPENFLAG_NONBLOCK) ? VFS_IOFLAG_NOBLOCK : 0;
	ret = h->Node->Type->Write(h->Node, h->Position, Length, Buffer, flags);
	if(ret != Length)	LOG("%i/%i written", ret, Length);
	if(ret == (size_t)-1)	return -1;

	h->Position += ret;
	return ret;
}

/**
 * \fn Uint64 VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, const void *Buffer)
 * \brief Write data to a file at a given offset
 */
size_t VFS_WriteAt(int FD, Uint64 Offset, size_t Length, const void *Buffer)
{
	tVFS_Handle	*h;
	size_t	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if(!h->Node->Type || !h->Node->Type->Write)	return 0;

	if( !MM_GetPhysAddr(h->Node->Type->Write) ) {
		Log_Error("VFS", "Node type %p(%s) write method is junk %p",
			h->Node->Type, h->Node->Type->TypeName,
			h->Node->Type->Write);
		return -1;
	}
	Uint flags = 0;
	flags |= (h->Mode & VFS_OPENFLAG_NONBLOCK) ? VFS_IOFLAG_NOBLOCK : 0;
	ret = h->Node->Type->Write(h->Node, Offset, Length, Buffer, flags);
	if(ret == (size_t)-1)	return -1;
	return ret;
}

/**
 * \fn Uint64 VFS_Tell(int FD)
 * \brief Returns the current file position
 */
Uint64 VFS_Tell(int FD)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	return h->Position;
}

/**
 * \fn int VFS_Seek(int FD, Sint64 Offset, int Whence)
 * \brief Seek to a new location
 * \param FD	File descriptor
 * \param Offset	Where to go
 * \param Whence	From where
 */
int VFS_Seek(int FD, Sint64 Offset, int Whence)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	// Set relative to current position
	if(Whence == 0) {
		LOG("(FD%x)->Position += %lli", FD, Offset);
		h->Position += Offset;
		return 0;
	}
	
	// Set relative to end of file
	if(Whence < 0) {
		if( h->Node->Size == (Uint64)-1 )	return -1;

		LOG("(FD%x)->Position = %llx - %llx", FD, h->Node->Size, Offset);
		h->Position = h->Node->Size - Offset;
		return 0;
	}
	
	// Set relative to start of file
	LOG("(FD%x)->Position = %llx", FD, Offset);
	h->Position = Offset;
	return 0;
}

/**
 * \fn int VFS_IOCtl(int FD, int ID, void *Buffer)
 * \brief Call an IO Control on a file
 */
int VFS_IOCtl(int FD, int ID, void *Buffer)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h) {
		LOG("FD%i is invalid", FD);
		errno = EINVAL;
		return -1;
	}

	if(!h->Node->Type || !h->Node->Type->IOCtl) {
		LOG("FD%i does not have an IOCtl method");
		errno = EINVAL;
		return -1;
	}
	return h->Node->Type->IOCtl(h->Node, ID, Buffer);
}

/**
 * \fn int VFS_FInfo(int FD, tFInfo *Dest, int MaxACLs)
 * \brief Retrieve file information
 * \return Number of ACLs stored
 */
int VFS_FInfo(int FD, tFInfo *Dest, int MaxACLs)
{
	tVFS_Handle	*h;
	 int	max;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;

	if( h->Mount )
		Dest->mount = h->Mount->Identifier;
	else
		Dest->mount = 0;
	Dest->inode = h->Node->Inode;	
	Dest->uid = h->Node->UID;
	Dest->gid = h->Node->GID;
	Dest->size = h->Node->Size;
	Dest->atime = h->Node->ATime;
	Dest->ctime = h->Node->MTime;
	Dest->mtime = h->Node->CTime;
	Dest->numacls = h->Node->NumACLs;
	
	Dest->flags = 0;
	if(h->Node->Flags & VFS_FFLAG_DIRECTORY)	Dest->flags |= 0x10;
	if(h->Node->Flags & VFS_FFLAG_SYMLINK)	Dest->flags |= 0x20;
	
	max = (MaxACLs < h->Node->NumACLs) ? MaxACLs : h->Node->NumACLs;
	memcpy(&Dest->acls, h->Node->ACLs, max*sizeof(tVFS_ACL));
	
	return max;
}

// === EXPORTS ===
EXPORT(VFS_Read);
EXPORT(VFS_Write);
EXPORT(VFS_ReadAt);
EXPORT(VFS_WriteAt);
EXPORT(VFS_IOCtl);
EXPORT(VFS_Seek);
EXPORT(VFS_Tell);

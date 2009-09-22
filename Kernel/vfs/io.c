/*
 * AcessMicro VFS
 * - File IO Passthru's
 */
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"

#define DEBUG	0

#if DEBUG
#else
# undef ENTER
# undef LOG
# undef LEAVE
# define ENTER(...)
# define LOG(...)
# define LEAVE(...)
#endif

// === CODE ===
/**
 * \fn Uint64 VFS_Read(int FD, Uint64 Length, void *Buffer)
 * \brief Read data from a node (file)
 */
Uint64 VFS_Read(int FD, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	ENTER("iFD XLength pBuffer", FD, Length, Buffer);
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_READ) || h->Node->Flags & VFS_FFLAG_DIRECTORY ) {
		LEAVE('i', -1);
		return -1;
	}

	if(!h->Node->Read) {
		LEAVE('i', 0);
		return 0;
	}
	
	ret = h->Node->Read(h->Node, h->Position, Length, Buffer);
	if(ret == -1) {
		LEAVE('i', -1);
		return -1;
	}
	
	h->Position += ret;
	LEAVE('X', ret);
	return ret;
}

/**
 * \fn Uint64 VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read data from a given offset (atomic)
 */
Uint64 VFS_ReadAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_READ) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if(!h->Node->Read) {
		Warning("VFS_ReadAt - Node %p, does not have a read method", h->Node);
		return 0;
	}
	ret = h->Node->Read(h->Node, Offset, Length, Buffer);
	if(ret == -1)	return -1;
	return ret;
}

/**
 * \fn Uint64 VFS_Write(int FD, Uint64 Length, void *Buffer)
 * \brief Read data from a node (file)
 */
Uint64 VFS_Write(int FD, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if(!h->Node->Write)	return 0;
	
	ret = h->Node->Write(h->Node, h->Position, Length, Buffer);
	if(ret == -1)	return -1;
	h->Position += ret;
	return ret;
}

/**
 * \fn Uint64 VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write data to a file at a given offset (atomic)
 */
Uint64 VFS_WriteAt(int FD, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tVFS_Handle	*h;
	Uint64	ret;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	if( !(h->Mode & VFS_OPENFLAG_WRITE) )	return -1;
	if( h->Node->Flags & VFS_FFLAG_DIRECTORY )	return -1;

	if(!h->Node->Write)	return 0;
	ret = h->Node->Write(h->Node, Offset, Length, Buffer);
	if(ret == -1)	return -1;
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
 * \fn int VFS_Seek(int FD, Sint64 Distance, int Whence)
 * \brief Seek to a new location
 * \param FD	File descriptor
 * \param Distance	Where to go
 * \param Whence	From where
 */
int VFS_Seek(int FD, Sint64 Distance, int Whence)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;
	
	// Set relative to current position
	if(Whence == 0) {
		h->Position += Distance;
		return 0;
	}
	
	// Set relative to end of file
	if(Whence < 0) {
		h->Position = h->Node->Size - Distance;
		return 0;
	}
	
	// Set relative to start of file
	h->Position = Distance;
	return 0;
}

/**
 * \fn int VFS_IOCtl(int FD, int ID, void *Buffer)
 */
int VFS_IOCtl(int FD, int ID, void *Buffer)
{
	tVFS_Handle	*h;
	
	h = VFS_GetHandle(FD);
	if(!h)	return -1;

	if(!h->Node->IOCtl)	return -1;
	return h->Node->IOCtl(h->Node, ID, Buffer);
}

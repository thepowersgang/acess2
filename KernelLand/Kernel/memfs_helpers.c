/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * memfs_helpers.c
 * - Helpers for in-memory filesystems
 */
#include <memfs_helpers.h>

// === CODE ===
void MemFS_InitDir(tMemFS_DirHdr *Dir)
{
	MemFS_InitFile(&Dir->FileHdr);
}
void MemFS_InitFile(tMemFS_FileHdr *File)
{
	File->Next = NULL;
}

int MemFS_ReadDir(tMemFS_DirHdr *Dir, int Pos, char Name[FILENAME_MAX])
{
	 int	i = 0;
	// TODO: Lock
	for( tMemFS_FileHdr *file = Dir->FirstChild; file; file = file->Next, i ++ )
	{
		if( i == Pos )
		{
			strncpy(Name, file->Name, FILENAME_MAX);
			return 0;
		}
	}
	return -EINVAL;
}
tMemFS_FileHdr *MemFS_FindDir(tMemFS_DirHdr *Dir, const char *Name)
{
	// TODO: Lock
	for( tMemFS_FileHdr *file = Dir->FirstChild; file; file = file->Next )
	{
		if( strcmp(file->Name, Name) == 0 )
			return file;
	}
	return NULL;
}
tMemFS_FileHdr *MemFS_Remove(tMemFS_DirHdr *Dir, const char *Name)
{
	UNIMPLEMENTED();
	return NULL;
}
bool MemFS_Insert(tMemFS_DirHdr *Dir, tMemFS_FileHdr *File)
{
	UNIMPLEMENTED();
	return false;
}


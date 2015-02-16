/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * memfs_helpers.h
 * - Helpers for in-memory filesystems
 *
 * Provides name lookup, iteration, and node insertion
 */
#ifndef _MEMFS_HELPERS_H_
#define _MEMFS_HELPERS_H_

#include <vfs.h>

typedef struct sMemFS_FileHdr	tMemFS_FileHdr;
typedef struct sMemFS_DirHdr	tMemFS_DirHdr;

extern void	MemFS_InitDir (tMemFS_DirHdr  *Dir);
extern void	MemFS_InitFile(tMemFS_FileHdr *File);

/*
 * \brief Fetch the name of the file at the specified position
 * \return standard tVFS_NodeType.ReadDir return values
 */
extern int	MemFS_ReadDir(tMemFS_DirHdr *Dir, int Pos, char Name[FILENAME_MAX]);
/*
 * \brief Look up a file in a directory
 */
extern tMemFS_FileHdr	*MemFS_FindDir(tMemFS_DirHdr *Dir, const char *Name);
/**
 * \brief Remove a named file from a directory
 * \return File header for \a Name, or NULL if not found
 */
extern tMemFS_FileHdr	*MemFS_Remove(tMemFS_DirHdr *Dir, const char *Name);
/**
 * \brief Insert a pre-constructed file header into the directory
 * \param Dir	Directory
 * \return false if name already exists
 */
extern bool	MemFS_Insert(tMemFS_DirHdr *Dir, tMemFS_FileHdr *File);

struct sMemFS_FileHdr
{
	tMemFS_FileHdr	*Next;
	const char	*Name;
};

struct sMemFS_DirHdr
{
	tMemFS_FileHdr	FileHdr;
	//tRWLock	Lock;
	tMemFS_FileHdr	*FirstChild;
};

#endif


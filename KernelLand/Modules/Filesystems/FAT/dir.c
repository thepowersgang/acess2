/*
 * Acess2 FAT12/16/32 Driver
 * - By John Hodge (thePowersGang)
 *
 * dir.c
 * - Directory access/manipulation code
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include "common.h"

// === PROTOTYPES ===
void	FAT_int_ProperFilename(char *dest, const char *src);
 int	FAT_int_CreateName(fat_filetable *ft, const Uint16 *LongFileName, char *Dest);
 int	FAT_int_ConvertUTF16_to_UTF8(Uint8 *Dest, const Uint16 *Source);
 int	FAT_int_ConvertUTF8_to_UTF16(Uint16 *Dest, const Uint8 *Source);

 int	FAT_int_GetEntryByName(tVFS_Node *DirNode, const char *Name, fat_filetable *Entry);
 int	FAT_int_GetEntryByCluster(tVFS_Node *DirNode, Uint32 Cluster, fat_filetable *Entry);
 int	FAT_int_ReadDirSector(tVFS_Node *Node, int Sector, fat_filetable *Buffer);
#if SUPPORT_WRITE
 int	FAT_int_WriteDirEntry(tVFS_Node *Node, int ID, fat_filetable *Entry);
#endif
#if USE_LFN
Uint16	*FAT_int_GetLFN(tVFS_Node *Node, int ID);
void	FAT_int_DelLFN(tVFS_Node *Node, int ID);
#endif
 int	FAT_ReadDir(tVFS_Node *Node, int ID, char Dest[FILENAME_MAX]);
tVFS_Node	*FAT_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
tVFS_Node	*FAT_GetNodeFromINode(tVFS_Node *Root, Uint64 Inode);
#if SUPPORT_WRITE
tVFS_Node	*FAT_Mknod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	FAT_int_IsValid83Filename(const char *Name);
 int	FAT_Link(tVFS_Node *DirNode, const char *NewName, tVFS_Node *Node);
 int	FAT_Relink(tVFS_Node *node, const char *OldName, const char *NewName);
#endif

// === CODE ===

/**
 * \brief Converts a FAT directory entry name into a proper filename
 * \param dest	Destination array (must be at least 13 bytes in size)
 * \param src	8.3 filename (concatenated, e.g 'FILE1   TXT')
 */
void FAT_int_ProperFilename(char *dest, const char *src)
{
	 int	inpos, outpos;
	
	// Name
	outpos = 0;
	for( inpos = 0; inpos < 8; inpos++ ) {
		if(src[inpos] == ' ')	break;
		dest[outpos++] = src[inpos];
	}
	inpos = 8;
	// Check for empty extensions
	if(src[8] != ' ')
	{
		dest[outpos++] = '.';
		for( ; inpos < 11; inpos++)	{
			if(src[inpos] == ' ')	break;
			dest[outpos++] = src[inpos];
		}
	}
	dest[outpos++] = '\0';
	
	//LOG("dest='%s'", dest);
}

/**
 * \fn char *FAT_int_CreateName(fat_filetable *ft, Uint8 *LongFileName)
 * \brief Converts either a LFN or a 8.3 Name into a proper name
 * \param ft	Pointer to the file's entry in the parent directory
 * \param LongFileName	Long file name pointer
 * \return Filename as a heap string
 */
int FAT_int_CreateName(fat_filetable *ft, const Uint16 *LongFileName, char *Dest)
{
	ENTER("pft sLongFileName", ft, LongFileName);
	#if USE_LFN
	if(LongFileName && LongFileName[0] != 0)
	{
		 int	len = FAT_int_ConvertUTF16_to_UTF8(NULL, LongFileName);
		if( len > FILENAME_MAX ) {
			LEAVE('i', -1);
			return -1;
		}
		FAT_int_ConvertUTF16_to_UTF8((Uint8*)Dest, LongFileName);
	}
	else
	{
	#endif
		FAT_int_ProperFilename(Dest, ft->name);
	#if USE_LFN
	}
	#endif
	LEAVE('i', 0);
	return 0;
}

#if USE_LFN
int FAT_int_CompareUTF16_UTF8(const Uint16 *Str16, const char *Str8)
{
	 int	pos16 = 0, pos8 = 0;
	const Uint8	*str8 = (const Uint8 *)Str8;
	
	while( Str16[pos16] && str8[pos8] )
	{
		Uint32	cp8, cp16;
		if( Str16[pos16] & 0x8000 ) {
			// Do something!
			cp16 = 0;
		}
		else {
			cp16 = Str16[pos16];
			pos16 ++;
		}
		pos8 += ReadUTF8(str8 + pos8, &cp8);
	
		if(cp16 == cp8)	continue ;
		
		if(cp16 < cp8)
			return -1;
		else
			return 1;
	}
	if(Str16[pos16] == str8[pos8])
		return 0;
	if(Str16[pos16] < str8[pos8])
		return -1;
	else
		return 1;
}

int FAT_int_ConvertUTF16_to_UTF8(Uint8 *Dest, const Uint16 *Source)
{
	 int	len = 0;
	for( ; *Source; Source ++ )
	{
		// TODO: Decode/Reencode
		if( Dest )
			Dest[len] = *Source;
		len += 1;
	}
	if( Dest )
		Dest[len] = 0;
	return len;
}

int FAT_int_ConvertUTF8_to_UTF16(Uint16 *Dest, const Uint8 *Source)
{
	 int	len = 0;
	while( *Source )
	{
		Uint32	cp;
		int cpl;
		
		cpl = ReadUTF8(Source, &cp);
		if(cp < 0x8000) {
			if( Dest )
				Dest[len] = cp;
			len ++;
		}
		else {
			// TODO!
		}
		Source += cpl;
	}
	Dest[len] = 0;
	return len;
}

int FAT_int_ParseLFN(const fat_filetable *Entry, Uint16 *Buffer)
{
	const fat_longfilename	*lfnInfo;
	 int	ofs;
	
	lfnInfo = (const void*)Entry;
	
	if(lfnInfo->id & 0x40) {
		memset(Buffer, 0, 256*2);
	}
	ofs = (lfnInfo->id & 0x3F) * 13 - 1;
	if( ofs >= 255 )
		return -1;
	
	Buffer[ofs--] = lfnInfo->name3[1];	Buffer[ofs--] = lfnInfo->name3[0];
	Buffer[ofs--] = lfnInfo->name2[5];	Buffer[ofs--] = lfnInfo->name2[4];
	Buffer[ofs--] = lfnInfo->name2[3];	Buffer[ofs--] = lfnInfo->name2[2];
	Buffer[ofs--] = lfnInfo->name2[1];	Buffer[ofs--] = lfnInfo->name2[0];
	Buffer[ofs--] = lfnInfo->name1[4];	Buffer[ofs--] = lfnInfo->name1[3];
	Buffer[ofs--] = lfnInfo->name1[2];	Buffer[ofs--] = lfnInfo->name1[1];
	Buffer[ofs--] = lfnInfo->name1[0];
	
	if((lfnInfo->id&0x3F) == 1)
		return 1;
	return 0;
}
#endif

int FAT_int_GetEntryByName(tVFS_Node *DirNode, const char *Name, fat_filetable *Entry)
{
	fat_filetable	fileinfo[16];
	char	tmpName[13];
	#if USE_LFN
	Uint16	lfn[256];
	 int	lfnId = -1;
	#endif

	ENTER("pDirNode sName pEntry", DirNode, Name, Entry);

	for( int i = 0; ; i++ )
	{
		if((i & 0xF) == 0) {
			if(FAT_int_ReadDirSector(DirNode, i/16, fileinfo))
			{
				LEAVE('i', -1);
				return -1;
			}
		}
		
		//Check if the files are free
		if(fileinfo[i&0xF].name[0] == '\0')	break;	// End of List marker
		if(fileinfo[i&0xF].name[0] == '\xE5')	continue;	// Free entry
		
		
		#if USE_LFN
		// Long File Name Entry
		if(fileinfo[i & 0xF].attrib == ATTR_LFN)
		{
			if( FAT_int_ParseLFN(&fileinfo[i&0xF], lfn) )
				lfnId = i+1;
			continue ;
		}
		// Remove LFN if it does not apply
		if(lfnId != i)	lfn[0] = 0;
		#else
		if(fileinfo[i&0xF].attrib == ATTR_LFN)	continue;
		#endif

		// Get Real Filename
		FAT_int_ProperFilename(tmpName, fileinfo[i&0xF].name);
//		LOG("tmpName = '%s'", tmpName);
//		#if DEBUG
//		Debug_HexDump("FAT tmpName", tmpName, strlen(tmpName));
//		#endif
/*
		#if DEBUG && USE_LFN
		if(lfnId == i)
		{
			Uint8 lfntmp[256*3+1];
			FAT_int_ConvertUTF16_to_UTF8(lfntmp, lfn);
			LOG("lfntmp = '%s'", lfntmp);
		}
		#endif
*/
	
		// Only the long name is case sensitive, 8.3 is not
		#if USE_LFN
		if(strucmp(tmpName, Name) == 0 || FAT_int_CompareUTF16_UTF8(lfn, Name) == 0)
		#else
		if(strucmp(tmpName, Name) == 0)
		#endif
		{
			memcpy(Entry, fileinfo + (i&0xF), sizeof(*Entry));
			LOG("Found %s at %i", Name, i);
			LEAVE('i', i);
			return i;
		}
	}
	
	LEAVE('i', -1);
	return -1;
}

int FAT_int_GetEntryByCluster(tVFS_Node *DirNode, Uint32 Cluster, fat_filetable *Entry)
{
	 int	ents_per_sector = 512 / sizeof(fat_filetable); 
	fat_filetable	fileinfo[ents_per_sector];
	 int	i, sector;

	if( Mutex_Acquire(&DirNode->Lock) ) {
		return -EINTR;
	}
	
	sector = 0;
	for( i = 0; ; i ++ )
	{
		if( i == 0 || i == ents_per_sector )
		{
			if(FAT_int_ReadDirSector(DirNode, sector, fileinfo))
			{
				LOG("ReadDirSector failed");
				break ;
			}
			i = 0;
			sector ++;
		}
	
		// Check for free/end of list
		if(fileinfo[i].name[0] == '\0') break;	// End of List marker
		if(fileinfo[i].name[0] == '\xE5')	continue;	// Free entry
		
		if(fileinfo[i].attrib == ATTR_LFN)	continue;

		LOG("fileinfo[i].cluster = %x:%04x", fileinfo[i].clusterHi, fileinfo[i].cluster);
		#if DEBUG
		{
			char	tmpName[13];
			FAT_int_ProperFilename(tmpName, fileinfo[i].name);
			LOG("tmpName = '%s'", tmpName);
		}
		#endif
		
	
		if(fileinfo[i].cluster != (Cluster & 0xFFFF))	continue;
		if(fileinfo[i].clusterHi != ((Cluster >> 16) & 0xFFFF))	continue;
	
		memcpy(Entry, &fileinfo[i], sizeof(*Entry));
		Mutex_Release(&DirNode->Lock);
		return i;
	}
	
	Mutex_Release(&DirNode->Lock);
	return -ENOENT;
}

/* 
 * ====================
 *     Directory IO
 * ====================
 */

/**
 * \brief Reads a sector from the disk
 * \param Node	Directory node to read
 * \param Sector	Sector number in the directory to read
 * \param Buffer	Destination buffer for the read data
 */
int FAT_int_ReadDirSector(tVFS_Node *Node, int Sector, fat_filetable *Buffer)
{
	Uint64	addr;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode iSector pEntry", Node, Sector, Buffer);
	
	// Parse address
	if(FAT_int_GetAddress(Node, Sector * 512, &addr, NULL))
	{
		LEAVE('i', 1);
		return 1;
	}
	
	LOG("addr = 0x%llx", addr);
	// Read Sector
	if(VFS_ReadAt(disk->fileHandle, addr, 512, Buffer) != 512)
	{
		LEAVE('i', 1);
		return 1;
	}
	
	LEAVE('i', 0);
	return 0;
}

#if SUPPORT_WRITE
/**
 * \brief Write a sector to the disk
 * \param Node	Directory node to write
 * \param Sector	Sector number in the directory to write
 * \param Buffer	Source data
 */
int FAT_int_WriteDirSector(tVFS_Node *Node, int Sector, const fat_filetable *Buffer)
{
	Uint64	addr;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode iSector pEntry", Node, Sector, Buffer);
	
	// Parse address
	if(FAT_int_GetAddress(Node, Sector * 512, &addr, NULL))
	{
		LEAVE('i', 1);
		return 1;
	}
	
	// Read Sector
	if(VFS_WriteAt(disk->fileHandle, addr, 512, Buffer) != 512)
	{
		LEAVE('i', 1);
		return 1;
	}
	
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Writes an entry to the disk
 * \todo Support expanding a directory
 * \param Node	Directory node
 * \param ID	ID of entry to update
 * \param Entry	Entry data
 * \return Zero on success, non-zero on error
 */
int FAT_int_WriteDirEntry(tVFS_Node *Node, int ID, fat_filetable *Entry)
{
	Uint64	addr = 0;
	 int	tmp;
	Uint32	cluster = 0;
	tFAT_VolInfo	*disk = Node->ImplPtr;
	
	ENTER("pNode iID pEntry", Node, ID, Entry);
	
	tmp = FAT_int_GetAddress(Node, ID * sizeof(fat_filetable), &addr, &cluster);
	if( tmp )
	{
		//TODO: Allocate a cluster
		cluster = FAT_int_AllocateCluster(Node->ImplPtr, cluster);
		if(cluster == -1) {
			Log_Warning("FAT", "Unable to allocate an other cluster for %p", Node);
			LEAVE('i', 1);
			return 1;
		}
		FAT_int_GetAddress(Node, ID * sizeof(fat_filetable), &addr, &cluster);
	}
	

	LOG("addr = 0x%llx", addr);
	
	// Wriet data to disk
	VFS_WriteAt(disk->fileHandle, addr, sizeof(fat_filetable), Entry);
	
	LEAVE('i', 0);
	return 0;
}
#endif

#if USE_LFN	
/**
 * \fn Uint16 *FAT_int_GetLFN(tVFS_Node *node)
 * \brief Return pointer to LFN cache entry
 * \param Node	Directory node
 * \param ID	ID of the short name
 * \return Pointer to the LFN cache entry
 */
Uint16 *FAT_int_GetLFN(tVFS_Node *Node, int ID)
{
	if( Mutex_Acquire( &Node->Lock ) ) {
		return NULL;
	}
	
	// TODO: Thread Safety (Lock things)
	tFAT_LFNCache	*cache = Node->Data;
	
	// Create a cache if it isn't there
	if(!cache) {
		cache = Node->Data = malloc( sizeof(tFAT_LFNCache) + sizeof(tFAT_LFNCacheEnt) );
		cache->NumEntries = 1;
		cache->Entries[0].ID = ID;
		cache->Entries[0].Data[0] = 0;
		Mutex_Release( &Node->Lock );
		//Log_Debug("FAT", "Return = %p (new)", cache->Entries[0].Data);
		return cache->Entries[0].Data;
	}
	
	// Scan for this entry
	int firstFree = -1;
	for( int i = 0; i < cache->NumEntries; i++ )
	{
		if( cache->Entries[i].ID == ID ) {
			Mutex_Release( &Node->Lock );
			//Log_Debug("FAT", "Return = %p (match)", cache->Entries[i].Data);
			return cache->Entries[i].Data;
		}
		if( cache->Entries[i].ID == -1 && firstFree == -1 )
			firstFree = i;
	}
	
	 int	cache_entry = firstFree;
	
	if(firstFree == -1)
	{
		size_t newsize = offsetof(tFAT_LFNCache, Entries[cache->NumEntries+1]);
		tFAT_LFNCache *new_alloc = realloc( Node->Data, newsize );
		if( !new_alloc ) {
			Log_Error("FAT", "realloc() fail, unable to allocate %zi for LFN cache", newsize);
			Mutex_Release( &Node->Lock );
			return NULL;
		}
		Node->Data = new_alloc;
		//Log_Debug("FAT", "Realloc (%i)\n", i);
		cache = Node->Data;
		cache_entry = cache->NumEntries;
		cache->NumEntries ++;
	}
	else
	{
		cache_entry = firstFree;
	}
	
	// Create new entry
	cache->Entries[ cache_entry ].ID = ID;
	cache->Entries[ cache_entry ].Data[0] = '\0';
	
	Mutex_Release( &Node->Lock );
	//Log_Debug("FAT", "Return = %p (firstFree, i = %i)", cache->Entries[i].Data, i);
	return cache->Entries[ cache_entry ].Data;
}

/**
 * \fn void FAT_int_DelLFN(tVFS_Node *node)
 * \brief Delete a LFN cache entry
 * \param Node	Directory node
 * \param ID	File Entry ID
 */
void FAT_int_DelLFN(tVFS_Node *Node, int ID)
{
	tFAT_LFNCache	*cache = Node->Data;
	 int	i;
	
	// Fast return
	if(!cache)	return;
	
	// Scan for a current entry
	for( i = 0; i < cache->NumEntries; i++ )
	{
		if( cache->Entries[i].ID == ID )
			cache->Entries[i].ID = -1;
	}
	return ;
}
#endif

/**
 * \fn char *FAT_ReadDir(tVFS_Node *Node, int ID)
 * \param Node	Node structure of directory
 * \param ID	Directory position
 * \return Filename as a heap string, NULL or VFS_SKIP
 */
int FAT_ReadDir(tVFS_Node *Node, int ID, char Dest[FILENAME_MAX])
{
	fat_filetable	fileinfo[16];	// sizeof(fat_filetable)=32, so 16 per sector
	 int	a;
	#if USE_LFN
	Uint16	*lfn = NULL;
	#endif
	
	ENTER("pNode iID", Node, ID);
	
	if(FAT_int_ReadDirSector(Node, ID/16, fileinfo))
	{
		LOG("End of chain, end of dir");
		LEAVE('i', -EIO);
		return -EIO;
	}
	
	// Offset in sector
	a = ID % 16;

	LOG("fileinfo[%i].name[0] = 0x%x", a, (Uint8)fileinfo[a].name[0]);
	
	// Check if this is the last entry
	if( fileinfo[a].name[0] == '\0' ) {
		Node->Size = ID;
		LOG("End of list");
		LEAVE('i', -ENOENT);
		return -ENOENT;	// break
	}
	
	// Check for empty entry
	if( (Uint8)fileinfo[a].name[0] == 0xE5 ) {
		LOG("Empty Entry");
		LEAVE_RET('i', 1);	// Skip
	}
	
	#if USE_LFN
	// Get Long File Name Cache
	if(fileinfo[a].attrib == ATTR_LFN)
	{
		fat_longfilename	*lfnInfo;
		
		lfnInfo = (fat_longfilename *) &fileinfo[a];
		
		// Get cache for corresponding file
		// > ID + Index gets the corresponding short node
		lfn = FAT_int_GetLFN( Node, ID + (lfnInfo->id & 0x3F) );

		a = FAT_int_ParseLFN(&fileinfo[a], lfn);
		if( a < 0 ) {
			LOG("Invalid LFN, error");
			LEAVE_RET('i', -EIO);
		}

		LEAVE_RET('i', 1);	// Skip
	}
	#endif
	
	// Check if it is a volume entry
	if(fileinfo[a].attrib & 0x08) {
		LEAVE_RET('i', 1);	// Skip
	}
	// Ignore .
	if(fileinfo[a].name[0] == '.' && fileinfo[a].name[1] == ' ') {
		LEAVE_RET('i', 1);	// Skip
	}
	// and ..
	if(fileinfo[a].name[0] == '.' && fileinfo[a].name[1] == '.' && fileinfo[a].name[2] == ' ') {
		LEAVE_RET('i', 1);	// Skip
	}
	
	LOG("name='%c%c%c%c%c%c%c%c.%c%c%c'",
		fileinfo[a].name[0], fileinfo[a].name[1], fileinfo[a].name[2], fileinfo[a].name[3],
		fileinfo[a].name[4], fileinfo[a].name[5], fileinfo[a].name[6], fileinfo[a].name[7],
		fileinfo[a].name[8], fileinfo[a].name[9], fileinfo[a].name[10] );
	
	#if USE_LFN
	lfn = FAT_int_GetLFN(Node, ID);
	//Log_Debug("FAT", "lfn = %p'%s'", lfn, lfn);
	FAT_int_CreateName(&fileinfo[a], lfn, Dest);
	#else
	FAT_int_CreateName(&fileinfo[a], NULL, Dest);
	#endif
	
	LEAVE('i', 0);
	return 0;
}

/**
 * \fn tVFS_Node *FAT_FindDir(tVFS_Node *node, char *name)
 * \brief Finds an entry in the current directory
 */
tVFS_Node *FAT_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	fat_filetable	fileent;
	
	ENTER("pNode sname", Node, Name);	

	// Fast Returns
	if(!Name || Name[0] == '\0') {
		LEAVE('n');
		return NULL;
	}

	if( FAT_int_GetEntryByName(Node, Name, &fileent) == -1 ) {
		LEAVE('n');
		return NULL;
	}
	

	tVFS_Node *ret = FAT_int_CreateNode(Node, &fileent);
	LOG("Found %s as %p", Name, ret);
	LEAVE('p', ret);
	return ret;
}

tVFS_Node *FAT_GetNodeFromINode(tVFS_Node *Root, Uint64 Inode)
{
	tFAT_VolInfo	*disk = Root->ImplPtr;
	tVFS_Node	*dirnode, *ret;
	fat_filetable	ft;

	ENTER("pRoot XInode", Root, Inode);

	ret = FAT_int_GetNode(disk, Inode & 0xFFFFFFFF);
	if( ret ) {
		if( (ret->Inode >> 32) != 0 ) {
			LOG("Node in cache, quick return");
			LEAVE('p', ret);
			return ret;
		}
		else {
			LOG("Node cached, but incomplete");
			// Fall on through
		}
		ret = NULL;
	}
	
	dirnode = FAT_int_CreateIncompleteDirNode(disk, Inode >> 32);

	int id = FAT_int_GetEntryByCluster(dirnode, Inode & 0xFFFFFFFF, &ft);
	if( id != -1 ) {
		ret = FAT_int_CreateNode(dirnode, &ft);
	}

	dirnode->Type->Close(dirnode);

	LEAVE('p', ret);
	return ret;
}

#if SUPPORT_WRITE
/**
 * \brief Create a new node
 */
tVFS_Node *FAT_Mknod(tVFS_Node *DirNode, const char *Name, Uint Flags)
{
	tFAT_VolInfo	*disk = DirNode->ImplPtr;
	 int	rv;
	fat_filetable	ft;
	memset(&ft, 0, sizeof(ft));

	ENTER("pDirNode sName xFlags", DirNode, Name, Flags);
	
	// Allocate a cluster
	Uint32 cluster = FAT_int_AllocateCluster(disk, -1);
	LOG("Cluster 0x%07x allocated", cluster);
	
	// Create a temporary file table entry for an empty node
	ft.cluster = cluster & 0xFFFF;
	ft.clusterHi = cluster >> 16;
	ft.size = 0;
	if( Flags & VFS_FFLAG_DIRECTORY )
		ft.attrib = ATTR_DIRECTORY;
	else
		ft.attrib = 0;
	
	tVFS_Node *newnode = FAT_int_CreateNode(DirNode, &ft);
	if( !newnode ) {
		errno = -EINTERNAL;
		return NULL;
	}
	LOG("newnode = %p", newnode);

	// Call link
	if( (rv = FAT_Link(DirNode, Name, newnode)) ) {
		newnode->ImplInt |= FAT_FLAG_DELETE;
	}
	LEAVE('p', newnode);
	return newnode;
}

/**
 * \brief Internal - Checks if a character is valid in an 8.3 filename
 */
static inline int is_valid_83_char(unsigned char ch)
{
	if( '0' <= ch && ch <= '9' )
		return 1;
	if( 'A' <= ch && ch <= 'Z' )
		return 1;
	if( 'a' <= ch && ch <= 'z' )
		return 1;
	if( strchr("$%'-_@~`#!(){}^#&", ch) )
		return 1;
	if( ch > 128 )
		return 1;
	return 0;
}

Uint8 FAT_int_UnicodeTo83(Uint32 Input)
{
	Input = toupper(Input);
	// Input = unicode_to_oem(Input);
	if( Input > 256 )
		Input = '_';
	if(!is_valid_83_char(Input))
		Input = '_';
	return Input;
}

/**
 * \brief Internal - Determines if a filename is a valid 8.3 filename
 */
int FAT_int_IsValid83Filename(const char *Name)
{
	 int	i, j;

	if( !Name[0] || Name[0] == '.' )
		return 0;

	// Check filename portion
	for( i = 0; Name[i] && i < 8; i ++ )
	{
		if( Name[i] == '.' )
			break;
		if( !is_valid_83_char(Name[i]) )
			return 0;
	}
	// If the next char is not \0 or '.', it's not valid
	if( Name[i] && Name[i++] != '.' )
		return 0;
	
	// Check the extension portion
	for( j = 0; Name[i+j] && j < 3; j ++ )
	{
		if( !is_valid_83_char(Name[i+j]) )
			return 0;
	}
	
	// After the extension must be the end
	if( Name[i+j] )
		return 0;
	
	return 1;
}

Uint8 FAT_int_MakeLFNChecksum(const char *ShortName)
{
	Uint8 ret = 0;
	for( int i = 0; i < 11; i++ )
	{
		// ret = (ret >>> 1) + ShortName[i]
		// where >>> is rotate right
		ret = ((ret & 1) ? 0x80 : 0x00) + (ret >> 1) + ShortName[i];
	}
	return ret;
}

/**
 * \brief Create a new name for a file
 * \note Since FAT doesn't support reference counting, this will cause double-references if
 *       a file is hardlinked and not unlinked
 */
int FAT_Link(tVFS_Node *DirNode, const char *NewName, tVFS_Node *NewNode)
{
	Uint16	lfn[256];
	fat_filetable	ft;
	 int	nLFNEnt = 0;
	const int eps = 512 / sizeof(fat_filetable);
	fat_filetable	fileinfo[eps];
	
	if( Mutex_Acquire( &DirNode->Lock ) ) {
		return EINTR;
	}

	// -- Ensure duplicates aren't created --
	if( FAT_int_GetEntryByName(DirNode, NewName, &ft) >= 0 ) {
		Mutex_Release( &DirNode->Lock );
		return EEXIST;
	}
	
	// -- Create filetable entry --
	#if 0
	{
		 int	bDirty = 0;
		 int	inofs = 0;
		while( NewName[inofs] && NewName[inofs] == '.' )
			inofs ++, bDirty = 1;
		for( int i = 0; i < 8 && NewName[inofs] && NewName[inofs] != '.'; i ++ )
		{
			Uint32	cp;
			inofs += ReadUTF8(NewName + inofs, &cp);
			// Spaces are silently skipped
			if(isspace(cp)) {
				i --, bDirty = 1;
				continue ;
			}
			ft.name[i] = FAT_int_UnicodeTo83(cp);
			if(ft.name[i] != cp)
				bDirty = 1;
		}
		while( NewName[inofs] && NewName[inofs] != '.' )
			inofs ++, bDirty = 1;
		for( ; i < 8+3 && NewName[inofs]; i ++ )
		{
			Uint32	cp;
			inofs += ReadUTF8(NewName + inofs, &cp);
			// Spaces are silently skipped
			if(isspace(cp)) {
				i --, bDirty = 1;
				continue ;
			}
			ft.name[i] = FAT_int_UnicodeTo83(cp);
			if(ft.name[i] != cp)
				bDirty = 1;
		}
		if( !NewName[inofs] )	bDirty = 1;
		
		if( bDirty )
		{
			int lfnlen = FAT_int_ConvertUTF8_to_UTF16(lfn, (const Uint8*)NewName);
			lfn[lfnlen] = 0;
			nLFNEnt = DivUp(lfnlen, 13);
		}
	}
	#endif
	 int	bNeedsLFN = !FAT_int_IsValid83Filename(NewName);
	if( bNeedsLFN )
	{
		int lfnlen = FAT_int_ConvertUTF8_to_UTF16(lfn, (const Uint8*)NewName);
		lfn[lfnlen] = 0;
		nLFNEnt = DivUp(lfnlen, 13);
	
		// Create a base mangled filetable entry
		int i, j = 0;
		while(NewName[j] == '.')	j ++;	// Eat leading dots
		for( i = 0; i < 6 && NewName[j] && NewName[j] != '.'; i ++, j ++ )
		{
			if( !isalpha(NewName[j]) && !is_valid_83_char(NewName[j]) )
				ft.name[i] = '_';
			else
				ft.name[i] = toupper(NewName[j]);
		}
		ft.name[i++] = '~';
		ft.name[i++] = '1';
		while(i < 8)	ft.name[i++] = ' ';
		while(NewName[j] && NewName[j] != '.')	j ++;
		for( ; i < 8+3 && NewName[j]; i ++, j ++ )
		{
			if( NewName[j] == '.' )
				i --;
			else if( !is_valid_83_char(NewName[j]) )
				ft.name[i] = '_';
			else
				ft.name[i] = toupper(NewName[j]);
		}
		while(i < 8+3)	ft.name[i++] = ' ';
		
		// - Ensure there isn't a duplicate short-name
		 int	bIsDuplicate = 1;
		while( bIsDuplicate )
		{
			bIsDuplicate = 0;	// Assume none			

			// Scan directory
			for( int id = 0; ; id ++ )
			{
				if( id % eps == 0 )
				{
					if(FAT_int_ReadDirSector(DirNode, id/eps, fileinfo))
						break;	// end of cluster chain
				}
				
				// End of file list
				if( fileinfo[id%eps].name[0] == '\0' )	break;
				// Empty entry
				if( fileinfo[id%eps].name[0] == '\xE5' )	continue;
				// LFN entry
				if( fileinfo[id%eps].attrib == ATTR_LFN )	continue;
				
				// Is this a duplicate?
				if( memcmp(ft.name, fileinfo[id%eps].name, 8+3) == 0 ) {
					bIsDuplicate = 1;
					break;
				}
				
				// No - move a long
			}
			
			// If a duplicate was found, increment the suffix
			if( bIsDuplicate )
			{
				if( ft.name[7] == '9' ) {
					// TODO: Expand into ~00
					Log_Error("FAT", "TODO: Use two digit LFN suffixes");
					Mutex_Release(&DirNode->Lock);
					return ENOTIMPL;
				}
				
				ft.name[7] += 1;
			}
		}
	}
	else
	{
		// Create pure filetable entry
		 int	i;
		// - Copy filename
		for( i = 0; i < 8 && *NewName && *NewName != '.'; i ++, NewName++ )
			ft.name[i] = *NewName;
		// - Pad with spaces
		for( ; i < 8; i ++ )
			ft.name[i] = ' ';
		// - Eat '.'
		if(*NewName)
			NewName ++;
		// - Copy extension
		for( ; i < 8+3 && *NewName; i ++, NewName++ )
			ft.name[i] = *NewName;
		// - Pad with spaces
		for( ; i < 8+3; i ++ )
			ft.name[i] = ' ';
	}

	ft.attrib = 0;
	if(NewNode->Flags & VFS_FFLAG_DIRECTORY )
		ft.attrib |= ATTR_DIRECTORY;	
	ft.ntres     = 0;
	FAT_int_GetFATTimestamp(NewNode->CTime, &ft.cdate, &ft.ctime, &ft.ctimems);
//	ft.ctimems   = ft.ctimems;
	ft.ctime     = LittleEndian16(ft.ctime);
	ft.cdate     = LittleEndian16(ft.cdate);
	FAT_int_GetFATTimestamp(NewNode->MTime, &ft.mdate, &ft.mtime, NULL);
	ft.mtime     = LittleEndian16(ft.mtime);
	ft.mdate     = LittleEndian16(ft.mdate);
	FAT_int_GetFATTimestamp(NewNode->ATime, &ft.adate, NULL, NULL);
	ft.adate     = LittleEndian16(ft.adate);
	ft.clusterHi = LittleEndian16((NewNode->Inode >> 16) & 0xFFFF);
	ft.cluster   = LittleEndian16(NewNode->Inode & 0xFFFF);
	ft.size      = LittleEndian32(NewNode->Size);

	LOG("ft.name = '%.11s'", ft.name);

	// -- Add entry to the directory --
	// Locate a range of nLFNEnt + 1 free entries
	 int	end_id = -1;
	 int	range_first = 0, range_last = -1;
	for( int id = 0; ; id ++ )
	{
		if( id % eps == 0 )
		{
			if(FAT_int_ReadDirSector(DirNode, id/eps, fileinfo))
				break;	// end of cluster chain
		}
		
		// End of file list, break out
		if( fileinfo[id%eps].name[0] == '\0' ) {
			if( id - range_first == nLFNEnt )
				range_last = id;
			end_id = id;
			break;
		}
		
		// If an entry is occupied, clear the range
		if( fileinfo[id%eps].name[0] != '\xE5' ) {
			range_first = id + 1;
			continue ;
		}
		
		// Free entry, check if we have enough
		if( id - range_first == nLFNEnt ) {
			range_last = id;
			break;
		}
		// Check the next one
	}
	if( range_last == -1 )
	{
		// - If there are none, defragment the directory?
		
		// - Else, expand the directory
		if( end_id == -1 ) {
			// End of cluster chain
		}
		else {
			// Just end of block
		}
		// - and if that fails, return an error
		Log_Warning("FAT", "TODO: Impliment directory expansion / defragmenting");
		Mutex_Release(&DirNode->Lock);
		return ENOTIMPL;
	}

	// Calculate the checksum used for LFN
	Uint8	lfn_checksum = 0;
	if( nLFNEnt )
	{
		lfn_checksum = FAT_int_MakeLFNChecksum(ft.name);
	}

	// Insert entries	
	if( range_first % eps != 0 )
		FAT_int_ReadDirSector(DirNode, range_first/eps, fileinfo);
	for( int id = range_first; id <= range_last; id ++ )
	{
		if( id % eps == 0 ) {
			if( id != range_first )
				FAT_int_WriteDirSector(DirNode, (id-1)/eps, fileinfo);
			FAT_int_ReadDirSector(DirNode, id/eps, fileinfo);
		}
		
		if( id == range_last ) {
			// Actual entry
			memcpy(fileinfo + id % eps, &ft, sizeof(fat_filetable));
		}
		else {
			// Long filename
			int lfnid = (nLFNEnt - (id - range_first));
			int ofs = (lfnid-1) * 13;
			 int	i=0, j=0;
			fat_longfilename *lfnent = (void*)( fileinfo + id%eps );
			
			lfnent->id = 0x40 | lfnid;
			lfnent->attrib = ATTR_LFN;
			lfnent->type = 0;
			lfnent->firstCluster = 0;
			lfnent->checksum = lfn_checksum;	// ???

			for( i = 0; i < 13; i ++ )
			{
				Uint16	wd;
				if( (wd = lfn[ofs+j]) )	j ++;
				wd = LittleEndian16(wd);
				if(i < 5)
					lfnent->name1[i    ] = wd;
				else if( i < 5+6 )
					lfnent->name2[i-5  ] = wd;
				else
					lfnent->name3[i-5-6] = wd;
			}
		}
	}
	FAT_int_WriteDirSector(DirNode, range_last/eps, fileinfo);

	Mutex_Release( &DirNode->Lock );
	return 0;
}

/**
 * \fn int FAT_Relink(tVFS_Node *Node, char *OldName, char *NewName)
 * \brief Rename / Delete a file
 */
int FAT_Unlink(tVFS_Node *Node, const char *OldName)
{
	tVFS_Node	*child;
	fat_filetable	ft;
	
	if( Mutex_Acquire(&Node->Lock) ) {
		return EINTR;
	}

	int id = FAT_int_GetEntryByName(Node, OldName, &ft);
	if(id == -1) {
		Mutex_Release(&Node->Lock);
		return ENOTFOUND;
	}

	child = FAT_int_CreateNode(Node->ImplPtr, &ft);
	if( !child ) {
		Mutex_Release(&Node->Lock);
		return EINVAL;
	}
	child->ImplInt |= FAT_FLAG_DELETE;	// Mark for deletion on close

	// TODO: If it has a LFN, remove that too

	// Delete from the directory
	ft.name[0] = '\xE5';
	FAT_int_WriteDirEntry(Node, id, &ft);

	// Close child
	child->Type->Close( child );
	Mutex_Release( &Node->Lock );
	return EOK;
}
#endif

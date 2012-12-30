/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * nativefs.c
 * - Host filesystem access
 */
#define DEBUG	1
#define off_t	_acess_off_t
#define sprintf _acess_sprintf
#include <acess.h>	// Acess
#include <vfs.h>	// Acess
#undef off_t
#undef sprintf
#include <dirent.h>	// Posix
#include <sys/stat.h>	// Posix
#include <stdio.h>	// Posix

//NOTES:
// tVFS_Node->ImplPtr is a pointer to the filesystem flags (tNativeFS)
// tVFS_Node->Data is the path string (heap string)
// tVFS_Node->ImplInt is the path length

// === STRUCTURES ===
typedef struct
{
	 int	InodeHandle;
	 int	bReadOnly;
}	tNativeFS;

// === PROTOTYPES ===
 int	NativeFS_Install(char **Arguments);
tVFS_Node	*NativeFS_Mount(const char *Device, const char **Arguments);
void	NativeFS_Unmount(tVFS_Node *Node);
tVFS_Node	*NativeFS_FindDir(tVFS_Node *Node, const char *Name);
 int	NativeFS_ReadDir(tVFS_Node *Node, int Position, char Dest[FILENAME_MAX]);
size_t	NativeFS_Read(tVFS_Node *Node, _acess_off_t Offset, size_t Length, void *Buffer);
size_t	NativeFS_Write(tVFS_Node *Node, _acess_off_t Offset, size_t Length, const void *Buffer);
void	NativeFS_Close(tVFS_Node *Node);

// === GLOBALS ===
tVFS_NodeType	gNativeFS_FileNodeType = {
	.Read = NativeFS_Read,
	.Write = NativeFS_Write,
	.Close = NativeFS_Close
};
tVFS_NodeType	gNativeFS_DirNodeType = {
	.FindDir = NativeFS_FindDir,
	.ReadDir = NativeFS_ReadDir,
	.Close = NativeFS_Close
};
tVFS_Driver	gNativeFS_Driver = {
	.Name = "nativefs",
	.InitDevice = NativeFS_Mount,
	.Unmount = NativeFS_Unmount
};

// === CODE ===
int NativeFS_Install(char **Arguments)
{
	VFS_AddDriver(&gNativeFS_Driver);
	return 0;
}

tVFS_Node *NativeFS_Mount(const char *Device, const char **Arguments)
{
	tVFS_Node	*ret;
	tNativeFS	*info;
	DIR	*dp;
	
	dp = opendir(Device);
	if(!dp) {
		Log_Warning("NativeFS", "ERROR: Unable to open device root '%s'", Device);
		return NULL;
	}
	
	// Check if directory exists
	// Parse flags from arguments
	info = malloc(sizeof(tNativeFS));
	info->InodeHandle = Inode_GetHandle();
	info->bReadOnly = 0;
	// Create node
	ret = malloc(sizeof(tVFS_Node));
	memset(ret, 0, sizeof(tVFS_Node));
	ret->Data = strdup(Device);
	ret->ImplInt = strlen(ret->Data);
	ret->ImplPtr = info;
	ret->Inode = (Uint64)(tVAddr)dp;
	ret->Flags = VFS_FFLAG_DIRECTORY;

	ret->Type = &gNativeFS_DirNodeType;	

	return ret;
}

void NativeFS_Unmount(tVFS_Node *Node)
{
	tNativeFS	*info = Node->ImplPtr;
	Inode_ClearCache( info->InodeHandle );
	closedir( (void *)(tVAddr)Node->Inode );
	free(Node->Data);
	free(Node);
	free(info);
}

void NativeFS_Close(tVFS_Node *Node)
{
	tNativeFS	*info = Node->ImplPtr;
	DIR	*dp = (Node->Flags & VFS_FFLAG_DIRECTORY) ? (DIR*)(tVAddr)Node->Inode : 0;
	FILE	*fp = (Node->Flags & VFS_FFLAG_DIRECTORY) ? 0 : (FILE*)(tVAddr)Node->Inode;
	
	if( Inode_UncacheNode( info->InodeHandle, Node->Inode ) == 1 ) {
		if(dp)	closedir(dp);
		if(fp)	fclose(fp);
	}
}

tVFS_Node *NativeFS_FindDir(tVFS_Node *Node, const char *Name)
{
	char	*path;
	tNativeFS	*info = Node->ImplPtr;
	tVFS_Node	baseRet;
	struct stat statbuf;

	ENTER("pNode sName", Node, Name);
	
	// Create path
	path = malloc(Node->ImplInt + 1 + strlen(Name) + 1);
	strcpy(path, Node->Data);
	path[Node->ImplInt] = '/';
	strcpy(path + Node->ImplInt + 1, Name);
	
	LOG("path = '%s'", path);
	
	// Check if file exists
	if( stat(path, &statbuf) ) {
		free(path);
		LOG("Doesn't exist");
		LEAVE('n');
		return NULL;
	}
	
	memset(&baseRet, 0, sizeof(tVFS_Node));
	
	// Check file type
	if( S_ISDIR(statbuf.st_mode) )
	{
		LOG("Directory");
		baseRet.Inode = (Uint64)(tVAddr) opendir(path);
		baseRet.Type = &gNativeFS_DirNodeType;
		baseRet.Flags |= VFS_FFLAG_DIRECTORY;
		baseRet.Size = -1;
	}
	else
	{
		LOG("File");
		baseRet.Inode = (Uint64)(tVAddr) fopen(path, "r+");
		baseRet.Type = &gNativeFS_FileNodeType;
		
		fseek( (FILE*)(tVAddr)baseRet.Inode, 0, SEEK_END );
		baseRet.Size = ftell( (FILE*)(tVAddr)baseRet.Inode );
	}
	
	// Create new node
	baseRet.ImplPtr = info;
	baseRet.ImplInt = strlen(path);
	baseRet.Data = path;
	
	LEAVE('-');
	return Inode_CacheNode(info->InodeHandle, &baseRet);
}

int NativeFS_ReadDir(tVFS_Node *Node, int Position, char Dest[FILENAME_MAX])
{
	struct dirent	*ent;
	DIR	*dp = (void*)(tVAddr)Node->Inode;

	ENTER("pNode iPosition", Node, Position);

	// TODO: Keep track of current position in the directory
	// TODO: Lock node during this
	rewinddir(dp);
	do {
		ent = readdir(dp);
	} while(Position-- && ent);

	if( !ent ) {
		LEAVE('i', -ENOENT);
		return -ENOENT;
	}
	
	strncpy(Dest, ent->d_name, FILENAME_MAX);

	// TODO: Unlock node	

	LEAVE('i', 0);
	return 0;
}

size_t NativeFS_Read(tVFS_Node *Node, _acess_off_t Offset, size_t Length, void *Buffer)
{
	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	if( fseek( (FILE *)(tVAddr)Node->Inode, Offset, SEEK_SET ) != 0 )
	{
		LEAVE('i', 0);
		return 0;
	}
	LEAVE('-');
	return fread( Buffer, 1, Length, (FILE *)(tVAddr)Node->Inode );
}

size_t NativeFS_Write(tVFS_Node *Node, _acess_off_t Offset, size_t Length, const void *Buffer)
{
	FILE	*fp = (FILE *)(tVAddr)Node->Inode;
	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	if( fseek( fp, Offset, SEEK_SET ) != 0 )
	{
		LEAVE('i', 0);
		return 0;
	}
	size_t ret = fwrite( Buffer, 1, Length, fp );
	fflush( fp );
	LEAVE('i', ret);
	return ret;

}

/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 *
 * actions.c
 * - High level actions that call the VFS
 * # Kernel-space compiled
 */
#include <acess.h>
#include <disktool_common.h>
#include <ctype.h>

// === IMPORTS ===
extern int	NativeFS_Install(char **Arguments);

// === PROTOTYPES ===
void	DiskTool_Initialise(void)	__attribute__((constructor(101)));
size_t	DiskTool_int_TranslatePath(char *Buffer, const char *Path);
 int	DiskTool_int_TranslateOpen(const char *File, int Mode);

// === CODE ===
void DiskTool_Initialise(void)
{
	VFS_Init();
	NativeFS_Install(NULL);
	VFS_MkDir("/Native");
	VFS_Mount("/", "/Native", "nativefs", "");
}

int DiskTool_MountImage(const char *Identifier, const char *Path)
{
	// Validate Identifier and make mountpoint string
	char mountpoint[sizeof("/Mount/") + strlen(Identifier) + 1];
	strcpy(mountpoint, "/Mount/");
	strcat(mountpoint, Identifier);
	
	// Translate path	
	size_t tpath_len = DiskTool_int_TranslatePath(NULL, Path);
	if(tpath_len == -1)
		return -1;
	char tpath[tpath_len-1];
	DiskTool_int_TranslatePath(tpath, Path);
	
	// Call mount
	// TODO: Detect filesystem?
	return VFS_Mount(tpath, mountpoint, "fat", "");
}

int DiskTool_Copy(const char *Source, const char *Destination)
{
	return -1;
}

int DiskTool_ListDirectory(const char *Directory)
{
	int fd = DiskTool_int_TranslateOpen(Directory, 2);
	if(fd == -1) {
//		fprintf(stderr, "Can't open '%s'\n", Directory);
		return -1;
	}

	printf("Directory listing of '%s'\n", Directory);	

	char	name[256];
	while( VFS_ReadDir(fd, name) )
	{
		printf("%s\n", name);
	}
	
	VFS_Close(fd);
	
	return 0;
}

// --- Internal helpers ---
size_t DiskTool_int_TranslatePath(char *Buffer, const char *Path)
{
	 int	len;
	const char *colon = strchr(Path, ':');
	if( colon )
	{
		const char *pos;
		for(pos = Path; pos < colon; pos ++)
		{
			if( !isalpha(*pos) )
				goto native_path;
		}
		
		len = strlen("/Mount/");
		len += strlen(Path);
		if( Buffer ) {
			strcpy(Buffer, "/Mount/");
			strncat(Buffer+strlen("/Mount/"), Path, colon - Path);
			strcat(Buffer, colon + 1);
		}
		return len;
	}
	
native_path:
	len = strlen("/Native");
	len += strlen( getenv("PWD") ) + 1;
	len += strlen(Path);
	if( Buffer ) {
		strcpy(Buffer, "/Native");
		strcat(Buffer, getenv("PWD"));
		strcat(Buffer, "/");
		strcat(Buffer, Path);
	}
	return len;
}

int DiskTool_int_TranslateOpen(const char *File, int Mode)
{
	size_t tpath_len = DiskTool_int_TranslatePath(NULL, File);
	if(tpath_len == -1)
		return -1;
	char tpath[tpath_len-1];
	DiskTool_int_TranslatePath(tpath, File);

//	printf("Opening '%s'\n", tpath);	

	switch(Mode)
	{
	case 0:	// Read
		return VFS_Open(tpath, VFS_OPENFLAG_READ);
	case 1:	// Write
		return VFS_Open(tpath, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	case 2:	// Directory
		return VFS_Open(tpath, VFS_OPENFLAG_READ|VFS_OPENFLAG_EXEC);
	default:
		return -1;
	}
}


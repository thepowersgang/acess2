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

// === IMPORTS ===
extern int	NativeFS_Install(char **Arguments);

// === PROTOTYPES ===
void	DiskTool_Initialise(void)	__attribute__((constructor(101)));
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
	int src = DiskTool_int_TranslateOpen(Source, VFS_OPENFLAG_READ);
	if( src == -1 ) {
		Log_Error("DiskTool", "Unable to open %s for reading", Source);
		return -1;
	}
	int dst = DiskTool_int_TranslateOpen(Destination, VFS_OPENFLAG_WRITE|VFS_OPENFLAG_CREATE);
	if( dst == -1 ) {
		Log_Error("DiskTool", "Unable to open %s for writing", Destination);
		VFS_Close(src);
		return -1;
	}

	char	buf[1024];
	size_t	len, total = 0;
	while( (len = VFS_Read(src, sizeof(buf), buf)) == sizeof(buf) )
		VFS_Write(dst, len, buf), total += len;
	VFS_Write(dst, len, buf), total += len;

	Log_Notice("DiskTool", "Copied %i from %s to %s", total, Source, Destination);

	VFS_Close(dst);
	VFS_Close(src);
	
	return 0;
}

int DiskTool_ListDirectory(const char *Directory)
{
	int fd = DiskTool_int_TranslateOpen(Directory, VFS_OPENFLAG_READ|VFS_OPENFLAG_DIRECTORY);
	if(fd == -1) {
//		fprintf(stderr, "Can't open '%s'\n", Directory);
		return -1;
	}

	Log("Directory listing of '%s'", Directory);	

	char	name[256];
	while( VFS_ReadDir(fd, name) )
	{
		Log("- %s", name);
	}
	
	VFS_Close(fd);
	
	return 0;
}

// --- Internal helpers ---
int DiskTool_int_TranslateOpen(const char *File, int Flags)
{
	size_t tpath_len = DiskTool_int_TranslatePath(NULL, File);
	if(tpath_len == -1)
		return -1;
	char tpath[tpath_len-1];
	DiskTool_int_TranslatePath(tpath, File);

	return VFS_Open(tpath, Flags);
}


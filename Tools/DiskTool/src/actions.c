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

// === PROTOTYPES ===
size_t	DiskTool_int_TranslatePath(char *Buffer, const char *Path);
 int	DiskTool_int_TranslateOpen(const char *File, int Mode);

// === CODE ===
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

// --- Internal helpers ---
size_t DiskTool_int_TranslatePath(char *Buffer, const char *Path)
{
	const char *colon = strchr(Path, ':');
	if( colon )
	{
		const char *pos;
		for(pos = Path; pos < colon; pos ++)
		{
			if( !isalpha(*pos) )
				goto native_path;
		}
		
		return -1;
	}
	
native_path:
	if( Buffer )
		strcpy(Buffer, Path);
	return strlen(Path);
}

int DiskTool_int_TranslateOpen(const char *File, int Mode)
{
	// Determine if the source is a mounted image or a file on the source FS
	return -1;
}


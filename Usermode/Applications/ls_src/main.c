/*
 * Acess2 LS command
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	BUF_SIZE	1024

// === CONSTANTS ===
enum eFileTypes {
	FTYPE_NORMAL,
	FTYPE_EXEC,
	FTYPE_DIR,
	FTYPE_SYMLINK
};

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	ShowUsage(char *ProgName);
void	ParseArguments(int argc, char *argv[]);
void	SortFileList();
void	DisplayFile(char *Filename);

// === GLOBALS ===
// --- Flags ---
 int	gbShowAll = 0;
 int	gbShowImplicit = 0;
 int	gbViewExtended = 0;
 int	gbViewHumanReadable = 0;
// --- Parameters ---
char	*gsDirectory = NULL;
// --- Working Set ---
char	**gFileList;
 int	giNumFiles = 0;

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	fd, tmp;
	 int	i;
	char	buf[BUF_SIZE+1];
	t_sysFInfo	info;
	 int	space = 0;

	// Arguments Check
	ParseArguments(argc, argv);

	// Open Directory
	fd = _SysOpen(gsDirectory, OPENFLAG_READ|OPENFLAG_EXEC);
	if(fd == -1) {
		printf("Unable to open '%s' for reading\n", gsDirectory);
		return EXIT_FAILURE;
	}

	// Check that it is a directory
	_SysFInfo(fd, &info, 0);
	if( !(info.flags & FILEFLAG_DIRECTORY) ) {
		fprintf(stderr, "'%s' is not a directory\n", gsDirectory);
		_SysClose(fd);
		return EXIT_FAILURE;
	}

	// Check if we should include the implicit . and ..
	if(gbShowImplicit)
	{
		space = 16;
		gFileList = malloc(space*sizeof(char*));
		gFileList[giNumFiles++] = ".";
		gFileList[giNumFiles++] = "..";
	}

	// Traverse Directory
	while( (tmp = _SysReadDir(fd, buf)) > 0 )
	{
		// Error check
		if(tmp < 0) {
			_SysClose(fd);
			return EXIT_FAILURE;
		}
		
		// Allocate Space
		if(space == giNumFiles)
		{
			space += 16;
			gFileList = realloc(gFileList, space*sizeof(char*));
			if(gFileList == NULL) {
				_SysClose(fd);
				return EXIT_FAILURE;
			}
		}
		gFileList[giNumFiles++] = strdup(buf);
	}

	// Sort File List according to rules passed
	SortFileList();

	// Show the file list
	for( i = 0; i < giNumFiles; i ++ )
	{
		DisplayFile( gFileList[i] );
	}

	_SysClose(fd);
	printf("\n");

	return EXIT_SUCCESS;
}

/**
 * \fn void ShowUsage(char *ProgName)
 */
void ShowUsage(char *ProgName)
{
	fprintf(stderr, "Usage: %s [options] [<directory>]\n", ProgName);
	fprintf(stderr, "\n");
}

/**
 * \fn void ParseArguments(int argc, char *argv[])
 * \brief Parse the command line arguments
 */
void ParseArguments(int argc, char *argv[])
{
	 int	i;
	char	*str;
	for( i = 1; i < argc; i ++ )
	{
		str = argv[i];
		// Flags
		if(str[0] == '-')
		{
			if(str[1] == '-')
			{
				continue;
			}
			str = &str[1];
			for( ; *str; str++ )
			{
				switch(*str)
				{
				// Show All
				case 'a':	gbShowAll = 1;	gbShowImplicit = 1;	continue;
				// Almost All
				case 'A':	gbShowAll = 1;	gbShowImplicit = 0;	continue;
				// Extended List
				case 'l':	gbViewExtended = 1;	continue;
				// Human readable sizes
				case 'h':	gbViewHumanReadable = 1;	continue;
				default:
					fprintf(stderr, "%s: Unknown option '%c'\n", argv[0], *str);
					ShowUsage(argv[0]);
					exit(EXIT_FAILURE);
				}
			}
			continue;
		}
		
		if(gsDirectory == NULL) {
			gsDirectory = argv[i];
		}
	}
	
	// Apply Defaults
	if(!gsDirectory)	gsDirectory = ".";
}

/**
 * \fn int strcmpp(void *p1, void *p2)
 * \brief Compares two strings given pointers to their pointers
 */
int strcmpp(const void *p1, const void *p2)
{
	return strcmp( *(char **)p1, *(char **)p2 );
}

/**
 * \fn void SortFileList()
 * \brief Sorts the filled file list
 */
void SortFileList()
{
	qsort(gFileList, giNumFiles, sizeof(char*), strcmpp);
}

/**
 * \fn void DisplayFile(char *Filename)
 * \brief Prints out the information for a file
 */
void DisplayFile(char *Filename)
{
	t_sysFInfo	info;
	t_sysACL	acl;
	char	*path;
	 int	fd;
	 int	dirLen = strlen(gsDirectory);
	 int	type = FTYPE_NORMAL, perms = 0;
	uint64_t	size = 0;
	 int	owner = 0, group = 0;
	
	// Check if we should skip this file
	if(!gbShowAll && Filename[0] == '.')	return;
	
	// Allocate path
	path = malloc(dirLen+1+strlen(Filename)+1);
	if(!path) {
		fprintf(stderr, "heap exhausted\n");
		exit(EXIT_FAILURE);
	}
	
	// Create path
	strcpy(path, gsDirectory);
	path[dirLen] = '/';
	strcpy(&path[dirLen+1], Filename);
	
	// Get file type
	fd = _SysOpen(path, 0);
	if(fd == -1) {
		fprintf(stderr, "Unable to open '%s'\n", path);
	}
	else {
		// Get Info
		_SysFInfo(fd, &info, 0);
		// Get Type
		if(info.flags & FILEFLAG_DIRECTORY) {
			type = FTYPE_DIR;
		}
		else if(info.flags & FILEFLAG_SYMLINK) {
			type = FTYPE_SYMLINK;
		}
		else {
			type = FTYPE_NORMAL;
		}
		// Get Size
		size = info.size;
		// Get Owner and Group
		owner = info.uid;
		group = info.gid;
		
		// Get Permissions
		// - Owner
		acl.object = info.uid;
		_SysGetACL(fd, &acl);
		if(acl.perms & 1)	perms |= 0400;	// R
		if(acl.perms & 2)	perms |= 0200;	// W
		if(acl.perms & 8)	perms |= 0100;	// X
		// - Group
		acl.object = info.gid | 0x80000000;
		_SysGetACL(fd, &acl);
		if(acl.perms & 1)	perms |= 0040;	// R
		if(acl.perms & 2)	perms |= 0020;	// W
		if(acl.perms & 8)	perms |= 0010;	// X
		// - World
		acl.object = 0xFFFFFFFF;
		_SysGetACL(fd, &acl);
		if(acl.perms & 1)	perms |= 0004;	// R
		if(acl.perms & 2)	perms |= 0002;	// W
		if(acl.perms & 8)	perms |= 0001;	// X
		
		// Close file
		_SysClose(fd);
	}
	free(path);	// We're finished with it
	
	// Extended view?
	if(gbViewExtended)
	{
		char	permStr[11] = " ---------";
		if(type == FTYPE_DIR)	permStr[0] = 'd';
		if(perms & 0400)	permStr[1] = 'r';
		if(perms & 0200)	permStr[2] = 'w';
		if(perms & 0100)	permStr[3] = 'x';
		if(perms & 0040)	permStr[4] = 'r';
		if(perms & 0020)	permStr[5] = 'w';
		if(perms & 0010)	permStr[6] = 'x';
		if(perms & 0004)	permStr[7] = 'r';
		if(perms & 0002)	permStr[8] = 'w';
		if(perms & 0001)	permStr[9] = 'x';
		printf("%s %4i %4i ", permStr, owner, group);
		if(gbViewHumanReadable && type != FTYPE_DIR) {
			if(size < 2048) {	// < 2 KiB
				printf("%4lli B   ", size);
			}
			else if(size < 2048*1024) {	// < 2 MiB
				printf("%4lli KiB ", size>>10);
			}
			else if(size < (uint64_t)2048*1024*1024) {	// < 2 GiB
				printf("%4lli MiB ", size>>20);
			}
			else if(size < (uint64_t)2048*1024*1024*1024) {	// < 2 TiB
				printf("%4lli GiB ", size>>30);
			}
			else {	// Greater than 2 TiB (if your files are larger than this, you are Doing It Wrong [TM])
				printf("%4lli TiB ", size>>40);
			}
		} else {
			printf("%8lli ", size);
		}
	}
	
	switch(type)
	{
	case FTYPE_DIR:		printf("\x1B[32m");	break;	// Green
	case FTYPE_SYMLINK:	printf("\x1B[34m");	break;	// Blue
	case FTYPE_NORMAL:	break;
	}
	printf("%s%s\n", Filename, (type==FTYPE_DIR?"/":""));
	printf("\x1B[00m");	// Reset
}

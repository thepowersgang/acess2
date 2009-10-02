/*
 * Acess2 LS command
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	BUF_SIZE	1024

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	ShowUsage(char *ProgName);
void	ParseArguments(int argc, char *argv[]);
void	SortFileList();

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
	char	buf[BUF_SIZE+1];
	t_sysFInfo	info;
	 int	space = 0;

	// Arguments Check
	ParseArguments(argc, argv);

	// Open Directory
	fd = open(gsDirectory, OPENFLAG_READ|OPENFLAG_EXEC);
	if(fd == -1) {
		printf("Unable to open '%s' for reading\n", gsDirectory);
		return EXIT_FAILURE;
	}

	// Check that it is a directory
	finfo(fd, &info, 0);
	if( !(info.flags & FILEFLAG_DIRECTORY) ) {
		fprintf(stderr, "'%s' is a directory\n", gsDirectory);
		close(fd);
		return EXIT_FAILURE;
	}

	// Traverse Directory
	while( (tmp = readdir(fd, buf)) )
	{
		// Error check
		if(tmp < 0) {
			close(fd);
			return EXIT_FAILURE;
		}
		
		// Allocate Space
		if(space == giNumFiles)
		{
			space += 16;
			gFileList = realloc(gFileList, space*sizeof(char*));
			if(gFileList == NULL) {
				close(fd);
				return EXIT_FAILURE;
			}
		}
		gFileList[giNumFiles++] = strdup(buf);
	}

	// Sort File List according to rules passed
	SortFileList();

	close(fd);
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
					fprintf(stderr, "%s: Unknown option '%c'\n", *str);
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
	
	printf("gsDirectory = '%s'\n", gsDirectory);
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

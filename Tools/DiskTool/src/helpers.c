/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 *
 * helpers.c
 */
#include <stdlib.h>
#include <acess_logging.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

// === GLOBALS ===
char	gsWorkingDirectory[1024];


// === CODE ===
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

	if( !gsWorkingDirectory[0] ) {	
		getcwd(gsWorkingDirectory, 1024);
	}

	len = strlen("/Native");
	len += strlen( gsWorkingDirectory ) + 1;
	len += strlen(Path);
	if( Buffer ) {
		strcpy(Buffer, "/Native");
		strcat(Buffer, gsWorkingDirectory);
		strcat(Buffer, "/");
		strcat(Buffer, Path);
	}
	return len;
}

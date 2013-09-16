/*
 * Acess2 OS Userland - insmod
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Core
 */
#include <stdio.h>
#include <errno.h>
#include <acess/sys.h>

#define MODDIR	"/Acess/Modules/"

// === CODE ===
void Usage(const char *progname)
{
	fprintf(stderr, "Usage: %s <module>\n", progname);
}


int main(int argc, char *argv[])
{
	if( argc != 2 ) {
		Usage(argv[0]);
		return 1;
	}

	const char *modname = argv[1];
	
	char path[strlen(MODDIR)+strlen(modname)+1];
	strcpy(path, MODDIR);
	strcat(path, modname);
	
	int rv = _SysLoadModule(path);
	if( rv )
	{
		fprintf(stderr, "_SysLoadModule(\"%s\"): %s\n", path, strerror(rv));
	}
	else {
		printf("Loaded module '%s'\n", path);
	}

	return 0;
}

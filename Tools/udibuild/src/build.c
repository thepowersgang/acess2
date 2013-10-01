/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * build.c
 * - Compilation functions
 */
#include <stdlib.h>
#include <stdio.h>
#include "include/build.h"
#include "include/common.h"

#ifndef __GNUC__
# define __attribute__(...)
#endif

// === PROTOTYPES ===
char	*get_objfile(tIniFile *opts, const char *srcfile);

// === CODE ===
int Build_CompileFile(tIniFile *opts, const char *abi, tUdiprops *udiprops, tUdiprops_Srcfile *srcfile)
{
	// Select compiler from opts [abi]
	const char *cc_prog = IniFile_Get(opts, abi, "CC", NULL);
	if( !cc_prog ) {
		fprintf(stderr, "No 'CC' defined for ABI %s\n", abi);
		return 1;
	}
	
	// Build up compiler's command line
	// - Include CFLAGS from .ini file
	// - defines from udiprops
	// - Object file is srcfile with .o appended
	//  > Place in 'obj/' dir?
	char *objfile = get_objfile(opts, srcfile->Filename);
	char *cmd = mkstr("%s -DUDI_ABI_is_%s %s %s -c %s -o %s",
		cc_prog,
		abi,
		IniFile_Get(opts, abi, "CFLAGS", ""),
		srcfile->CompileOpts ? srcfile->CompileOpts : "",
		srcfile->Filename, objfile);
	printf("--- Compiling: %s\n", srcfile->Filename);
	 int rv = system(cmd);
	free(cmd);
	free(objfile);
	
	return rv;
}

int Build_LinkObjects(tIniFile *opts, const char *abi, tUdiprops *udiprops)
{
	return 0;
}

char *get_objfile(tIniFile *opts, const char *srcfile)
{
	return mkstr("%s.o", srcfile);
}

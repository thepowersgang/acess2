/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * build.c
 * - Compilation functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "include/build.h"

#ifndef __GNUC__
# define __attribute__(...)
#endif

// === PROTOTYPES ===
char	*get_objfile(tIniFile *opts, const char *srcfile);
char	*mkstr(const char *fmt, ...) __attribute__((format(printf,1,2)));

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

char *mkstr(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	va_start(args, fmt);
	char *ret = malloc(len+1);
	vsnprintf(ret, len+1, fmt, args);
	va_end(args);
	return ret;
}


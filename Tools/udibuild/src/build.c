/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * build.c
 * - Compilation functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>	// mkdir
#include <unistd.h>	// unlink
#include "include/build.h"
#include "include/common.h"

// === PROTOTYPES ===
char	*get_objfile(tIniFile *opts, const char *abi, const char *srcfile);
char	*get_udipropsfile(tIniFile *opts, const char *abi);

// === CODE ===
int Build_CompileFile(tIniFile *opts, const char *abi, tUdiprops *udiprops, tUdiprops_Srcfile *srcfile)
{
	// Select compiler from opts [abi]
	const char *cc_prog = IniFile_Get(opts, abi, "CC", NULL);
	if( !cc_prog ) {
		fprintf(stderr, "No 'CC' defined for ABI %s\n", abi);
		return -1;
	}
	
	// Build up compiler's command line
	// - Include CFLAGS from .ini file
	// - defines from udiprops
	// - Object file is srcfile with .o appended
	//  > Place in 'obj/' dir?
	char *objfile = get_objfile(opts, abi, srcfile->Filename);
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

int Build_CreateUdiprops(tIniFile *opts, const char *abi, tUdiprops *udiprops)
{
	const char *cc_prog = IniFile_Get(opts, abi, "CC", NULL);
	
	char	*filename = get_udipropsfile(opts, abi);
	FILE *fp = fopen(filename, "w");
	fprintf(fp, "char udiprops[] __attribute__((section(\".udiprops\"))) = \n");
	for( int i = 0; i < udiprops->nLines; i ++ ) {
		// TODO: Escape " in string
		fprintf(fp, " \"%s\"\n", udiprops->Lines[i]);
	}
	fprintf(fp, " ;\n");
	fclose(fp);

	char *cmd = mkstr("%s %s -c %s -o %s.o",
		cc_prog, IniFile_Get(opts, abi, "CFLAGS", ""),
		filename, filename);

	int rv = system(cmd);
	free(cmd);
	unlink(filename);	
	free(filename);

	return rv;
}

int Build_LinkObjects(tIniFile *opts, const char *abi, tUdiprops *udiprops)
{
	const char *linker = IniFile_Get(opts, abi, "LD", NULL);
	if( !linker ) {
		fprintf(stderr, "No 'LD' defined for ABI %s\n", abi);
		return -1;
	}

	char	*objfiles[udiprops->nSourceFiles];
	size_t	objfiles_len = 0;
	for( int i = 0; i < udiprops->nSourceFiles; i ++ ) {
		objfiles[i] = get_objfile(opts, abi, udiprops->SourceFiles[i]->Filename);
		objfiles_len += strlen(objfiles[i])+1;
	}
	
	// Create command string
	char *objfiles_str = malloc(objfiles_len);
	objfiles_len = 0;
	for( int i = 0; i < udiprops->nSourceFiles; i ++ ) {
		strcpy(objfiles_str + objfiles_len, objfiles[i]);
		objfiles_len += strlen(objfiles[i])+1;
		objfiles_str[objfiles_len-1] = ' ';
		free( objfiles[i] );
	}
	objfiles_str[objfiles_len-1] = '\0';

	mkdir("bin", 0755);
	char *abidir = mkstr("bin/%s", abi);
	mkdir(abidir, 0755);
	free(abidir);
	
	char *udiprops_c = get_udipropsfile(opts, abi);
	char *cmd = mkstr("%s -r %s -o bin/%s/%s -s %s",// %s.o",
		linker, IniFile_Get(opts, abi, "LDFLAGS", ""),
		abi, udiprops->ModuleName, objfiles_str, udiprops_c
		);
	printf("--- Linking: bin/%s/%s\n", abi, udiprops->ModuleName);
	printf("%s\n", cmd);
	int rv = system(cmd);
	free(cmd);
	free(udiprops_c);
	free(objfiles_str);

	return rv;
}

char *get_objfile(tIniFile *opts, const char *abi, const char *srcfile)
{
	return mkstr("%s.o", srcfile);
}

char *get_udipropsfile(tIniFile *opts, const char *abi)
{
	return mkstr(".udiprops.c");
}


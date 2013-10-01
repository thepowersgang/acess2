/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * udiprops.c
 * - udiprops.txt parsing (for udibuild only)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include "include/udiprops.h"

// === CODE ===
static int _get_token_sym(const char *str, const char **outstr, ...)
{
	va_list args;
	va_start(args, outstr);
	const char *sym;
	for( int idx = 0; (sym = va_arg(args, const char *)); idx ++ )
	{
		size_t len = strlen(sym);
		if( memcmp(str, sym, len) != 0 )
			continue ;
		if( str[len] && !isspace(str[len]) )
			continue ;
		
		// Found it!
		*outstr = str + len;
		while( isspace(**outstr) )
			(*outstr) ++;
		return idx;
	}
	va_end(args);

	const char *end = str;
	while( !isspace(*end) )
		end ++;
//	fprintf(stderr, "udiprops: Unknown token '%.*s'\n", end-str, str);

	*outstr = NULL;
	return -1;
}

static void rtrim(char *str)
{
	char *pos = str;
	while( *pos )
		pos ++;
	while( pos != str && isspace(pos[-1]) )
		*--pos = '\0';
}

tUdiprops *Udiprops_LoadBuild(const char *Filename)
{
	FILE *fp = fopen(Filename, "r");
	if( !fp ) {
		perror("Udiprops_LoadBuild");
		return NULL;
	}

	char	*current_compile_opts = NULL;	
	 int	n_srcfiles = 0;
	tUdiprops *ret = calloc( 1, sizeof(tUdiprops) );

	char buf[512];
	while( fgets(buf, sizeof(buf)-1, fp) )
	{
		char *str = buf;
		{
			char *hash = strchr(str, '#');
			if( hash )	*hash = '\0';
		}
		rtrim(str);
		if( !str[0] )	continue ;
		
		int sym = _get_token_sym(str, (const char**)&str,
			"source_files", "compile_options", "source_requires", NULL);
		switch(sym)
		{
		case 0:	// source_files
			for(char *file = strtok(str, " \t"); file; file = strtok(NULL, " \t") )
			{
				tUdiprops_Srcfile *srcfile = malloc(sizeof(tUdiprops_Srcfile)+strlen(file)+1);
				srcfile->CompileOpts = current_compile_opts;
				srcfile->Filename = (void*)(srcfile+1);
				strcpy((char*)srcfile->Filename, file);
				
				n_srcfiles ++;
				ret->SourceFiles = realloc(ret->SourceFiles, (n_srcfiles+1)*sizeof(void*));
				ret->SourceFiles[n_srcfiles-1] = srcfile;
				ret->SourceFiles[n_srcfiles] = NULL;
			}
			break;
		case 1:	// compile_options
			current_compile_opts = malloc(strlen(str)+1);
			strcpy(current_compile_opts, str);
			break;
		case 2:	// source_requires
			// TODO: Use source_requires
			break;
		}
	}
	
	// "Intentional" memory leak
	// - current_compile_opts not freed, and shared between srcfiles
	// - If two compile_options statements appear in a row, one is definitely leaked

	fclose(fp);
	return ret;
}


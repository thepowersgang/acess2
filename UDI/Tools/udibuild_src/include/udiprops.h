/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * udiprops.h
 * - udiprops.txt parsing for udibuild
 */
#ifndef _UDIPROPS_H_
#define _UDIPROPS_H_

typedef struct sUdiprops	tUdiprops;

typedef struct sUdiprops_Srcfile tUdiprops_Srcfile;

struct sUdiprops_Srcfile
{
	const char	*Filename;
	const char	*CompileOpts;
};

struct sUdiprops
{
	const char	*ModuleName;
	 int	nSourceFiles;
	tUdiprops_Srcfile	**SourceFiles;
	
	 int	nLines;
	char	**Lines;
};

extern tUdiprops	*Udiprops_LoadBuild(const char *Filename);

#endif


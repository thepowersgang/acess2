/*
 * Acess Text Editor (ATE)
 * - By John Hodge (thePowersGang)
 *
 * include/file.h
 * - In-memory file structures
 */
#ifndef _ATE__FILE_H_
#define _ATE__FILE_H_
#include <stdio.h>

typedef struct sFileLine
{
	 int	Num;

	// State data for hilighting
	 int	HilightState;
	char	*Rendered;

	 int	Space;
	 int	Length;
	char	Data[];
} tFileLine;

typedef struct sFile
{
	FILE	*Handle;
	 int	nLines;
	tFileLine	**Lines;	// TODO: Handle very large files?
	 int	NameOfs;
	const char	Path[];
} tFile;

#endif


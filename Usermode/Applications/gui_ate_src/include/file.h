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

typedef struct sFileLine	tFileLine;

struct sFileLine
{
	tFileLine	*Next;
	tFileLine	*Prev;
	 int	Num;

	// State data for hilighting
	 int	HilightState;
	char	*Rendered;

	 int	Space;
	 int	Length;
	char	*Data;
};

typedef struct sFile
{
	 int	nLines;
	tFileLine	*FirstLine;
	tFileLine	*CurrentLine;
	 int	CursorOfs;
	
	 int	NameOfs;
	const char	Path[];
} tFile;

enum eFile_DeleteType
{
	DELTYPE_BACK,
	DELTYPE_BACK_WORD,
	DELTYPE_FORWARD,
	DELTYPE_FORWARD_WORD,
	DELTYPE_TO_EOL,
	DELTYPE_TO_SOL,
	DELTYPE_LINE
};

#endif


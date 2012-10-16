/*
 * Acess Text Editor (ATE)
 * - By John Hodge (thePowersGang)
 *
 * edit.c
 * - File Loading / Manipulation
 */
#include <stdio.h>
#include "include/file.h"
#include "include/syntax.h"

// === CODE ===
tFile *File_New(void)
{
	tFile *ret = malloc(sizeof(tFile) + 1);
	ret->Handle = NULL;
	ret->nLines = 0;
	ret->Lines = NULL;
	ret->NameOfs = 0;
	ret->Path[0] = 0;
	return ret;
}

tFile *File_Load(const char *Path)
{
	return NULL;
}

int File_Save(tFile *File)
{
	
	//file->bIsDirty = 0;
	return -1;
}

int File_Close(tFile *File, int bDiscard)
{
	//if( file->bIsDirty && !bDiscard )
	//	return 1;
	if( file->Handle )
		fclose(File->Handle);
	
	return 0;
}


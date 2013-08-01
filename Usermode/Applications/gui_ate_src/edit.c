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
#include <assert.h>

static const int MAXLINE = 1024;
static const int LINEALLOCSIZE = 64;

// === CODE ===
tFile *File_New(void)
{
	tFile *ret = calloc(1, sizeof(tFile) + 1);
	ret->nLines = 0;
	ret->FirstLine = NULL;
	ret->NameOfs = 0;
	ret->Path[0] = 0;
	return ret;
}

tFile *File_Load(const char *Path)
{
	FILE	*fp = fopen(Path, "r");
	if( !fp ) {
		return NULL;
	}

	// Create file structure
	tFile	*ret = calloc(1, sizeof(tFile) + strlen(Path) + 1);
	assert(ret);

	const char *lastslash = strrchr(Path, '/');
	ret->NameOfs = (lastslash ? lastslash - Path + 1 : 0);
	strcpy(ret->Path, Path);

	// Read in lines
	int nLines = 0;
	char	tmpbuf[MAXLINE];
	tFileLine	*lastline = NULL;
	while( fgets(tmpbuf, MAXLINE, fp) )
	{
		tFileLine	*new = malloc(sizeof(tFileLine));
		assert(new);
		new->Prev = lastline;
		new->Next = NULL;
		if(lastline)	lastline->Next = new;
		new->Length = strlen(tmpbuf);
		new->Space = (new->Length + LINEALLOCSIZE-1) & ~(LINEALLOCSIZE-1);
		
		new->Data = malloc(new->Space);
		assert(new->Data);
		memcpy(new->Data, tmpbuf, new->Length);
		nLines ++;
	}
	ret->nLines = nLines;

	return ret;
}

int File_Save(tFile *File)
{
	
	//file->bIsDirty = 0;
	return -1;
}

int File_Close(tFile *File, int bDiscard)
{
	if( file->bIsDirty && !bDiscard )
		return 1;
	if( file->Handle )
		fclose(File->Handle);

	while( file->FirstLine )
	{
		tFileLine	*next = file->FirstLine->Next;
		// TODO: Highlighting free
		free(file->FirstLine->Data);
		free(file->FirstLine);
		file->FirstLine = next;
	}
	
	free(file);
	
	return 0;
}

int File_InsertBytes(tFile *File, void *Buffer, size_t Bytes)
{
	
}

int File_Delete(tFile *File, enum eFile_DeleteType Type)
{
}

/**
 * Amt = INT_MAX : End of file
 * Amt = INT_MIN : Start of file
 */
int File_CursorDown(tFile *File, int Amount)
{
}

/**
 * |Amt| = 1 : Single character
 * |Amt| = 2 : Word
 * |Amt| = 3 : Start/End of line
 */
int File_CursorRight(tFile *File, int Amount)
{
}

void *File_GetAbsLine(tFile *File, unsigned int LineNum)
{
	tFileLine	*line = File->FirstLine;
	while( LineNum-- && line )
		line = line->Next;
	return line;
}

void *File_GetRelLine(tFile *File, unsigned int LinesBeforeCurrent)
{
	tFileLine	*line = File->CurrentLine;
	while(LinesBeforeCurrent -- && line->Prev)
		line = line->Prev;
	return line;
}

void *File_GetRenderedData(tFile *File, void *Handle, size_t const char **StringPtr)
{
	if( !Handle )
		return NULL;
	tFileLine	*line = Handle;
	if( StringPtr )
		*StringPtr = (line->Rendered ? line->Rendered : line->Data);
	return line->Next;
}


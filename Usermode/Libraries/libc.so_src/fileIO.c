/*
AcessOS Basic C Library
*/
#include "config.h"
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "stdio_int.h"

#define DEBUG_BUILD	0

// === CONSTANTS ===

// === PROTOTYPES ===
struct sFILE	*get_file_struct();

// === GLOBALS ===
struct sFILE	_iob[STDIO_MAX_STREAMS];	// IO Buffer

// === CODE ===
/**
 * \fn FILE *freopen(FILE *fp, char *file, char *mode)
 */
FILE *freopen(FILE *fp, char *file, char *mode)
{
	 int	openFlags = 0;
	 int	i;
	
	// Sanity Check Arguments
	if(!fp || !file || !mode)	return NULL;
	
	if(fp->Flags) {
		fflush(fp);
		close(fp->FD);
	}
	
	// Get main mode
	switch(mode[0])
	{
	case 'r':	fp->Flags = FILE_FLAG_MODE_READ;	break;
	case 'w':	fp->Flags = FILE_FLAG_MODE_WRITE;	break;
	case 'a':	fp->Flags = FILE_FLAG_MODE_APPEND;	break;
	case 'x':	fp->Flags = FILE_FLAG_MODE_EXEC;	break;
	default:
		return NULL;
	}
	// Get Modifiers
	for(i=1;mode[i];i++)
	{
		switch(mode[i])
		{
		case '+':	fp->Flags |= FILE_FLAG_M_EXT;
		}
	}
	
	// Get Open Flags
	switch(mode[0])
	{
	// Read
	case 'r':	openFlags = OPENFLAG_READ;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_WRITE;
		break;
	// Write
	case 'w':	openFlags = OPENFLAG_WRITE;
		if(fp->Flags & FILE_FLAG_M_EXT)
			openFlags |= OPENFLAG_READ;
		break;
	// Execute
	case 'x':	openFlags = OPENFLAG_EXEC;
		break;
	}
	
	//Open File
	fp->FD = reopen(fp->FD, file, openFlags);
	if(fp->FD == -1) {
		fp->Flags = 0;
		return NULL;
	}
	
	if(mode[0] == 'a') {
		seek(fp->FD, 0, SEEK_END);	//SEEK_END
	}
	
	return fp;
}
/**
 \fn FILE *fopen(char *file, char *mode)
 \brief Opens a file and returns the pointer
 \param file	String - Filename to open
 \param mode	Mode to open in
*/
FILE *fopen(char *file, char *mode)
{
	FILE	*retFile;
	
	// Sanity Check Arguments
	if(!file || !mode)	return NULL;
	
	// Create Return Structure
	retFile = get_file_struct();
	
	return freopen(retFile, file, mode);
}

void fclose(FILE *fp)
{
	close(fp->FD);
	free(fp);
}

void fflush(FILE *fp)
{
	///\todo Implement
}

/**
 * \fn int fprintf(FILE *fp, const char *format, ...)
 * \brief Print a formatted string to a stream
 */
int fprintf(FILE *fp, const char *format, ...)
{
	 int	size;
	char	*buf;
	va_list	args;
	
	if(!fp || !format)	return -1;
	
	// Get Size
	va_start(args, format);
	size = ssprintfv((char*)format, args);
	va_end(args);
	
	// Allocate buffer
	buf = (char*)malloc(size+1);
	buf[size] = '\0';
	
	// Print
	va_start(args, format);
	sprintfv(buf, (char*)format, args);
	va_end(args);
	
	// Write to stream
	write(fp->FD, size+1, buf);
	
	// Free buffer
	free(buf);
	
	// Return written byte count
	return size;
}

// --- INTERNAL ---
/**
 * \fn FILE *get_file_struct()
 * \brief Returns a file descriptor structure
 */
FILE *get_file_struct()
{
	 int	i;
	for(i=0;i<STDIO_MAX_STREAMS;i++)
	{
		if(_iob[i].Flags == 0)	return &_iob[i];
	}
	return NULL;
}

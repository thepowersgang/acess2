/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 * 
 * Window Manager and Widget Control
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <uri.h>

// === IMPORTS ===
extern tImage *Image_SIF_Parse(void *Buffer, size_t Size);

// === CODE ===
/**
 * \brief Open an image from a URI
 */
tImage *Image_Load(const char *URI)
{
	#if 0
	tURI	*uri;
	 int	filesize;
	void	*buf;
	tImage	*img;
	void	*fp;
	
	uri = URI_Parse(URI);
	
	fp = URI_Open(URI_MODE_READ, uri);
	if(!fp)	return NULL;
	free(uri);
	
	filesize = URI_GetSize(fp);
	buf = malloc( filesize );
	if(!buf) {
		URI_Close(fp);
		return NULL;
	}
	
	URI_Read(fp, filesize, buf);
	URI_Close(fp);
	
	img = Image_SIF_Parse(buf, filesize);
	free(buf);
	
	return img;
	#else
	return NULL;
	#endif
}

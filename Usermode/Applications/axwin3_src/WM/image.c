/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 * 
 * image.c
 * - Image loading
 */
#include <common.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uri.h>
#include "include/image.h"

// === IMPORTS ===
extern tImage *Image_SIF_Parse(void *Buffer, size_t Size);

// === PROTOTYPES ===
 int	UnBase64(uint8_t *Dest, char *Src, int BufSize);

// === CODE ===
/**
 * \brief Open an image from a URI
 */
tImage *Image_Load(const char *URI)
{
	tURI	*uri;
	 int	filesize;
	char	*buf;
	tImage	*img;
	
	uri = URI_Parse(URI);
	if( !uri ) {
		_SysDebug("Image_Load: Unable parse as URI '%s'", URI);
		return NULL;
	}
	
	if( strcmp(uri->Proto, "file") == 0 )
	{
		FILE	*fp;
		 int	tmp;
		fp = fopen(uri->Path, "rb");
		if(!fp) {
			_SysDebug("Image_Load: Unable to open '%s'", uri->Path);
			free(uri);
			return NULL;
		}
		
		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = malloc( filesize );
		if(!buf) {
			_SysDebug("Image_Load: malloc() failed!");
			fclose(fp);
			free(uri);
			return NULL;
		}
		
		tmp = fread(buf, 1, filesize, fp);
		if(tmp != filesize) {
			_SysDebug("Image_Load: fread() failed - %i != filesize (%i)", tmp, filesize);
		}
		fclose(fp);
	}
	else if( strcmp(uri->Proto, "base64") == 0 )
	{
		// 4 bytes of base64 = 3 bytes of binary (base 256)
		filesize = strlen( uri->Path ) * 3 / 4;
		buf = malloc(filesize);
		if(!buf) {
			_SysDebug("Image_Load: malloc() failed!");
			free(uri);
			return NULL;
		}
		
		filesize = UnBase64((uint8_t*)buf, uri->Path, filesize);
	}
	else
	{
		_SysDebug("Image_Load: Unknow protocol '%s'", uri->Proto);
		free(uri);
		return NULL;
	}
	
	img = Image_SIF_Parse(buf, filesize);
	free(buf);
	free(uri);
	if( !img ) {
		_SysDebug("Image_Load: Unable to parse SIF from '%s'\n", URI);
		return NULL;
	}
	
	return img;
}

/**
 * \brief Decode a Base64 value
 */
int UnBase64(uint8_t *Dest, char *Src, int BufSize)
{
	uint32_t	val = 0;
	 int	i, j;
	char	*start_src = Src;
	
	for( i = 0; i+2 < BufSize; i += 3 )
	{
		val = 0;
		for( j = 0; j < 4; j++, Src ++ ) {
			if('A' <= *Src && *Src <= 'Z')
				val |= (*Src - 'A') << ((3-j)*6);
			else if('a' <= *Src && *Src <= 'z')
				val |= (*Src - 'a' + 26) << ((3-j)*6);
			else if('0' <= *Src && *Src <= '9')
				val |= (*Src - '0' + 52) << ((3-j)*6);
			else if(*Src == '+')
				val |= 62 << ((3-j)*6);
			else if(*Src == '/')
				val |= 63 << ((3-j)*6);
			else if(!*Src)
				break;
			else if(*Src != '=')
				j --;	// Ignore invalid characters
		}
		Dest[i  ] = (val >> 16) & 0xFF;
		Dest[i+1] = (val >> 8) & 0xFF;
		Dest[i+2] = val & 0xFF;
		if(j != 4)	break;
	}
	
	// Finish things off
	if(i   < BufSize)
		Dest[i] = (val >> 16) & 0xFF;
	if(i+1 < BufSize)
		Dest[i+1] = (val >> 8) & 0xFF;
	
	return Src - start_src;
}

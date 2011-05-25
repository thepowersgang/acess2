/*
 * Acess2 Image Viewer
 * By John Hodge (thePowersGang)
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <acess/sys.h>
#include <uri.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	*SIF_Load(tURIFile *FP, int *Width, int *Height, uint32_t Magic);

// === CONSTANTS ===
const struct {
	const char	*Name;
	uint32_t	Mask;
	uint32_t	Ident;
	void	*(*Load)(tURIFile *FP, int *Width, int *Height, uint32_t Magic);
}	caFileTypes[] = {
	{"SIF", 0xFFFF, 0x51F0, SIF_Load}
	};
#define NUM_FILE_TYPES	(sizeof(caFileTypes)/sizeof(caFileTypes[0]))

// === CODE ===
int main(int argc, char *argv[])
{
	char	*imageURI;
	tURI	*uri;
	tURIFile	*fp;
	uint32_t	idDWord = 0;
	 int	i;
	uint32_t	*buffer;
	 int	w, h;
	 int	termW, termH;
	 int	tmp;
	
	// --- Parse Arguments
	if( argc < 2 ) {
		fprintf(stderr, "Usage: %s <image>\n", argv[0]);
		return 0;
	}
	imageURI = argv[1];
	
	// --- Open File
	uri = URI_Parse(imageURI);
	printf("uri = {Proto:'%s',Host:'%s',Port:%i('%s'),Path:'%s'}\n",
		uri->Proto, uri->Host, uri->PortNum, uri->PortStr, uri->Path);
	fp = URI_Open(URI_MODE_READ, uri);
	printf("fp = %p\n", fp);
	if(!fp) {
		fprintf(stderr, "ERROR: Unable to open '%s'\nFile unable to open\n", imageURI);
		return -1;
	}
	free(uri);
	
	
	// --- Determine file type
	i = URI_Read(fp, 4, &idDWord);
	printf("i = %i\n", i);
	printf("idDWord = 0x%08lx\n", idDWord);
	for( i = 0; i < NUM_FILE_TYPES; i++ )
	{
		if( (idDWord & caFileTypes[i].Mask) == caFileTypes[i].Ident )
			break;
	}
	if( i == NUM_FILE_TYPES ) {
		fprintf(stderr, "ERROR: Unable to open '%s'\nUnknown file type\n", imageURI);
		return -2;
	}
	
	// --- Load
	buffer = caFileTypes[i].Load(fp, &w, &h, idDWord);
	if( !buffer ) {
		fprintf(stderr, "ERROR: Unable to open '%s'\nParsing failed\n", imageURI);
		return -3;
	}
	
	printf("w=%i,h=%i\n", w, h);
	
	// --- Display
	termW = ioctl(1, 5, NULL);	termW *= 8;	ioctl(1, 5, &termW);
	termH = ioctl(1, 6, NULL);	termH *= 16;	ioctl(1, 6, &termH);
	//printf("termW = %i, termH = %i\n", termW, termH);
	tmp = 1;	ioctl(1, 4, &tmp);
	
	seek(1, 0, SEEK_SET);
	for( i = 0; i < (h < termH ? h : termH); i++ )
	{
		write(1, (w < termW ? w : termW) * 4, buffer + i*w);
		if(w < termW)
			seek(1, (termW-w)*4, SEEK_CUR);
	}
	
	for( ;; );
	
	return 0;
}

// === Simple Image Format Loader ===
void *SIF_Load(tURIFile *FP, int *Width, int *Height, uint32_t Magic)
{
	uint16_t	flags = Magic >> 16;
	uint16_t	w, h;
	 int	comp = flags & 3;
	 int	ofs, i;
	uint32_t	*ret;
	
	// First 4 bytes were read by main()
	URI_Read(FP, 2, &w);
	URI_Read(FP, 2, &h);
	
	ret = malloc(w * h * 4);
	
	switch(comp)
	{
	// Uncompressed 32-bpp data
	case 0:
		URI_Read( FP, w * h * 4, ret );
		*Width = w;
		*Height = h;
		return ret;
	
	// 1.7.32 RLE
	// (1 Flag, 7-bit size, 32-bit value)
	case 1:
		ofs = 0;
		while( ofs < w*h )
		{
			uint8_t	len;
			uint32_t	val;
			URI_Read(FP, 1, &len);
			if(len & 0x80) {
				len &= 0x7F;
				URI_Read(FP, len, ret+ofs);
				ofs += len;
			}
			else {
				URI_Read(FP, 4, &val);
				while(len--)	ret[ofs++] = val;
			}
		}
		return ret;
	
	case 3:
		// Alpha
		ofs = 0;
		while( ofs < w*h )
		{
			uint8_t	len, val;
			URI_Read(FP, 1, &len);
			if(len & 0x80) {
				len &= 0x7F;
				while(len--) {
					URI_Read(FP, 1, &val);
					ret[ofs++] = (uint32_t)val << 24;
				}
			}
			else {
				URI_Read(FP, 1, &val);
				while(len--)
					ret[ofs++] = (uint32_t)val << 24;
			}
		}
		// Red, Green, Blue
		for( i = 0; i < 4; i++ )
		{
			ofs = 0;
			while( ofs < w*h )
			{
				uint8_t	len, val;
				URI_Read(FP, 1, &len);
				if(len & 0x80) {
					len &= 0x7F;
					while(len--) {
						URI_Read(FP, 1, &val);
						ret[ofs++] |= (uint32_t)val << (24-i*8);
					}
				}
				else {
					URI_Read(FP, 1, &val);
					while(len--)
						ret[ofs++] |= (uint32_t)val << (24-i*8);
				}
			}
		}
		return ret;
	
	default:
		fprintf(stderr, "Warning: Unknown compression scheme %i for SIF\n", comp);
		return NULL;
	}
	
}

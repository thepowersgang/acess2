/*
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <image.h>
//#include <image_sif.h>

// === CODE ===
int SoMain(void)
{
	return 0;
}

/**
 */
tImage *Image_SIF_Parse(void *Buffer, size_t Size)
{
	uint16_t	magic;
	uint16_t	flags;
	uint16_t	w, h;
	 int	ofs, i;
	tImage_SIF	*ret;
	 int	bRevOrder;
	 int	fileOfs = 0;
	 int	comp, fmt;
	 int	sampleSize = 4;
	
	// Get magic word and determine byte ordering
	magic = *(uint16_t*)Buffer+fileOfs;	fileOfs += 2;
	if(magic == 0x51F0)
		bRevOrder = 0;
	else if(magic == 0xF051)
		bRevOrder = 1;
	else {
		return NULL;
	}
	
	// Read flags
	flags = *(uint16_t*)Buffer+fileOfs;	fileOfs += 2;
	comp = flags & 7;
	fmt = (flags >> 3) & 7;
	
	// Read dimensions
	w = *(uint16_t*)Buffer+fileOfs;	fileOfs += 2;
	h = *(uint16_t*)Buffer+fileOfs;	fileOfs += 2;
	
	
	// Allocate space
	ret = calloc(1, sizeof(tImage) + w * h * sampleSize);
	ret->Width = w;
	ret->Height = h;
	
	// Get image format
	switch(fmt)
	{
	case 0:	// ARGB
		ret->Format = IMGFMT_ARGB;
		sampleSize = 4;
		break;
	case 1:	// RGB
		ret->Format = IMGFMT_RGB;
		sampleSize = 3;
		break;
	default:
		free(ret);
		return NULL;
	}
	
	switch(comp)
	{
	// Uncompressed 32-bpp data
	case 0:
		if( fileOfs + w*h*sampleSize > Size ) {
			memcpy(ret->Data, Buffer+fileOfs, Size-fileOfs);
		}
		else {
			memcpy(ret->Data, Buffer+fileOfs, w*h*sampleSize);
		}
		return ret;
	
	// 1.7.n*8 RLE
	// (1 Flag, 7-bit size, 32-bit value)
	case 1:
		ofs = 0;
		while( ofs < w*h )
		{
			uint8_t	len;
			if( fileOfs + 1 > Size )	return ret;
			len = *(uint8_t*)Buffer+fileOfs;	fileOfs += 1;
			if(len & 0x80) {
				len &= 0x7F;
				if( fileOfs + len*sampleSize > Size ) {
					memcpy(ret->Data + ofs*sampleSize, Buffer+fileOfs, Size-fileOfs);
					return ret;
				}
				else {
					memcpy(ret->Data + ofs*sampleSize, Buffer+fileOfs, len*sampleSize);
				}
				ofs += len;
			}
			else {
				uint8_t	tmp[sampleSize];
				
				if( fileOfs + sampleSize > Size )	return ret;
				
				for(i=0;i<sampleSize;i++)
					tmp[i] = *(uint8_t*)Buffer+fileOfs;	fileOfs += 1;
				
				i = 0;
				while(len--) {
					for(i=0;i<sampleSize;i++)
						ret->Data[ofs++] = tmp[i];
				}
			}
		}
		return ret;
	
	// Channel 1.7.8 RLE
	case 3:
		// Alpha, Red, Green, Blue
		for( i = 0; i < sampleSize; i++ )
		{
			ofs = i;
			while( ofs < w*h*sampleSize )
			{
				uint8_t	len, val;
				if( fileOfs + 1 > Size )	return ret;
				len = *(uint8_t*)Buffer+fileOfs;	fileOfs += 1;
				if(len & 0x80) {
					len &= 0x7F;
					while(len--) {
						if( fileOfs + 1 > Size )	return ret;
						val = *(uint8_t*)Buffer+fileOfs;	fileOfs += 1;
						if(i == 0)
							ret->Data[ofs] = val;
						else
							ret->Data[ofs] |= val;
						ofs += sampleSize;
					}
				}
				else {
					if( fileOfs + 1 > Size )	return ret;
					val = *(uint8_t*)Buffer+fileOfs;	fileOfs += 1;
					if(i == 0) {
						while(len--) {
							ret->Data[ofs] = val;		ofs += sampleSize;
						}
					}
					else {
						while(len--) {
							ret->Data[ofs] |= val;	ofs += sampleSize;
						}
					}
				}
			}
		}
		return ret;
	
	default:
		fprintf(stderr, "Warning: Unknown compression scheme %i for SIF\n", comp);
		return NULL;
	}
}
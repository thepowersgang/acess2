#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

enum eCompressionModes {
	COMP_NONE,
	COMP_RLE1x32,
	COMP_ZLIB,
	COMP_RLE4x8,
	COMP_AUTO = 8
};

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
size_t	CompressRLE1x32(void *Dest, const void *Src, size_t Size);
size_t	CompressRLE4x8(void *Dest, const void *Src, size_t Size);
size_t	RLE1_7(void *Dest, const void *Src, size_t Size, int BlockSize, int BlockStep, int MinRunLength);
uint32_t	GetARGB(SDL_Surface *srf, int x, int y);

// === CODE ===
int main(int argc, char *argv[])
{
	FILE	*fp;
	 int	bufLen;
	enum eCompressionModes	comp = COMP_AUTO;
	 int	w, h;
	 int	pixel_count;
	uint32_t	*buffer, *buffer2;
	
	const char	*input_file = NULL, *output_file = NULL;

	for( int i = 1; i < argc; i ++ )
	{
		if( argv[i][0] != '-' ) {
			if( !input_file )
				input_file = argv[i];
			else if( !output_file )
				output_file = argv[i];
			else {
				// Error
			}
		}
		else if( argv[i][1] != '-' ) {
			// Single char args
		}
		else {
			// Long args
			if( strcmp(argv[i], "--uncompressed") == 0 ) {
				// Force compression mode to '0'
				comp = COMP_NONE;
			}
			else if( strcmp(argv[i], "--rle4x8") == 0 ) {
				comp = COMP_RLE4x8;
			}
			else if( strcmp(argv[i], "--rle1x32") == 0 ) {
				comp = COMP_RLE1x32;
			}
			else {
				// Error
			}
		}
	}
	if( !input_file || !output_file ) {
		fprintf(stderr, "Usage: %s <infile> <outfile>\n", argv[0]);
		return 1;
	}
	
	// Read image
	{
		SDL_Surface *img = IMG_Load(input_file);
		if( !img )	return -1;
		
		w = img->w;
		h = img->h;
		pixel_count = w * h;
		buffer = malloc( pixel_count * 4 );
		
		for( int y = 0, i = 0; y < img->h; y++ )
		{
			for( int x = 0; x < img->w; x ++ )
			{
				buffer[i++] = GetARGB(img, x, y);
			}
		}
		SDL_FreeSurface(img);
	}
	
	if( comp == COMP_AUTO ) 
	{
		// Try encoding using a single RLE stream
		size_t	rle32length = CompressRLE1x32(NULL, buffer, pixel_count);
		// Try encoding using separate RLE streams for each channel
		size_t	rle4x8length = CompressRLE4x8(NULL, buffer, pixel_count);
		
	//	printf("raw length = %i\n", pixel_count * 4);
	//	printf("rle32length = %u\n", (unsigned int)rle32length);
	//	printf("rle4x8length = %u\n", (unsigned int)rle4x8length);
		
		if( rle32length <= rle4x8length ) {
			comp = COMP_RLE1x32;	// 32-bit RLE
		}
		else {
			comp = COMP_RLE4x8;	// 4x8 bit RLE
		}
	}

	switch(comp)
	{
	case COMP_NONE:
		bufLen = pixel_count*4;
		buffer2 = buffer;
		buffer = NULL;
		break;
	case COMP_RLE1x32:
		bufLen = CompressRLE1x32(NULL, buffer, pixel_count);
		buffer2 = malloc(bufLen);
		bufLen = CompressRLE1x32(buffer2, buffer, pixel_count);
		break;
	case COMP_RLE4x8:
		bufLen = CompressRLE4x8(NULL, buffer, pixel_count);
		buffer2 = malloc(bufLen);
		bufLen = CompressRLE4x8(buffer2, buffer, pixel_count);
		break;
	default:
		fprintf(stderr, "Unknown compresion %i\n", comp);
		return 2;
	}

	free(buffer);
	
	// Open and write
	fp = fopen(output_file, "w");
	if( !fp )	return -1;
	
	uint16_t	word;
	word = 0x51F0;	fwrite(&word, 2, 1, fp);
	word = comp&7;	fwrite(&word, 2, 1, fp);
	word = w;	fwrite(&word, 2, 1, fp);
	word = h;	fwrite(&word, 2, 1, fp);
	
	fwrite(buffer2, bufLen, 1, fp);
	free(buffer2);
	fclose(fp);
	
	return 0;
}

size_t CompressRLE1x32(void *Dest, const void *Src, size_t Size)
{
	return RLE1_7(Dest, Src, Size, 4, 4, 2);
}
size_t CompressRLE4x8(void *Dest, const void *Src, size_t Size)
{
	size_t	ret = 0;
	ret += RLE1_7(Dest ? Dest + ret : NULL, Src+0, Size, 1, 4, 3);
	ret += RLE1_7(Dest ? Dest + ret : NULL, Src+1, Size, 1, 4, 3);
	ret += RLE1_7(Dest ? Dest + ret : NULL, Src+2, Size, 1, 4, 3);
	ret += RLE1_7(Dest ? Dest + ret : NULL, Src+3, Size, 1, 4, 3);
	return ret;
}

#define USE_VERBATIM	1

/**
 * \brief Run Length Encode a data stream
 * \param Dest	Destination buffer (can be NULL)
 * \param Src	Source data
 * \param Size	Size of source data
 * \param BlockSize	Size of a data element (in bytes)	
 * \param BlockStep	Separation of the beginning of each data element (must be > 0 and should be >= BlockSize)
 * \param MinRunLength	Minimum run length for RLE to be used (must be be >= 1)
 * 
 * This function produces a RLE stream encoded with a 7-bit size and a
 * verbatim flag (allowing strings of non-rle data to be included in the
 * data for efficiency) of the data blocks from \a Src. Each block is
 * \a BlockSize bytes in size.
 * 
 * \a BlockStep allows this function to encode data that is interlaced with
 * other data that you may not want to RLE together (for example, different
 * colour channels in an image).
 */
size_t RLE1_7(void *Dest, const void *Src, size_t Size, int BlockSize, int BlockStep, int MinRunLength)
{
	const uint8_t	*src = Src;
	uint8_t	*dest = Dest;
	 int	ret = 0;
	 int	nVerb = 0, nRLE = 0;	// Debugging
	
	//printf("RLE1_7: (Dest=%p, Src=%p, Size=%lu, BlockSize=%i, BlockStep=%i, MinRunLength=%i)\n",
	//	Dest, Src, Size, BlockSize, BlockStep, MinRunLength);
	
	// Prevent errors
	if( Size < BlockSize )	return -1;
	if( !Src )	return -1;
	if( BlockSize <= 0 || BlockStep <= 0 )	return -1;
	if( MinRunLength < 1 )	return -1;
	
	// Scan through the data stream
	for( int i = 0; i < Size; i ++ )
	{
		//printf("i = %i, ", i);
		 int	runlen;
		// Check forward for and get the "run length"
		for( runlen = 1; runlen < 127 && i+runlen < Size; runlen ++ )
		{
			// Check for equality
			if( memcmp(&src[i*BlockStep], &src[(i+runlen)*BlockStep], BlockSize) != 0 )
				break;
		}
		
		#if USE_VERBATIM
		// Check for a verbatim range (runlength of `MinRunLength` or less)
		if( runlen <= MinRunLength )
		{
			 int	nSame = 0;
			// Get the length of the verbatim run
			for( runlen = 1; runlen < 127 && i+runlen < Size; runlen ++ )
			{
				// Check for equality
				if( memcmp(&src[i*BlockStep], &src[(i+runlen)*BlockStep], BlockSize) != 0 )
				{
					nSame = 0;
				}
				else
				{
					nSame ++;
					if( nSame > MinRunLength ) {
						runlen -= nSame-1;
						break;
					}
				}
			}
			
			// Save data
			if( dest )
			{
				dest[ret++] = runlen | 0x80;	// Length (with high bit set)
				// Data
				for( int k = 0; k < runlen; k ++ )
				{
					memcpy( &dest[ret], &src[(i+k)*BlockStep], BlockSize );
					ret += BlockSize;
				}
			}
			else {
				ret += 1 + runlen*BlockSize;
			}
			
			nVerb ++;
		}
		// RLE Range
		else {
		#endif	// USE_VERBATIM
			if( dest ) {
				dest[ret++] = runlen;	// Length (with high bit unset)
				memcpy(&dest[ret], &src[i*BlockStep], BlockSize);
				ret += BlockSize;
			}
			else {
				ret += 1 + BlockSize;
			}
			
			nRLE ++;
		#if USE_VERBATIM
		}
		#endif
		
		i += runlen-1;
	}
	
	//printf("nVerb = %i, nRLE = %i\n", nVerb, nRLE);
	return ret;
}

uint32_t GetARGB(SDL_Surface *srf, int x, int y)
{
	uint8_t	r, g, b, a;
	SDL_GetRGBA(
		*(uint32_t*)( srf->pixels + srf->format->BytesPerPixel*x + y * srf->pitch ),
		srf->format, &r, &g, &b, &a
		);
	return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
}

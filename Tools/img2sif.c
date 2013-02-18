#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
size_t	RLE1_7(void *Dest, void *Src, size_t Size, int BlockSize, int BlockStep, int MinRunLength);
uint32_t	GetARGB(SDL_Surface *srf, int x, int y);

// === CODE ===
int main(int argc, char *argv[])
{
	FILE	*fp;
	size_t	rle32length, rle4x8length;
	 int	bufLen;
	 int	comp;
	 int	w, h;
	 int	pixel_count;
	uint32_t	*buffer, *buffer2;

	if( argc != 3 ) {
		fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
		return 1;
	}
	
	// Read image
	{
		SDL_Surface *img = IMG_Load(argv[1]);
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
	
	// Try encoding using a single RLE stream
	rle32length = RLE1_7(NULL, buffer, pixel_count, 4, 4, 3);
	
	// Try encoding using separate RLE streams for each channel
	rle4x8length  = RLE1_7(NULL, buffer, pixel_count, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+1, pixel_count, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+2, pixel_count, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+3, pixel_count, 1, 4, 2);
	
//	printf("raw length = %i\n", pixel_count * 4);
//	printf("rle32length = %u\n", (unsigned int)rle32length);
//	printf("rle4x8length = %u\n", (unsigned int)rle4x8length);
	
	if( rle32length < rle4x8length ) {
		comp = 1;	// 32-bit RLE
		buffer2 = malloc( rle32length );
		bufLen = RLE1_7(buffer2, buffer, pixel_count, 4, 4, 2);
	}
	else {
		comp = 3;	// 4x8 bit RLE
		buffer2 = malloc( rle4x8length );
		rle4x8length  = RLE1_7(buffer2, buffer, pixel_count, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+1, pixel_count, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+2, pixel_count, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+3, pixel_count, 1, 4, 3);
		bufLen = rle4x8length;
	}
	free(buffer);
	
	// Open and write
	fp = fopen(argv[2], "w");
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
size_t RLE1_7(void *Dest, void *Src, size_t Size, int BlockSize, int BlockStep, int MinRunLength)
{
	uint8_t	*src = Src;
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

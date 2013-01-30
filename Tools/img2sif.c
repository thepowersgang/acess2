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
	SDL_Surface	*img;
	uint16_t	word;
	uint32_t	val;
	 int	y, x, i;
	 int	rle32length, rle4x8length;
	 int	bufLen;
	uint32_t	*buffer, *buffer2;
	
	img = IMG_Load(argv[1]);
	if( !img )	return -1;
	
	buffer = malloc( img->w * img->h * 4 );
	
	i = 0;
	for( y = 0; y < img->h; y++ )
	{
		for( x = 0; x < img->w; x ++ )
		{
			val = GetARGB(img, x, y);
			buffer[i++] = val;
		}
	}
	
	SDL_FreeSurface(img);
	
	// Try encoding using a single RLE stream
	rle32length = RLE1_7(NULL, buffer, img->w * img->h, 4, 4, 3);
	
	// Try encoding using separate RLE streams for each channel
	rle4x8length  = RLE1_7(NULL, buffer, img->w * img->h, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+1, img->w * img->h, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+2, img->w * img->h, 1, 4, 2);
	rle4x8length += RLE1_7(NULL, ((uint8_t*)buffer)+3, img->w * img->h, 1, 4, 2);
	
	printf("raw length = %i\n", img->w * img->h * 4);
	printf("rle32length = %i\n", rle32length);
	printf("rle4x8length = %i\n", rle4x8length);
	
	if( rle32length < rle4x8length ) {
		comp = 1;	// 32-bit RLE
		buffer2 = malloc( rle32length );
		bufLen = RLE1_7(buffer2, buffer, img->w * img->h, 4, 4, 2);
	}
	else {
		comp = 3;	// 4x8 bit RLE
		buffer2 = malloc( rle4x8length );
		rle4x8length  = RLE1_7(buffer2, buffer, img->w * img->h, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+1, img->w * img->h, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+2, img->w * img->h, 1, 4, 3);
		rle4x8length += RLE1_7(buffer2+rle4x8length, ((uint8_t*)buffer)+3, img->w * img->h, 1, 4, 3);
		bufLen = rle4x8length;
	}
	
	// Open and write
	fp = fopen(argv[2], "w");
	if( !fp )	return -1;
	
	word = 0x51F0;	fwrite(&word, 2, 1, fp);
	word = comp&7;	fwrite(&word, 2, 1, fp);
	word = img->w;	fwrite(&word, 2, 1, fp);
	word = img->h;	fwrite(&word, 2, 1, fp);
	
	fwrite(buffer2, bufLen, 1, fp);
	
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
	 int	i, j, k;
	 int	nVerb = 0, nRLE = 0;	// Debugging
	
	//printf("RLE1_7: (Dest=%p, Src=%p, Size=%lu, BlockSize=%i, BlockStep=%i, MinRunLength=%i)\n",
	//	Dest, Src, Size, BlockSize, BlockStep, MinRunLength);
	
	// Prevent errors
	if( Size < BlockSize )	return -1;
	if( !Src )	return -1;
	if( BlockSize <= 0 || BlockStep <= 0 )	return -1;
	if( MinRunLength < 1 )	return -1;
	
	// Scan through the data stream
	for( i = 0; i < Size; i ++ )
	{
		//printf("i = %i, ", i);
		// Check forward for and get the "run length"
		for( j = i + 1; j < i + 127 && j < Size; j ++ )
		{
			// Check for equality
			for( k = 0; k < BlockSize; k ++ ) {
				if( src[j*BlockStep+k] != src[i*BlockStep+k] )	break;
			}
			// If not, break
			if( k < BlockSize )	break;
		}
		
		#if USE_VERBATIM
		// Check for a verbatim range (runlength of `MinRunLength` or less)
		if( j - i <= MinRunLength ) {
			 int	nSame = 0;
			// Get the length of the verbatim run
			for( j = i + 1; j < i + 127 && j < Size; j ++ )
			{
				// Check for equality
				for( k = 0; k < BlockSize; k ++ ) {
					if( src[j*BlockStep+k] != src[i*BlockStep+k] )	break;
				}
				if( k == BlockSize ) {
					nSame ++;
					if( nSame > MinRunLength )
						break;
				}
				else {
					nSame = 0;
				}
			}
			
			// Save data
			if( dest ) {
				dest[ret++] = (j - i) | 0x80;	// Length (with high bit set)
				// Data
				for( k = 0; k < (j - i)*BlockSize; k ++ )
					dest[ret++] = src[i*BlockStep+k];
			}
			else {
				ret += 1 + BlockSize*(j - i);
			}
			
			nVerb ++;
		}
		// RLE Range
		else {
		#endif	// USE_VERBATIM
			// length = j - i (maxes out at 127)
			if( dest ) {
				dest[ret++] = j - i;	// Length (with high bit unset)
				for( k = 0; k < BlockSize; k ++ ) {
					dest[ret++] = src[i*BlockStep+k];
				}
			}
			else {
				ret += 1 + BlockSize;
			}
			
			nRLE ++;
		#if USE_VERBATIM
		}
		#endif
		
		i = j;
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

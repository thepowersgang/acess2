/*
 */
#ifndef _IMAGE_H_
#define _IMAGE_H_

// === TYPES ===
typedef struct sImage	tImage;
struct sImage
{
	short	Width;
	short	Height;
	 int	Format;
	uint8_t	Data[];
};

// === CONSTANTS ===
enum eImageFormats
{
	IMGFMT_BGRA,
	IMGFMT_RGB,
	NUM_IMGFMTS
};

#endif

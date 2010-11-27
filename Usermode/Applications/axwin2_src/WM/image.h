/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#ifndef _IMAGE_H_
#define _IMAGE_H_

typedef struct sImage	tImage;

struct sImage
{
	short	Width;
	short	Height;
	 int	Format;
	uint8_t	Data[];
};

enum eImageFormats
{
	IMGFMT_BGRA,
	IMGFMT_RGB,
	NUM_IMGFMTS
};

// === PROTOTYPES ===
extern tImage	*Image_Load(const char *URI);

#endif

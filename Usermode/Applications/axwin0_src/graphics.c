/*
Acess OS Window Manager
*/
#include "header.h"

#define ABS(x)	((x)<0?-(x):(x))
#define PLOT(x,y,colour)	(gScreenBuffer[(y)*SCREEN_WIDTH+(x)] = (colour))

void draw_line(int x1, int y1, int x2, int y2, Uint32 colour)
{
	int dy, dx;
	int	i;
	float	step;
	
	dy = ABS(y2 - y1);
	dx = ABS(x2 - x1);
	
	if(dx == 0) {
		if(y1 < y2) {
			for(i=y1;i<y2;i++)
				PLOT(x1, i, colour);
		} else {
			for(i=y2;i<y1;i++)
				PLOT(x1, i, colour);
		}
	}
	
	if(x1 < x2) {
		step = (dy/dx);	step = (y2>y1?step:-step);
		for(i=x1;i<x2;i++)	PLOT(i, y1+(int)(step*i), colour);
	} else {
		step = (dy/dx);	step = (y2>y1?-step:step);
		for(i=x2;i<x1;i++)	PLOT(i, y1+(int)(step*i), colour);
	}
}

void draw_rect(int x, int y, int w, int h, Uint32 colour)
{
	Uint32	*p = gScreenBuffer+y*SCREEN_WIDTH+x;
	int		i, j;
	
	for(j=0;j<h;j++) {
		for(i=0;i<w;i++)	*(p+i) = colour;
		p += SCREEN_WIDTH;
	}
}

void draw_bmp(BITMAP *bmp, RECT *rc)
{
	int w, h, i, j;
	Uint32	a;
	int	r,g,b;
	Uint32	*buf, *bi;
	Uint16	*bi16;
	Uint8	*bi8;
	
	#if DEBUG
		k_printf("draw_bmp: (bmp=0x%x, rc=0x%x)\n", bmp, rc);
	#endif
	
	buf = gScreenBuffer+rc->y1*SCREEN_WIDTH+rc->x1;
	bi = bmp->data;	bi16 = bmp->data;	bi8 = bmp->data;
	w = bmp->width;
	h = bmp->height;
	
	#if DEBUG
		k_printf(" draw_bmp: bmp->bpp = %i\n", bmp->bpp);
	#endif
	
	switch(bmp->bpp)
	{
	// === 32bit Colour ===
	case 32:
		for(i = 0; i < h; i++)
		{
			for(j = 0; j < w; j++)
			{
				a = *bi >> 24;
				switch(a)
				{
				case 0xFF:	//Fully Visible
					buf[j] = *bi;
					break;
				case 0x80:	// 50/50
					r = ((*bi>>16)&0xFF) + ((*buf>>16)&0xFF);
					g = ((*bi>>8)&0xFF) + ((*buf>>8)&0xFF);
					b = ((*bi)&0xFF) + ((*buf)&0xFF);
					r >>= 1;	g >>= 1;	b >>= 1;
					buf[j] = (r<<16) | (g<<8) | b;
					break;
				case 0x00:	// Fully Transparent
					break;
				default:	// Everything else
					r = ((*bi>>16)&0xFF)*(255-a) + ((*buf>>16)&0xFF)*a;
					g = ((*bi>>8)&0xFF)*(255-a) + ((*buf>>8)&0xFF)*a;
					b = ((*bi)&0xFF)*(255-a) + ((*buf)&0xFF)*a;
					r >>= 8;	g >>= 8;	b >>= 8;
					buf[j] = (r<<16) | (g<<8) | b;
					break;
				}
				bi++;
			}
			buf += SCREEN_WIDTH;
		}
		break;
	}
}

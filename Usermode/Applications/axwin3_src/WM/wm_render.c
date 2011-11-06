/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm_render.c
 * - Window rendering functions
 */
#include <common.h>
#include <wm.h>
#include <wm_internals.h>
#include <stdlib.h>

// === CODE ===
void WM_Render_FillRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour)
{
	uint32_t	*dest;
	 int	i;
//	_SysDebug("WM_Render_FilledRect(%p, 0x%x...", Window, Colour);
//	_SysDebug(" (%i,%i), %ix%i)", X, Y, W, H);
	// Clip to window dimensions
	if(X < 0) { W += X; X = 0; }
	if(Y < 0) { H += Y; Y = 0; }
	if(W <= 0 || H <= 0)	return;
	if(X >= Window->W)	return;
	if(Y >= Window->H)	return;
	if(X + W > Window->W)	W = Window->W - X;
	if(Y + H > Window->H)	H = Window->H - Y;
//	_SysDebug(" Clipped to (%i,%i), %ix%i", X, Y, W, H);

	if(!Window->RenderBuffer) {
		Window->RenderBuffer = malloc(Window->W*Window->H*4);
	}

	dest = (uint32_t*)Window->RenderBuffer + Y*Window->W + X;
	while( H -- )
	{
		for( i = W; i --; )
			*dest++ = Colour;
		dest += Window->W - W;
	}
}

void WM_Render_DrawRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour)
{	
	WM_Render_FillRect(Window, X, Y, W, 1, Colour);
	WM_Render_FillRect(Window, X, Y+H-1, W, 1, Colour);
	WM_Render_FillRect(Window, X, Y, 1, H, Colour);
	WM_Render_FillRect(Window, X+W-1, Y, 1, H, Colour);
}

/**
 * \brief Draw an image to the screen
 * \todo Maybe have support for an offset in the image
 */
void WM_Render_DrawImage(tWindow *Window, int X, int Y, int W, int H, tImage *Image)
{
	 int	x, y;
	uint32_t	*dest;
	uint8_t	*data;
	
	// Sanity please
	if( !Image )	return ;

	// Allocate
	if(!Window->RenderBuffer) {
		Window->RenderBuffer = malloc(Window->W*Window->H*4);
	}
	
	// Bounds Check
	if( X >= Window->W )	return ;
	if( Y >= Window->H )	return ;
	
	// Wrap to image size
	if( W > Image->Width )	W = Image->Width;
	if( H > Image->Height )	H = Image->Height;
	
	// Wrap to screen size
	if( X + W > Window->W )	W = Window->W - X;
	if( Y + H > Window->H )	H = Window->H - Y;

	dest = (uint32_t*)Window->RenderBuffer + Y * Window->W + X;
	data = Image->Data;

	// Do the render
	switch( Image->Format )
	{
	case IMGFMT_BGRA:
		for( y = 0; y < H; y ++ )
		{
			 int	r, g, b, a;	// New
			 int	or, og, ob;	// Original
			for( x = 0; x < W; x ++ )
			{
				b = data[x*4+0]; g = data[x*4+1]; r = data[x*4+2]; a = data[x*4+3];
				if( a == 0 )	continue;	// 100% transparent
				ob = dest[x]&0xFF; og = (dest[x] >> 8)&0xFF; or = (dest[x] >> 16)&0xFF;
				// Handle Alpha
				switch(a)
				{
				// Transparent: Handled above
				// Solid
				case 0xFF:	break;
				// Half
				case 0x80:
					r = (or + r) / 2;
					g = (og + g) / 2;
					b = (ob + b) / 2;
					break;
				// General
				default:
					r = (or * (255-a) + r * a) / 255;
					g = (og * (255-a) + g * a) / 255;
					b = (ob * (255-a) + b * a) / 255;
					break;
				}
				dest[x] = b | (g << 8) | (r << 16);
			}
			data += Image->Width * 4;
			dest += Window->W;
		}
		break;
	
	// RGB
	case IMGFMT_RGB:
		for( y = 0; y < H; y ++ )
		{
			for( x = 0; x < W; x ++ )
			{
				//        Blue           Green                Red
				dest[x] = data[x*3+2] | (data[x*3+1] << 8) | (data[x*3+0] << 16);
			}
			data += W * 3;
			dest += Window->W;
		}
		break;
	default:
		_SysDebug("ERROR: Unknown image format %i\n", Image->Format);
		break;
	}
}


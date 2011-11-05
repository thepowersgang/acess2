/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Window manager core
 */
#include <common.h>
#include <wm_renderer.h>
#include <stdlib.h>
#include <string.h>
#include <video.h>

// === GLOBALS ===
tWMRenderer	*gpWM_Renderers;
tWindow	*gpWM_RootWindow;

// === CODE ===
void WM_Initialise(void)
{
	WM_CreateWindow(NULL, 0x0088FF, "Background");
	gpWM_RootWindow->W = giScreenWidth;
	gpWM_RootWindow->H = giScreenHeight;
	gpWM_RootWindow->Flags = WINFLAG_SHOW;
}

void WM_RegisterRenderer(tWMRenderer *Renderer)
{
	// TODO: Catch out duplicates
	Renderer->Next = gpWM_Renderers;
	gpWM_Renderers = Renderer;
}

// --- Manipulation
tWindow *WM_CreateWindow(tWindow *Parent, int RendererArg, const char *RendererName)
{
	tWMRenderer	*renderer;
	tWindow	*ret;
	
	// - Get Renderer
	for( renderer = gpWM_Renderers; renderer; renderer = renderer->Next )
	{
		if(strcmp(RendererName, renderer->Name) == 0)
			break;
	}
	if(renderer == NULL)
		return NULL;

	if(!Parent)
		Parent = gpWM_RootWindow;

	// - Call create window function
	ret = renderer->CreateWindow(RendererArg);
	ret->Parent = Parent;
	ret->Renderer = renderer;
	ret->Flags = WINFLAG_CLEAN;	// Note, not acutally clean, but it makes invaidate work

	// Append to parent
	if(Parent)
	{
		if(Parent->LastChild)
			Parent->LastChild->NextSibling = ret;
		else
			Parent->FirstChild = ret;
		ret->PrevSibling = Parent->LastChild;
		Parent->LastChild = ret;
	}
	else
	{
		gpWM_RootWindow = ret;
	}

	// - Return!
	return ret;
}

tWindow *WM_CreateWindowStruct(size_t ExtraSize)
{
	tWindow	*ret;
	
	ret = calloc( sizeof(tWindow) + ExtraSize, 1 );
	ret->RendererInfo = ret + 1;	// Get end of tWindow
	
	return ret;
}

void WM_ShowWindow(tWindow *Window, int bShow)
{
	// TODO: Message window
	if(bShow)
		Window->Flags |= WINFLAG_SHOW;
	else
		Window->Flags &= ~WINFLAG_SHOW;
	WM_Invalidate(Window);
}

int WM_MoveWindow(tWindow *Window, int X, int Y)
{
	// Clip coordinates
	if(X + Window->W < 0)	X = -Window->W + 1;
	if(Y + Window->H < 0)	Y = -Window->H + 1;
	if(X >= giScreenWidth)	X = giScreenWidth - 1;
	if(Y >= giScreenHeight)	Y = giScreenHeight - 1;
	
	Window->X = X;	Window->Y = Y;

	WM_Invalidate(Window);

	return 0;
}

int WM_ResizeWindow(tWindow *Window, int W, int H)
{
	if(W <= 0 || H <= 0 )	return 1;
	if(Window->X + W < 0)	Window->X = -W + 1;
	if(Window->Y + H < 0)	Window->Y = -H + 1;
	
	Window->W = W;	Window->H = H;

	WM_Invalidate(Window);
	
	return 0;
}

void WM_Invalidate(tWindow *Window)
{
	// Don't invalidate twice (speedup)
	if( !(Window->Flags & WINFLAG_CLEAN) )	return;

	_SysDebug("Invalidating %p");
	
	// Mark for re-render
	Window->Flags &= ~WINFLAG_CLEAN;

	// Mark up the tree that a child window has changed	
	while( (Window = Window->Parent) )
	{
		_SysDebug("Childclean removed from %p", Window);
		Window->Flags &= ~WINFLAG_CHILDCLEAN;
	}
}

// --- Rendering / Update
void WM_int_UpdateWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	// Render
	if( !(Window->Flags & WINFLAG_CLEAN) )
	{
		Window->Renderer->Redraw(Window);
		Window->Flags |= WINFLAG_CLEAN;
	}
	
	// Process children
	if( !(Window->Flags & WINFLAG_CHILDCLEAN) )
	{
		for( child = Window->FirstChild; child; child = child->NextSibling )
		{
			WM_int_UpdateWindow(child);
		}
		Window->Flags |= WINFLAG_CHILDCLEAN;
	}
	
}

void WM_int_BlitWindow(tWindow *Window)
{
	tWindow	*child;

	// Ignore hidden windows
	if( !(Window->Flags & WINFLAG_SHOW) )
		return ;
	
	Video_Blit(Window->RenderBuffer, Window->X, Window->Y, Window->W, Window->H);
	
	for( child = Window->FirstChild; child; child = child->NextSibling )
	{
		WM_int_BlitWindow(child);
	}
}

void WM_Update(void)
{
	// Don't redraw if nothing has changed
	if( (gpWM_RootWindow->Flags & WINFLAG_CHILDCLEAN) )
		return ;	

	// - Iterate through visible windows, updating them as needed
	WM_int_UpdateWindow(gpWM_RootWindow);
	
	// - Draw windows from back to front to the render buffer
	WM_int_BlitWindow(gpWM_RootWindow);

	Video_Update();
}

// --- WM Render Routines
// TODO: Move to another file?
void WM_Render_FillRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour)
{
	uint32_t	*dest;
	 int	i;
	_SysDebug("WM_Render_FilledRect(%p, 0x%x...", Window, Colour);
	_SysDebug(" (%i,%i), %ix%i)", X, Y, W, H);
	// Clip to window dimensions
	if(X < 0) { W += X; X = 0; }
	if(Y < 0) { H += Y; Y = 0; }
	if(X >= Window->W)	return;
	if(Y >= Window->H)	return;
	if(X + W > Window->W)	W = Window->W - X;
	if(Y + H > Window->H)	H = Window->H - Y;
	_SysDebug(" Clipped to (%i,%i), %ix%i", X, Y, W, H);

	// Render to buffer
	// Create if needed?

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

void WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, void *Font, tColour Colour, const char *Text)
{
	// TODO: Implement
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


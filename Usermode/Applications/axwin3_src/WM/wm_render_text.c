/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * wm_render_text.c
 * - WM Text Rendering
 */
#include <common.h>
#include <wm_internals.h>
#include <stdlib.h>

// === TYPES ===
typedef struct sGlyph	tGlyph;

struct sGlyph
{
	struct sGlyph	*Next;
	struct sGlyph	*Prev;
	
	uint32_t	Codepoint;
	
	// Effective dimensions (distance to move 'cursor')
	short	Width;
	short	Height;
	
	// Distance from the current cursor position to render at
	short	OffsetX;
	short	OffsetY;
	
	// True dimensions (size of the bitmap
	short	TrueWidth;
	short	TrueHeight;
	
	// Bitmap Data
	uint8_t	Bitmap[];	// 8-bit alpha	
};

struct sFont
{
	struct sFont	*Next;
	 int	ReferenceCount;
	
	tGlyph	*AsciiGlyphs[128];	// Glyphs 0-127
	
	tGlyph	*FirstGlyph;
	tGlyph	*LastGlyph;
	
	tGlyph	*(*CacheGlyph)(struct sFont *this, uint32_t Codepoint);
	
};


// === PROTOTYPES ===
 int	WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, tFont *Font, tColour Color, const char *Text);
void	WM_Render_GetTextDims(tFont *Font, const char *Text, int *W, int *H);
tGlyph	*_GetGlyph(tFont *Font, uint32_t Codepoint);
void	_RenderGlyph(tWindow *Window, short X, short Y, tGlyph *Glyph, uint32_t Color);
tGlyph	*_SystemFont_CacheGlyph(tFont *Font, uint32_t Codepoint);
 int	ReadUTF8(const char *Input, uint32_t *Output);

// === GLOBALS ===
tFont	gSystemFont = {
	.CacheGlyph = _SystemFont_CacheGlyph
};

// === CODE ===
/**
 * \brief Draw text to the screen
 */
int WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, tFont *Font, tColour Colour, const char *Text)
{
	 int	xOfs = 0;
	tGlyph	*glyph;
	uint32_t	ch = 0;

//	_SysDebug("WM_Render_DrawText: (X=%i,Y=%i,W=%i,H=%i,Font=%p,", X, Y, W, H, Font);
//	_SysDebug("  Colour=%08x,Text='%s')", Colour, Text);
	
	if(!Text)	return 0;

	// Check the bounds
	if(W < 0 || X < 0 || X >= Window->W)	return 0;
	if(X + W > Window->W)	W = Window->W - X;
	
	if(H < 0 || Y < 0 || Y >= Window->H)	return 0;
	if(Y + H > Window->H)	H = Window->H - Y;
	
	// Handle NULL font (system default monospace)
	if( !Font )	Font = &gSystemFont;
	
	while( *Text )
	{
		// Read character
		Text += ReadUTF8(Text, &ch);
		
		// Find (or load) the glyph
		glyph = _GetGlyph(Font, ch);
		if( !glyph )	continue ;	// If not found, just don't render it
		
		// End render if it will overflow the provided range
		if( xOfs + glyph->TrueWidth > W )
			break;
		
		_RenderGlyph(Window, X + xOfs, Y, glyph, Colour);
		xOfs += glyph->Width;
	}
	
	return xOfs;
}

void WM_Render_GetTextDims(tFont *Font, const char *Text, int *W, int *H)
{
	 int	w=0, h=0;
	uint32_t	ch;
	tGlyph	*glyph;
	if( !Font )	Font = &gSystemFont;
	
	while( *Text )
	{
		Text += ReadUTF8(Text, &ch);
		glyph = _GetGlyph(Font, ch);
		if( !glyph )	continue;
		
		w += glyph->Width;
		if( h < glyph->Height )	h = glyph->Height;
	}
	
	if(W)	*W = w;
	if(H)	*H = h;
}

tGlyph *_GetGlyph(tFont *Font, uint32_t Codepoint)
{
	tGlyph	*next = NULL, *prev = NULL;
	tGlyph	*new;
	
	// Check for ASCII
	if( Codepoint < 128 )
	{
		if( Font->AsciiGlyphs[Codepoint] == NULL ) {
			Font->AsciiGlyphs[Codepoint] = Font->CacheGlyph(Font, Codepoint);
		}
		
		return Font->AsciiGlyphs[Codepoint];
	}
	
	// If within the range
	if( Font->FirstGlyph && Font->FirstGlyph->Codepoint < Codepoint && Codepoint < Font->LastGlyph->Codepoint )
	{
		// Find what end is "closest"
		if( Codepoint - Font->FirstGlyph->Codepoint < Font->LastGlyph->Codepoint - Codepoint )
		{
			// Start from the bottom
			for( next = Font->FirstGlyph;
				 next && next->Codepoint < Codepoint;
				 prev = next, next = next->Next
				 );
			
			if( next->Codepoint == Codepoint )
				return next;
			
		}
		else
		{
			// Start at the top
			// NOTE: The roles of next and prev are reversed here to allow 
			//       the insert to be able to assume that `prev` is the
			//       previous entry, and `next` is the next.
			for( prev = Font->LastGlyph;
				 prev && prev->Codepoint > Codepoint;
				 next = prev, prev = prev->Prev
				 );
			if( prev->Codepoint == Codepoint )
				return prev;
		}
	}
	else
	{
		// If below first
		if( !Font->FirstGlyph ||  Font->FirstGlyph->Codepoint > Codepoint ) {
			prev = NULL;
			next = Font->FirstGlyph;
		}
		// Above last
		else {
			prev = Font->LastGlyph;
			next = NULL;
		}
	}
	
	// Load new
	new = Font->CacheGlyph(Font, Codepoint);
	if( !new )	return NULL;
	
	// Add to list
	// - Forward link
	if( prev ) {
		new->Next = prev->Next;
		prev->Next = new;
	}
	else {
		new->Next = Font->FirstGlyph;
		Font->FirstGlyph = new;
	}
	
	// - Backlink
	if( next ) {
		new->Prev = next->Prev;
		next->Prev = new;
	}
	else {
		new->Prev = Font->LastGlyph;
		Font->LastGlyph = new;
	}
	
	// Return
	return new;
}

/**
 */
void _RenderGlyph(tWindow *Window, short X, short Y, tGlyph *Glyph, uint32_t Color)
{
	 int	xStart = 0, yStart = 0;
	 int	x, y, dst_x;
	uint32_t	*outBuf;
	uint8_t	*inBuf;

	X += Glyph->OffsetX;
	if( X < 0 ) {	// If -ve, skip the first -X pixels
		xStart = -X;
		X = 0;
	}

	Y += Glyph->OffsetY;
	if( Y < 0 ) {	// If -ve, skip the first -Y lines
		yStart = -Y;
		Y = 0;
	}

//	_SysDebug("X = %i, Y = %i", X, Y);
	outBuf = (uint32_t*)Window->RenderBuffer + Y*Window->W + X;
	inBuf = Glyph->Bitmap + yStart*Glyph->TrueWidth;

	for( y = yStart; y < Glyph->TrueHeight; y ++ )
	{
		for( x = xStart, dst_x = 0; x < Glyph->TrueWidth; x ++, dst_x ++ )
		{
			outBuf[dst_x] = Video_AlphaBlend( outBuf[dst_x], Color, inBuf[x] );
		}
		outBuf += Window->W;
		inBuf += Glyph->TrueWidth;
	}
}

// Load system font (8x16 monospace)
#include "font_8x16.h"

/*
 */
tGlyph *_SystemFont_CacheGlyph(tFont *Font, uint32_t Codepoint)
{
	 int	i;
	uint8_t	index = 0;
	tGlyph	*ret;
	uint8_t	*data;

//	_SysDebug("_SystemFont_CacheGlyph: (Font=%p, Codepoint=0x%06x)", Font, Codepoint);
	
	if( Codepoint < 128 ) {
		index = Codepoint;
	}
	else {
		index = '?';	// Unknown glyphs come out as a question mark
	}

//	_SysDebug(" index = %i", index);

	ret = malloc( sizeof(tGlyph) + FONT_WIDTH*FONT_HEIGHT );
	if( !ret ) {
		_SysDebug("ERROR: malloc(%i) failed", sizeof(tGlyph) + FONT_WIDTH*FONT_HEIGHT);
		return NULL;
	}

	ret->Codepoint = Codepoint;

	ret->Width = FONT_WIDTH;
	ret->Height = FONT_HEIGHT;
	
	ret->TrueWidth = FONT_WIDTH;
	ret->TrueHeight = FONT_HEIGHT;

	ret->OffsetX = 0;
	ret->OffsetY = 0;
	
	data = &VTermFont[index * FONT_HEIGHT];

	for( i = 0; i < FONT_HEIGHT; i ++ )
	{
		ret->Bitmap[ i * 8 + 0 ] = data[i] & (1 << 7) ? 255 : 0;
		ret->Bitmap[ i * 8 + 1 ] = data[i] & (1 << 6) ? 255 : 0;
		ret->Bitmap[ i * 8 + 2 ] = data[i] & (1 << 5) ? 255 : 0;
		ret->Bitmap[ i * 8 + 3 ] = data[i] & (1 << 4) ? 255 : 0;
		ret->Bitmap[ i * 8 + 4 ] = data[i] & (1 << 3) ? 255 : 0;
		ret->Bitmap[ i * 8 + 5 ] = data[i] & (1 << 2) ? 255 : 0;
		ret->Bitmap[ i * 8 + 6 ] = data[i] & (1 << 1) ? 255 : 0;
		ret->Bitmap[ i * 8 + 7 ] = data[i] & (1 << 0) ? 255 : 0;
	}

	return ret;
}


/**
 * \fn int ReadUTF8(char *Input, uint32_t *Val)
 * \brief Read a UTF-8 character from a string
 */
int ReadUTF8(const char *Input, uint32_t *Val)
{
	const uint8_t	*str = (const uint8_t *)Input;
	*Val = 0xFFFD;	// Assume invalid character
	
	// ASCII
	if( !(*str & 0x80) ) {
		*Val = *str;
		return 1;
	}
	
	// Middle of a sequence
	if( (*str & 0xC0) == 0x80 ) {
		return 1;
	}
	
	// Two Byte
	if( (*str & 0xE0) == 0xC0 ) {
		*Val = (*str & 0x1F) << 6;	// Upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 2;
	}
	
	// Three Byte
	if( (*str & 0xF0) == 0xE0 ) {
		*Val = (*str & 0x0F) << 12;	// Upper 4 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 3;
	}
	
	// Four Byte
	if( (*str & 0xF1) == 0xF0 ) {
		*Val = (*str & 0x07) << 18;	// Upper 3 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 12;	// Middle-upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle-lower 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 4;
}





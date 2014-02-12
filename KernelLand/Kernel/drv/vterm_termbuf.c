/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_termbuf.c
 * - Virtual Terminal - Terminal buffer manipulation
 */
#define DEBUG	0
#define DEBUG_CHECKHEAP	0
#include "vterm.h"

#if DEBUG_CHECKHEAP
# define HEAP_VALIDATE()	Heap_Validate()
#else
# define HEAP_VALIDATE()	do{}while(0)
#endif

extern int	Term_HandleVT100(tVTerm *Term, int Len, const char *Buf);

// === CODE ===

/**
 * \fn void VT_int_PutString(tVTerm *Term, const Uint8 *Buffer, Uint Count)
 * \brief Print a string to the Virtual Terminal
 */
void VT_int_PutString(tVTerm *Term, const Uint8 *Buffer, Uint Count)
{
	// Iterate
	for( int ofs = 0; ofs < Count; )
	{
		int esc_len = Term_HandleVT100(Term, Count - ofs, (const void*)(Buffer + ofs));
		if( esc_len < 0 ) {
			esc_len = -esc_len;
			//LOG("%i '%*C'", esc_len, esc_len, Buffer+ofs);
			LOG("%i '%.*s'", esc_len, esc_len, Buffer+ofs);
			VT_int_PutRawString(Term, Buffer + ofs, esc_len);
			//Debug("Raw string '%.*s'", esc_len, Buffer+ofs);
		}
		else {
			//Debug("Escape code '%.*s'", esc_len, Buffer+ofs);
		}
		ASSERTCR(esc_len, >, 0, );
		ofs += esc_len;
	}
	// Update Screen
	VT_int_UpdateScreen( Term, 1 );
}

void VT_int_PutRawString(tVTerm *Term, const Uint8 *String, size_t Bytes)
{
	for( int i = 0; i < Bytes; ) {
		Uint32	val;
		i += ReadUTF8(String+i, &val);
		VT_int_PutChar(Term, val);
	}
}

/**
 * \fn void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
 * \brief Write a single character to a VTerm
 */
void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
{
	 int	i;
	
	HEAP_VALIDATE();
	
	size_t	limit = VT_int_GetBufferRows(Term) * Term->TextWidth;
	size_t	write_pos = *VT_int_GetWritePosPtr(Term);
	tVT_Char	*buffer = (Term->Flags & VT_FLAG_ALTBUF ? Term->AltBuf : Term->Text);

	ASSERTC(write_pos, >=, 0);

	// Scroll entire buffer (about to write outside limit)
	if( write_pos >= limit )
	{
		ASSERTC(write_pos, <, limit + Term->TextWidth);
		VT_int_ScrollText(Term, 1);
		write_pos -= Term->TextWidth;
	}

	// Bring written cell into view
	if( !(Term->Flags & VT_FLAG_ALTBUF) )
	{
		size_t	onescreen = Term->TextWidth*Term->TextHeight;
		if( write_pos >= Term->ViewPos + onescreen )
		{
			size_t	new_pos = write_pos - (write_pos % Term->TextWidth) - onescreen + Term->TextWidth;
			size_t	count = (new_pos - Term->ViewPos) / Term->TextWidth;
			VT_int_ScrollFramebuffer(Term, count);
			//Debug("VT_int_PutChar: VScroll down to %i", new_pos/Term->TextWidth);
			Term->ViewPos = new_pos;
		}
		else if( write_pos < Term->ViewPos )
		{
			size_t	new_pos = write_pos - (write_pos % Term->TextWidth);
			size_t	count = (Term->ViewPos - new_pos) / Term->TextWidth;
			VT_int_ScrollFramebuffer(Term, -count);
			//Debug("VT_int_PutChar: VScroll up to %i", new_pos/Term->TextWidth);
			Term->ViewPos = new_pos;
		}
		else
		{
			// no action, cell is visible
		}
	}
	
	switch(Ch)
	{
	case '\0':	// Ignore NULL byte
		return;
	case '\n':
		VT_int_UpdateScreen( Term, 0 );	// Update the line before newlining
		write_pos += Term->TextWidth;
		// TODO: Scroll display down if needed
	case '\r':
		write_pos -= write_pos % Term->TextWidth;
		break;
	
	case '\t': {
		size_t	col =  write_pos % Term->TextWidth;
		do {
			buffer[ write_pos ].Ch = '\0';
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos ++;
			col ++;
		} while( (col & 7) && col < Term->TextWidth );
		break; }
	
	case '\b':
		// Backspace is invalid at Offset 0
		if(write_pos == 0)	break;
		
		write_pos --;
		// Single Character
		if(buffer[ write_pos ].Ch != '\0') {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			break;
		}
		// Tab
		i = 7;	// Limit it to 8
		do {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos --;
		} while(write_pos && i-- && buffer[ write_pos ].Ch == '\0');
		if(buffer[ write_pos ].Ch != '\0')
			write_pos ++;
		break;
	
	default:
		buffer[ write_pos ].Ch = Ch;
		buffer[ write_pos ].Colour = Term->CurColour;
		// Update the line before wrapping
		if( (write_pos + 1) % Term->TextWidth == 0 )
			VT_int_UpdateScreen( Term, 0 );
		write_pos ++;
		break;
	}
	
	ASSERTC(write_pos, <=, limit);
	*VT_int_GetWritePosPtr(Term) = write_pos;

	HEAP_VALIDATE();
	
	//LEAVE('-');
}

void VT_int_ScrollText(tVTerm *Term, int Count)
{
	tVT_Char	*buf;
	 int	scroll_top, scroll_height;
	
	HEAP_VALIDATE();

	// Get buffer pointer and attributes	
	size_t	height = VT_int_GetBufferRows(Term);
	size_t	*write_pos_ptr = VT_int_GetWritePosPtr(Term);
	
	if( Term->Flags & VT_FLAG_ALTBUF )
	{
		buf = Term->AltBuf;
		scroll_top = Term->ScrollTop;
		scroll_height = Term->ScrollHeight;
	}
	else
	{
		buf = Term->Text;
		scroll_top = 0;
		scroll_height = height;
	}
	
	const int init_write_pos = *write_pos_ptr;

	// Scroll text upwards (more space at bottom)
	if( Count > 0 )
	{
		// Set up
		if(Count > scroll_height)	Count = scroll_height;
		size_t	chars = Term->TextWidth*Count;
		size_t	base = Term->TextWidth*scroll_top;
		size_t	len = Term->TextWidth*(scroll_height - Count);
		
		// Scroll terminal cache
		ASSERTC( base + chars + len, <=, Term->TextWidth*height );
		memmove( &buf[base], &buf[base+chars], len*sizeof(tVT_Char) );
		
		// Clear last rows
		for( int i = 0; i < chars; i ++ )
		{
			ASSERTC(base + len + i, <, Term->TextWidth*height);
			buf[ base + len + i ].Ch = 0;
			buf[ base + len + i ].Colour = Term->CurColour;
		}
		
		// Update Screen
		VT_int_ScrollFramebuffer( Term, Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = base;
		else
			Term->WritePos = Term->ViewPos + Term->TextWidth*(Term->TextHeight - Count);
		for( int i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
			*write_pos_ptr += Term->TextWidth;
		}
	}
	else
	{
		Count = -Count;
		if(Count > scroll_height)	Count = scroll_height;
		
		size_t	chars = Term->TextWidth*Count;
		size_t	base = Term->TextWidth*scroll_top;
		size_t	len = Term->TextWidth*scroll_height - chars;
		
		// Scroll terminal cache
		ASSERTC( base + chars + len, <=, Term->TextWidth*height );
		memmove( &buf[base+chars], &buf[base], len*sizeof(tVT_Char) );

		// Clear preceding rows
		for( int i = 0; i < chars; i ++ )
		{
			buf[ i ].Ch = 0;
			buf[ i ].Colour = Term->CurColour;
		}
		
		// Update screen (shift framebuffer, re-render revealed lines)
		VT_int_ScrollFramebuffer( Term, -Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = Term->TextWidth*scroll_top;
		else
			Term->WritePos = Term->ViewPos;
		for( int i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
		}
	}
	
	*write_pos_ptr = init_write_pos;

	HEAP_VALIDATE();
}

/**
 * \brief Clears a line in a virtual terminal
 * \param Term	Terminal to modify
 * \param Num	Line number to clear
 */
void VT_int_ClearLine(tVTerm *Term, int Row)
{
	HEAP_VALIDATE();
	
	size_t	height = VT_int_GetBufferRows(Term);
	tVT_Char	*buffer = (Term->Flags & VT_FLAG_ALTBUF ? Term->AltBuf : Term->Text);
	ASSERTCR(Row, >=, 0, );
	ASSERTCR(Row, <, height, );
	
	size_t	base = Row * Term->TextWidth;
	
	for( int i = 0; i < Term->TextWidth; i ++ )
	{
		buffer[ base + i ].Ch = 0;
		buffer[ base + i ].Colour = Term->CurColour;
	}
	
	HEAP_VALIDATE();
}

void VT_int_ClearInLine(tVTerm *Term, int Row, int FirstCol, int LastCol)
{
	HEAP_VALIDATE();
	
	size_t	height = VT_int_GetBufferRows(Term);
	tVT_Char	*buffer = (Term->Flags & VT_FLAG_ALTBUF ? Term->AltBuf : Term->Text);
	ASSERTCR(Row, >=, 0, );
	ASSERTCR(Row, <, height, );

	ASSERTCR(FirstCol, <=, LastCol, );
	ASSERTCR(FirstCol, <, Term->TextWidth, );
	ASSERTCR(LastCol, <=, Term->TextWidth, );
	
	size_t	base = Row * Term->TextWidth;
	for( int i = FirstCol; i < LastCol; i ++ )
	{
		ASSERTC(base + i, <, height * Term->TextWidth);
		buffer[ base + i ].Ch = 0;
		buffer[ base + i ].Colour = Term->CurColour;
	}
	
	HEAP_VALIDATE();
}

/**
 * \brief Update the screen mode
 * \param Term	Terminal to update
 * \param NewMode	New mode to set
 * \param NewWidth	New framebuffer width
 * \param NewHeight	New framebuffer height
 */
void VT_int_Resize(tVTerm *Term, int NewWidth, int NewHeight)
{
	 int	oldW = Term->Width;
	 int	oldTW = Term->TextWidth;
	 int	oldH = Term->Height;
	 int	oldTH = Term->TextHeight;
	tVT_Char	*oldTBuf = Term->Text;
	Uint32	*oldFB = Term->Buffer;
	 int	w, h;
	
	HEAP_VALIDATE();
	
	// TODO: Increase RealWidth/RealHeight when this happens
	if(NewWidth > giVT_RealWidth)	NewWidth = giVT_RealWidth;
	if(NewHeight > giVT_RealHeight)	NewHeight = giVT_RealHeight;
	
	// Fast exit if no resolution change
	if(NewWidth == Term->Width && NewHeight == Term->Height)
		return ;
	
	// Calculate new dimensions
	Term->Width = NewWidth;
	Term->Height = NewHeight;
	Term->TextWidth = NewWidth / giVT_CharWidth;
	Term->TextHeight = NewHeight / giVT_CharHeight;
	Term->ScrollHeight = Term->TextHeight - (oldTH - Term->ScrollHeight) - Term->ScrollTop;
	
	// Allocate new buffers
	// - Text
	Term->Text = calloc(
		Term->TextWidth * Term->TextHeight * (giVT_Scrollback+1),
		sizeof(tVT_Char)
		);
	if(oldTBuf) {
		// Copy old buffer
		w = (oldTW > Term->TextWidth) ? Term->TextWidth : oldTW;
		h = (oldTH > Term->TextHeight) ? Term->TextHeight : oldTH;
		h *= giVT_Scrollback + 1;
		for( int i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Text[i*Term->TextWidth],
				&oldTBuf[i*oldTW],
				w*sizeof(tVT_Char)
				);	
		}
		free(oldTBuf);
	}
	
	// - Alternate Text
	Term->AltBuf = realloc(
		Term->AltBuf,
		Term->TextWidth * Term->TextHeight * sizeof(tVT_Char)
		);
	
	// - Framebuffer
	if(oldFB) {
		Term->Buffer = calloc( Term->Width * Term->Height, sizeof(Uint32) );
		// Copy old buffer
		w = (oldW > Term->Width) ? Term->Width : oldW;
		h = (oldH > Term->Height) ? Term->Height : oldH;
		for( int i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Buffer[i*Term->Width],
				&oldFB[i*oldW],
				w*sizeof(Uint32)
				);
		}
		free(oldFB);
	}
	
	// Debug
	#if 0
	switch(Term->Mode)
	{
	case TERM_MODE_TEXT:
		Log_Log("VTerm", "Set VT %p to text mode (%ix%i)",
			Term, Term->TextWidth, Term->TextHeight);
		break;
	case TERM_MODE_FB:
		Log_Log("VTerm", "Set VT %p to framebuffer mode (%ix%i)",
			Term, Term->Width, Term->Height);
		break;
	//case TERM_MODE_2DACCEL:
	//case TERM_MODE_3DACCEL:
	//	return;
	}
	#endif

	HEAP_VALIDATE();
}


void VT_int_ToggleAltBuffer(tVTerm *Term, int Enabled)
{	
	if(Enabled)
		Term->Flags |= VT_FLAG_ALTBUF;
	else
		Term->Flags &= ~VT_FLAG_ALTBUF;
	VT_int_UpdateScreen(Term, 1);
}

size_t *VT_int_GetWritePosPtr(tVTerm *Term)
{
	return ((Term->Flags & VT_FLAG_ALTBUF) ? &Term->AltWritePos : &Term->WritePos);
}

size_t VT_int_GetBufferRows(tVTerm *Term)
{
	return ((Term->Flags & VT_FLAG_ALTBUF) ? 1 : (giVT_Scrollback+1))*Term->TextHeight;
}


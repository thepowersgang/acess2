/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_vt100.c
 * - Virtual Terminal - VT100 (Kinda) Emulation
 */
#include "vterm.h"

// === CONSTANTS ===
const Uint16	caVT100Colours[] = {
		// Black, Red, Green, Yellow, Blue, Purple, Cyan, Gray
		// Same again, but bright
		VT_COL_BLACK, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0xAAA,
		VT_COL_GREY, 0xF00, 0x0F0, 0xFF0, 0x00F, 0xF0F, 0x0FF, VT_COL_WHITE
	};

// === CODE ===
/**
 * \brief Handle a standard large escape code
 * 
 * Handles any escape code of the form \x1B[n,...A where n is an integer
 * and A is any letter.
 */
void VT_int_ParseEscape_StandardLarge(tVTerm *Term, char CmdChar, int argc, int *args)
{
	 int	tmp = 1;
	switch(CmdChar)
	{
	// Left
	case 'D':
		tmp = -1;
	// Right
	case 'C':
		if(argc == 1)	tmp *= args[0];
		if( Term->Flags & VT_FLAG_ALTBUF )
		{
			if( (Term->AltWritePos + tmp) % Term->TextWidth == 0 ) {
				Term->AltWritePos -= Term->AltWritePos % Term->TextWidth;
				Term->AltWritePos += Term->TextWidth - 1;
			}
			else
				Term->AltWritePos += tmp;
		}
		else
		{
			if( (Term->WritePos + tmp) % Term->TextWidth == 0 ) {
				Term->WritePos -= Term->WritePos % Term->TextWidth;
				Term->WritePos += Term->TextWidth - 1;
			}
			else
				Term->WritePos += tmp;
		}
		break;
	
	// Erase
	case 'J':
		switch(args[0])
		{
		case 0:	// Erase below
			break;
		case 1:	// Erase above
			break;
		case 2:	// Erase all
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i = Term->TextHeight;
				while( i-- )	VT_int_ClearLine(Term, i);
				Term->AltWritePos = 0;
				VT_int_UpdateScreen(Term, 1);
			}
			else
			{
				 int	i = Term->TextHeight * (giVT_Scrollback + 1);
				while( i-- )	VT_int_ClearLine(Term, i);
				Term->WritePos = 0;
				Term->ViewPos = 0;
				VT_int_UpdateScreen(Term, 1);
			}
			break;
		}
		break;
	
	// Erase in line
	case 'K':
		switch(args[0])
		{
		case 0:	// Erase to right
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i, max;
				max = Term->Width - Term->AltWritePos % Term->Width;
				for( i = 0; i < max; i ++ )
					Term->AltBuf[Term->AltWritePos+i].Ch = 0;
			}
			else
			{
				 int	i, max;
				max = Term->Width - Term->WritePos % Term->Width;
				for( i = 0; i < max; i ++ )
					Term->Text[Term->WritePos+i].Ch = 0;
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		case 1:	// Erase to left
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i = Term->AltWritePos % Term->Width;
				while( i -- )
					Term->AltBuf[Term->AltWritePos++].Ch = 0;
			}
			else
			{
				 int	i = Term->WritePos % Term->Width;
				while( i -- )
					Term->Text[Term->WritePos++].Ch = 0;
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		case 2:	// Erase all
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				VT_int_ClearLine(Term, Term->AltWritePos / Term->Width);
			}
			else
			{
				VT_int_ClearLine(Term, Term->WritePos / Term->Width);
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		}
		break;
	
	// Set cursor position
	case 'H':
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = args[0] + args[1]*Term->TextWidth;
		else
			Term->WritePos = args[0] + args[1]*Term->TextWidth;
		//Log_Debug("VTerm", "args = {%i, %i}", args[0], args[1]);
		break;
	
	// Scroll up `n` lines
	case 'S':
		tmp = -1;
	// Scroll down `n` lines
	case 'T':
		if(argc == 1)	tmp *= args[0];
		if( Term->Flags & VT_FLAG_ALTBUF )
			VT_int_ScrollText(Term, tmp);
		else
		{
			if(Term->ViewPos/Term->TextWidth + tmp < 0)
				break;
			if(Term->ViewPos/Term->TextWidth + tmp  > Term->TextHeight * (giVT_Scrollback + 1))
				break;
			
			Term->ViewPos += Term->TextWidth*tmp;
		}
		break;
	
	// Set Font flags
	case 'm':
		for( ; argc--; )
		{
			 int	colour_idx;
			// Flags
			if( 0 <= args[argc] && args[argc] <= 8)
			{
				switch(args[argc])
				{
				case 0:	Term->CurColour = DEFAULT_COLOUR;	break;	// Reset
				case 1:	Term->CurColour |= 0x80000000;	break;	// Bright
				case 2:	Term->CurColour &= ~0x80000000;	break;	// Dim
				}
			}
			// Foreground Colour
			else if(30 <= args[argc] && args[argc] <= 37) {
				// Get colour index, accounting for bright bit
				colour_idx = args[argc]-30 + ((Term->CurColour>>28) & 8);
				Term->CurColour &= 0x8000FFFF;
				Term->CurColour |= (Uint32)caVT100Colours[ colour_idx ] << 16;
			}
			// Background Colour
			else if(40 <= args[argc] && args[argc] <= 47) {
				// Get colour index, accounting for bright bit
				colour_idx = args[argc]-40 + ((Term->CurColour>>12) & 8);
				Term->CurColour &= 0xFFFF8000;
				Term->CurColour |= caVT100Colours[ colour_idx ];
			}
			// Foreground Colour - bright
			else if(90 <= args[argc] && args[argc] <= 97 ) {
				colour_idx = args[argc]-90 + 8;
				Term->CurColour &= 0x8000FFFF;
				Term->CurColour |= (Uint32)caVT100Colours[ colour_idx ] << 16;
			}
			// Background Colour - bright
			else if(100 <= args[argc] && args[argc] <= 107 ) {
				colour_idx = args[argc]-100 + 8;
				Term->CurColour &= 0xFFFF8000;
				Term->CurColour |= (Uint32)caVT100Colours[ colour_idx ];
			}
			else {
				Log_Warning("VTerm", "Unknown font flag %i", args[argc]);
			}
		}
		break;
	
	// Set scrolling region
	case 'r':
		if( argc != 2 )	break;
		Term->ScrollTop = args[0];
		Term->ScrollHeight = args[1] - args[0];
		break;
	
	default:
		Log_Warning("VTerm", "Unknown control sequence '\\x1B[%c'", CmdChar);
		break;
	}
}

/**
 * \fn int VT_int_ParseEscape(tVTerm *Term, const char *Buffer)
 * \brief Parses a VT100 Escape code
 */
int VT_int_ParseEscape(tVTerm *Term, const char *Buffer, size_t Bytes)
{
	char	c;
	 int	argc = 0, j = 0;
	 int	args[6] = {0,0,0,0};
	 int	bQuestionMark = 0;

	if( Bytes == j )	return j;
	c = Buffer[j++];

	switch(c)
	{
	//Large Code
	case '[':
		// Get Arguments
		if(Bytes == j)	return j;
		c = Buffer[j++];
		if(c == '?') {
			bQuestionMark = 1;
			if(Bytes == j)	return j;
			c = Buffer[j++];
		}
		if( '0' <= c && c <= '9' )
		{
			do {
				if(c == ';') {
					if(Bytes == j)	return j;
					c = Buffer[j++];
				}
				while('0' <= c && c <= '9') {
					args[argc] *= 10;
					args[argc] += c-'0';
					if(Bytes == j)	return j;
					c = Buffer[j++];
				}
				argc ++;
			} while(c == ';');
		}
		
		// Get Command
		if( !('a' <= c && c <= 'z') && !('A' <= c && c <= 'Z') )
		{
			// Error
			return j;
		}
		
		if( bQuestionMark )
		{
			switch(c)
			{
			// DEC Private Mode Set
			case 'h':
				if(argc != 1)	break;
				switch(args[0])
				{
				case 25:
					Term->Flags &= ~VT_FLAG_HIDECSR;
					break;
				case 1047:
					VT_int_ToggleAltBuffer(Term, 1);
					break;
				}
				break;
			case 'l':
				if(argc != 1)	break;
				switch(args[0])
				{
				case 25:
					Term->Flags |= VT_FLAG_HIDECSR;
					break;
				case 1047:
					VT_int_ToggleAltBuffer(Term, 0);
					break;
				}
				break;
			default:
				Log_Warning("VTerm", "Unknown control sequence '\\x1B[?%c'", c);
				break;
			}
		}
		else
		{
			VT_int_ParseEscape_StandardLarge(Term, c, argc, args);
		}
		break;
	case '\0':
		// Ignore \0
		break;
	default:
		//Log_Notice("VTerm", "TODO: Handle short escape codes");
		{
			static volatile int tmp = 0;
			if(tmp == 0) {
				tmp = 1;
				Debug("VTerm: Unknown short 0x%x", c);
				tmp = 0;
			}
		}
		break;
	}
	
	//Log_Debug("VTerm", "j = %i, Buffer = '%s'", j, Buffer);
	return j;
}

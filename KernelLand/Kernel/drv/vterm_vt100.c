/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_vt100.c
 * - Virtual Terminal - VT100 (Kinda) Emulation
 */
#define DEBUG	0
#include "vterm.h"

#define sTerminal	sVTerm
#include "../../../Usermode/Applications/gui_terminal_src/vt100.c"

void *Display_GetTermState(tTerminal *Term) {
	return Term->VT100Info;;
}
void Display_SetTermState(tTerminal *Term, void *State) {
	Term->VT100Info = State;
}

void Display_AddText(tTerminal *Term, size_t Length, const char *UTF8Text)
{
	LOG("'%*C'", Length, UTF8Text);
	VT_int_PutRawString(Term, (const void*)UTF8Text, Length);
}
void Display_Newline(tTerminal *Term, bool bCarriageReturn)
{
	LOG("");
	VT_int_PutChar(Term, '\n');
}
void Display_SetScrollArea(tTerminal *Term, int Start, int Count)
{
	LOG("(%i,+%i)", Start, Count);
	Term->ScrollTop = Start;
	Term->ScrollHeight = Count;
}
void Display_ScrollDown(tTerminal *Term, int CountDown)
{
	LOG("(%i)", CountDown);
	VT_int_UpdateScreen(Term, 0);
	if( Term->Flags & VT_FLAG_ALTBUF )
		VT_int_ScrollText(Term, CountDown);
	else
	{
		if(Term->ViewPos/Term->TextWidth + CountDown < 0)
			return ;
		if(Term->ViewPos/Term->TextWidth + CountDown  > Term->TextHeight * (giVT_Scrollback + 1))
			return ;
		
		Term->ViewPos += Term->TextWidth * CountDown;
	}
}
void Display_SetCursor(tTerminal *Term, int Row, int Col)
{
	LOG("(R%i,C%i)", Row, Col);
	VT_int_UpdateScreen(Term, 0);
	 int	maxrows = ((Term->Flags & VT_FLAG_ALTBUF) ? 1 : (giVT_Scrollback+1))*Term->TextHeight;
	ASSERTCR( Row, >=, 0, );
	ASSERTCR( Row, <, maxrows, );
	ASSERTCR( Col, >=, 0, );
	ASSERTCR( Col, <, Term->TextWidth, );
	*(Term->Flags & VT_FLAG_ALTBUF ? &Term->AltWritePos : &Term->WritePos) = Row*Term->TextWidth + Col;
}
void Display_MoveCursor(tTerminal *Term, int RelRow, int RelCol)
{
	LOG("(R+%i,C+%i)", RelRow, RelCol);
	int	*wrpos = (Term->Flags & VT_FLAG_ALTBUF ? &Term->AltWritePos : &Term->WritePos);

	if( RelCol != 0 )
	{
		// 
		if( RelCol < 0 )
		{
			 int	avail = *wrpos % Term->TextWidth;
			if( RelCol < -avail )
				RelCol = -avail;
		}
		else
		{
			size_t	avail = Term->TextWidth - (*wrpos % Term->TextWidth);
			if(RelCol > avail)
				RelCol = avail;
		}
		*wrpos += RelCol;
	}
	if( RelRow != 0 )
	{
		 int	currow = *wrpos / Term->TextWidth;
		 int	maxrows = ((Term->Flags & VT_FLAG_ALTBUF) ? 1 : (giVT_Scrollback+1))*Term->TextHeight;
		if( RelRow < 0 )
		{
			if( RelRow < -currow )
				RelRow = -currow;
		}
		else
		{
			if( currow + RelRow > maxrows-1 )
				RelRow = maxrows-1 - currow;
		}
		*wrpos += RelRow*Term->TextWidth;
	}
	LOG("=(R%i,C%i)", *wrpos / Term->TextWidth, *wrpos % Term->TextWidth);
}
void Display_SaveCursor(tTerminal *Term)
{
	Term->SavedWritePos = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltWritePos : Term->WritePos;
	LOG("Saved = %i", Term->SavedWritePos);
}
void Display_RestoreCursor(tTerminal *Term)
{
	 int	max = ((Term->Flags & VT_FLAG_ALTBUF) ? 1 : (giVT_Scrollback+1))*Term->TextHeight * Term->TextWidth;
	 int	*wrpos = ((Term->Flags & VT_FLAG_ALTBUF) ? &Term->AltWritePos : &Term->WritePos);
	if( Term->SavedWritePos >= max )
		*wrpos = max-1;
	else
		*wrpos = Term->SavedWritePos;
	LOG("Restored %i", *wrpos);
}
// 0: All, 1: Forward, -1: Reverse
void Display_ClearLine(tTerminal *Term, int Dir)
{
	 int	*wrpos = (Term->Flags & VT_FLAG_ALTBUF ? &Term->AltWritePos : &Term->WritePos);
	tVT_Char	*buffer = (Term->Flags & VT_FLAG_ALTBUF ? Term->AltBuf : Term->Text);

	LOG("(Dir=%i)", Dir);	

	// Erase all
	if( Dir == 0 ) {
		VT_int_ClearLine(Term, *wrpos / Term->Width);
		VT_int_UpdateScreen(Term, 0);
	}
	// Erase to right
	else if( Dir == 1 ) {
		int max = Term->Width - *wrpos % Term->Width;
		for( int i = 0; i < max; i ++ )
			buffer[*wrpos+i].Ch = 0;
		VT_int_UpdateScreen(Term, 0);
	}
	// Erase to left
	else if( Dir == -1 ) {
		int start = *wrpos - *wrpos % Term->Width;
		for( int i = start; i < *wrpos; i ++ )
			buffer[i].Ch = 0;
		VT_int_UpdateScreen(Term, 0);
	}
	else {
		// ERROR!
		ASSERTC(Dir, >=, -1);
		ASSERTC(Dir, <=, 1);
	}
}
void Display_ClearLines(tTerminal *Term, int Dir)
{
	LOG("(Dir=%i)", Dir);	
	 int	*wrpos = (Term->Flags & VT_FLAG_ALTBUF ? &Term->AltWritePos : &Term->WritePos);
	
	// All
	if( Dir == 0 ) {
		int count;
		
		if( Term->Flags & VT_FLAG_ALTBUF ) {
			count = Term->TextHeight;
		}
		else {
			count = Term->TextHeight * (giVT_Scrollback + 1);
			Term->ViewPos = 0;
		}
		while( count -- )
			VT_int_ClearLine(Term, count);
		*wrpos = 0;
		VT_int_UpdateScreen(Term, 1);
	}
	// Downwards
	else if( Dir == 1 ) {
		Log_Warning("VTerm", "TODO: ClearLines Down");
	}
	// Upwards
	else if( Dir == -1 ) {
		Log_Warning("VTerm", "TODO: ClearLines Up");
	}
	else {
		// ERROR!
		ASSERTC(Dir, >=, -1);
		ASSERTC(Dir, <=, 1);
	}
}
void Display_ResetAttributes(tTerminal *Term)
{
	LOG("");	
	Term->CurColour = DEFAULT_COLOUR;
}
void Display_SetForeground(tTerminal *Term, uint32_t RGB)
{
	LOG("(%06x)", RGB);
	Term->CurColour &= 0x8000FFFF;
	Term->CurColour |= (Uint32)VT_Colour24to12(RGB) << 16;
	
}
void Display_SetBackground(tTerminal *Term, uint32_t RGB)
{
	LOG("(%06x)", RGB);
	Term->CurColour &= 0xFFFF8000;
	Term->CurColour |= (Uint32)VT_Colour24to12(RGB) <<  0;
}
void Display_Flush(tTerminal *Term)
{
	LOG("");
	VT_int_UpdateScreen(Term, 0);
}
void Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled)
{
	LOG("(%B)", AltBufEnabled);
	VT_int_ToggleAltBuffer(Term, AltBufEnabled);
}
void Display_SetTitle(tTerminal *Term, const char *Title)
{
	// ignore
}


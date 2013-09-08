/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * display.c
 * - Abstract display manipulation methods
 */
#include "include/display.h"
#include <acess/sys.h>	// _SysDebug
#include <stdlib.h>	// exit
#include <string.h>
#include <unicode.h>
#include <stdio.h>
#include <axwin3/axwin.h>
#include <axwin3/richtext.h>
#include <stdbool.h>
#include <assert.h>

#define UNIMPLIMENTED()	do{_SysDebug("UNIMPLIMENTED %s", __func__); exit(-1);}while(0)

// === EXTERN ==
extern tHWND	gMainWindow;

typedef struct sLine	tLine;

struct sLine {
	char	*Data;
	// TODO: Cache offsets to avoid scan-forward
	size_t	Len;
	size_t	Size;
	bool	IsDirty;
};

struct sTerminal {
	 int	ViewCols;
	 int	ViewRows;

	 int	CursorRow;
	 int	CursorCol;

	size_t	CursorByte;

	bool	bUsingAltBuf;

	size_t	ViewOffset;	
	size_t	TotalLines;
	struct sLine	*PriBuf;
	
	struct sLine	*AltBuf;
};

// === GLOBALS ===
tTerminal	gMainBuffer;
 int	giCurrentLine;
 int	giCurrentLinePos;	// byte offset, not column
 int	giCurrentCol;
 int	giFirstDispLine;	// First displayed line
 int	giFirstLine;	// Ring buffer start
char	**gasDisplayLines;
 int	*gaiDisplayLineSizes;
char	*gabDisplayLinesDirty;

// === CODE ===
tTerminal *Display_Init(int Cols, int Lines, int ExtraScrollbackLines)
{
	tTerminal	*term = &gMainBuffer;
	term->ViewCols = Cols;
	term->ViewRows = Lines;
	term->TotalLines = Lines + ExtraScrollbackLines;
	term->PriBuf = calloc( sizeof(tLine), (Lines + ExtraScrollbackLines) );
	
	AxWin3_RichText_SetLineCount(gMainWindow, Lines+ExtraScrollbackLines);
	AxWin3_RichText_SetCursorType(gMainWindow, 1);	// TODO: enum
	return term;
}

// Return the byte length of a single on-screen character
size_t _GetCharLength(size_t AvailLength, const char *Text, uint32_t *BaseCodepoint)
{
	if( !AvailLength )
		return 0;
	
	size_t	charlen = ReadUTF8(Text, BaseCodepoint);
	
	while(charlen < AvailLength)
	{
		uint32_t	cp;
		size_t size = ReadUTF8(Text+charlen, &cp);
		if( Unicode_IsPrinting(cp) )
			break;
		charlen += size;
	}
	
	return charlen;
}

tLine *Display_int_GetCurLine(tTerminal *Term)
{
	 int	lineidx = Term->CursorRow + (Term->bUsingAltBuf ? 0 : Term->ViewOffset);
	return (Term->bUsingAltBuf ? Term->AltBuf : Term->PriBuf) + lineidx;
}

size_t Display_int_PushCharacter(tTerminal *Term, size_t AvailLength, const char *Text)
{
	tLine	*lineptr = Display_int_GetCurLine(Term);
	uint32_t	cp;
	size_t	charlen = _GetCharLength(AvailLength, Text, &cp);
	bool	bOverwrite = Unicode_IsPrinting(cp);
	
	// Figure out how much we need to shift the stream
	 int	shift;
	if( bOverwrite ) {
		size_t nextlen = _GetCharLength(
			lineptr->Len-Term->CursorByte,
			lineptr->Data+Term->CursorByte,
			NULL);
		shift = charlen - nextlen;
	}
	else {
		shift = charlen;
	}
	
	// Ensure we have space enough
	if( !lineptr->Data || shift > 0 ) {
		const size_t	size_step = 64;
		assert(shift > 0);
		lineptr->Size = (lineptr->Len+shift+1 + size_step-1) & ~(size_step-1);
		void *tmp = realloc(lineptr->Data, lineptr->Size);
		if( !tmp )	perror("Display_int_PushCharacter - realloc");
		lineptr->Data = tmp;
	}
	
	// Insert into stream
	char	*base = lineptr->Data + Term->CursorByte;
	if( Term->CursorByte == lineptr->Len ) {
		// No shifting needed
	}
	else if( shift >= 0 ) {
		size_t	bytes = lineptr->Len - (Term->CursorByte+shift);
		memmove(base+shift, base, bytes);
	}
	else {
		shift = -shift;
		size_t	bytes = lineptr->Len - (Term->CursorByte+shift);
		memmove(base, base+shift, bytes);
	}
	memcpy(base, Text, charlen);
	lineptr->IsDirty = true;
	lineptr->Len += shift;
	lineptr->Data[lineptr->Len] = 0;	// NULL Terminate

	Term->CursorByte += charlen;
	
	// HACKY: Prevents the CursorCol++ in Display_AddText from having an effect
	if( !bOverwrite )
		Term->CursorCol --;
	
	return charlen;
}

void Display_AddText(tTerminal *Term, size_t Length, const char *UTF8Text)
{
	//_SysDebug("%i '%.*s'", Length, Length, UTF8Text);
	while( Length > 0 )
	{
		size_t used = Display_int_PushCharacter(Term, Length, UTF8Text);
	
		Length -= used;
		UTF8Text += used;
		
		Term->CursorCol ++;
		if( Term->CursorCol == Term->ViewCols ) {
			Display_Newline(Term, 1);
		}
	}
}

void Display_Newline(tTerminal *Term, bool bCarriageReturn)
{
//	Display_Flush();

	// Going down!
	Term->CursorRow ++;
	if( Term->CursorRow == Term->TotalLines ) {
		// TODO: Scrolling
	}
	
	if( bCarriageReturn ) {
		Term->CursorByte = 0;
		Term->CursorCol = 0;
		return ;
	}

	tLine	*line = Display_int_GetCurLine(Term);
	
	Term->CursorByte = 0;
	 int	old_col = Term->CursorCol;
	Term->CursorCol = 0;

	size_t	ofs = 0;
	while( Term->CursorCol < old_col && ofs < line->Len ) {
		ofs += _GetCharLength(line->Len-ofs, line->Data+ofs, NULL);
		Term->CursorCol ++;
	}
	Term->CursorByte = ofs;

	while( Term->CursorCol < old_col )
		Display_AddText(Term, 1, " ");
}

void Display_SetCursor(tTerminal *Term, int Row, int Col)
{
	UNIMPLIMENTED();
}

void Display_MoveCursor(tTerminal *Term, int RelRow, int RelCol)
{
	if( RelRow != 0 )
	{
		UNIMPLIMENTED();
	}
	
	if( RelCol != 0 )
	{
		int req_col = Term->CursorCol + RelCol;
		if( req_col < 0 )	req_col = 0;
		if( req_col > Term->ViewCols )	req_col = Term->ViewCols;

		tLine	*line = Display_int_GetCurLine(Term);
		size_t	ofs = 0;
		for( int i = 0; i < req_col; i ++ )
		{
			size_t clen = _GetCharLength(line->Len-ofs, line->Data+ofs, NULL);
			if( clen == 0 ) {
				req_col = i;
				break;
			}
			ofs += clen;
		}

		Term->CursorCol = req_col;
		Term->CursorByte = ofs;
	}
}

void Display_ClearLine(tTerminal *Term, int Dir)	// 0: All, 1: Forward, -1: Reverse
{
	if( Dir == 0 )
	{
		tLine	*line = Display_int_GetCurLine(Term);
		// Completely clear line
		if( line->Data )
			free(line->Data);
		line->Data = NULL;
		line->IsDirty = true;
	}
	else if( Dir == 1 )
	{
		// Forward clear (truncate)
	}
	else if( Dir == -1 )
	{
		// Reverse clear (replace with spaces)
	}
	else
	{
		// BUGCHECK
	}
}

void Display_ClearLines(tTerminal *Term, int Dir)	// 0: All, 1: Forward, -1: Reverse
{
	if( Dir == 0 )
	{
		// Push giDisplayLines worth of empty lines
		// Move cursor back up by giDisplayLines
	}
	else if( Dir == 1 )
	{
		// Push (giDisplayLines - (giCurrentLine-giFirstDispLine)) and reverse
	}
	else if( Dir == -1 )
	{
		// Reverse clear (replace with spaces)
	}
	else
	{
		// BUGCHECK
	}
}

void Display_SetForeground(tTerminal *Term, uint32_t RGB)
{
	char	buf[7+1];
	sprintf(buf, "\1%06x", RGB&0xFFFFFF);
	Display_AddText(Term, 7, buf);
}

void Display_SetBackground(tTerminal *Term, uint32_t RGB)
{
	char	buf[7+1];
	sprintf(buf, "\2%06x", RGB&0xFFFFFF);
	Display_AddText(Term, 7, buf);
}

void Display_Flush(tTerminal *Term)
{
	for( int i = 0; i < Term->ViewRows; i ++ )
	{
		 int	line = (Term->ViewOffset + i) % Term->TotalLines;
		tLine	*lineptr = &Term->PriBuf[line];
		if( !lineptr->IsDirty )
			continue;
		_SysDebug("Line %i+%i '%.*s'", Term->ViewOffset, i, lineptr->Len, lineptr->Data);
		AxWin3_RichText_SendLine(gMainWindow, Term->ViewOffset + i, lineptr->Data );
		lineptr->IsDirty = 0;
	}
	AxWin3_RichText_SetCursorPos(gMainWindow, Term->CursorRow, Term->CursorCol);
}

void Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled)
{
	UNIMPLIMENTED();
}


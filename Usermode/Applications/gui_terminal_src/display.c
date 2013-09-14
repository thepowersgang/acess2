/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * display.c
 * - Abstract display manipulation methods
 */
#define DEBUG	0
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

#if DEBUG
# define DEBUGS(v...)	_SysDebug(v)
#else
# define DEBUGS(v...)	do{}while(0)
#endif

#define UNIMPLIMENTED()	do{_SysDebug("UNIMPLIMENTED %s", __func__); exit(-1);}while(0)

static inline int MIN(int a, int b) { return (a < b ? a : b); }
static inline int MAX(int a, int b) { return (a > b ? a : b); }

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

	 int	ScrollTop;
	 int	ScrollRows;

	 int	CursorRow;
	 int	CursorCol;

	 int	SavedRow;
	 int	SavedCol;

	size_t	CursorByte;

	bool	bUsingAltBuf;
	bool	bHaveSwappedBuffers;
	
	 int	OtherBufRow;
	 int	OtherBufCol;

	size_t	ViewOffset;	
	size_t	TotalLines;
	struct sLine	*PriBuf;
	
	struct sLine	*AltBuf;
};

// === PROTOTYPES ===
void	Display_int_SetCursor(tTerminal *Term, int Row, int Col);

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
	//_SysDebug("lineidx = %i", lineidx);
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
		//_SysDebug("GetCharLen(%i-%i, %p+%i, NULL)", lineptr->Len, Term->CursorByte,
		//	lineptr->Data, Term->CursorByte);
		if( Term->CursorByte )
			assert(lineptr->Data);
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
		size_t	newsize = (lineptr->Len+shift+1 + size_step-1) & ~(size_step-1);
		if( newsize > lineptr->Size ) {
			lineptr->Size = newsize;
			void *tmp = realloc(lineptr->Data, lineptr->Size);
			if( !tmp )	perror("Display_int_PushCharacter - realloc");
			//_SysDebug("realloc gave %p from %p for line %i", tmp, lineptr->Data,
			//	Term->CursorRow);
			lineptr->Data = tmp;
		}
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
	DEBUGS("%i += %i '%.*s'", Term->CursorRow, Length, Length, UTF8Text);
	while( Length > 0 )
	{
		if( Term->CursorCol == Term->ViewCols ) {
			Display_Newline(Term, 1);
		}
		size_t used = Display_int_PushCharacter(Term, Length, UTF8Text);
	
		Length -= used;
		UTF8Text += used;
		
		Term->CursorCol ++;
	}
}

void Display_Newline(tTerminal *Term, bool bCarriageReturn)
{
	// Going down!
	if( Term->bUsingAltBuf )
	{
		if( Term->CursorRow == Term->ScrollTop + Term->ScrollRows-1 ) {
			Display_ScrollDown(Term, 1);
		}
		else if( Term->CursorRow == Term->ViewRows-1 ) {
			if( Term->ScrollRows == 0 ) {
				// Scroll entire buffer
				Display_ScrollDown(Term, 1);
			}
			else {
				// Don't go down a line
			}
		}
		else {
			// No scroll needed
			Term->CursorRow ++;
		}
	}
	else
	{
		if( Term->CursorRow == Term->TotalLines-1 ) {
			Display_ScrollDown(Term, 1);
		}
		else {
			Term->CursorRow ++;
		}
	}
	
	if( bCarriageReturn )
	{
		Term->CursorByte = 0;
		Term->CursorCol = 0;
	}
	else
	{
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
}

void Display_SetScrollArea(tTerminal *Term, int Start, int Count)
{
	assert(Start >= 0);
	assert(Count >= 0);
	Term->ScrollTop = Start;
	Term->ScrollRows = MIN(Count, Term->ViewRows - Start);
}

void Display_ScrollDown(tTerminal *Term, int Count)
{
	 int	top, max;
	tLine	*buffer;
	
	if( Term->bUsingAltBuf )
	{
		top = (Term->ScrollRows == 0 ? 0 : Term->ScrollTop);
		max = (Term->ScrollRows == 0 ? Term->ViewRows : Term->ScrollRows);
		buffer = Term->AltBuf;
	}
	else
	{
		top = 0;
		max = Term->TotalLines;
		buffer = Term->PriBuf;
	}
	
	assert(Count < max);
	assert(Count > -max);

	DEBUGS("Scroll %p %i-%i down by %i", buffer, top, max, Count);
	
	buffer += top;

	 int	clear_top, clear_max;
	if( Count < 0 )
	{
		// -ve: Scroll up, move buffer contents down
		Count = -Count;
		for( int i = max-Count; i < max; i ++ )
			free(buffer[i].Data);
		memmove(buffer+Count, buffer, (max-Count)*sizeof(*buffer));
		
		clear_top = 0;
		clear_max = Count;
	}
	else
	{
		for( int i = 0; i < Count; i ++ )
			free(buffer[i].Data);
		memmove(buffer, buffer+Count, (max-Count)*sizeof(*buffer));
		clear_top = max-Count;
		clear_max = max;
	}
	// Clear exposed lines
	for( int i = clear_top; i < clear_max; i ++ )
	{
		buffer[i].Data = NULL;
		buffer[i].Len = 0;
		buffer[i].Size = 0;
		buffer[i].IsDirty = true;
	}
	// Send scroll command to GUI
	AxWin3_RichText_ScrollRange(gMainWindow, top, max, Count);
	
	Display_int_SetCursor(Term, Term->CursorRow, Term->CursorCol);
}

void Display_SetCursor(tTerminal *Term, int Row, int Col)
{
	assert(Row >= 0);
	assert(Col >= 0);

	DEBUGS("Set cursor R%i,C%i", Row, Col);	

	if( !Term->bUsingAltBuf ) {
		_SysDebug("NOTE: Using \\e[%i;%iH outside of alternat buffer is undefined", Row, Col);
	}
	
	// NOTE: This may be interesting outside of AltBuffer
	Display_int_SetCursor(Term, Row, Col);	
}
void Display_int_SetCursor(tTerminal *Term, int Row, int Col)
{
	Term->CursorRow = Row;
	tLine	*line = Display_int_GetCurLine(Term);
	size_t ofs = 0;
	 int	i;
	for( i = 0; i < Col; i ++ )
	{
	
		size_t clen = _GetCharLength(line->Len-ofs, line->Data+ofs, NULL);
		if( clen == 0 ) {
			break;
		}
		ofs += clen;
	}
	Term->CursorCol = i;
	Term->CursorByte = ofs;
	// Move to exactly the column specified
	for( ; i < Col; i ++ ) {
		Display_int_PushCharacter(Term, 1, " ");
		Term->CursorCol ++;
	}
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

void Display_SaveCursor(tTerminal *Term)
{
	Term->SavedRow = Term->CursorRow;
	Term->SavedCol = Term->CursorCol;
}
void Display_RestoreCursor(tTerminal *Term)
{
	Display_SetCursor(Term, Term->SavedRow, Term->SavedCol);
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
		Term->CursorCol = 0;
		Term->CursorByte = 0;
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

void Display_ResetAttributes(tTerminal *Term)
{
	UNIMPLIMENTED();
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
	 int	viewOfs = (Term->bUsingAltBuf ? 0 : Term->ViewOffset);
	tLine	*buffer = (Term->bUsingAltBuf ? Term->AltBuf : Term->PriBuf );
	AxWin3_RichText_SetLineCount(gMainWindow, (Term->bUsingAltBuf ? Term->ViewRows : Term->TotalLines));
	for( int i = 0; i < Term->ViewRows; i ++ )
	{
		 int	line = (viewOfs + i) % Term->TotalLines;
		tLine	*lineptr = &buffer[line];
		// Swapping buffers should cause a full resend
		if( !Term->bHaveSwappedBuffers && !lineptr->IsDirty )
			continue;
		DEBUGS("Line %i+%i %p '%.*s'", viewOfs, i, lineptr->Data, lineptr->Len, lineptr->Data);
		AxWin3_RichText_SendLine(gMainWindow, viewOfs + i,
			lineptr->Data ? lineptr->Data : "" );
		lineptr->IsDirty = false;
	}
	AxWin3_RichText_SetCursorPos(gMainWindow, Term->CursorRow, Term->CursorCol);
	Term->bHaveSwappedBuffers = false;
}

void Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled)
{
	if( Term->bUsingAltBuf == AltBufEnabled )
	{
		// Nothing to do, so do nothing
		return ;
	}

	int row = Term->OtherBufRow;
	int col = Term->OtherBufCol;
	Term->OtherBufRow = Term->CursorRow;
	Term->OtherBufCol = Term->CursorCol;
	
	Term->bUsingAltBuf = AltBufEnabled;
	Term->bHaveSwappedBuffers = true;
	if( AltBufEnabled )
	{
		if( !Term->AltBuf )
		{
			Term->AltBuf = calloc( sizeof(Term->AltBuf[0]), Term->ViewRows );
		}
		AxWin3_RichText_SetLineCount(gMainWindow, Term->ViewRows);
	}
	else
	{
		AxWin3_RichText_SetLineCount(gMainWindow, Term->TotalLines);
	}
	Display_int_SetCursor(Term, row, col);
}


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

#define UNIMPLIMENTED()	do{_SysDebug("UNIMPLIMENTED %s", __func__); exit(-1);}while(0)

// === EXTERN ==
extern tHWND	gMainWindow;

// === GLOBALS ===
 int	giDisplayCols;
 int	giDisplayLines;
 int	giDisplayTotalLines;
 int	giDisplayBufSize;
 int	giCurrentLine;
 int	giCurrentLinePos;	// byte offset, not column
 int	giCurrentCol;
 int	giFirstDispLine;	// First displayed line
 int	giFirstLine;	// Ring buffer start
char	**gasDisplayLines;
 int	*gaiDisplayLineSizes;
char	*gabDisplayLinesDirty;

// === CODE ===
void Display_Init(int Cols, int Lines, int ExtraScrollbackLines)
{
	giDisplayCols = Cols;
	giDisplayLines = Lines;
	giDisplayTotalLines = Lines + ExtraScrollbackLines;
	gasDisplayLines = calloc( sizeof(char*), (Lines + ExtraScrollbackLines) );
	gaiDisplayLineSizes = calloc( sizeof(int), (Lines + ExtraScrollbackLines) );
	gabDisplayLinesDirty = calloc( sizeof(char), (Lines + ExtraScrollbackLines) );
}

void Display_int_PushString(int Length, const char *Text)
{
	_SysDebug("Line %i += %i '%*C'", giCurrentLine, Length, Length, Text);
	if( !gasDisplayLines[giCurrentLine] || giCurrentLinePos + Length >= gaiDisplayLineSizes[giCurrentLine] )
	{
		 int	reqsize = giCurrentLinePos + Length;
		gaiDisplayLineSizes[giCurrentLine] = (reqsize + 32-1) & ~(32-1);
		void *tmp = realloc(gasDisplayLines[giCurrentLine], gaiDisplayLineSizes[giCurrentLine]);
		if( !tmp )	perror("Display_AddText - realloc");
		gasDisplayLines[giCurrentLine] = tmp;
	}

	memcpy(gasDisplayLines[giCurrentLine]+giCurrentLinePos, Text, Length);
	gabDisplayLinesDirty[giCurrentLine] = 1;
	gasDisplayLines[giCurrentLine][giCurrentLinePos+Length] = 0;
	
}

void Display_AddText(int Length, const char *UTF8Text)
{
	_SysDebug("%i '%.*s'", Length, Length, UTF8Text);
	// Copy as many characters (not bytes, have to trim off the last char) as we can to the current line
	// - then roll over to the next line
	while( Length > 0 )
	{
		 int	space = giDisplayCols - giCurrentCol;
		 int	bytes = 0;
		while( space && bytes < Length )
		{
			uint32_t	cp;
			bytes += ReadUTF8(UTF8Text+bytes, &cp);
			if( Unicode_IsPrinting(cp) )
				space --;
		}
	
		Display_int_PushString(bytes, UTF8Text);

		UTF8Text += bytes;
		_SysDebug("Length(%i) -= bytes(%i)", Length, bytes);
		Length -= bytes;
		if( Length != 0 )
		{
			// Next line
			giCurrentLinePos = 0;
			giCurrentCol = 0;
			giCurrentLine ++;
		}
	}
}

void Display_Newline(int bCarriageReturn)
{
	Display_Flush();

	// Going down!
	giCurrentLine ++;
	if( giCurrentLine == giDisplayLines )
		giCurrentLine = 0;
	if( giCurrentLine == giFirstLine )
	{
		giFirstLine ++;
		if(giFirstLine == giDisplayLines)
			giFirstLine = 0;
	}
	
	if( bCarriageReturn ) {
		giCurrentLinePos = 0;
		giCurrentCol = 0;
	}
	else {
		giCurrentLinePos = 0;
		 int	i = giCurrentCol;
		if( !gasDisplayLines[giCurrentLine] )
		{
			giCurrentCol = 0;
			while(i--)
				Display_AddText(1, " ");
		}
		else
		{
			while( i -- )
			{
				uint32_t	cp;
				giCurrentLinePos += ReadUTF8(gasDisplayLines[giCurrentLine]+giCurrentLinePos, &cp);
				if( !Unicode_IsPrinting(cp) )
					i ++;
			}
		}
	}
}

void Display_SetCursor(int Row, int Col)
{
	UNIMPLIMENTED();
}

void Display_MoveCursor(int RelRow, int RelCol)
{
	UNIMPLIMENTED();
}

void Display_ClearLine(int Dir)	// 0: All, 1: Forward, -1: Reverse
{
	if( Dir == 0 )
	{
		// Completely clear line
		if( gasDisplayLines[giCurrentLine] )
			free(gasDisplayLines[giCurrentLine]);
		gasDisplayLines[giCurrentLine] = NULL;
		gabDisplayLinesDirty[giCurrentLine] = 1;
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

void Display_ClearLines(int Dir)	// 0: All, 1: Forward, -1: Reverse
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

void Display_SetForeground(uint32_t RGB)
{
	UNIMPLIMENTED();
}

void Display_SetBackground(uint32_t RGB)
{
	UNIMPLIMENTED();
}

void Display_Flush(void)
{
	 int	i;
	for( i = 0; i < giDisplayCols; i ++ )
	{
		 int	line = (giFirstLine + i) % giDisplayTotalLines;
		if( !gabDisplayLinesDirty[line] )
			continue;
		_SysDebug("Line %i+%i '%s'", giFirstLine, i, gasDisplayLines[line]);
		AxWin3_RichText_SendLine(gMainWindow, giFirstLine + i, gasDisplayLines[line] );
		gabDisplayLinesDirty[line] = 0;
	}
	
	// force redraw?
	AxWin3_FocusWindow(gMainWindow);
}

void Display_ShowAltBuffer(int AltBufEnabled)
{
	UNIMPLIMENTED();
}


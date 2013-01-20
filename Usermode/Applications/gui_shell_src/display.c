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

#define UNIMPLIMENTED()	do{_SysDebug("UNIMPLIMENTED %s", __func__); exit(-1);}while(0)

#define MAX_LINES	100

// === GLOBALS ===
 int	giCurrentLine;
 int	giCurrentLinePos;	// byte offset, not column
 int	giCurrentCol;
 int	giFirstLine;	// Ring buffer start
char	**gasDisplayLines;

// === CODE ===
void Display_Init(void)
{
	gasDisplayLines = malloc( sizeof(char*) * MAX_LINES );
}

void Display_AddText(int Length, const char *UTF8Text)
{
	_SysDebug("%i '%.*s'", Length, Length, UTF8Text);
	UNIMPLIMENTED();
}

void Display_Newline(int bCarriageReturn)
{
	UNIMPLIMENTED();
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
	UNIMPLIMENTED();
}

void Display_ClearLines(int Dir)	// 0: All, 1: Forward, -1: Reverse
{
	UNIMPLIMENTED();
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
	UNIMPLIMENTED();
}


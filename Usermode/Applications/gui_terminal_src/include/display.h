/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * display.h
 * - RichText terminal translation
 */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>
#include <stddef.h>	// size_t
#include <stdbool.h>

typedef struct sTerminal	tTerminal;

extern tTerminal	*Display_Init(int Cols, int Lines, int ExtraScrollbackLines);

// TermState is a variable used by the terminal emulation code
extern void	*Display_GetTermState(tTerminal *Term);
extern void	Display_SetTermState(tTerminal *Term, void *State);

extern void	Display_SendInput(tTerminal *Term, const char *String);

extern void	Display_AddText(tTerminal *Term, size_t Length, const char *UTF8Text);
extern void	Display_Newline(tTerminal *Term, bool bCarriageReturn);
extern void	Display_SetScrollArea(tTerminal *Term, int Start, int Count);	// Only valid in AltBuffer
extern void	Display_ScrollDown(tTerminal *Term, int Count);
extern void	Display_SetCursor(tTerminal *Term, int Row, int Col);
extern void	Display_MoveCursor(tTerminal *Term, int RelRow, int RelCol);
extern void	Display_SaveCursor(tTerminal *Term);
extern void	Display_RestoreCursor(tTerminal *Term);
extern void	Display_ClearLine(tTerminal *Term, int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_ClearLines(tTerminal *Term, int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_ResetAttributes(tTerminal *Term);
extern void	Display_SetForeground(tTerminal *Term, uint32_t RGB);
extern void	Display_SetBackground(tTerminal *Term, uint32_t RGB);
/**
 * \brief Ensure that recent updates are flushed to the server
 * \note Called at the end of an "input" buffer
 */
extern void	Display_Flush(tTerminal *Term);

/**
 * \brief Switch the display to the alternate buffer (no scrollback)
 */
extern void	Display_ShowAltBuffer(tTerminal *Term, bool AltBufEnabled);

extern void	Display_SetTitle(tTerminal *Term, const char *Title);

#endif


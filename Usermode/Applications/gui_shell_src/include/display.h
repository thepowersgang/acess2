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

extern void	Display_Init(int Cols, int Lines, int ExtraScrollbackLines);

extern void	Display_AddText(int Length, const char *UTF8Text);
extern void	Display_Newline(int bCarriageReturn);
extern void	Display_SetCursor(int Row, int Col);
extern void	Display_MoveCursor(int RelRow, int RelCol);
extern void	Display_ClearLine(int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_ClearLines(int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_SetForeground(uint32_t RGB);
extern void	Display_SetBackground(uint32_t RGB);
/**
 * \brief Ensure that recent updates are flushed to the server
 * \note Called at the end of an "input" buffer
 */
extern void	Display_Flush(void);

/**
 * \brief Switch the display to the alternate buffer (no scrollback)
 */
extern void	Display_ShowAltBuffer(int AltBufEnabled);

#endif


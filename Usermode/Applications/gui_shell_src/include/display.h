/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * display.h
 * - RichText Termianl Translation
 */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

extern void	Display_AddText(int Length, const char *UTF8Text);
extern void	Display_Newline(int bCarriageReturn);
extern void	Display_SetCursor(int Row, int Col);
extern void	Display_MoveCursor(int RelRow, int RelCol);
extern void	Display_ClearLine(int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_ClearLines(int Dir);	// 0: All, 1: Forward, -1: Reverse
extern void	Display_SetForeground(uint32_t RGB);
extern void	Display_SetBackground(uint32_t RGB);

#endif


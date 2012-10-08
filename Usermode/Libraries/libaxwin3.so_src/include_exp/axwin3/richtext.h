/*
 * Acess2 GUI Version 3 (AxWin3)
 * - By John Hodge (thePowersGang)
 *
 * richtext.h
 * - Rich Text line editor
 */
#ifndef _AXWIN3_RICHTEXT_H_
#define _AXWIN3_RICHTEXT_H_

#include <stdint.h>

typedef	int	(*tAxWin3_RichText_KeyHandler)(tHWND Window, int bPress, uint32_t Sym, uint32_t Unicode);
typedef int	(*tAxWin3_RichText_MouseHandler)(tHWND Window, int bPress, int Button, int Row, int Col);

#define AXWIN3_RICHTEXT_NOSCROLL	0x0001
enum eAxWin3_RichText_CursorType {
	AXWIN3_RICHTEXT_CURSOR_NONE,
	AXWIN3_RICHTEXT_CURSOR_VLINE,	// Vertical line
	AXWIN3_RICHTEXT_CURSOR_ULINE,	// Underline
	AXWIN3_RICHTEXT_CURSOR_INV,	// Inverted
};

tHWND	AxWin3_RichText_CreateWindow(tHWND Parent, int Flags);
void	AxWin3_RichText_EnableScroll(tHWND Parent, int bEnable);
void	AxWin3_RichText_SetKeyHandler(tHWND Window, tAxWin3_RichText_KeyHandler Handler);
void	AxWin3_RichText_SetMouseHandler(tHWND Window, tAxWin3_RichText_MouseHandler Handler);
void	AxWin3_RichText_SetBackground(tHWND Window, uint32_t ARGB_Colour);
void	AxWin3_RichText_SetDefaultColour(tHWND Window, uint32_t ARGB_Colour);
void	AxWin3_RichText_SetFont(tHWND Window, const char *FontName, int PointSize);
void	AxWin3_RichText_SetCursorType(tHWND Parent, int Type);
void	AxWin3_RichText_SetCursorBlink(tHWND Parent, int bBlink);
void	AxWin3_RichText_SetCursorPos(tHWND Window, int Row, int Column);

#endif


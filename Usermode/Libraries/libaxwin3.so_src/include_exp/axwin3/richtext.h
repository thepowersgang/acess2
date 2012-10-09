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

extern tHWND	AxWin3_RichText_CreateWindow(tHWND Parent, int Flags);
extern void	AxWin3_RichText_SetKeyHandler(tHWND Window, tAxWin3_RichText_KeyHandler Handler);
extern void	AxWin3_RichText_SetMouseHandler(tHWND Window, tAxWin3_RichText_MouseHandler Handler);
extern void	AxWin3_RichText_EnableScroll(tHWND Window, int bEnable);
extern void	AxWin3_RichText_SetLineCount(tHWND Window, int Lines);
extern void	AxWin3_RichText_SetColCount(tHWND Window, int Cols);
extern void	AxWin3_RichText_SetBackground(tHWND Window, uint32_t ARGB_Colour);
extern void	AxWin3_RichText_SetDefaultColour(tHWND Window, uint32_t ARGB_Colour);
extern void	AxWin3_RichText_SetFont(tHWND Window, const char *FontName, int PointSize);
extern void	AxWin3_RichText_SetCursorType(tHWND Window, int Type);
extern void	AxWin3_RichText_SetCursorBlink(tHWND Window, int bBlink);
extern void	AxWin3_RichText_SetCursorPos(tHWND Window, int Row, int Column);

#endif


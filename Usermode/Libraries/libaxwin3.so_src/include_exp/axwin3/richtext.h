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
#include "axwin.h"

typedef	int	(*tAxWin3_RichText_KeyHandler)(tHWND Window, int bPress, uint32_t Sym, uint32_t Unicode);
typedef int	(*tAxWin3_RichText_MouseHandler)(tHWND Window, int bPress, int Button, int Row, int Col);
typedef int	(*tAxWin3_RichText_LineHandler)(tHWND Window, int Row);

#define AXWIN3_RICHTEXT_NOSCROLL	0x0001	// Disables server-side scrolling
#define AXWIN3_RICHTEXT_READONLY	0x0002	// Disables automatic insertion of translated characters
enum eAxWin3_RichText_CursorType {
	AXWIN3_RICHTEXT_CURSOR_NONE,
	AXWIN3_RICHTEXT_CURSOR_VLINE,	// Vertical line
	AXWIN3_RICHTEXT_CURSOR_ULINE,	// Underline
	AXWIN3_RICHTEXT_CURSOR_INV,	// Inverted
};

extern tHWND	AxWin3_RichText_CreateWindow(tHWND Parent, int Flags);
extern void	AxWin3_RichText_SetKeyHandler(tHWND Window, tAxWin3_RichText_KeyHandler Handler);
extern void	AxWin3_RichText_SetMouseHandler(tHWND Window, tAxWin3_RichText_MouseHandler Handler);
/**
 * \brief Sets the function called when the server requests an update on a line's contents
 */
extern void	AxWin3_RichText_SetLineHandler(tHWND Window, tAxWin3_RichText_LineHandler Handler);
extern void	AxWin3_RichText_EnableScroll(tHWND Window, int bEnable);
extern void	AxWin3_RichText_SetLineCount(tHWND Window, int Lines);
extern void	AxWin3_RichText_SetColCount(tHWND Window, int Cols);
extern void	AxWin3_RichText_SetBackground(tHWND Window, uint32_t ARGB_Colour);
extern void	AxWin3_RichText_SetDefaultColour(tHWND Window, uint32_t ARGB_Colour);
extern void	AxWin3_RichText_SetFont(tHWND Window, const char *FontName, int PointSize);
extern void	AxWin3_RichText_SetCursorType(tHWND Window, int Type);
extern void	AxWin3_RichText_SetCursorBlink(tHWND Window, int bBlink);
extern void	AxWin3_RichText_SetCursorPos(tHWND Window, int Row, int Column);
/*
 * \brief Scroll the specified range of data down (moving lines up)
 * \note This is NOT a visual scroll, it scrolls the data
 *
 * Top/Bottom `DownCount` lines are discarded (bottom if DownCount is -ve)
 * UNLESS DownCount is -ve and RangeCount is -1 (indicating insertion of lines)
 */
extern void	AxWin3_RichText_ScrollRange(tHWND Window, int FirstRow, int RangeCount, int DownCount);
extern void	AxWin3_RichText_SendLine(tHWND Window, int Line, const char *Text);

#endif


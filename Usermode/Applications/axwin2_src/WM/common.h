/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "wm.h"
#include "image.h"
//#include "font.h"

// === GLOBALS ===
extern char	*gsTerminalDevice;
extern char	*gsMouseDevice;

extern int	giScreenWidth;
extern int	giScreenHeight;
extern uint32_t	*gpScreenBuffer;

extern int	giTerminalFD;
extern int	giMouseFD;

// === Functions ===
extern void	memset32(void *ptr, uint32_t val, size_t count);
// --- Video ---
extern void	Video_Update(void);
extern void	Video_FillRect(short X, short Y, short W, short H, uint32_t Color);
extern void	Video_DrawRect(short X, short Y, short W, short H, uint32_t Color);
extern void	Video_DrawText(short X, short Y, short W, short H, void *Font, int Point, uint32_t Color, char *Text);
// --- Debug Hack ---
extern void	_SysDebug(const char *Format, ...);
#endif

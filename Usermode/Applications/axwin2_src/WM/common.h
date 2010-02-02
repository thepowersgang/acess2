/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

extern char	*gsTerminalDevice;
extern char	*gsMouseDevice;

extern int	giScreenWidth;
extern int	giScreenHeight;
extern uint32_t	*gpScreenBuffer;

extern int	giTerminalFD;
extern int	giMouseFD;

// === Functions ===
extern void	memset32(void *ptr, uint32_t val, size_t count);

#endif

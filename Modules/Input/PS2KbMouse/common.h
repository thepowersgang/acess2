/*
 * Acess2
 * 
 * PS2 Keyboard/Mouse Driver
 *
 * common.h
 * - Shared definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

extern int	KB_Install(char **Arguments);
extern int	PS2Mouse_Install(char **Arguments);

extern void	KBC8042_Init(void);
extern void	KBC8042_EnableMouse(void);

extern void	KB_HandleScancode(Uint8 scancode);
extern void	PS2Mouse_HandleInterrupt(Uint8 InputByte);

#endif

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

extern void	PL050_Init(Uint32 KeyboardBase, Uint8 KeyboardIRQ, Uint32 MouseBase, Uint8 MouseIRQ);
extern void	PL050_EnableMouse(void);

extern void	KB_HandleScancode(Uint8 scancode);
extern void	PS2Mouse_HandleInterrupt(Uint8 InputByte);

extern void	(*gpPS2Mouse_EnableFcn)(void);

#endif

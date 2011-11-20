/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * wm_input.h
 * - Window Manager Input
 */
#ifndef _WM_INPUT_H_
#define _WM_INPUT_H_

extern void	WM_Input_MouseMoved(int OldX, int OldY, int NewX, int NewY);
extern void	WM_Input_MouseButton(int X, int Y, int Button, int Pressed);
extern void	WM_Input_KeyDown(uint32_t Character, uint32_t Scancode);
extern void	WM_Input_KeyFire(uint32_t Character, uint32_t Scancode);
extern void	WM_Input_KeyUp  (uint32_t Character, uint32_t Scancode);

#endif


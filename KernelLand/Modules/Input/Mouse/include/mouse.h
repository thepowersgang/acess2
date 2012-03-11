/*
 * Acess2 Kernel - Mouse Mulitplexing Driver
 * - By John Hodge (thePowersGang)
 *
 * include/mouse.h
 * - Mouse mulitplexing interface header
 */
#ifndef _MOUSE__MOUSE_H_
#define _MOUSE__MOUSE_H_

typedef struct sMouse	tMouse;

tMouse	*Mouse_Register(const char *Name, int NumButtons, int NumAxies);
void	Mouse_RemoveInstance(tMouse *Handle);
void	Mouse_HandleEvent(tMouse *Handle, Uint32 ButtonState, int AxisDeltas[4]);

#endif


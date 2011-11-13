/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/decorator.h
 * - Decorator definitions
 */
#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <wm_internals.h>

extern void	Decorator_UpdateBorderSize(tWindow *Window);
extern void	Decorator_Redraw(tWindow *Window);
extern int	Decorator_HandleMessage(tWindow *Window, int Message, int Length, void *Data);

#endif


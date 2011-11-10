/*
 * Acess2 GUI Version 3 (AxWin3)
 * - By John Hodge (thePowersGang)
 *
 * menu.h
 * - Menu window type
 */
#ifndef _AXWIN3_MENU_H_
#define _AXWIN3_MENU_H_

typedef void	(*tAxWin3_Menu_Callback)(void *Ptr);
typedef struct sAxWin3_MenuItem	tAxWin3_MenuItem;

extern tHWND	AxWin3_Menu_Create(tHWND Parent);
extern void	AxWin3_Menu_ShowAt(tHWND Menu, int X, int Y);

extern tAxWin3_MenuItem	*AxWin3_Menu_AddItem(
		tHWND Menu, const char *Label,
		tAxWin3_Menu_Callback Cb, void *Ptr,
		int Flags, tHWND SubMenu
		);
extern tAxWin3_MenuItem	*AxWin3_Menu_GetItem(tHWND Menu, int Index);
extern void	AxWin3_Menu_SetFlags(tAxWin3_MenuItem *Item, int Flags, int Mask);

#endif


/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * menu_messages.h
 * - AxWin3 Menu IPC Messages
 */
#ifndef _MENU_MESSAGES_H_
#define _MENU_MESSAGES_H_

#include "ipcmessages.h"

enum eMenuMessages
{
	MSG_MENU_ADDITEM = 0x1000,
	MSG_MENU_SETFLAGS,
	
	MSG_MENU_SELECT
};

typedef struct
{
	uint16_t	ID;
	uint16_t	Flags;
	uint32_t	SubMenuID;
	char	Label[];
} tMenuMsg_AddItem;

typedef struct
{
	uint16_t	ID;
	uint16_t	Value;
	uint16_t	Mask;
} tMenuMsg_SetFlags;

typedef struct
{
	uint16_t	ID;
} tMenuMsg_Select;

#endif


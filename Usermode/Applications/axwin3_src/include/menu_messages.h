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

// Client->Server IPC methods
enum eMenuIPCCalls
{
	IPC_MENU_ADDITEM,
	IPC_MENU_SETFLAGS
};

typedef struct
{
	uint16_t	ID;
	uint16_t	Flags;
	uint32_t	SubMenuID;
	char	Label[];
} tMenuIPC_AddItem;

typedef struct
{
	uint16_t	ID;
	uint16_t	Value;
	uint16_t	Mask;
} tMenuIPC_SetFlags;

// Server->Client messages
enum eMenuMessages
{
	MSG_MENU_SELECT = 0x1000
};

typedef struct
{
	uint16_t	ID;
} tMenuMsg_Select;

#endif


/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * wm.c
 * - Core window management functions
 */
#include <axwin3/axwin.h>
#include <stdlib.h>
#include <string.h>
#include "include/ipc.h"
#include "include/internal.h"

#define WINDOWS_PER_ALLOC	(63)

typedef struct sWindowBlock	tWindowBlock;

typedef struct sAxWin3_Window	tWindow;

// === STRUCTURES ===
struct sWindowBlock
{
	tWindowBlock	*Next;
	tWindow	*Windows[WINDOWS_PER_ALLOC];
};

// === GLOBALS ===
 int	giAxWin3_LowestFreeWinID;
 int	giAxWin3_HighestUsedWinID;
tWindowBlock	gAxWin3_WindowList;

// === CODE ===
tWindow *AxWin3_int_CreateWindowStruct(uint32_t ServerID, int ExtraBytes)
{
	tWindow	*ret;
	
	ret = calloc(sizeof(tWindow) + ExtraBytes, 1);
	ret->ServerID = ServerID;

	return ret;
}

tHWND AxWin3_CreateWindow(tHWND Parent, const char *Renderer, int Flags, int DataBytes, void **DataPtr)
{
	tWindow	*ret;
	 int	newWinID;
	 int	dataSize = sizeof(tIPCMsg_CreateWin) + strlen(Renderer) + 1;
	tAxWin_IPCMessage	*msg;
	tIPCMsg_CreateWin	*create_win;

	// TODO: Validate `Parent`
	
	// Allocate a window ID
	{
		 int	idx;
		tWindowBlock *block, *prev;
		block = &gAxWin3_WindowList;
		newWinID = giAxWin3_LowestFreeWinID;
		for( idx = 0; block; newWinID ++ )
		{
			if( block->Windows[idx] == NULL )
				break;
			idx ++;
			if(idx == WINDOWS_PER_ALLOC) {
				prev = block;
				block = block->Next;
				idx = 0;
			}
		}
		
		if( !block )
		{
			block = calloc(sizeof(tWindowBlock), 1);
			prev->Next = block;
			idx = 0;
		}
		
		ret = block->Windows[idx] = AxWin3_int_CreateWindowStruct(newWinID, DataBytes);

		if(newWinID > giAxWin3_HighestUsedWinID)
			giAxWin3_HighestUsedWinID = newWinID;
		if(newWinID == giAxWin3_LowestFreeWinID)
			giAxWin3_HighestUsedWinID ++;
	}

	// Create message
	msg = AxWin3_int_AllocateIPCMessage(Parent, IPCMSG_CREATEWIN, 0, dataSize);
	create_win = (void*)msg->Data;	
	create_win->NewWinID = newWinID;
	create_win->Flags = Flags;
	strcpy(create_win->Renderer, Renderer);

	// Send and clean up
	AxWin3_int_SendIPCMessage(msg);
	free(msg);

	// TODO: Detect errors in AxWin3_CreateWindow

	// Return success
	if(DataPtr)	*DataPtr = ret->Data;
	return ret;
}

void AxWin3_DestroyWindow(tHWND Window)
{
}

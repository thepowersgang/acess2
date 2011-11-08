/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * internal.h
 * - Internal definitions
 */
#ifndef _IPC_H_
#define _IPC_H_

#include <ipcmessages.h>

extern const char	*gsAxWin3_int_ServerDesc;

extern tAxWin_IPCMessage	*AxWin3_int_AllocateIPCMessage(tHWND Window, int Message, int Flags, int ExtraBytes);
extern void	AxWin3_int_SendIPCMessage(tAxWin_IPCMessage *Msg);
extern tAxWin_IPCMessage	*AxWin3_int_GetIPCMessage(void);
extern void	AxWin3_int_HandleMessage(tAxWin_IPCMessage *Msg);


#endif


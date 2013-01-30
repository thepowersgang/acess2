/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * nettest.h
 * - Common functions
 */
#ifndef _NETTEST_H_
#define _NETTEST_H_

#include <stddef.h>

extern int	NativeNic_AddDev(char *Desc);

extern int	NetTest_AddAddress(const char *SetAddrString);

extern void	*NetTest_OpenTap(const char *Name);
extern size_t	NetTest_WritePacket(void *Handle, size_t Size, const void *Data);
extern size_t	NetTest_ReadPacket(void *Handle, size_t MaxSize, void *Data);

extern size_t	NetTest_WriteStdout(const void *Data, size_t Size);

extern void	NetTest_Suite_Netcat(const char *Addr, int Port);

extern int	Net_ParseAddress(const char *String, void *Addr);
extern int	Net_OpenSocket_TCPC(int AddrType, void *Addr, int Port);

#endif


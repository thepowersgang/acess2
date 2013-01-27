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

extern void	*NetTest_OpenTap(const char *Name);
extern size_t	NetTest_WritePacket(void *Handle, size_t Size, const void *Data);
extern size_t	NetTest_ReadPacket(void *Handle, size_t MaxSize, void *Data);

#endif


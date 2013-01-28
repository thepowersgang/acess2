/*
 * Acess2 IP Stack
 * - By John Hodge
 *
 * interface.h
 * - Interface manipulation/access definitions
 */
#ifndef _IPSTACK__INTERFACE_H_
#define _IPSTACK__INTERFACE_H_

extern tInterface	gIP_LoopInterface;
extern tVFS_NodeType	gIP_RootNodeType;
extern tInterface	*IPStack_AddInterface(const char *Device, int Type, const char *Name);

#endif


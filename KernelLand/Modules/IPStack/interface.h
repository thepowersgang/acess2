
#ifndef _IPSTACK__INTERFACE_H_
#define _IPSTACK__INTERFACE_H_

extern tInterface	gIP_LoopInterface;
extern tVFS_NodeType	gIP_RootNodeType;
extern tInterface	*IPStack_AddInterface(const char *Device, const char *Name);

#endif


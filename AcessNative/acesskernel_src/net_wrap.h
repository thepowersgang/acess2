
#ifndef _NET_WRAP_H_
#define _NET_WRAP_H_

extern void	Net_Wrap_Init(void);
extern void	*Net_Wrap_ConnectTcp(void *Node, short SrcPort, short DstPtr, int Type, const void *Addr);
extern size_t	Net_Wrap_ReadSocket(void *Handle, size_t Bytes, void *Dest);
extern size_t	Net_Wrap_WriteSocket(void *Handle, size_t Bytes, const void *Dest);
extern void	Net_Wrap_CloseSocket(void *Handle);

#endif


/*
 * net.h
 * - Network interface IO
 */
#ifndef _NET_H_
#define _NET_H_

#include <stddef.h>
#include <stdbool.h>

#define MTU	1520

extern int	Net_Open(int IfNum, const char *Path);
extern void	Net_Close(int IfNum);


extern size_t	Net_Receive(int IfNum, size_t MaxLen, void *DestBuf, unsigned int Timeout);
extern void	Net_Send(int IfNum, size_t Length, const void *Buf);

#endif


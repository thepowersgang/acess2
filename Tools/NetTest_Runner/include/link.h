/*
 */
#ifndef _LINK_H_
#define _LINK_H_

#define ETHER_PROTO_IPV4	0x0800

#include <stdint.h>
#include <stdbool.h>

extern void	Link_Send(int IfNum, const void *Src, const void *Dst, uint16_t Proto, int BufCount, size_t BufLens[], const void *Bufs[]);

extern bool	Link_Pkt_Check(size_t len, const void *data, size_t *ofs, const void *Src, const void *Dst, uint16_t Proto);

#endif


/*
 * ip.h
 * - IP-layer Test Wrapper (v5/v6)
 */
#ifndef _IP_H_
#define _IP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define IPPROTO_TCP	6

#define IP_CHECKSUM_START	0xFFFF

extern uint16_t	IP_Checksum(uint16_t Prev, size_t Length, const void *Data);

extern void	IP_Send(int IfNum, int AF, const void *Src, const void *Dst, uint8_t proto,
	int BufCount, size_t BufLens[], const void *Bufs[]);

extern bool	IP_Pkt_Check(size_t len, const void *data, size_t *ofs, int AF, const void *Src, const void *Dst, uint8_t proto);

#endif


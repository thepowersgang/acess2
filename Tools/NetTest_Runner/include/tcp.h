/*
 * tcp.h
 * - TCP Test Wrapper
 */
#ifndef _TCP_H_
#define _TCP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
	 int	IFNum;
	 int	AF;
	const void	*LAddr;
	const void	*RAddr;
	uint16_t	LPort;
	uint16_t	RPort;
	uint16_t	Window;
	uint32_t	LSeq;
	uint32_t	RSeq;
} tTCPConn;

#define TCP_FIN	0x01
#define TCP_SYN	0x02
#define TCP_RST	0x04
#define TCP_PSH	0x08
#define TCP_ACK	0x10
#define TCP_URG	0x20


extern void	TCP_SendC(const tTCPConn *Conn, uint8_t flags, size_t data_len, const void *data);
extern void	TCP_Send(int IF, int AF, const void *IP, short sport, short dport, uint32_t seq, uint32_t ack, uint8_t flags, uint16_t window, size_t data_len, const void *data);

// The following skip the next check of each field
extern void	TCP_SkipCheck_Seq(bool Skip);

extern bool	TCP_Pkt_CheckC(size_t len, const void *data, size_t *ofs, size_t *out_len, const tTCPConn *Conn, uint8_t flags);
extern bool	TCP_Pkt_Check(size_t len, const void *data, size_t *ofs, size_t *out_len, int AF, const void *IP, short sport, short dport, uint32_t seq, uint32_t ack, uint8_t flags);

// - Get a field from a previously validated packet
extern uint32_t	TCP_Pkt_GetSeq(size_t len, const void *data, int AF);

#endif


/*
 * Acess2 Kernel
 * - Sun RPC (RFC 1057) implementation
 * proto.h
 * - Protocol definition
 */
#ifndef _SUNRPC_PROTO_H_
#define _SUNRPC_PROTO_H_

struct sRPC_CallBody
{
	Uint32	RPCVers;	//!< Version (2)
	Uint32	Program;	//!< Program Identifier
	Uint32	Version;	//!< Program Version
	
};

struct sRPC_Message
{
	tNet32	XID;	//!< Transaction Identifier
	union
	{
		struct sRPC_CallBody	Call;
	};
};

#endif


/*
 */
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

enum eMessageTypes
{
	MSG_TYPE_NULL,
	MSG_TYPE_SERVER,	// Server message
	
	MSG_TYPE_NOTICE,	// NOTICE command
	MSG_TYPE_JOIN,	// JOIN command
	MSG_TYPE_PART,	// PART command
	MSG_TYPE_QUIT,	// QUIT command
	
	MSG_TYPE_STANDARD,	// Standard line
	MSG_TYPE_ACTION,	// /me
	
	MSG_TYPE_UNK
};

enum eMessageClass
{
	MSG_CLASS_BARE,	// source is unused, just gets timestamped
	MSG_CLASS_CLIENT,	// source optional, prefixed by '-!-'
	MSG_CLASS_WALL, 	// [SOURCE] MSG 	-- Server-provided message
	MSG_CLASS_MESSAGE,	// <SOURCE> MSG
	MSG_CLASS_ACTION,	// * SOURCE MSG
};

typedef struct sMessage	tMessage;

//extern tMessage	*Message_AppendF(tServer *Server, int Type, const char *Src, const char *Dst, const char *Fmt, ...) __attribute__((format(__printf__,5,6)));
//extern tMessage	*Message_Append(tServer *Server, int Type, const char *Source, const char *Dest, const char *Message);

#endif


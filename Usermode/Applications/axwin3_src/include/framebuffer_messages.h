/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * framebuffer_messages.h
 * - Drawing Surface / Framebuffer Window Type
 */
#ifndef _AXWIN3__FRAMEBUFFER_MESSAGES_H_
#define _AXWIN3__FRAMEBUFFER_MESSAGES_H_

enum eFrameBuffer_IPCCalls
{
	IPC_FB_COMMIT,
	IPC_FB_NEWBUF,
	IPC_FB_UPLOAD,
	IPC_FB_DOWNLOAD,	// Replies with requested data
	IPC_FB_BLIT,
	IPC_FB_FILL,
	N_IPC_FB
};

typedef struct
{
	uint16_t	Buffer;
	uint16_t	W, H;
} tFBIPC_NewBuf;

typedef struct
{
	uint16_t	Buffer;
	uint16_t	W, H;
	uint16_t	X, Y;
	uint32_t	ImageData[];
} tFBIPC_Transfer;

typedef struct
{
	uint16_t	Source;
	uint16_t	Dest;
	uint16_t	SrcX, SrcY;
	uint16_t	DstX, DstY;
	uint16_t	W, H;
} tFBIPC_Blit;

typedef struct
{
	uint16_t	Buffer;
	uint16_t	X, Y;
	uint16_t	W, H;
	uint32_t	Colour;
} tFBIPC_Fill;

#endif


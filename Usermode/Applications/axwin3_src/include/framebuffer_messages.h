/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * framebuffer_messages.h
 * - Drawing Surface / Framebuffer Window Type
 */
#ifndef _AXWIN3__FRAMEBUFFER_MESSAGES_H_
#define _AXWIN3__FRAMEBUFFER_MESSAGES_H_

enum
{
	MSG_FB_COMMIT = 0x1000,
	MSG_FB_NEWBUF,
	MSG_FB_UPLOAD,
	MSG_FB_DOWNLOAD,
	MSG_FB_BLIT,
	MSG_FB_FILL
};

typedef struct
{
	uint16_t	Buffer;
	uint16_t	W, H;
} tFBMsg_NewBuf;

typedef struct
{
	uint16_t	Buffer;
	uint16_t	W, H;
	uint16_t	X, Y;
	uint32_t	ImageData[];
} tFBMsg_Transfer;

typedef struct
{
	uint16_t	Source;
	uint16_t	Dest;
	uint16_t	SrcX, SrcY;
	uint16_t	DstX, DstY;
	uint16_t	W, H;
} tFBMsg_Blit;

typedef struct
{
	uint16_t	Buffer;
	uint16_t	X, Y;
	uint16_t	W, H;
	uint32_t	Colour;
} tFBMsg_Fill;

#endif


/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * renderer_passthru.c
 * - Passthrough window render (framebuffer essentially)
 */
#include <common.h>
#include <wm_renderer.h>
#include <string.h>
#include <framebuffer_messages.h>
#include <wm_messages.h>

// === TYPES ===
typedef struct
{
	short	W, H;
	void	*Data;
} tFBBuffer;
typedef struct
{
	tFBBuffer	BackBuffer;
	 int	MaxBufferCount;
	tFBBuffer	*Buffers[];
} tFBWin;

// === PROTOTYPES ===
tWindow	*Renderer_Framebuffer_Create(int Flags);
void	Renderer_Framebuffer_Redraw(tWindow *Window);
 int	_Handle_Commit(tWindow *Target, size_t Len, const void *Data);
 int	_Handle_CreateBuf(tWindow *Target, size_t Len, const void *Data);
 int	Renderer_Framebuffer_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Framebuffer = {
	.Name = "FrameBuffer",
	.CreateWindow = Renderer_Framebuffer_Create,
	.Redraw = Renderer_Framebuffer_Redraw,
	.HandleMessage = Renderer_Framebuffer_HandleMessage,
	.nIPCHandlers = N_IPC_FB,
	.IPCHandlers = {
		[IPC_FB_COMMIT] = _Handle_Commit,
		[IPC_FB_NEWBUF] = _Handle_CreateBuf,
		//[IPC_FB_SUBBUF] = _Handle_SubBuf,
	}
};

// === CODE ===
int Renderer_Framebuffer_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Framebuffer);
	return 0;
}

tWindow	*Renderer_Framebuffer_Create(int Flags)
{
	tWindow	*ret;
	tFBWin	*info;
	const int	max_buffers = 10;	// TODO: Have this be configurable
	
	ret = WM_CreateWindowStruct( sizeof(*info) + max_buffers*sizeof(tFBBuffer*) );
	info = ret->RendererInfo;
	
	info->BackBuffer.W = 0;
	info->BackBuffer.H = 0;
	info->BackBuffer.Data = NULL;
	info->MaxBufferCount = max_buffers;
	memset( info->Buffers, 0, sizeof(tFBBuffer*) * max_buffers );
	
	return ret;
}

void Renderer_Framebuffer_Redraw(tWindow *Window)
{
	
}

// --- ---
tFBBuffer *_GetBuffer(tWindow *Win, uint16_t ID)
{
	tFBWin	*info = Win->RendererInfo;
	if( ID == 0xFFFF )
		return &info->BackBuffer;
	else if( ID >= info->MaxBufferCount )
		return NULL;
	else
		return info->Buffers[ ID ];
}

void _Blit(
	tFBBuffer *Dest, uint16_t DX, uint16_t DY,
	const tFBBuffer *Src, uint16_t SX, uint16_t SY,
	uint16_t W, uint16_t H
	)
{
	uint32_t	*dest_data = Dest->Data;
	const uint32_t	*src_data = Src->Data;
	// First, some sanity
	if( DX > Dest->W )	return ;
	if( DY > Dest->H )	return ;
	if( SX > Src->W )	return ;
	if( SY > Src->H )	return ;
	
	if( DX + W > Dest->W )	W = Dest->W - DX;
	if( SX + W > Src->W )	W = Src->W - SX;
	if( DY + H > Dest->H )	H = Dest->H - DY;
	if( SY + H > Src->H )	H = Src->H - SY;
	
	dest_data += (DY * Dest->W) + DX;
	src_data  += (SY * Src->W ) + SX;
	for( int i = 0; i < H; i ++ )
	{
		memcpy( dest_data, src_data, W * 4 );
		dest_data += Dest->W;
		src_data += Src->W;
	}
}

void _Fill(tFBBuffer *Buf, uint16_t X, uint16_t Y, uint16_t W, uint16_t H, uint32_t Colour)
{
	uint32_t	*data = Buf->Data;
	
	if( X > Buf->W )	return ;
	if( Y > Buf->H )	return ;
	if( X + W > Buf->W )	W = Buf->W - X;
	if( Y + H > Buf->H )	H = Buf->H - Y;
	
	data += (Y * Buf->W) + X;
	for( int i = 0; i < H; i ++ )
	{
		for( int j = 0; j < W; j ++ )
			*data++ = Colour;
		data += Buf->W - W;
	}
}

// --- ---
int _Handle_Commit(tWindow *Target, size_t Len, const void *Data)
{
	// Do a window invalidate
	return 0;
}

int _Handle_CreateBuf(tWindow *Target, size_t Len, const void *Data)
{
	const tFBIPC_NewBuf	*msg = Data;
	tFBBuffer	*buf;
	tFBWin	*info = Target->RendererInfo;
	
	if( Len < sizeof(*msg) )	return -1;
	
	if( msg->Buffer == -1 || msg->Buffer >= info->MaxBufferCount ) {
		// Can't reallocate -1
		return 1;
	}
	
	if( info->Buffers[msg->Buffer] ) {
		free(info->Buffers[msg->Buffer]);
		info->Buffers[msg->Buffer] = NULL;
	}
	
	buf = malloc(sizeof(tFBBuffer) + msg->W * msg->H * 4);
	buf->W = msg->W;
	buf->H = msg->H;
	buf->Data = buf + 1;
	
	info->Buffers[msg->Buffer] = buf;
	
	return 0;
}

int _Handle_Upload(tWindow *Target, size_t Len, const void *Data)
{
	const tFBIPC_Transfer	*msg = Data;
	tFBBuffer	*dest, src;
	
	if( Len < sizeof(*msg) )	return -1;
	if( Len < sizeof(*msg) + msg->W * msg->H * 4 )	return -1;
	
	dest = _GetBuffer(Target, msg->Buffer);

	src.W = msg->W;	
	src.H = msg->H;	
	src.Data = (void*)msg->ImageData;

	_Blit( dest, msg->X, msg->Y,  &src, 0, 0,  msg->W, msg->H );
	return 0;
}

int _Handle_Download(tWindow *Target, size_t Len, const void *Data)
{
	const tFBIPC_Transfer	*msg = Data;
	tFBBuffer	dest, *src;
	
	if( Len < sizeof(*msg) )	return -1;
	
	src = _GetBuffer(Target, msg->Buffer);

	tFBIPC_Transfer	*ret;
	 int	retlen = sizeof(*ret) + msg->W*msg->H*4;
	ret = malloc( retlen );

	dest.W = ret->W = msg->W;
	dest.H = ret->H = msg->H;
	dest.Data = ret->ImageData;
	
	_Blit( &dest, 0, 0,  src, msg->X, msg->Y,  msg->W, msg->H );

	WM_SendIPCReply(Target, IPC_FB_DOWNLOAD, retlen, ret);

	free(ret);

	return 0;
}

int _Handle_LocalBlit(tWindow *Target, size_t Len, const void *Data)
{
	const tFBIPC_Blit	*msg = Data;
	tFBBuffer	*dest, *src;
	
	if( Len < sizeof(*msg) )	return -1;
	
	src = _GetBuffer(Target, msg->Source);
	dest = _GetBuffer(Target, msg->Dest);

	_Blit( dest, msg->DstX, msg->DstY,  src, msg->SrcX, msg->SrcY,  msg->W, msg->H );
	return 0;
}

int _Handle_FillRect(tWindow *Target, size_t Len, const void *Data)
{
	const tFBIPC_Fill	*msg = Data;
	tFBBuffer	*dest;
	
	if( Len < sizeof(*msg) )	return -1;
	
	dest = _GetBuffer(Target, msg->Buffer);
	
	_Fill( dest, msg->X, msg->Y, msg->W, msg->H, msg->Colour );
	return 0;
}

int Renderer_Framebuffer_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	switch(Msg)
	{
	case WNDMSG_RESIZE:
		// Resize the framebuffer
		return 1;	// Pass on
	
	// Messages that get passed on
	case WNDMSG_MOUSEBTN:
		return 1;
	}
	return 1;
}



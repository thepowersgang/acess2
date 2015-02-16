/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_2d.c
 * - 2D command processing
 */
#define DEBUG	0
#include "vterm.h"
#include <acess/devices/pty_cmds.h>

void	VT_int_SetCursorPos(tVTerm *Term, int X, int Y);
void	VT_int_SetCursorBitmap(tVTerm *Term, int W, int H);
size_t	VT_int_FillCursorBitmap(tVTerm *Term, size_t DataOfs, size_t Length, const void *Data);
 int	VT_int_2DCmd_SetCursorPos(void *Handle, size_t Offset, size_t Length, const void *Data);
 int	VT_int_2DCmd_SetCursorBitmap(void *Handle, size_t Offset, size_t Length, const void *Data);
 int	VT_int_2DCmd_SendData(void *Handle, size_t Offset, size_t Length, const void *Data);

// === CODE ===
void VT_int_SetCursorPos(tVTerm *Term, int X, int Y)
{
	if( Term->Mode == PTYBUFFMT_TEXT )
	{
		tVT_Pos	*wrpos = VT_int_GetWritePosPtr(Term);
		wrpos->Row = X;
		wrpos->Col = Y + (Term->Flags & VT_FLAG_ALTBUF ? 0 : Term->ViewTopRow);
		VT_int_UpdateCursor(Term, 0);
	}
	else
	{
		Term->VideoCursorX = X;
		Term->VideoCursorY = Y;
		VT_int_UpdateCursor(Term, 1);
	}
}

void VT_int_SetCursorBitmap(tVTerm *Term, int W, int H)
{
	LOG("WxH = %ix%i", W, H);
	if(Term->VideoCursor)
	{
		if(W * H != Term->VideoCursor->W * Term->VideoCursor->H) {
			free(Term->VideoCursor);
			Term->VideoCursor = NULL;
		}
	}
	if(!Term->VideoCursor) {
		Term->VideoCursor = malloc(sizeof(*Term->VideoCursor) + W*H*sizeof(Uint32));
		if(!Term->VideoCursor) {
			Log_Error("VTerm", "Unable to allocate memory for cursor");
			return ;
		}
	}
	Term->VideoCursor->W = W;
	Term->VideoCursor->H = H;
	Term->VideoCursor->XOfs = 0;
	Term->VideoCursor->YOfs = 0;
}

size_t VT_int_FillCursorBitmap(tVTerm *Term, size_t DataOfs, size_t Length, const void *Data)
{
	if( !Term->VideoCursor )
		return 0;
	size_t	limit = Term->VideoCursor->W * Term->VideoCursor->H * 4;
	if( DataOfs > limit )
		return 0;
	if( Length > limit )
		Length = limit;
	if( DataOfs + Length > limit )
		Length = limit - DataOfs;
	
	LOG("memcpy(VideoCursor->Data + 0x%x, Data, 0x%x)", DataOfs, Length);
	memcpy((char*)Term->VideoCursor->Data + DataOfs, Data, Length);
	
	if(gpVT_CurTerm == Term)
		VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSORBITMAP, Term->VideoCursor);
	return Length;
}

// --- 2D Command deserialising ---
int VT_int_2DCmd_SetCursorPos(void *Handle, size_t Offset, size_t Length, const void *Data)
{
	struct ptycmd_setcursorpos	cmd;
	if( Length < sizeof(cmd) )	return 0;
	memcpy(&cmd, Data, sizeof(cmd));

	VT_int_SetCursorPos(Handle, cmd.x, cmd.y);
	
	return sizeof(cmd);
}

int VT_int_2DCmd_SetCursorBitmap(void *Handle, size_t Offset, size_t Length, const void *Data)
{
	struct ptycmd_setcursorbmp	cmd;
	tVTerm	*term = Handle;
	size_t	ret = 0;

	if( Offset == 0 ) {
		if( Length < sizeof(cmd) )	return 0;
		memcpy(&cmd, Data, sizeof(cmd));
		VT_int_SetCursorBitmap(Handle, cmd.w, cmd.h);
		ret = sizeof(cmd);
		Length -= ret;
		Data = (const char*)Data + ret;
		Offset += ret;
	}
	
	ASSERTC(Offset, >=, sizeof(cmd));

	if( Length > 0 )
	{
		ret += VT_int_FillCursorBitmap(Handle, Offset - sizeof(cmd), Length, Data);
	}

	LOG("%i + %i ==? %i", ret, Offset, term->Cmd2D.CurrentSize);
	if( ret + Offset >= term->Cmd2D.CurrentSize )
		return ret;

	return -ret;
}

int VT_int_2DCmd_SendData(void *Handle, size_t Offset, size_t Length, const void *Data)
{
	tVTerm	*term = Handle;
	struct ptycmd_senddata	cmd;
	size_t	ret = 0;
	
	if( Offset == 0 )
	{
		if( Length < sizeof(cmd) )
			return 0;
		memcpy(&cmd, Data, sizeof(cmd));
		
		ret = sizeof(cmd);
		Offset += ret;
		Length -= ret;
		Data = (const char*)Data + ret;
		
		term->Cmd2D.CmdInfo.Push.Offset = cmd.ofs*4;
	}
	
	ASSERTC(Offset, >=, sizeof(cmd));
	
	if( Length > 0 )
	{
		size_t	bytes = MIN(term->Width*term->Height*4 - term->Cmd2D.CmdInfo.Push.Offset, Length);
		LOG("bytes = %i (0x%x), Length = %i", bytes, bytes, Length);
		
		VT_int_PutFBData(term, term->Cmd2D.CmdInfo.Push.Offset, bytes, Data );
		term->Cmd2D.CmdInfo.Push.Offset += bytes;
		ret += bytes;
		
		LOG("bytes(%i) ==? 0 || ret(%i) + Offset(%i) ==? %i",
			bytes, ret, Offset, term->Cmd2D.CurrentSize);
		if( bytes == 0 || ret + Offset >= term->Cmd2D.CurrentSize )
			return ret;
	}
	
	return -ret;
}

// > 0: Command complete
// = 0: Not enough data to start
// < 0: Ate -n bytes, still need more
typedef int	(*tVT_2DCmdHandler)(void *Handle, size_t Offset, size_t Length, const void *Data);
tVT_2DCmdHandler	gVT_2DCmdHandlers[] = {
	[PTY2D_CMD_SETCURSORPOS] = VT_int_2DCmd_SetCursorPos,
	[PTY2D_CMD_SETCURSORBMP] = VT_int_2DCmd_SetCursorBitmap,
	[PTY2D_CMD_SEND] = VT_int_2DCmd_SendData,
};
const int	ciVT_Num2DCmdHandlers = sizeof(gVT_2DCmdHandlers)/sizeof(gVT_2DCmdHandlers[0]);

void VT_int_Handle2DCmd(void *Handle, size_t Length, const void *Data)
{
	const uint8_t	*bdata = Data;
	tVTerm	*term = Handle;

	LOG("Length = 0x%x", Length);
	// If a command didn't consume all the data it said it would, we have to clean up
_eat:
	if( term->Cmd2D.PreEat )
	{
		size_t	nom = MIN(term->Cmd2D.PreEat, Length);
		bdata += nom;
		Length -= nom;
		LOG("Discarding %i/%i ignored bytes of input", nom, term->Cmd2D.PreEat);
		term->Cmd2D.PreEat -= nom;
	}

	while( Length > 0 )
	{
		const int cachesize = sizeof(term->Cmd2D.Cache);
		size_t	len, adjust;
		const void	*dataptr;
	
		// Check for command resume
		if( term->Cmd2D.Current )
		{
			LOG("Resuming %i at +0x%x with 0x%x", term->Cmd2D.Current, term->Cmd2D.Offset, Length);
			dataptr = (void*)bdata;
			len = Length;
			adjust = 0;
		}
		// else begin a new command
		else
		{
			// If the new data would fit in the cache, or the cache is already populated
			// use the cache
			// - The cache should fit the header for every command, so all good
			if( Length < cachesize || term->Cmd2D.CachePos != 0 )
			{
				adjust = term->Cmd2D.CachePos;
				size_t newbytes = MIN(cachesize - term->Cmd2D.CachePos, Length);
				memcpy(term->Cmd2D.Cache + term->Cmd2D.CachePos, bdata, newbytes);
				term->Cmd2D.CachePos += newbytes;
				dataptr = (void*)term->Cmd2D.Cache;
				len = term->Cmd2D.CachePos;
			}
			else
			{
				dataptr = (void*)bdata;
				len = Length;
				adjust = 0;
			}
			const struct ptycmd_header	*hdr = dataptr;

			// If there's not enough for the common header, wait for more
			if( len < sizeof(*hdr) )
			{
				return ;
			}			

			// Parse header
			term->Cmd2D.Offset = 0;
			term->Cmd2D.Current = hdr->cmd;
			term->Cmd2D.CurrentSize = (hdr->len_low | (hdr->len_hi << 8)) * 4;
			if( term->Cmd2D.CurrentSize == 0 ) {
				Log_Warning("VTerm", "Command size too small (==0)");
				term->Cmd2D.CurrentSize = 2;
			}
			LOG("Started %i with %s data",
				term->Cmd2D.Current, (dataptr == bdata ? "direct" : "cache"));
		}

		// Sanity check
		if( term->Cmd2D.Current >= ciVT_Num2DCmdHandlers || !gVT_2DCmdHandlers[term->Cmd2D.Current] )
		{
			Log_Notice("VTerm", "2D Comand %i not handled", term->Cmd2D.Current);
			if( Length < term->Cmd2D.CurrentSize )
				break;
			term->Cmd2D.CachePos = 0;
			term->Cmd2D.Current = 0;
			term->Cmd2D.PreEat = term->Cmd2D.CurrentSize;
			goto _eat;
		}
		
		const tVT_2DCmdHandler*	handler = &gVT_2DCmdHandlers[term->Cmd2D.Current];
		#if 0
		if( term->Cmd2D.Offset == 0 )
		{
			if( len < handler->HeaderLength ) {
				return ;
			}
			rv = handler->Header(Handle, len, dataptr);
		}
		else
		{
			rv = hander->Body(Handle, term->Cmd2D.Offset, len, dataptr);
		}
		#endif
		
		// Call Handler	
		int rv = (*handler)(Handle, term->Cmd2D.Offset, len, dataptr);
		LOG("2DCmd %i: rv=%i", term->Cmd2D.Current, rv);
		
		// If it returned 0 on the first call, it lacks space for the header
		if( rv == 0 && term->Cmd2D.Offset == 0 )
		{
			ASSERT( term->Cmd2D.CachePos != cachesize );
			// Clear current command because this command hasn't started yet
			term->Cmd2D.Current = 0;
			// Return, restart happens once all data is ready
			return ;
		}

		// Consume the byte count returned (adjust is the number of bytes that were already cached)
		size_t used_bytes = (rv < 0 ? -rv : rv) - adjust;
		Length -= used_bytes;
		bdata += used_bytes;
		term->Cmd2D.CachePos = 0;
		// If a negative count was returned, more data is expected
		if( rv < 0 ) {
			ASSERT( -rv <= len );
			LOG(" Incomplete");
			term->Cmd2D.Offset += -rv;
			continue ;
		}
		ASSERT(rv <= len);
		term->Cmd2D.Current = 0;

		// Eat up any uneaten data
		ASSERT( term->Cmd2D.Offset + rv <= term->Cmd2D.CurrentSize );
		if( term->Cmd2D.Offset + rv < term->Cmd2D.CurrentSize )
		{
			size_t	diff = term->Cmd2D.CurrentSize - (term->Cmd2D.Offset + rv);
			LOG("Left %i bytes", diff);
			term->Cmd2D.PreEat = diff;
			goto _eat;
		}
		LOG("Done (%i bytes left)", Length);
	}
}

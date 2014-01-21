/*
 * Acess2 Kernel
 * - By John Hodge
 *
 * drvutil.c
 * - Video Driver Helper Functions
 */
#define DEBUG	0
#include <acess.h>
#include <api_drv_video.h>

// === TYPES ===

// === PROTOTYPES ===
//int	DrvUtil_Video_2DStream(void *Ent, void *Buffer, int Length, tDrvUtil_Video_2DHandlers *Handlers, int SizeofHandlers);
//size_t	DrvUtil_Video_WriteLFB(int Mode, tDrvUtil_Video_BufInfo *FBInfo, size_t Offset, size_t Length, void *Src);
//void	DrvUtil_Video_SetCursor(tDrvUtil_Video_BufInfo *Buf, tVideo_IOCtl_Bitmap *Bitmap);
//void	DrvUtil_Video_DrawCursor(tDrvUtil_Video_BufInfo *Buf, int X, int Y);
void	DrvUtil_Video_RenderCursor(tDrvUtil_Video_BufInfo *Buf);
//void	DrvUtil_Video_RemoveCursor(tDrvUtil_Video_BufInfo *Buf);
void	DrvUtil_Video_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour);
void	DrvUtil_Video_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H);

// === GLOBALS ===
tDrvUtil_Video_2DHandlers	gDrvUtil_Stub_2DFunctions = {
	NULL,
	DrvUtil_Video_2D_Fill,
	DrvUtil_Video_2D_Blit
};
tVideo_IOCtl_Bitmap	gDrvUtil_TextModeCursor = {
	8, 16,
	0, 0,
	{
		0, 0, 0         , 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0,-1, 0xFF000000, 0, 0, 0, 0, 0,
		0, 0, 0         , 0, 0, 0, 0, 0,
		0, 0, 0         , 0, 0, 0, 0, 0
	}
};

// === CODE ===
// --- Video Driver Helpers ---
int DrvUtil_Video_2DStream(void *Ent, const void *Buffer, int Length,
	tDrvUtil_Video_2DHandlers *Handlers, int SizeofHandlers)
{
	const Uint8	*stream = Buffer;
	 int	rem = Length;
	 int	op;

	Uint16	tmp[6];

	while( rem )
	{
		rem --;
		op = *stream;
		stream ++;
		
		if(op > NUM_VIDEO_2DOPS) {
			Log_Warning("DrvUtil",
				"DrvUtil_Video_2DStream: Unknown operation %i",
				op);
			return Length-rem;
		}
		
		if(op*sizeof(void*) > SizeofHandlers) {
			Log_Warning("DrvUtil",
				"DrvUtil_Video_2DStream: Driver does not support op %i",
				op);
			return Length-rem;
		}
		
		switch(op)
		{
		case VIDEO_2DOP_NOP:	break;
		
		case VIDEO_2DOP_FILL:
			if(rem < 12)	return Length-rem;
			memcpy(tmp, stream, 6*2);
			
			if(!Handlers->Fill) {
				Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Driver"
					" does not support VIDEO_2DOP_FILL");
				return Length-rem;
			}
			
			Handlers->Fill(
				Ent, 
				tmp[0], tmp[1], tmp[2], tmp[3],
				tmp[4] | ((Uint32)tmp[5] << 16)
				);
			
			rem -= 12;
			stream += 12;
			break;
		
		case VIDEO_2DOP_BLIT:
			if(rem < 12)	return Length-rem;
			memcpy(tmp, stream, 6*2);
			
			if(!Handlers->Blit) {
				Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Driver"
					" does not support VIDEO_2DOP_BLIT");
				return Length-rem;
			}
			
			Handlers->Blit(
				Ent,
				tmp[0], tmp[1], tmp[2], tmp[3],
				tmp[4], tmp[5]
				);
			
			rem -= 12;
			stream += 12;
			break;
		
		}
	}
	return 0;
}

int DrvUtil_Video_WriteLFB(tDrvUtil_Video_BufInfo *FBInfo, size_t Offset, size_t Length, const void *Buffer)
{
	Uint8	*dest;
	const Uint32	*src = Buffer;
	 int	csr_x, csr_y;
	 int	x, y;
	 int	bytes_per_px = (FBInfo->Depth + 7) / 8;
	size_t	ofs;
	ENTER("pFBInfo xOffset xLength pBuffer",
		FBInfo, Offset, Length, Buffer);

	csr_x = FBInfo->CursorX;
	csr_y = FBInfo->CursorY;

	if( FBInfo->BackBuffer )
		dest = FBInfo->BackBuffer;
	else
		dest = FBInfo->Framebuffer;
	
	DrvUtil_Video_RemoveCursor(FBInfo);

	switch( FBInfo->BufferFormat )
	{
	case VIDEO_BUFFMT_TEXT:
		{
		const tVT_Char	*chars = Buffer;
		 int	widthInChars = FBInfo->Width/giVT_CharWidth;
		 int	heightInChars = FBInfo->Height/giVT_CharHeight;
		 int	i;
	
		LOG("bytes_per_px = %i", bytes_per_px);
		LOG("widthInChars = %i, heightInChars = %i", widthInChars, heightInChars);
	
		Length /= sizeof(tVT_Char);	Offset /= sizeof(tVT_Char);
		
		x = Offset % widthInChars;	y = Offset / widthInChars;
		LOG("x = %i, y = %i", x, y);	
	
		// Sanity Check
		if(Offset > heightInChars * widthInChars)	LEAVE_RET('i', 0);
		if(y >= heightInChars)	LEAVE_RET('i', 0);
		
		if( Offset + Length > heightInChars*widthInChars )
		{
			Length = heightInChars*widthInChars - Offset;
		}
		
		LOG("dest = %p", dest);
		ofs = y * giVT_CharHeight * FBInfo->Pitch;
		dest += ofs;
		
		for( i = 0; i < Length; i++ )
		{
			if( y >= heightInChars )
			{
				Log_Notice("DrvUtil", "Stopped at %i", i);
				break;
			}

			VT_Font_Render(
				chars->Ch,
				dest + x*giVT_CharWidth*bytes_per_px, FBInfo->Depth, FBInfo->Pitch,
				VT_Colour12toN(chars->BGCol, FBInfo->Depth),
				VT_Colour12toN(chars->FGCol, FBInfo->Depth)
				);
			
			chars ++;
			x ++;
			if( x >= widthInChars )
			{
				x = 0;
				y ++;
				dest += FBInfo->Pitch*giVT_CharHeight;
				LOG("dest = %p", dest);
			}
		}
		if( x > 0 ) 
			dest += FBInfo->Pitch*giVT_CharHeight;
		Length = i * sizeof(tVT_Char);
		
		break; }
	
	case VIDEO_BUFFMT_FRAMEBUFFER:
		if(FBInfo->Width*FBInfo->Height*4 < Offset+Length)
		{
			Log_Warning("DrvUtil", "DrvUtil_Video_WriteLFB - Framebuffer Overflow");
			return 0;
		}
		
		switch(FBInfo->Depth)
		{
		case 15:
		case 16:
			Log_Warning("DrvUtil", "TODO: Support 15/16 bpp modes in LFB write");
			dest = NULL;
			break;
		case 24:
			x = Offset % FBInfo->Width;
			y = Offset / FBInfo->Width;
			ofs = y*FBInfo->Pitch;
			dest += ofs;
			for( ; Length >= 4; Length -= 4 )
			{
				dest[x*3+0] = *src & 0xFF;
				dest[x*3+1] = (*src >> 8) & 0xFF;
				dest[x*3+2] = (*src >> 16) & 0xFF;
				x ++;
				if(x == FBInfo->Width) {
					dest += FBInfo->Pitch;
					x = 0;
				}
			}
			break;
		case 32:
			// Copy to Frambuffer
			if( FBInfo->Pitch != FBInfo->Width*4 )
			{
				Uint32	*px;
				// Pitch isn't 4*Width
				x = Offset % FBInfo->Width;
				y = Offset / FBInfo->Height;
				
				ofs = y*FBInfo->Pitch;
				dest += ofs;
				px = (void*)dest;

				for( ; Length >= 4; Length -= 4 )
				{
					px[x++] = *src ++;
					if( x == FBInfo->Width ) {
						x = 0;
						dest += FBInfo->Pitch;
						px = (void*)dest;
					}
				}
				if( x > 0 ) {
					dest += FBInfo->Pitch;
				}
			}
			else
			{
				ofs = Offset;
				dest += ofs;
				memcpy(dest, Buffer, Length);
				dest += Length;
			}
			break;
		default:
			Log_Warning("DrvUtil", "DrvUtil_Video_WriteLFB - Unknown bit depth %i", FBInfo->Depth);
			dest = NULL;
			break;
		}
		break;
	
	case VIDEO_BUFFMT_2DSTREAM:
		Length = DrvUtil_Video_2DStream(
			FBInfo, Buffer, Length,
			&gDrvUtil_Stub_2DFunctions, sizeof(gDrvUtil_Stub_2DFunctions)
			);
		dest = NULL;
		break;
	
	default:
		LEAVE('i', -1);
		return -1;
	}
	if( FBInfo->BackBuffer && dest ) {
		void	*_dst = (char*)FBInfo->Framebuffer + ofs;
		void	*_src = (char*)FBInfo->BackBuffer + ofs;
		size_t	len = ((tVAddr)dest - (tVAddr)FBInfo->BackBuffer) - ofs;
	//	Log_Debug("DrvUtil", "Copy from BB %p to FB %p 0x%x bytes", _src, _dst, len);
		memcpy(_dst, _src, len);
	}

	DrvUtil_Video_DrawCursor(FBInfo, csr_x, csr_y);

	LEAVE('x', Length);
	return Length;
}

int DrvUtil_Video_SetCursor(tDrvUtil_Video_BufInfo *Buf, tVideo_IOCtl_Bitmap *Bitmap)
{
	 int	csrX = Buf->CursorX, csrY = Buf->CursorY;
	size_t	size;

	ENTER("pBuf pBitmap", Buf, Bitmap);

	// Clear old bitmap
	if( Buf->CursorBitmap )
	{
		LOG("Clearing old cursor");
		DrvUtil_Video_RemoveCursor(Buf);
		if( !Bitmap || Bitmap->W != Buf->CursorBitmap->W || Bitmap->H != Buf->CursorBitmap->H )
		{
			free( Buf->CursorSaveBuf );
			Buf->CursorSaveBuf = NULL;
		}
		if( Buf->CursorBitmap != &gDrvUtil_TextModeCursor)
			free(Buf->CursorBitmap);
		Buf->CursorBitmap = NULL;
	}
	
	// If the new bitmap is null, disable drawing
	if( !Bitmap )
	{
		Buf->CursorX = -1;
		Buf->CursorY = -1;
		LEAVE('i', 0);
		return 0;
	}

	// Sanity check the bitmap
	LOG("Sanity checking plox");
	if( !CheckMem(Bitmap, sizeof(*Bitmap)) || !CheckMem(Bitmap->Data, Bitmap->W*Bitmap->H*sizeof(Uint32)) )
	{
		Log_Warning("DrvUtil", "DrvUtil_Video_SetCursor: Bitmap (%p) is in invalid memory", Bitmap);
		errno = -EINVAL;
		LEAVE('i', -1);
		return -1;
	}
	ASSERTCR(Bitmap->W, >, 0, -1);
	ASSERTCR(Bitmap->H, >, 0, -1);
	ASSERTCR(Bitmap->XOfs, <, Bitmap->W, -1);
	ASSERTCR(Bitmap->XOfs, >, -Bitmap->W, -1);
	ASSERTCR(Bitmap->YOfs, <, Bitmap->H, -1);
	ASSERTCR(Bitmap->YOfs, >, -Bitmap->H, -1);

	// Don't take a copy of the DrvUtil provided cursor
	if( Bitmap == &gDrvUtil_TextModeCursor )
	{
		LOG("No copy (provided cursor)");
		Buf->CursorBitmap = Bitmap;
	}
	else
	{
		LOG("Make copy");
		size = sizeof(tVideo_IOCtl_Bitmap) + Bitmap->W*Bitmap->H*4;
		
		// Take a copy
		Buf->CursorBitmap = malloc( size );
		memcpy(Buf->CursorBitmap, Bitmap, size);
	}
	
	// Restore cursor position
	LOG("Drawing");
	DrvUtil_Video_DrawCursor(Buf, csrX, csrY);
	LEAVE('i', 0);
	return 0;
}

void DrvUtil_Video_DrawCursor(tDrvUtil_Video_BufInfo *Buf, int X, int Y)
{
	 int	render_ox=0, render_oy=0, render_w, render_h;

	ENTER("pBuf iX iY", Buf, X, Y);
	DrvUtil_Video_RemoveCursor(Buf);

	// X < 0 disables the cursor
	if( X < 0 ) {
		Buf->CursorX = -1;
		LEAVE('-');
		return ;
	}

	// Sanity checking
	if( X < 0 || Y < 0 || X >= Buf->Width || Y >= Buf->Height ) {
		LEAVE('-');
		return ;
	}

	// Ensure the cursor is enabled
	if( !Buf->CursorBitmap ) {
		LEAVE('-');
		return ;
	}
	
	// Save cursor position (for changing the bitmap)
	Buf->CursorX = X;	Buf->CursorY = Y;
	// Apply cursor's center offset
	X -= Buf->CursorBitmap->XOfs;
	Y -= Buf->CursorBitmap->YOfs;
	
	// Get the width of the cursor on screen (clipping to right/bottom edges)
	ASSERTC(Buf->Width, >, 0);
	ASSERTC(Buf->Height, >, 0);
	ASSERTC(Buf->CursorBitmap->W, >, 0);
	ASSERTC(Buf->CursorBitmap->H, >, 0);
	
	render_w = MIN(Buf->Width  - X, Buf->CursorBitmap->W);
	render_h = MIN(Buf->Height - Y, Buf->CursorBitmap->H);
	//render_w = X > Buf->Width  - Buf->CursorBitmap->W ? Buf->Width  - X : Buf->CursorBitmap->W;
	//render_h = Y > Buf->Height - Buf->CursorBitmap->H ? Buf->Height - Y : Buf->CursorBitmap->H;

	ASSERTC(render_w, >=, 0);
	ASSERTC(render_h, >=, 0);

	// Clipp to left/top edges
	if(X < 0) {	render_ox = -X;	X = 0;	}
	if(Y < 0) {	render_oy = -Y;	Y = 0;	}

	// Save values
	Buf->CursorRenderW = render_w;	Buf->CursorRenderH = render_h;
	Buf->CursorDestX   = X;      	Buf->CursorDestY = Y;
	Buf->CursorReadX   = render_ox;	Buf->CursorReadY = render_oy;

	LOG("%ix%i at %i,%i offset %i,%i",
		render_w, render_h, X, Y, render_ox, render_oy);

	// Call render routine
	DrvUtil_Video_RenderCursor(Buf);
	LEAVE('-');
}

void DrvUtil_Video_RenderCursor(tDrvUtil_Video_BufInfo *Buf)
{
	 int	src_x = Buf->CursorReadX, src_y = Buf->CursorReadY;
	 int	render_w = Buf->CursorRenderW, render_h = Buf->CursorRenderH;
	 int	dest_x = Buf->CursorDestX, dest_y = Buf->CursorDestY;
	 int	bytes_per_px = (Buf->Depth + 7) / 8;
	 int	save_pitch = Buf->CursorBitmap->W * bytes_per_px;
	void	*dest;
	Uint32	*src;
	 int	x, y;

	dest = (Uint8*)Buf->Framebuffer + dest_y*Buf->Pitch + dest_x*bytes_per_px;
	src = Buf->CursorBitmap->Data + src_y * Buf->CursorBitmap->W + src_x;
	
	LOG("dest = %p, src = %p", dest, src);

	// Allocate save buffer if not already
	if( !Buf->CursorSaveBuf )
		Buf->CursorSaveBuf = malloc( Buf->CursorBitmap->W*Buf->CursorBitmap->H*bytes_per_px );

	ASSERTC(render_w, >=, 0);
	ASSERTC(render_h, >=, 0);

	LOG("Saving back");
	// Save behind the cursor
	for( y = 0; y < render_h; y ++ )
		memcpy(
			(Uint8*)Buf->CursorSaveBuf + y*save_pitch,
			(Uint8*)dest + y*Buf->Pitch,
			render_w*bytes_per_px
			);

	// Draw the cursor
	switch(Buf->Depth)
	{
	case 15:
	case 16:
		//Log_Warning("DrvUtil", "TODO: Support 15/16 bpp modes in cursor draw");
		//Proc_PrintBacktrace();
		break;
	case 24:
		LOG("24-bit render");
		for( y = 0; y < render_h; y ++ )
		{
			Uint8	*px;
			px = dest;
			for(x = 0; x < render_w; x ++, px += 3)
			{
				Uint32	value = src[x];
				// TODO: Should I implement alpha blending?
				if(value & 0xFF000000)
				{
					px[0] = value & 0xFF;
					px[1] = (value >> 8) & 0xFF;
					px[2] = (value >> 16) & 0xFF;
				}
				else
					;
			}
			src += Buf->CursorBitmap->W;
			dest = (Uint8*)dest + Buf->Pitch;
		}
		break;
	case 32:
		LOG("32-bit render");
		for( y = 0; y < render_h; y ++ )
		{
			Uint32	*px;
			px = dest;
			for(x = 0; x < render_w; x ++, px ++)
			{
				Uint32	value = src[x];
				// TODO: Should I implement alpha blending?
				if(value & 0xFF000000)
					*px = value;
				else
					;	// NOP, completely transparent
			}
			LOG("row %i/%i (%p-%P) done", y+1, render_h, dest, MM_GetPhysAddr(dest));
			src += Buf->CursorBitmap->W;
			dest = (Uint8*)dest + Buf->Pitch;
		}
		break;
	default:
		Log_Error("DrvUtil", "RenderCursor - Unknown bit depth %i", Buf->Depth);
		Buf->CursorX = -1;
		break;
	}
}

void DrvUtil_Video_RemoveCursor(tDrvUtil_Video_BufInfo *Buf)
{
	 int	bytes_per_px = (Buf->Depth + 7) / 8;
	 int	y, save_pitch;
	Uint8	*dest, *src;

	// Just a little sanity
	if( !Buf->CursorBitmap || Buf->CursorX == -1 )	return ;
	if( !Buf->CursorSaveBuf )	return ;

//	Debug("DrvUtil_Video_RemoveCursor: (Buf=%p) dest_x=%i, dest_y=%i", Buf, Buf->CursorDestX, Buf->CursorDestY);

	// Set up
	save_pitch = Buf->CursorBitmap->W * bytes_per_px;
	dest = (Uint8*)Buf->Framebuffer + Buf->CursorDestY * Buf->Pitch + Buf->CursorDestX*bytes_per_px;
	src = Buf->CursorSaveBuf;
	
	// Copy each line back
	for( y = 0; y < Buf->CursorRenderH; y ++ )
	{
		memcpy( dest, src, Buf->CursorRenderW * bytes_per_px );
		src += save_pitch;
		dest += Buf->Pitch;
	}
	
	// Set the cursor as removed
	Buf->CursorX = -1;
}

void DrvUtil_Video_2D_Fill(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour)
{
	tDrvUtil_Video_BufInfo	*FBInfo = Ent;

	switch( FBInfo->Depth )
	{
	case 32: {
		// TODO: Be less hacky
		size_t	pitch = FBInfo->Pitch/4;
		size_t	ofs = Y*pitch + X;
		Uint32	*buf = (Uint32*)FBInfo->Framebuffer + ofs;
		Uint32	*cbuf = NULL;
		if( FBInfo->BackBuffer )
			cbuf = (Uint32*)FBInfo->BackBuffer + ofs;
		while( H -- )
		{
			Uint32 *line;
			line = buf;
			for(int i = W; i--; line++)
				*line = Colour;
			buf += pitch;
			if( cbuf ) {
				line = cbuf;
				for(int i = W; i--; line++ )
					*line = Colour;
				cbuf += pitch;
			}
		}
		break; }
	default:
		// TODO: Handle non-32bit modes
		Log_Warning("DrvUtil", "TODO: <32bpp _Fill");
		break;
	}
}

void DrvUtil_Video_2D_Blit(void *Ent, Uint16 DstX, Uint16 DstY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H)
{
	tDrvUtil_Video_BufInfo	*FBInfo = Ent;
	 int	scrnpitch = FBInfo->Pitch;
	 int	bytes_per_px = (FBInfo->Depth + 7) / 8;
	 int	dst = DstY*scrnpitch + DstX;
	 int	src = SrcY*scrnpitch + SrcX;
	 int	tmp;
	Uint8	*framebuffer = FBInfo->Framebuffer;
	Uint8	*backbuffer = FBInfo->BackBuffer;
	Uint8	*sourcebuffer = (FBInfo->BackBuffer ? FBInfo->BackBuffer : FBInfo->Framebuffer);
	
	//Log("Vesa_2D_Blit: (Ent=%p, DstX=%i, DstY=%i, SrcX=%i, SrcY=%i, W=%i, H=%i)",
	//	Ent, DstX, DstY, SrcX, SrcY, W, H);
	
	if(SrcX + W > FBInfo->Width)	W = FBInfo->Width - SrcX;
	if(DstX + W > FBInfo->Width)	W = FBInfo->Width - DstX;
	if(SrcY + H > FBInfo->Height)	H = FBInfo->Height - SrcY;
	if(DstY + H > FBInfo->Height)	H = FBInfo->Height - DstY;
	
	//Debug("W = %i, H = %i", W, H);
	
	if(W == FBInfo->Width && FBInfo->Pitch == FBInfo->Width*bytes_per_px)
	{
		memmove(framebuffer + dst, sourcebuffer + src, H*FBInfo->Pitch);
		if( backbuffer )
			memmove(backbuffer + dst, sourcebuffer + src, H*FBInfo->Pitch);
	}
	else if( dst > src )
	{
		// Reverse copy
		dst += H*scrnpitch;
		src += H*scrnpitch;
		while( H -- )
		{
			dst -= scrnpitch;
			src -= scrnpitch;
			tmp = W*bytes_per_px;
			for( tmp = W; tmp --; )
			{
				size_t	src_o = src + tmp;
				size_t	dst_o = src + tmp;
				framebuffer[dst_o] = sourcebuffer[src_o];
				if( backbuffer )
					backbuffer[dst_o] = sourcebuffer[src_o];
			}
		}
	}
	else {
		// Normal copy is OK
		while( H -- )
		{
			memcpy(framebuffer + dst, sourcebuffer + src, W*bytes_per_px);
			if( backbuffer )
				memcpy(backbuffer + dst, sourcebuffer + src, W*bytes_per_px);
			dst += scrnpitch;
			src += scrnpitch;
		}
	}
	//Log("Vesa_2D_Blit: RETURN");
}
	


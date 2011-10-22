/*
 * Acess2 Kernel
 * - By John Hodge
 *
 * drvutil.c
 * - Common Driver/Filesystem Helper Functions
 */
#define DEBUG	0
#include <acess.h>
#include <api_drv_disk.h>
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
		 0, 0         , 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		-1, 0xFF000000, 0, 0, 0, 0, 0, 0,
		 0, 0         , 0, 0, 0, 0, 0, 0
	}
};

// === CODE ===
// --- Video Driver Helpers ---
int DrvUtil_Video_2DStream(void *Ent, void *Buffer, int Length,
	tDrvUtil_Video_2DHandlers *Handlers, int SizeofHandlers)
{
	void	*stream = Buffer;
	 int	rem = Length;
	 int	op;
	while( rem )
	{
		rem --;
		op = *(Uint8*)stream;
		stream = (void*)((tVAddr)stream + 1);
		
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
			
			if(!Handlers->Fill) {
				Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Driver"
					" does not support VIDEO_2DOP_FILL");
				return Length-rem;
			}
			
			Handlers->Fill(
				Ent,
				((Uint16*)stream)[0], ((Uint16*)stream)[1],
				((Uint16*)stream)[2], ((Uint16*)stream)[3],
				((Uint32*)stream)[2]
				);
			
			rem -= 12;
			stream = (void*)((tVAddr)stream + 12);
			break;
		
		case VIDEO_2DOP_BLIT:
			if(rem < 12)	return Length-rem;
			
			if(!Handlers->Blit) {
				Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Driver"
					" does not support VIDEO_2DOP_BLIT");
				return Length-rem;
			}
			
			Handlers->Blit(
				Ent,
				((Uint16*)stream)[0], ((Uint16*)stream)[1],
				((Uint16*)stream)[2], ((Uint16*)stream)[3],
				((Uint16*)stream)[4], ((Uint16*)stream)[5]
				);
			
			rem -= 12;
			stream = (void*)((tVAddr)stream + 12);
			break;
		
		}
	}
	return 0;
}

int DrvUtil_Video_WriteLFB(tDrvUtil_Video_BufInfo *FBInfo, size_t Offset, size_t Length, void *Buffer)
{
	Uint8	*dest;
	ENTER("pFBInfo xOffset xLength pBuffer",
		Mode, FBInfo, Offset, Length, Buffer);
	switch( FBInfo->BufferFormat )
	{
	case VIDEO_BUFFMT_TEXT:
		{
		tVT_Char	*chars = Buffer;
		 int	bytes_per_px = (FBInfo->Depth + 7) / 8;
		 int	widthInChars = FBInfo->Width/giVT_CharWidth;
		 int	heightInChars = FBInfo->Height/giVT_CharHeight;
		 int	x, y, i;
	
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
		
		dest = FBInfo->Framebuffer;
		LOG("dest = %p", dest);
		dest += y * giVT_CharHeight * FBInfo->Pitch;
		LOG("dest = %p", dest);
		
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
		Length = i * sizeof(tVT_Char);
		}
		break;
	
	case VIDEO_BUFFMT_FRAMEBUFFER:
		if(FBInfo->Width*FBInfo->Height*4 < Offset+Length)
		{
			Log_Warning("DrvUtil", "DrvUtil_Video_WriteLFB - Framebuffer Overflow");
			return 0;
		}
		
		//TODO: Handle non 32-bpp framebuffer modes
		if( FBInfo->Depth != 32 ) {
			Log_Warning("DrvUtil", "DrvUtil_Video_WriteLFB - Don't support non 32-bpp FB mode");
			return 0;
		}	

		
		//TODO: Handle pitch != Width*BytesPerPixel
		// Copy to Frambuffer
		dest = (Uint8 *)FBInfo->Framebuffer + Offset;
		memcpy(dest, Buffer, Length);
		break;
	
	case VIDEO_BUFFMT_2DSTREAM:
		Length = DrvUtil_Video_2DStream(
			FBInfo, Buffer, Length,
			&gDrvUtil_Stub_2DFunctions, sizeof(gDrvUtil_Stub_2DFunctions)
			);
		break;
	
	default:
		LEAVE('i', -1);
		return -1;
	}
	LEAVE('x', Length);
	return Length;
}

void DrvUtil_Video_SetCursor(tDrvUtil_Video_BufInfo *Buf, tVideo_IOCtl_Bitmap *Bitmap)
{
	 int	csrX = Buf->CursorX, csrY = Buf->CursorY;
	size_t	size;

	// Clear old bitmap
	if( Buf->CursorBitmap )
	{
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
		return ;
	}

	if( Bitmap != &gDrvUtil_TextModeCursor )
	{
		// Check the new bitmap is valid
		size = sizeof(tVideo_IOCtl_Bitmap) + Bitmap->W*Bitmap->H*4;
		if( !CheckMem(Bitmap, size) ) {
			Log_Warning("DrvUtil", "DrvUtil_Video_SetCursor: Bitmap (%p) is in invalid memory", Bitmap);
			return;
		}
		
		// Take a copy
		Buf->CursorBitmap = malloc( size );
		memcpy(Buf->CursorBitmap, Bitmap, size);
	}
	else
	{
		Buf->CursorBitmap = &gDrvUtil_TextModeCursor;
	}
	
	// Restore cursor position
	DrvUtil_Video_DrawCursor(Buf, csrX, csrY);
}

void DrvUtil_Video_DrawCursor(tDrvUtil_Video_BufInfo *Buf, int X, int Y)
{
	 int	render_ox=0, render_oy=0, render_w, render_h;

	DrvUtil_Video_RemoveCursor(Buf);

	// X < 0 disables the cursor
	if( X < 0 ) {
		Buf->CursorX = -1;
		return ;
	}

	// Sanity checking
	if( X < 0 || Y < 0 )	return;
	if( X >= Buf->Width || Y >= Buf->Height )	return;

	// Ensure the cursor is enabled
	if( !Buf->CursorBitmap )	return ;
	
	// Save cursor position (for changing the bitmap)
	Buf->CursorX = X;	Buf->CursorY = Y;

	// Apply cursor's center offset
	X -= Buf->CursorBitmap->XOfs;
	Y -= Buf->CursorBitmap->YOfs;
	
	// Get the width of the cursor on screen (clipping to right/bottom edges)
	render_w = X > Buf->Width  - Buf->CursorBitmap->W ? Buf->Width  - X : Buf->CursorBitmap->W;
	render_h = Y > Buf->Height - Buf->CursorBitmap->H ? Buf->Height - Y : Buf->CursorBitmap->H;

	// Clipp to left/top edges
	if(X < 0) {	render_ox = -X;	X = 0;	}
	if(Y < 0) {	render_oy = -Y;	Y = 0;	}

	// Save values
	Buf->CursorRenderW = render_w;	Buf->CursorRenderH = render_h;
	Buf->CursorDestX   = X;      	Buf->CursorDestY = Y;
	Buf->CursorReadX   = render_ox;	Buf->CursorReadY = render_oy;

	// Call render routine
	DrvUtil_Video_RenderCursor(Buf);
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

//	Debug("DrvUtil_Video_RenderCursor: (Buf=%p) dest_x=%i, dest_y=%i", Buf, dest_x, dest_y);

	dest = (Uint8*)Buf->Framebuffer + dest_y*Buf->Pitch + dest_x*bytes_per_px;
	src = Buf->CursorBitmap->Data + src_y * Buf->CursorBitmap->W + src_x;
	
	// Allocate save buffer if not already
	if( !Buf->CursorSaveBuf )
		Buf->CursorSaveBuf = malloc( Buf->CursorBitmap->W*Buf->CursorBitmap->H*bytes_per_px );

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
	case 32:
		for( y = 0; y < render_h; y ++ )
		{
			for(x = 0; x < render_w; x ++ )
			{
				// TODO: Should I implement alpha blending?
				if(src[x] & 0xFF000000)
					((Uint32*)dest)[x] = src[x];
				else
					;	// NOP, completely transparent
			}
			src += Buf->CursorBitmap->W;
			dest = (Uint8*)dest + Buf->Pitch;
		}
		break;
	default:
		Log_Error("DrvUtil", "TODO: Implement non 32-bpp cursor");
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

	// TODO: Handle non-32bit modes
	if( FBInfo->Depth != 32 )	return;

	// TODO: Be less hacky
	 int	pitch = FBInfo->Pitch/4;
	Uint32	*buf = (Uint32*)FBInfo->Framebuffer + Y*pitch + X;
	while( H -- ) {
		Uint32 *tmp;
		 int	i;
		tmp = buf;
		for(i=W;i--;tmp++)	*tmp = Colour;
		buf += pitch;
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
	
	//Log("Vesa_2D_Blit: (Ent=%p, DstX=%i, DstY=%i, SrcX=%i, SrcY=%i, W=%i, H=%i)",
	//	Ent, DstX, DstY, SrcX, SrcY, W, H);
	
	if(SrcX + W > FBInfo->Width)	W = FBInfo->Width - SrcX;
	if(DstX + W > FBInfo->Width)	W = FBInfo->Width - DstX;
	if(SrcY + H > FBInfo->Height)	H = FBInfo->Height - SrcY;
	if(DstY + H > FBInfo->Height)	H = FBInfo->Height - DstY;
	
	//Debug("W = %i, H = %i", W, H);
	
	if( dst > src ) {
		// Reverse copy
		dst += H*scrnpitch;
		src += H*scrnpitch;
		while( H -- ) {
			dst -= scrnpitch;
			src -= scrnpitch;
			tmp = W*bytes_per_px;
			for( tmp = W; tmp --; ) {
				*((Uint8*)FBInfo->Framebuffer + dst + tmp) = *((Uint8*)FBInfo->Framebuffer + src + tmp);
			}
		}
	}
	else {
		// Normal copy is OK
		while( H -- ) {
			memcpy((Uint8*)FBInfo->Framebuffer + dst, (Uint8*)FBInfo->Framebuffer + src, W*bytes_per_px);
			dst += scrnpitch;
			src += scrnpitch;
		}
	}
	//Log("Vesa_2D_Blit: RETURN");
}
	

// --- Disk Driver Helpers ---
Uint64 DrvUtil_ReadBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, Uint64 BlockSize, Uint Argument)
{
	Uint8	tmp[BlockSize];	// C99
	Uint64	block = Start / BlockSize;
	 int	offset = Start - block * BlockSize;
	 int	leading = BlockSize - offset;
	Uint64	num;
	 int	tailings;
	Uint64	ret;
	
	ENTER("XStart XLength pBuffer pReadBlocks XBlockSize xArgument",
		Start, Length, Buffer, ReadBlocks, BlockSize, Argument);
	
	// Non aligned start, let's fix that!
	if(offset != 0)
	{
		if(leading > Length)	leading = Length;
		LOG("Reading %i bytes from Block1+%i", leading, offset);
		ret = ReadBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('i', 0);
			return 0;
		}
		memcpy( Buffer, &tmp[offset], leading );
		
		if(leading == Length) {
			LEAVE('i', leading);
			return leading;
		}
		
		Buffer = (Uint8*)Buffer + leading;
		block ++;
		num = ( Length - leading ) / BlockSize;
		tailings = Length - num * BlockSize - leading;
	}
	else {
		num = Length / BlockSize;
		tailings = Length % BlockSize;
	}
	
	// Read central blocks
	if(num)
	{
		LOG("Reading %i blocks", num);
		ret = ReadBlocks(block, num, Buffer, Argument);
		if(ret != num ) {
			LEAVE('X', leading + ret * BlockSize);
			return leading + ret * BlockSize;
		}
	}
	
	// Read last tailing block
	if(tailings != 0)
	{
		LOG("Reading %i bytes from last block", tailings);
		block += num;
		Buffer = (Uint8*)Buffer + num * BlockSize;
		ret = ReadBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('X', leading + num * BlockSize);
			return leading + num * BlockSize;
		}
		memcpy( Buffer, tmp, tailings );
	}
	
	LEAVE('X', Length);
	return Length;
}

Uint64 DrvUtil_WriteBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, tDrvUtil_Callback WriteBlocks,
	Uint64 BlockSize, Uint Argument)
{
	Uint8	tmp[BlockSize];	// C99
	Uint64	block = Start / BlockSize;
	 int	offset = Start - block * BlockSize;
	 int	leading = BlockSize - offset;
	Uint64	num;
	 int	tailings;
	Uint64	ret;
	
	ENTER("XStart XLength pBuffer pReadBlocks pWriteBlocks XBlockSize xArgument",
		Start, Length, Buffer, ReadBlocks, WriteBlocks, BlockSize, Argument);
	
	// Non aligned start, let's fix that!
	if(offset != 0)
	{
		if(leading > Length)	leading = Length;
		LOG("Writing %i bytes to Block1+%i", leading, offset);
		// Read a copy of the block
		ret = ReadBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('i', 0);
			return 0;
		}
		// Modify
		memcpy( &tmp[offset], Buffer, leading );
		// Write Back
		ret = WriteBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('i', 0);
			return 0;
		}
		
		if(leading == Length) {
			LEAVE('i', leading);
			return leading;
		}
		
		Buffer = (Uint8*)Buffer + leading;
		block ++;
		num = ( Length - leading ) / BlockSize;
		tailings = Length - num * BlockSize - leading;
	}
	else {
		num = Length / BlockSize;
		tailings = Length % BlockSize;
	}
	
	// Read central blocks
	if(num)
	{
		LOG("Writing %i blocks", num);
		ret = WriteBlocks(block, num, Buffer, Argument);
		if(ret != num ) {
			LEAVE('X', leading + ret * BlockSize);
			return leading + ret * BlockSize;
		}
	}
	
	// Read last tailing block
	if(tailings != 0)
	{
		LOG("Writing %i bytes to last block", tailings);
		block += num;
		Buffer = (Uint8*)Buffer + num * BlockSize;
		// Read
		ret = ReadBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('X', leading + num * BlockSize);
			return leading + num * BlockSize;
		}
		// Modify
		memcpy( tmp, Buffer, tailings );
		// Write
		ret = WriteBlocks(block, 1, tmp, Argument);
		if(ret != 1) {
			LEAVE('X', leading + num * BlockSize);
			return leading + num * BlockSize;
		}
		
	}
	
	LEAVE('X', Length);
	return Length;
}

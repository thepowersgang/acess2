/*
 * Acess2
 * Common Driver/Filesystem Helper Functions
 */
#define DEBUG	0
#include <acess.h>
#include <api_drv_disk.h>
#include <api_drv_video.h>

// === CODE ===
// --- Video Driver Helpers ---
Uint64 DrvUtil_Video_2DStream(void *Ent, void *Buffer, int Length,
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
			Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Unknown"
				" operation %i", op);
		}
		
		if(op*sizeof(void*) > SizeofHandlers) {
			Log_Warning("DrvUtil", "DrvUtil_Video_2DStream: Driver does"
				" not support op %i", op);
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

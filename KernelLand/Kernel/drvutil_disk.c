/*
 * Acess2 Kernel
 * - By John Hodge
 *
 * drvutil_disk.c
 * - Storage Driver Helper Functions
 */
#define DEBUG	0
#include <acess.h>
#include <api_drv_disk.h>

// --- Disk Driver Helpers ---
size_t DrvUtil_ReadBlock(Uint64 Start, size_t Length, void *Buffer,
	tDrvUtil_Read_Callback ReadBlocks, size_t BlockSize, void *Argument)
{
	Uint8	tmp[BlockSize];	// C99
	Uint64	block = Start / BlockSize;
	 int	offset = Start - block * BlockSize;
	size_t	leading = BlockSize - offset;
	Uint64	num;
	 int	tailings;
	size_t	ret;
	
	ENTER("XStart XLength pBuffer pReadBlocks XBlockSize pArgument",
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
		leading = 0;
	}
	
	// Read central blocks
	if(num)
	{
		LOG("Reading %i blocks", num);
		ret = ReadBlocks(block, num, Buffer, Argument);
		if(ret != num ) {
			LOG("Incomplete read (%i != %i)", ret, num);
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

size_t DrvUtil_WriteBlock(Uint64 Start, size_t Length, const void *Buffer,
	tDrvUtil_Read_Callback ReadBlocks, tDrvUtil_Write_Callback WriteBlocks,
	size_t BlockSize, void *Argument)
{
	Uint8	tmp[BlockSize];	// C99
	Uint64	block = Start / BlockSize;
	size_t	offset = Start - block * BlockSize;
	size_t	leading = BlockSize - offset;
	Uint64	num;
	 int	tailings;
	size_t	ret;
	
	ENTER("XStart XLength pBuffer pReadBlocks pWriteBlocks XBlockSize pArgument",
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

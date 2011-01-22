/*
 */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#if __WIN32__
# include <windows.h>
#else
# include <sys/mman.h>
# include <errno.h>
#endif

// === PROTOTYPES ===
 int	AllocateMemory(uintptr_t VirtAddr, size_t ByteCount);
uintptr_t	FindFreeRange(size_t ByteCount, int MaxBits);

// === CODE ===
int AllocateMemory(uintptr_t VirtAddr, size_t ByteCount)
{
	uintptr_t	base = (VirtAddr >> 12) << 12;
	size_t	size = (VirtAddr & 0xFFF) + ByteCount;
	void	*tmp;
	#if __WIN32__
	tmp = VirtualAlloc(base, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if( tmp == NULL ) {
		printf("ERROR: Unable to allocate memory (%i)\n", GetLastError());
		return -1;
	}
	#else
	tmp = mmap((void*)base, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0);
	if( tmp == MAP_FAILED ) {
		return -1;
	}
	#endif
	return 0;
}

uintptr_t FindFreeRange(size_t ByteCount, int MaxBits)
{
	#if __WIN32__
	# error "Windows FindFreeRange() unimplemented"
	#else
	uintptr_t	base, ofs, size;
	uintptr_t	end = -1;
	static const int	PAGE_SIZE = 0x1000;
	
	size = (ByteCount + PAGE_SIZE - 1) / PAGE_SIZE;
	size *= PAGE_SIZE;

	end <<= (sizeof(intptr_t)*8-MaxBits);
	end >>= (sizeof(intptr_t)*8-MaxBits);
	printf("end = %p\n", (void*)end);
	
//	for( base = 0; base < end - size; base -= PAGE_SIZE )
	for( base = end - size + 1; base > 0; base -= PAGE_SIZE )
	{
		for( ofs = 0; ofs < size; ofs += PAGE_SIZE ) {
			if( msync( (void*)(base+ofs), 1, 0) == 0 )
				break;
			if( errno != ENOMEM )
				perror("FindFreeRange, msync");
		}
		if( ofs >= size ) {
			return base;
		}
	}
	return 0;
	#endif
}

/*
 */
#include <stdint.h>
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
	do
	{
		MEMORY_BASIC_INFORMATION	info;
		VirtualQuery( (void*)base, &info, sizeof(info) );
		if( info.State != MEM_FREE ) {
			printf("ERROR: Unable to allocate memory %p+0x%x, already allocated\n",
				(void*)base, size);
			base += 0x1000;
			if( size < 0x1000 )
				return 0;
			size -= 0x1000;
		}
		else
			break;
	} while( size >= 0x1000 );
	tmp = VirtualAlloc((void*)base, size, MEM_RESERVE|MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if( tmp == NULL ) {
		printf("ERROR: Unable to allocate memory %p+%x (0x%x)\n",
			(void*)base, size,
			(int)GetLastError());
		return -1;
	}
	#else
//	printf("AllocateMemory: mmap(%p, 0x%lx, ...)\n", (void*)base, ByteCount);
	tmp = mmap((void*)base, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0);
	if( tmp == MAP_FAILED ) {
		printf("ERROR: Unable to allocate memory\n");
		perror("AllocateMemory");
		return -1;
	}
//	printf("AllocateMemory: RETURN 0\n");
	#endif
	return 0;
}

uintptr_t FindFreeRange(size_t ByteCount, int MaxBits)
{
	uintptr_t	base, ofs, size;
	uintptr_t	end = -1;
	static const int	PAGE_SIZE = 0x1000;
	
	size = (ByteCount + PAGE_SIZE - 1) / PAGE_SIZE;
	size *= PAGE_SIZE;

	end <<= (sizeof(intptr_t)*8-MaxBits);
	end >>= (sizeof(intptr_t)*8-MaxBits);
//	printf("end = %p\n", (void*)end);
	
//	for( base = 0; base < end - size; base -= PAGE_SIZE )
	for( base = end - size + 1; base > 0; base -= PAGE_SIZE )
	{
		for( ofs = 0; ofs < size; ofs += PAGE_SIZE ) {
			#if __WIN32__
			MEMORY_BASIC_INFORMATION	info;
			VirtualQuery( (void*)(base + ofs), &info, sizeof(info) );
			if( info.State != MEM_FREE )
				break;
			#else
			if( msync( (void*)(base+ofs), 1, 0) == 0 )
				break;
			if( errno != ENOMEM )
				perror("FindFreeRange, msync");
			#endif
		}
		if( ofs >= size ) {
			return base;
		}
	}
	return 0;
}

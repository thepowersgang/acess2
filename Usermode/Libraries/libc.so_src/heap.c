/*
AcessOS Basic LibC
heap.c - Heap Manager
*/
#include <acess/sys.h>
#include <stdlib.h>
#include "lib.h"

// === Constants ===
#define MAGIC	0xACE55051	//AcessOS1
#define MAGIC_FREE	(~MAGIC)
#define BLOCK_SIZE	16	//Minimum
#define HEAP_INIT_SIZE	0x10000

typedef unsigned int Uint;

//Typedefs
typedef struct {
	Uint	magic;
	Uint	size;
}	heap_head;
typedef struct {
	heap_head	*header;
	Uint	magic;
}	heap_foot;

//Globals
void	*_heap_start = NULL;
void	*_heap_end = NULL;

//Prototypes
EXPORT void	*malloc(Uint bytes);
EXPORT void	free(void *mem);
EXPORT void	*realloc(void *mem, Uint bytes);
EXPORT void	*sbrk(int increment);
LOCAL void	*extendHeap(int bytes);
LOCAL uint	brk(Uint newpos);

//Code

/**
 \fn EXPORT void *malloc(size_t bytes)
 \brief Allocates memory from the heap space
 \param bytes	Integer - Size of buffer to return
 \return Pointer to buffer
*/
EXPORT void *malloc(size_t bytes)
{
	Uint	bestSize;
	Uint	closestMatch = 0;
	Uint	bestMatchAddr = 0;
	heap_head	*curBlock;
	
	// Initialise Heap
	if(_heap_start == NULL)
	{
		_heap_start = sbrk(0);
		_heap_end = _heap_start;
		extendHeap(HEAP_INIT_SIZE);
	}
	
	curBlock = _heap_start;
	
	bestSize = bytes + sizeof(heap_head) + sizeof(heap_foot) + BLOCK_SIZE - 1;
	bestSize = (bestSize/BLOCK_SIZE)*BLOCK_SIZE;	//Round up to block size
	
	while((Uint)curBlock < (Uint)_heap_end)
	{
		//SysDebug(" malloc: curBlock = 0x%x, curBlock->magic = 0x%x\n", curBlock, curBlock->magic);
		if(curBlock->magic == MAGIC_FREE)
		{
			if(curBlock->size == bestSize)
				break;
			if(bestSize < curBlock->size && (curBlock->size < closestMatch || closestMatch == 0)) {
				closestMatch = curBlock->size;
				bestMatchAddr = (Uint)curBlock;
			}
		}
		else if(curBlock->magic != MAGIC)
		{
			//Corrupt Heap
			//SysDebug("malloc: Corrupt Heap\n");
			return NULL;
		}
		curBlock = (heap_head*)((Uint)curBlock + curBlock->size);
	}
	
	if((Uint)curBlock < (Uint)_heap_start) {
		//SysDebug("malloc: Heap underrun for some reason\n");
		return NULL;
	}
	
	//Found a perfect match
	if((Uint)curBlock < (Uint)_heap_end) {
		curBlock->magic = MAGIC;
		return (void*)((Uint)curBlock + sizeof(heap_head));
	}
	
	//Out of Heap Space
	if(!closestMatch) {
		curBlock = extendHeap(bestSize);	//Allocate more
		if(curBlock == NULL) {
			//SysDebug("malloc: Out of Heap Space\n");
			return NULL;
		}
		curBlock->magic = MAGIC;
		return (void*)((Uint)curBlock + sizeof(heap_head));
	}
	
	//Split Block?
	if(closestMatch - bestSize > BLOCK_SIZE) {
		heap_foot	*foot;
		curBlock = (heap_head*)bestMatchAddr;
		curBlock->magic = MAGIC;
		curBlock->size = bestSize;
		foot = (heap_foot*)(bestMatchAddr + bestSize - sizeof(heap_foot));
		foot->header = curBlock;
		foot->magic = MAGIC;

		curBlock = (heap_head*)(bestMatchAddr + bestSize);
		curBlock->magic = MAGIC_FREE;
		curBlock->size = closestMatch - bestSize;
		
		foot = (heap_foot*)(bestMatchAddr + closestMatch - sizeof(heap_foot));
		foot->header = curBlock;
		
		((heap_head*)bestMatchAddr)->magic = MAGIC;	//mark as used
		return (void*)(bestMatchAddr + sizeof(heap_head));
	}
	
	//Don't Split the block
	((heap_head*)bestMatchAddr)->magic = MAGIC;
	return (void*)(bestMatchAddr+sizeof(heap_head));
}

/**
 \fn EXPORT void free(void *mem)
 \brief Free previously allocated memory
 \param mem	Pointer - Memory to free
*/
EXPORT void free(void *mem)
{
	heap_head	*head = mem;
	
	if(head->magic != MAGIC)	//Valid Heap Address
		return;
	
	head->magic = MAGIC_FREE;
	
	//Unify Right
	if((Uint)head + head->size < (Uint)_heap_end)
	{
		heap_head	*nextHead = (heap_head*)((Uint)head + head->size);
		if(nextHead->magic == MAGIC_FREE) {	//Is the next block free
			head->size += nextHead->size;	//Amalgamate
			nextHead->magic = 0;	//For Security
		}
	}
	//Unify Left
	if((Uint)head - sizeof(heap_foot) > (Uint)_heap_start)
	{
		heap_head	*prevHead;
		heap_foot	*prevFoot = (heap_foot *)((Uint)head - sizeof(heap_foot));
		if(prevFoot->magic == MAGIC) {
			prevHead = prevFoot->header;
			if(prevHead->magic == MAGIC_FREE) {
				prevHead->size += head->size;	//Amalgamate
				head->magic = 0;	//For Security
			}
		}
	}
}

/**
 \fn EXPORT void *realloc(void *oldPos, size_t bytes)
 \brief Reallocate a block of memory
 \param bytes	Integer - Size of new buffer
 \param oldPos	Pointer - Old Buffer
 \return Pointer to new buffer
*/
EXPORT void *realloc(void *oldPos, size_t bytes)
{
	void *ret;
	heap_head	*head;
	
	if(oldPos == NULL) {
		return malloc(bytes);
	}
	
	//Check for free space after block
	head = (heap_head*)((Uint)oldPos-sizeof(heap_head));
	
	//Hack to used free's amagamating algorithym and malloc's splitting
	free(oldPos);
	
	//Allocate new memory
	ret = malloc(bytes);
	if(ret == NULL)
		return NULL;
	
	//Copy Old Data
	if((Uint)ret != (Uint)oldPos) {
		memcpy(ret, oldPos, head->size-sizeof(heap_head)-sizeof(heap_foot));
	}
	
	//Return
	return ret;
}

/**
 * \fn LOCAL void *extendHeap(int bytes)
 * \brief Create a new block at the end of the heap area
 * \param bytes	Integer - Size reqired
 * \return Pointer to last free block
 */

LOCAL void *extendHeap(int bytes)
{
	heap_head	*head = _heap_end;
	heap_foot	*foot;
	
	//Expand Memory Space  (Use foot as a temp pointer)
	foot = sbrk(bytes);
	if(foot == (void*)-1)
		return NULL;
	
	
	//Create New Block
	// Header
	head->magic = MAGIC_FREE;	//Unallocated
	head->size = bytes;
	// Footer
	foot = _heap_end + bytes - sizeof(heap_foot);
	foot->header = head;
	foot->magic = MAGIC;
	
	//Combine with previous block if nessasary
	if(_heap_end != _heap_start && ((heap_foot*)((Uint)_heap_end-sizeof(heap_foot)))->magic == MAGIC) {
		heap_head	*tmpHead = ((heap_foot*)((Uint)_heap_end-sizeof(heap_foot)))->header;
		if(tmpHead->magic == MAGIC_FREE) {
			tmpHead->size += bytes;
			foot->header = tmpHead;
			head = tmpHead;
		}
	}
	
	_heap_end = (void*) ((Uint)foot+sizeof(heap_foot));
	return head;
}

/**
 \fn EXPORT void *sbrk(int increment)
 \brief Increases the program's memory space
 \param count	Integer - Size of heap increase
 \return Pointer to start of new region
*/
EXPORT void *sbrk(int increment)
{
	size_t newEnd;
	static size_t oldEnd = 0;
	static size_t curEnd = 0;

	//_SysDebug("sbrk: (increment=%i)\n", increment);

	if (oldEnd == 0)	curEnd = oldEnd = brk(0);

	//SysDebug(" sbrk: oldEnd = 0x%x\n", oldEnd);
	if (increment == 0)	return (void *) curEnd;

	newEnd = curEnd + increment;

	if (brk(newEnd) == curEnd)	return (void *) -1;
	oldEnd = curEnd;
	curEnd = newEnd;
	//SysDebug(" sbrk: newEnd = 0x%x\n", newEnd);

	return (void *) oldEnd;
}

/**
 * \fn EXPORT int IsHeap(void *ptr)
 */
EXPORT int IsHeap(void *ptr)
{
	#if 0
	heap_head	*head;
	heap_foot	*foot;
	#endif
	if( (Uint)ptr < (Uint)_heap_start )	return 0;
	if( (Uint)ptr > (Uint)_heap_end )	return 0;
	
	#if 0
	head = (void*)((Uint)ptr - 4);
	if( head->magic != MAGIC )	return 0;
	foot = (void*)( (Uint)ptr + head->size - sizeof(heap_foot) );
	if( foot->magic != MAGIC )	return 0;
	#endif
	return 1;
}

// === STATIC FUNCTIONS ===
/**
 * Does the job of brk(0)
 */
static void *FindHeapBase()
{
	#define MAX		0xC0000000	// Address
	#define THRESHOLD	512	// Pages
	uint	addr;
	uint	stretch = 0;
	
	// Scan address space
	for(addr = 0;
		addr < MAX;
		addr += 0x1000
		)
	{
		if( _SysGetPhys(addr) == 0 ) {
			stretch = 0;
		} else {
			stretch ++;
			if(stretch > THRESHOLD)
			{
				return (void*)( addr + stretch*0x1000 );
			}
		}
	}
	return NULL;
}

LOCAL uint brk(Uint newpos)
{
	static uint	curpos;
	uint	pages;
	uint	ret = curpos;
	 int	delta;
	
	//_SysDebug("brk: (newpos=0x%x)", newpos);
	
	// Find initial position
	if(curpos == 0)	curpos = (uint)FindHeapBase();
	
	// Get Current Position
	if(newpos == 0)	return curpos;
	
	if(newpos < curpos)	return newpos;
	
	delta = newpos - curpos;
	//_SysDebug(" brk: delta = 0x%x", delta);
	
	// Do we need to add pages
	if(curpos & 0xFFF && (curpos & 0xFFF) + delta < 0x1000)
		return curpos += delta;
	
	// Page align current position
	if(curpos & 0xFFF)	delta -= 0x1000 - (curpos & 0xFFF);
	curpos = (curpos + 0xFFF) & ~0xFFF;
	
	// Allocate Pages
	pages = (delta + 0xFFF) >> 12;
	while(pages--)
	{
		_SysAllocate(curpos);
		curpos += 0x1000;
		delta -= 0x1000;
	}
	
	// Bring the current position to exactly what we want
	curpos -= ((delta + 0xFFF) & ~0xFFF) - delta;
	
	return ret;	// Return old curpos
}

/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * heap.c
 * - Dynamic Memory (malloc/free)
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "lib.h"

#if 0
# define DEBUGS(s...)	_SysDebug(s)
#else
# define DEBUGS(s...)	do{}while(0)
#endif

// === Constants ===
#define MAGIC	0xACE55051	//AcessOS1
#define MAGIC_FREE	(~MAGIC)
#define BLOCK_SIZE	16	//Minimum
#define HEAP_INIT_SIZE	0x10000
#define DEBUG_HEAP	1

typedef unsigned int Uint;

// === TYPES ===
typedef struct {
	uint32_t	magic;
	size_t	size;
	// - 
	#if DEBUG_HEAP
	void	*Caller;
	size_t	RealSize;
	#endif
	char	data[];
}	heap_head;
typedef struct {
	heap_head	*header;
	uint32_t	magic;
}	heap_foot;

// === HELPERS ===
static inline heap_head *NEXT_HEAD(heap_head *ptr)
{
	return (void*)((char*)ptr + ptr->size);
}
static inline heap_foot *THIS_FOOT(heap_head *ptr)
{
	return (void*)((char*)ptr + ptr->size - sizeof(heap_foot));
}
static inline heap_foot *PREV_FOOT(heap_head *ptr)
{
	return (void*)((char*)ptr - sizeof(heap_foot));
}
static inline size_t CALC_BLOCK_SIZE(size_t bytes)
{
	size_t ret;
	ret = bytes + sizeof(heap_head) + sizeof(heap_foot) + BLOCK_SIZE - 1;
	ret = (ret/BLOCK_SIZE)*BLOCK_SIZE;	//Round up to block size
	return ret;
}

// === LOCAL VARIABLES ===
static heap_head	*_heap_start = NULL;
static heap_head	*_heap_end = NULL;
static const heap_head	_heap_zero_allocation;

// === PROTOTYPES ===
EXPORT void	*malloc(size_t bytes);
void	*_malloc(size_t bytes, void *owner);
EXPORT void	*calloc(size_t bytes, size_t count);
bool	_libc_free(void *mem);
EXPORT void	free(void *mem);
EXPORT void	*realloc(void *mem, size_t bytes);
EXPORT void	*sbrk(int increment);
LOCAL void	*extendHeap(int bytes);
static void	*FindHeapBase();
LOCAL uint	brk(uintptr_t newpos);
EXPORT int	Heap_Validate(int bDump);

//Code

/**
 \fn EXPORT void *malloc(size_t bytes)
 \brief Allocates memory from the heap space
 \param bytes	Integer - Size of buffer to return
 \return Pointer to buffer
*/
EXPORT void *malloc(size_t bytes)
{
	return _malloc(bytes, __builtin_return_address(0));
}

void *_malloc(size_t bytes, void *owner)
{
	size_t	closestMatch = 0;
	void	*bestMatchAddr = 0;

	// Initialise Heap
	if(_heap_start == NULL)
	{
		_heap_start = sbrk(0);
		_heap_end = _heap_start;
		extendHeap(HEAP_INIT_SIZE);
	}

	// Zero bytes, return a random area (in .rodata)
	if( bytes == 0 )
		return (void*)_heap_zero_allocation.data;	

	// Calculate the required block size
	size_t bestSize = CALC_BLOCK_SIZE(bytes);
	
	heap_head	*curBlock;
	for(curBlock = _heap_start; curBlock < _heap_end; curBlock = NEXT_HEAD(curBlock))
	{
		//_SysDebug(" malloc: curBlock = 0x%x, curBlock->magic = 0x%x\n", curBlock, curBlock->magic);
		if(curBlock->magic == MAGIC_FREE)
		{
			// Perfect match, nice
			if(curBlock->size == bestSize)
				break;
			// Check if this is a tighter match than the last free block
			if(bestSize < curBlock->size && (curBlock->size < closestMatch || closestMatch == 0)) {
				closestMatch = curBlock->size;
				bestMatchAddr = curBlock;
			}
		}
		else if(curBlock->magic != MAGIC)
		{
			//Corrupt Heap
			Heap_Validate(1);
			_SysDebug("malloc: Corrupt Heap when called by %p\n", owner);
			exit(128);
			return NULL;
		}
	}
	
	if(curBlock < _heap_start) {
		Heap_Validate(1);
		_SysDebug("malloc: Heap underrun for some reason\n");
		exit(128);
		return NULL;
	}
	
	//Found a perfect match
	if(curBlock < _heap_end) {
		curBlock->magic = MAGIC;
		curBlock->RealSize = bytes;
		curBlock->Caller = owner;
		return curBlock->data;
	}
	
	//Out of Heap Space
	if(!closestMatch) {
		curBlock = extendHeap(bestSize);	//Allocate more
		if(curBlock == NULL) {
			_SysDebug("malloc: Out of Heap Space\n");
			return NULL;
		}
		curBlock->magic = MAGIC;
		curBlock->RealSize = bytes;
		curBlock->Caller = owner;
		DEBUGS("malloc(0x%x) = %p (extend) 0x%x", bytes, curBlock->data, bestSize);
		return curBlock->data;
	}
	
	heap_head *besthead = (void*)bestMatchAddr;
	
	// Do we need to split this block?
	if(closestMatch - bestSize > BLOCK_SIZE) {
		heap_foot	*foot;
		
		// Block we're going to return
		curBlock = (heap_head*)bestMatchAddr;
		curBlock->magic = MAGIC;
		curBlock->size = bestSize;
		foot = THIS_FOOT(curBlock);
		foot->header = curBlock;
		foot->magic = MAGIC;

		// Remaining parts of the block
		curBlock = (heap_head*)(bestMatchAddr + bestSize);
		curBlock->magic = MAGIC_FREE;
		curBlock->size = closestMatch - bestSize;
		foot = THIS_FOOT(curBlock);
		foot->header = curBlock;
	
		// Mark return as used	
		DEBUGS("malloc(0x%x) = %p (split) 0x%x", bytes, besthead->data, bestSize);
	}
	else {
		// No need to split
		DEBUGS("malloc(0x%x) = %p (reuse) 0x%x", bytes, besthead->data, besthead->size);
	}
	
	besthead->magic = MAGIC;
	besthead->RealSize = bytes;
	besthead->Caller = owner;
	return besthead->data;
}

/**
 * \fn EXPORT void *calloc(size_t bytes, size_t count)
 * \brief Allocate and zero a block of memory
 * \param __nmemb	Number of memeber elements
 * \param __size	Size of one element
 */
EXPORT void *calloc(size_t __nmemb, size_t __size)
{
	void	*ret = _malloc(__size*__nmemb, __builtin_return_address(0));
	if(!ret)	return NULL;
	memset(ret, 0, __size*__nmemb);
	return ret;
}

/**
 \fn EXPORT void free(void *mem)
 \brief Free previously allocated memory
 \param mem	Pointer - Memory to free
*/
EXPORT void free(void *mem)
{
	if( !_libc_free(mem) ) {
		Heap_Validate(1);
		exit(0);
	}
}

bool _libc_free(void *mem)
{
	heap_head	*head = (heap_head*)mem - 1;

	// Free of NULL or the zero allocation does nothing
	if(!mem || mem == _heap_zero_allocation.data)
		return true;
	
	// Sanity check the head address
	if(head->magic != MAGIC) {
		if( head->magic != MAGIC_FREE ) {
			_SysDebug("Double free of %p by %p", mem, __builtin_return_address(0));
		}
		else {
			_SysDebug("Free of invalid pointer %p by ", mem, __builtin_return_address(0));
		}
		return false;
	}
	
	head->magic = MAGIC_FREE;
	DEBUGS("free(%p) : 0x%x bytes", mem, head->size);
	
	// Unify Right
	if( NEXT_HEAD(head) < _heap_end )
	{
		heap_head	*nextHead = NEXT_HEAD(head);
		// Is the next block free?
		if(nextHead->magic == MAGIC_FREE)
		{
			head->size += nextHead->size;	// Amalgamate
			THIS_FOOT(head)->header = head;
			nextHead->magic = 0;	//For Security
		}
	}
	
	// Unify Left
	if( head > _heap_start )
	{
		heap_foot *prevFoot = PREV_FOOT(head);
		if( prevFoot->magic != MAGIC )
		{
			_SysDebug("Heap corruption, previous foot magic invalid");
			Heap_Validate(1);
			return false;
		}
		
		heap_head *prevHead = prevFoot->header;
		if(prevHead->magic == MAGIC_FREE)
		{
			// Amalgamate
			prevHead->size += head->size;
			THIS_FOOT(prevHead)->header = prevHead;
			// For Security
			head->magic = 0;
			prevFoot->magic = 0;
			prevFoot->header = NULL;
		}
	}
	
	return true;
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
	if(oldPos == NULL || oldPos == _heap_zero_allocation.data) {
		return _malloc(bytes, __builtin_return_address(0));
	}

	size_t	reqd_size = CALC_BLOCK_SIZE(bytes);	

	// Check for free space within the block
	// - oldPos points to just after the header, so -1 gives the header
	heap_head *head = (heap_head*)oldPos - 1;
	if( head->size >= reqd_size ) {
		head->RealSize = bytes;
		return oldPos;
	}
	
	// Check for free space after the block
	heap_head *nexthead = NEXT_HEAD(head);
	assert( nexthead <= _heap_end );
	if( nexthead != _heap_end && nexthead->magic == MAGIC_FREE && head->size + nexthead->size >= reqd_size )
	{
		// Split next block
		if( head->size + nexthead->size > reqd_size )
		{
			size_t newblocksize = nexthead->size - (reqd_size - head->size);
			head->size = reqd_size;
			
			nexthead = NEXT_HEAD(head);
			nexthead->size = newblocksize;
			nexthead->magic = MAGIC_FREE;
			THIS_FOOT(nexthead)->header = nexthead;
		}
		else
		{
			head->size = reqd_size;
		}
		THIS_FOOT(head)->magic = MAGIC;
		THIS_FOOT(head)->header = head;
		head->RealSize = bytes;
		return oldPos;
	}
	
	// TODO: Should I check for free before too? What about before AND after?
	
	// Allocate new memory
	void *ret = _malloc(bytes, __builtin_return_address(0));
	if(ret == NULL)
		return NULL;
	heap_head *newhead = (heap_head*)ret - 1;
	
	// Copy Old Data
	assert( head->size < newhead->size );
	size_t copy_size = head->size-sizeof(heap_head)-sizeof(heap_foot);
	memcpy(ret, oldPos, copy_size);
	free(oldPos);
	
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
	foot = THIS_FOOT(head);
	foot->header = head;
	foot->magic = MAGIC;
	
	//Combine with previous block if nessasary
	if(_heap_end != _heap_start && ((heap_foot*)((uintptr_t)_heap_end-sizeof(heap_foot)))->magic == MAGIC) {
		heap_head	*tmpHead = ((heap_foot*)((uintptr_t)_heap_end-sizeof(heap_foot)))->header;
		if(tmpHead->magic == MAGIC_FREE) {
			tmpHead->size += bytes;
			foot->header = tmpHead;
			head = tmpHead;
		}
	}
	
	_heap_end = (void*) ((uintptr_t)foot+sizeof(heap_foot));
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
	static uintptr_t oldEnd = 0;
	static uintptr_t curEnd = 0;

	//_SysDebug("sbrk: (increment=%i)", increment);

	if (curEnd == 0) {
		oldEnd = curEnd = (uintptr_t)FindHeapBase();
		//_SysAllocate(curEnd);	// Allocate the first page
	}

	//_SysDebug(" sbrk: oldEnd = 0x%x", oldEnd);
	if (increment == 0)	return (void *) curEnd;

	oldEnd = curEnd;

	// Single Page
	if( (curEnd & 0xFFF) && (curEnd & 0xFFF) + increment < 0x1000 )
	{
		//if( curEnd & 0xFFF == 0 )
		//{
		//	if( !_SysAllocate(curEnd) )
		//	{
		//		_SysDebug("sbrk - Error allocating memory");
		//		return (void*)-1;
		//	}
		//}
		curEnd += increment;
		//_SysDebug("sbrk: RETURN %p (single page, no alloc)", (void *) oldEnd);
		return (void *)oldEnd;
	}

	increment -= curEnd & 0xFFF;
	curEnd += 0xFFF;	curEnd &= ~0xFFF;
	while( increment > 0 )
	{
		if( !_SysAllocate(curEnd) )
		{
			// Error?
			_SysDebug("sbrk - Error allocating memory");
			return (void*)-1;
		}
		increment -= 0x1000;
		curEnd += 0x1000;
	}

	//_SysDebug("sbrk: RETURN %p", (void *) oldEnd);
	return (void *) oldEnd;
}

/**
 * \fn EXPORT int IsHeap(void *ptr)
 */
EXPORT int IsHeap(void *ptr)
{
	if( ptr == _heap_zero_allocation.data )
		return 1;
	#if 0
	heap_head	*head;
	heap_foot	*foot;
	#endif
	if( (uintptr_t)ptr < (uintptr_t)_heap_start )	return 0;
	if( (uintptr_t)ptr > (uintptr_t)_heap_end )	return 0;
	
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
	#if 0
	#define MAX		0xC0000000	// Address
	#define THRESHOLD	512	// Pages
	uint	addr;
	uint	stretch = 0;
	uint64_t	tmp;
	
	// Scan address space
	for(addr = 0;
		addr < MAX;
		addr += 0x1000
		)
	{
		tmp = _SysGetPhys(addr);
		if( tmp != 0 ) {
			stretch = 0;
		} else {
			stretch ++;
			if(stretch > THRESHOLD)
			{
				return (void*)( addr - stretch*0x1000 );
			}
		}
		//__asm__ __volatile__ (
		//	"push %%ebx;mov %%edx,%%ebx;int $0xAC;pop %%ebx"
		//	::"a"(256),"d"("%x"),"c"(addr));
	}
	
	return NULL;
	#else
	return (void*)0x00900000;
	#endif
}

LOCAL uint brk(uintptr_t newpos)
{
	static uintptr_t	curpos;
	uint	pages;
	uint	ret = curpos;
	 int	delta;
	
	_SysDebug("brk: (newpos=0x%x)", newpos);
	
	// Find initial position
	if(curpos == 0)	curpos = (uintptr_t)FindHeapBase();
	
	// Get Current Position
	if(newpos == 0)	return curpos;
	
	if(newpos < curpos)	return newpos;
	
	delta = newpos - curpos;
	_SysDebug(" brk: delta = 0x%x", delta);
	
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

int Heap_Validate(int bDump)
{
	 int	rv = 0;
	heap_head *cur = _heap_start;
	while( cur < (heap_head*)_heap_end && rv == 0 )
	{
		if( cur->magic == MAGIC ) {
			if( bDump ) {
				_SysDebug("Used block %p[0x%x] - ptr=%p,Owner=%p,Size=0x%x",
					cur, cur->size, cur->data,
					cur->Caller, cur->RealSize
					);
			}
		}
		else if( cur->magic == MAGIC_FREE ) {
			if( bDump ) {
				_SysDebug("Free block %p[0x%x]", cur, cur->size, cur->data);
			}
		}
		else {
			_SysDebug("Block %p bad magic (0x%x)", cur, cur->magic);
			rv = 1;
			break ;
		}
		heap_foot *foot = THIS_FOOT(cur);
		if( foot->magic != MAGIC ) {
			_SysDebug("- %p: Foot magic clobbered (0x%x!=0x%x)", cur, foot->magic, MAGIC);
			rv = 1;
		}
		if( foot->header != cur ) {
			_SysDebug("- %p: Head pointer clobbered (%p)", cur, foot->header);
			rv = 1;
		}
	
		if( rv && !bDump ) {
			_SysDebug("%s block %p[0x%x] - ptr=%p,Owner=%p,Size=0x%x",
				(cur->magic == MAGIC ? "Used":"Free"),
				cur, cur->size, cur->data,
				cur->Caller, cur->RealSize
				);
		}	
	
		cur = NEXT_HEAD(cur);
	}
	if( rv && !bDump ) {
		_SysDebug("- Caller: %p", __builtin_return_address(0));
		exit(1);
	}
	return 0;
}


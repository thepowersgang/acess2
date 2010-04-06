/*
 * AcessOS Microkernel Version
 * heap.c
 */
#include <acess.h>
#include <mm_virt.h>
#include <heap.h>

#define WARNINGS	1
#define	DEBUG_TRACE	0

// === CONSTANTS ===
#define HEAP_BASE	0xE0800000
#define HEAP_MAX	0xF0000000	// 120MiB, Plenty
#define	HEAP_INIT_SIZE	0x8000	// 32 KiB
#define	BLOCK_SIZE	(sizeof(void*))	// 8 Machine Words
#define	COMPACT_HEAP	0	// Use 4 byte header?
#define	FIRST_FIT	0

#define	MAGIC_FOOT	0x2ACE5505
#define	MAGIC_FREE	0xACE55000
#define	MAGIC_USED	0x862B0505	// MAGIC_FOOT ^ MAGIC_FREE

// === PROTOTYPES ===
void	Heap_Install();
void	*Heap_Extend(int Bytes);
void	*Heap_Merge(tHeapHead *Head);
void	*malloc(size_t Bytes);
void	free(void *Ptr);
void	Heap_Dump();

// === GLOBALS ===
 int	glHeap;
void	*gHeapStart;
void	*gHeapEnd;

// === CODE ===
void Heap_Install()
{
	gHeapStart	= (void*)MM_KHEAP_BASE;
	gHeapEnd	= (void*)MM_KHEAP_BASE;
	Heap_Extend(HEAP_INIT_SIZE);
}

/**
 * \fn void *Heap_Extend(int Bytes)
 * \brief Extend the size of the heap
 */
void *Heap_Extend(int Bytes)
{
	Uint	i;
	tHeapHead	*head = gHeapEnd;
	tHeapFoot	*foot;
	
	// Bounds Check
	if( (Uint)gHeapEnd == MM_KHEAP_MAX )
		return NULL;
	
	// Bounds Check
	if( (Uint)gHeapEnd + ((Bytes+0xFFF)&~0xFFF) > MM_KHEAP_MAX ) {
		Bytes = MM_KHEAP_MAX - (Uint)gHeapEnd;
		return NULL;
	}
	
	// Heap expands in pages
	for(i=0;i<(Bytes+0xFFF)>>12;i++)
		MM_Allocate( (Uint)gHeapEnd+(i<<12) );
	
	// Increas heap end
	gHeapEnd += i << 12;
	
	// Create Block
	head->Size = (Bytes+0xFFF)&~0xFFF;
	head->Magic = MAGIC_FREE;
	foot = (void*)( (Uint)gHeapEnd - sizeof(tHeapFoot) );
	foot->Head = head;
	foot->Magic = MAGIC_FOOT;
	
	//Log(" Heap_Extend: head = %p", head);
	return Heap_Merge(head);	// Merge with previous block
}

/**
 * \fn void *Heap_Merge(tHeapHead *Head)
 * \brief Merges two ajacent heap blocks
 */
void *Heap_Merge(tHeapHead *Head)
{
	tHeapFoot	*foot;
	tHeapFoot	*thisFoot;
	tHeapHead	*head;
	
	//Log("Heap_Merge: (Head=%p)", Head);
	
	thisFoot = (void*)( (Uint)Head + Head->Size - sizeof(tHeapFoot) );
	if((Uint)thisFoot > (Uint)gHeapEnd)	return NULL;
	
	// Merge Left (Down)
	foot = (void*)( (Uint)Head - sizeof(tHeapFoot) );
	if( ((Uint)foot < (Uint)gHeapEnd && (Uint)foot > HEAP_BASE)
	&& foot->Head->Magic == MAGIC_FREE) {
		foot->Head->Size += Head->Size;	// Increase size
		thisFoot->Head = foot->Head;	// Change backlink
		Head->Magic = 0;	// Clear old head
		Head->Size = 0;
		Head = foot->Head;	// Save new head address
		foot->Head = NULL;	// Clear central footer
		foot->Magic = 0;
	}
	
	// Merge Right (Upwards)
	head = (void*)( (Uint)Head + Head->Size );
	if((Uint)head < (Uint)gHeapEnd && head->Magic == MAGIC_FREE)
	{
		Head->Size += head->Size;
		foot = (void*)( (Uint)Head + Head->Size - sizeof(tHeapFoot) );
		foot->Head = Head;	// Update Backlink
		thisFoot->Head = NULL;	// Clear old footer
		thisFoot->Magic = 0;
		head->Size = 0;		// Clear old header
		head->Magic = 0;
	}
	
	// Return new address
	return Head;
}

/**
 * \fn void *malloc(size_t Bytes)
 * \brief Allocate memory from the heap
 */
void *malloc(size_t Bytes)
{
	tHeapHead	*head, *newhead;
	tHeapFoot	*foot, *newfoot;
	tHeapHead	*best = NULL;
	Uint	bestSize = 0;	// Speed hack
	
	// Get required size
	Bytes = (Bytes + sizeof(tHeapHead) + sizeof(tHeapFoot) + BLOCK_SIZE-1) & ~(BLOCK_SIZE-1);
	
	// Lock Heap
	LOCK(&glHeap);
	
	// Traverse Heap
	for( head = gHeapStart;
		(Uint)head < (Uint)gHeapEnd;
		head = (void*)((Uint)head + head->Size)
		)
	{
		// Alignment Check
		if( head->Size & (BLOCK_SIZE-1) ) {
			#if WARNINGS
			Warning("Size of heap address %p is invalid not aligned (0x%x)", head, head->Size);
			Heap_Dump();
			#endif
			RELEASE(&glHeap);
			return NULL;
		}
		
		// Check if allocated
		if(head->Magic == MAGIC_USED)	continue;
		// Error check
		if(head->Magic != MAGIC_FREE)	{
			#if WARNINGS
			Warning("Magic of heap address %p is invalid (0x%x)", head, head->Magic);
			Heap_Dump();
			#endif
			RELEASE(&glHeap);	// Release spinlock
			return NULL;
		}
		
		// Size check
		if(head->Size < Bytes)	continue;
		
		// Perfect fit
		if(head->Size == Bytes) {
			head->Magic = MAGIC_USED;
			RELEASE(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			LOG("RETURN %p, to %p", best->Data, __builtin_return_address(0));
			#endif
			return best->Data;
		}
		
		// Break out of loop
		#if FIRST_FIT
		best = head;
		bestSize = head->Size;
		break;
		#else
		// or check if the block is the best size
		if(bestSize > head->Size) {
			best = head;
			bestSize = head->Size;
		}
		#endif
	}
	
	// If no block large enough is found, create one
	if(!best)
	{
		best = Heap_Extend( Bytes );
		// Check for errors
		if(!best) {
			RELEASE(&glHeap);	// Release spinlock
			return NULL;
		}
		// Check size
		if(best->Size == Bytes) {
			RELEASE(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			LOG("RETURN %p, to %p", best->Data, __builtin_return_address(0));
			#endif
			return best->Data;
		}
	}
	
	// Split Block
	newhead = (void*)( (Uint)best + Bytes );
	newfoot = (void*)( (Uint)best + Bytes - sizeof(tHeapFoot) );
	foot = (void*)( (Uint)best + best->Size - sizeof(tHeapFoot) );
	
	newfoot->Head = best;	// Create new footer
	newfoot->Magic = MAGIC_FOOT;
	newhead->Size = best->Size - Bytes;	// Create new header
	newhead->Magic = MAGIC_FREE;
	foot->Head = newhead;	// Update backlink in old footer
	best->Size = Bytes;		// Update size in old header
	best->Magic = MAGIC_USED;	// Mark block as used
	
	RELEASE(&glHeap);	// Release spinlock
	#if DEBUG_TRACE
	LOG("RETURN %p, to %p", best->Data, __builtin_return_address(0));
	#endif
	return best->Data;
}

/**
 * \fn void free(void *Ptr)
 * \brief Free an allocated memory block
 */
void free(void *Ptr)
{
	tHeapHead	*head;
	tHeapFoot	*foot;
	
	#if DEBUG_TRACE
	LOG("Ptr = %p", Ptr);
	LOG("Returns to %p", __builtin_return_address(0));
	#endif
	
	// Alignment Check
	if( (Uint)Ptr & (sizeof(Uint)-1) ) {
		Log_Warning("Heap", "free - Passed a non-aligned address (%p)", Ptr);
		return;
	}
	
	// Sanity check
	if((Uint)Ptr < (Uint)gHeapStart || (Uint)Ptr > (Uint)gHeapEnd)
	{
		Log_Warning("Heap", "free - Passed a non-heap address (%p < %p < %p)\n",
			gHeapStart, Ptr, gHeapEnd);
		return;
	}
	
	// Check memory block - Header
	head = (void*)( (Uint)Ptr - sizeof(tHeapHead) );
	if(head->Magic == MAGIC_FREE) {
		Log_Warning("Heap", "free - Passed a freed block (%p) by %p", head, __builtin_return_address(0));
		return;
	}
	if(head->Magic != MAGIC_USED) {
		Log_Warning("Heap", "free - Magic value is invalid (%p, 0x%x)\n", head, head->Magic);
		return;
	}
	
	// Check memory block - Footer
	foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
	if(foot->Head != head) {
		Log_Warning("Heap", "free - Footer backlink is incorrect (%p, 0x%x)\n", head, foot->Head);
		return;
	}
	if(foot->Magic != MAGIC_FOOT) {
		Log_Warning("Heap", "free - Footer magic is invalid (%p, %p = 0x%x)\n", head, &foot->Magic, foot->Magic);
		return;
	}
	
	// Lock
	LOCK( &glHeap );
	
	// Mark as free
	head->Magic = MAGIC_FREE;
	// Merge blocks
	Heap_Merge( head );
	
	// Release
	RELEASE( &glHeap );
}

/**
 */
void *realloc(void *__ptr, size_t __size)
{
	tHeapHead	*head = (void*)( (Uint)__ptr-8 );
	tHeapHead	*nextHead;
	tHeapFoot	*foot;
	Uint	newSize = (__size + sizeof(tHeapFoot)+sizeof(tHeapHead)+BLOCK_SIZE-1)&~(BLOCK_SIZE-1);
	
	// Check for reallocating NULL
	if(__ptr == NULL)	return malloc(__size);
	
	// Check if resize is needed
	if(newSize <= head->Size)	return __ptr;
	
	// Check if next block is free
	nextHead = (void*)( (Uint)head + head->Size );
	
	// Extend into next block
	if(nextHead->Magic == MAGIC_FREE && nextHead->Size+head->Size >= newSize)
	{
		Uint	size = nextHead->Size + head->Size;
		foot = (void*)( (Uint)nextHead + nextHead->Size - sizeof(tHeapFoot) );
		// Exact Fit
		if(size == newSize) {
			head->Size = newSize;
			foot->Head = head;
			nextHead->Magic = 0;
			nextHead->Size = 0;
			return __ptr;
		}
		// Create a new heap block
		nextHead = (void*)( (Uint)head + newSize );
		nextHead->Size = size - newSize;
		nextHead->Magic = MAGIC_FREE;
		foot->Head = nextHead;	// Edit 2nd footer
		head->Size = newSize;	// Edit first header
		// Create new footer
		foot = (void*)( (Uint)head + newSize - sizeof(tHeapFoot) );
		foot->Head = head;
		foot->Magic = MAGIC_FOOT;
		return __ptr;
	}
	
	// Extend downwards?
	foot = (void*)( (Uint)head - sizeof(tHeapFoot) );
	nextHead = foot->Head;
	if(nextHead->Magic == MAGIC_FREE && nextHead->Size+head->Size >= newSize)
	{
		Uint	size = nextHead->Size + head->Size;
		// Exact fit
		if(size == newSize)
		{
			Uint	oldDataSize;
			// Set 1st (new/lower) header
			nextHead->Magic = MAGIC_USED;
			nextHead->Size = newSize;
			// Get 2nd (old) footer
			foot = (void*)( (Uint)nextHead + newSize );
			foot->Head = nextHead;
			// Save old data size
			oldDataSize = head->Size - sizeof(tHeapFoot) - sizeof(tHeapHead);
			// Clear old header
			head->Size = 0;
			head->Magic = 0;
			memcpy(nextHead->Data, __ptr, oldDataSize);
		}
	}
	
	return NULL;
}

/**
 * \fn void *calloc(size_t num, size_t size)
 * \brief Allocate and Zero a buffer in memory
 * \param num	Number of elements
 * \param size	Size of each element
 */
void *calloc(size_t num, size_t size)
{
	void	*ret = malloc(num*size);
	if(ret == NULL)	return NULL;
	
	memset( ret, 0, num*size );
	
	return ret;
}

/**
 * \fn int IsHeap(void *Ptr)
 * \brief Checks if an address is a heap pointer
 */
int IsHeap(void *Ptr)
{
	tHeapHead	*head;
	if((Uint)Ptr < (Uint)gHeapStart)	return 0;
	if((Uint)Ptr > (Uint)gHeapEnd)	return 0;
	if((Uint)Ptr & (sizeof(Uint)-1))	return 0;
	
	head = (void*)( (Uint)Ptr - sizeof(tHeapHead) );
	if(head->Magic != MAGIC_USED && head->Magic != MAGIC_FREE)
		return 0;
	
	return 1;
}

#if WARNINGS
void Heap_Dump()
{
	tHeapHead	*head;
	tHeapFoot	*foot;
	
	head = gHeapStart;
	while( (Uint)head < (Uint)gHeapEnd )
	{		
		foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
		Log("%p (0x%x): 0x%08lx 0x%lx", head, MM_GetPhysAddr((Uint)head), head->Size, head->Magic);
		Log("%p 0x%lx", foot->Head, foot->Magic);
		Log("");
		
		// Sanity Check Header
		if(head->Size == 0) {
			Log("HALTED - Size is zero");
			break;
		}
		if(head->Size & (BLOCK_SIZE-1)) {
			Log("HALTED - Size is malaligned");
			break;
		}
		if(head->Magic != MAGIC_FREE && head->Magic != MAGIC_USED) {
			Log("HALTED - Head Magic is Bad");
			break;
		}
		
		// Check footer
		if(foot->Magic != MAGIC_FOOT) {
			Log("HALTED - Foot Magic is Bad");
			break;
		}
		if(head != foot->Head) {
			Log("HALTED - Footer backlink is invalid");
			break;
		}
		
		// All OK? Go to next
		head = foot->NextHead;
	}
}
#endif

// === EXPORTS ===
EXPORT(malloc);
EXPORT(realloc);
EXPORT(free);

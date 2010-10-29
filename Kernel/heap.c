/*
 * AcessOS Microkernel Version
 * heap.c
 */
#include <acess.h>
#include <mm_virt.h>
#include <heap_int.h>

#define WARNINGS	1
#define	DEBUG_TRACE	0
#define	VERBOSE_DUMP	0

// === CONSTANTS ===
#define	HEAP_INIT_SIZE	0x8000	// 32 KiB
#define	MIN_SIZE	(sizeof(void*))*8	// 8 Machine Words
#define	POW2_SIZES	0
#define	COMPACT_HEAP	0	// Use 4 byte header?
#define	FIRST_FIT	0

//#define	MAGIC_FOOT	0x2ACE5505
//#define	MAGIC_FREE	0xACE55000
//#define	MAGIC_USED	0x862B0505	// MAGIC_FOOT ^ MAGIC_FREE
#define	MAGIC_FOOT	0x544F4F46	// 'FOOT'
#define	MAGIC_FREE	0x45455246	// 'FREE'
#define	MAGIC_USED	0x44455355	// 'USED'

// === PROTOTYPES ===
void	Heap_Install(void);
void	*Heap_Extend(int Bytes);
void	*Heap_Merge(tHeapHead *Head);
void	*Heap_Allocate(const char *File, int Line, size_t Bytes);
void	*Heap_AllocateZero(const char *File, int Line, size_t Bytes);
void	*Heap_Reallocate(const char *File, int Line, void *Ptr, size_t Bytes);
void	Heap_Deallocate(void *Ptr);
void	Heap_Dump(void);
void	Heap_Stats(void);

// === GLOBALS ===
tMutex	glHeap;
void	*gHeapStart;
void	*gHeapEnd;

// === CODE ===
void Heap_Install(void)
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
	if( (tVAddr)gHeapEnd == MM_KHEAP_MAX )
		return NULL;
	
	// Bounds Check
	if( (tVAddr)gHeapEnd + ((Bytes+0xFFF)&~0xFFF) > MM_KHEAP_MAX ) {
		Bytes = MM_KHEAP_MAX - (tVAddr)gHeapEnd;
		return NULL;
	}
	
	// Heap expands in pages
	for(i=0;i<(Bytes+0xFFF)>>12;i++)
		MM_Allocate( (tVAddr)gHeapEnd+(i<<12) );
	
	// Increas heap end
	gHeapEnd += i << 12;
	
	// Create Block
	head->Size = (Bytes+0xFFF)&~0xFFF;
	head->Magic = MAGIC_FREE;
	foot = (void*)( (Uint)gHeapEnd - sizeof(tHeapFoot) );
	foot->Head = head;
	foot->Magic = MAGIC_FOOT;
	
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
	if( ((Uint)foot < (Uint)gHeapEnd && (Uint)foot > (Uint)gHeapStart)
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
 * \brief Allocate memory from the heap
 * \param File	Allocating source file
 * \param Line	Source line
 * \param Bytes	Size of region to allocate
 */
void *Heap_Allocate(const char *File, int Line, size_t __Bytes)
{
	tHeapHead	*head, *newhead;
	tHeapFoot	*foot, *newfoot;
	tHeapHead	*best = NULL;
	Uint	bestSize = 0;	// Speed hack
	size_t	Bytes;
	
	// Get required size
	#if POW2_SIZES
	Bytes = __Bytes + sizeof(tHeapHead) + sizeof(tHeapFoot);
	Bytes = 1UUL << LOG2(__Bytes);
	#else
	Bytes = (__Bytes + sizeof(tHeapHead) + sizeof(tHeapFoot) + MIN_SIZE-1) & ~(MIN_SIZE-1);
	#endif
	
	// Lock Heap
	Mutex_Acquire(&glHeap);
	
	// Traverse Heap
	for( head = gHeapStart;
		(Uint)head < (Uint)gHeapEnd;
		head = (void*)((Uint)head + head->Size)
		)
	{
		// Alignment Check
		#if POW2_SIZES
		if( head->Size != 1UUL << LOG2(head->Size) ) {
		#else
		if( head->Size & (MIN_SIZE-1) ) {
		#endif
			Mutex_Release(&glHeap);	// Release spinlock
			#if WARNINGS
			Log_Warning("Heap", "Size of heap address %p is invalid not aligned (0x%x)", head, head->Size);
			Heap_Dump();
			#endif
			return NULL;
		}
		
		// Check if allocated
		if(head->Magic == MAGIC_USED)	continue;
		// Error check
		if(head->Magic != MAGIC_FREE)	{
			Mutex_Release(&glHeap);	// Release spinlock
			#if WARNINGS
			Log_Warning("Heap", "Magic of heap address %p is invalid (0x%x)", head, head->Magic);
			Heap_Dump();
			#endif
			return NULL;
		}
		
		// Size check
		if(head->Size < Bytes)	continue;
		
		// Perfect fit
		if(head->Size == Bytes) {
			head->Magic = MAGIC_USED;
			head->File = File;
			head->Line = Line;
			Mutex_Release(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			Log("[Heap   ] Malloc'd %p (%i bytes), returning to %p", head->Data, head->Size,  __builtin_return_address(0));
			#endif
			return head->Data;
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
			Mutex_Release(&glHeap);	// Release spinlock
			return NULL;
		}
		// Check size
		if(best->Size == Bytes) {
			best->Magic = MAGIC_USED;	// Mark block as used
			best->File = File;
			best->Line = Line;
			Mutex_Release(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			Log("[Heap   ] Malloc'd %p (%i bytes), returning to %p", best->Data, best->Size, __builtin_return_address(0));
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
	best->ValidSize = __Bytes;
	best->Magic = MAGIC_USED;	// Mark block as used
	best->File = File;
	best->Line = Line;
	
	Mutex_Release(&glHeap);	// Release spinlock
	#if DEBUG_TRACE
	Log_Debug("Heap", "newhead(%p)->Size = 0x%x", newhead, newhead->Size);
	Log_Debug("Heap", "Malloc'd %p (0x%x bytes), returning to %s:%i",
		best->Data, best->Size, File, Line);
	#endif
	return best->Data;
}

/**
 * \fn void Heap_Deallocate(void *Ptr)
 * \brief Free an allocated memory block
 */
void Heap_Deallocate(void *Ptr)
{
	tHeapHead	*head;
	tHeapFoot	*foot;
	
	#if DEBUG_TRACE
	Log_Log("Heap", "free: Ptr = %p", Ptr);
	Log_Log("Heap", "free: Returns to %p", __builtin_return_address(0));
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
		Log_Warning("Heap", "free - Magic value is invalid (%p, 0x%x)", head, head->Magic);
		Log_Notice("Heap", "Allocated %s:%i", head->File, head->Line);
		return;
	}
	
	// Check memory block - Footer
	foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
	if(foot->Head != head) {
		Log_Warning("Heap", "free - Footer backlink is incorrect (%p, 0x%x)", head, foot->Head);
		Log_Notice("Heap", "Allocated %s:%i", head->File, head->Line);
		return;
	}
	if(foot->Magic != MAGIC_FOOT) {
		Log_Warning("Heap", "free - Footer magic is invalid (%p, %p = 0x%x)", head, &foot->Magic, foot->Magic);
		Log_Notice("Heap", "Allocated %s:%i", head->File, head->Line);
		return;
	}
	
	// Lock
	Mutex_Acquire( &glHeap );
	
	// Mark as free
	head->Magic = MAGIC_FREE;
	//head->File = NULL;
	//head->Line = 0;
	head->ValidSize = 0;
	// Merge blocks
	Heap_Merge( head );
	
	// Release
	Mutex_Release( &glHeap );
}

/**
 * \brief Increase/Decrease the size of an allocation
 * \param File	Calling File
 * \param Line	Calling Line
 * \param __ptr	Old memory
 * \param __size	New Size
 */
void *Heap_Reallocate(const char *File, int Line, void *__ptr, size_t __size)
{
	tHeapHead	*head = (void*)( (Uint)__ptr-sizeof(tHeapHead) );
	tHeapHead	*nextHead;
	tHeapFoot	*foot;
	Uint	newSize = (__size + sizeof(tHeapFoot)+sizeof(tHeapHead)+MIN_SIZE-1)&~(MIN_SIZE-1);
	
	// Check for reallocating NULL
	if(__ptr == NULL)	return Heap_Allocate(File, Line, __size);
	
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
			head->ValidSize = __size;
			head->File = File;
			head->Line = Line;
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
		head->File = File;
		head->Line = Line;
		head->ValidSize = __size;
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
		// Inexact fit, split things up
		if(size > newSize)
		{
			// TODO
			Warning("[Heap   ] TODO: Space efficient realloc when new size is smaller");
		}
		
		// Exact fit
		if(size >= newSize)
		{
			Uint	oldDataSize;
			// Set 1st (new/lower) header
			nextHead->Magic = MAGIC_USED;
			nextHead->Size = newSize;
			nextHead->File = File;
			nextHead->Line = Line;
			nextHead->ValidSize = __size;
			// Get 2nd (old) footer
			foot = (void*)( (Uint)nextHead + newSize );
			foot->Head = nextHead;
			// Save old data size
			oldDataSize = head->Size - sizeof(tHeapFoot) - sizeof(tHeapHead);
			// Clear old header
			head->Size = 0;
			head->Magic = 0;
			// Copy data
			memcpy(nextHead->Data, __ptr, oldDataSize);
			// Return
			return nextHead->Data;
		}
		// On to the expensive then
	}
	
	// Well, darn
	nextHead = Heap_Allocate( File, Line, __size );
	nextHead -= 1;
	nextHead->File = File;
	nextHead->Line = Line;
	nextHead->ValidSize = __size;
	
	memcpy(
		nextHead->Data,
		__ptr,
		head->Size - sizeof(tHeapFoot) - sizeof(tHeapHead)
		);
	
	free(__ptr);
	
	return nextHead->Data;
}

/**
 * \fn void *Heap_AllocateZero(const char *File, int Line, size_t Bytes)
 * \brief Allocate and Zero a buffer in memory
 * \param File	Allocating file
 * \param Line	Line of allocation
 * \param Bytes	Size of the allocation
 */
void *Heap_AllocateZero(const char *File, int Line, size_t Bytes)
{
	void	*ret = Heap_Allocate(File, Line, Bytes);
	if(ret == NULL)	return NULL;
	
	memset( ret, 0, Bytes );
	
	return ret;
}

/**
 * \fn int Heap_IsHeapAddr(void *Ptr)
 * \brief Checks if an address is a heap pointer
 */
int Heap_IsHeapAddr(void *Ptr)
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

/**
 */
void Heap_Validate(void)
{
	Heap_Dump();
}

#if WARNINGS
void Heap_Dump(void)
{
	tHeapHead	*head, *badHead;
	tHeapFoot	*foot = NULL;
	
	head = gHeapStart;
	while( (Uint)head < (Uint)gHeapEnd )
	{		
		foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
		#if VERBOSE_DUMP
		Log_Log("Heap", "%p (0x%llx): 0x%08lx (%i) %4C",
			head, MM_GetPhysAddr((Uint)head), head->Size, head->ValidSize, &head->Magic);
		Log_Log("Heap", "%p %4C", foot->Head, &foot->Magic);
		if(head->File) {
			Log_Log("Heap", "%sowned by %s:%i",
				(head->Magic==MAGIC_FREE?"was ":""), head->File, head->Line);
		}
		#endif
		
		// Sanity Check Header
		if(head->Size == 0) {
			Log_Warning("Heap", "HALTED - Size is zero");
			break;
		}
		if(head->Size & (MIN_SIZE-1)) {
			Log_Warning("Heap", "HALTED - Size is malaligned");
			break;
		}
		if(head->Magic != MAGIC_FREE && head->Magic != MAGIC_USED) {
			Log_Warning("Heap", "HALTED - Head Magic is Bad");
			break;
		}
		
		// Check footer
		if(foot->Magic != MAGIC_FOOT) {
			Log_Warning("Heap", "HALTED - Foot Magic is Bad");
			break;
		}
		if(head != foot->Head) {
			Log_Warning("Heap", "HALTED - Footer backlink is invalid");
			break;
		}
		
		#if VERBOSE_DUMP
		Log_Log("Heap", "");
		#endif
		
		// All OK? Go to next
		head = foot->NextHead;
	}
	
	// If the heap is valid, ok!
	if( (tVAddr)head == (tVAddr)gHeapEnd )
		return ;
	
	// Check for a bad return
	if( (tVAddr)head >= (tVAddr)gHeapEnd )
		return ;

	#if !VERBOSE_DUMP
	Log_Log("Heap", "%p (0x%llx): 0x%08lx (%i) %4C",
		head, MM_GetPhysAddr((Uint)head), head->Size, head->ValidSize, &head->Magic);
	Log_Log("Heap", "%p %4C", foot->Head, &foot->Magic);
	if(head->File) {
		Log_Log("Heap", "%sowned by %s:%i",
			(head->Magic==MAGIC_FREE?"was ":""), head->File, head->Line);
	}
	Log_Log("Heap", "");
	#endif
	
	
	badHead = head;
	
	// Work backwards
	foot = (void*)( (tVAddr)gHeapEnd - sizeof(tHeapFoot) );
	Log_Log("Heap", "==== Going Backwards ==== (from %p)", foot);
	head = foot->Head;
	while( (tVAddr)head >= (tVAddr)badHead )
	{
		Log_Log("Heap", "%p (0x%llx): 0x%08lx %i %4C",
			head, MM_GetPhysAddr((Uint)head), head->Size, head->ValidSize, &head->Magic);
		Log_Log("Heap", "%p %4C", foot->Head, &foot->Magic);
		if(head->File)
			Log_Log("Heap", "%sowned by %s:%i",
				(head->Magic!=MAGIC_USED?"was ":""),
				head->File, head->Line);
		Log_Log("Heap", "");
		
		// Sanity Check Header
		if(head->Size == 0) {
			Log_Warning("Heap", "HALTED - Size is zero");
			break;
		}
		if(head->Size & (MIN_SIZE-1)) {
			Log_Warning("Heap", " - Size is malaligned (&0x%x)", ~(MIN_SIZE-1));
			break ;
		}
		if(head->Magic != MAGIC_FREE && head->Magic != MAGIC_USED) {
			Log_Warning("Heap", "HALTED - Head Magic is Bad");
			break;
		}
		
		// Check footer
		if(foot->Magic != MAGIC_FOOT) {
			Log_Warning("Heap", "HALTED - Foot Magic is Bad");
			break;
		}
		if(head != foot->Head) {
			Log_Warning("Heap", "HALTED - Footer backlink is invalid");
			break;
		}
		
		if(head == badHead)	break;
		
		foot = (void*)( (tVAddr)head - sizeof(tHeapFoot) );
		head = foot->Head;
		Log_Debug("Heap", "head=%p", head);
	}
	
	Panic("Heap_Dump - Heap is corrupted, kernel panic!");
}
#endif

#if 1
void Heap_Stats(void)
{
	tHeapHead	*head;
	 int	nBlocks = 0;
	 int	nFree = 0;
	 int	totalBytes = 0;
	 int	freeBytes = 0;
	 int	maxAlloc=0, minAlloc=-1;
	 int	avgAlloc, frag, overhead;
	
	for(head = gHeapStart;
		(Uint)head < (Uint)gHeapEnd;
		head = (void*)( (Uint)head + head->Size )
		)
	{	
		nBlocks ++;
		totalBytes += head->Size;
		if( head->Magic == MAGIC_FREE )
		{
			nFree ++;
			freeBytes += head->Size;
		}
		else if( head->Magic == MAGIC_USED) {
			if(maxAlloc < head->Size)	maxAlloc = head->Size;
			if(minAlloc == -1 || minAlloc > head->Size)
				minAlloc = head->Size;
		}
		else {
			Log_Warning("Heap", "Magic on %p invalid, skipping remainder of heap", head);
			break;
		}
		
		// Print the block info?
		#if 1
		Log_Debug("Heap", "%p - 0x%x Owned by %s:%i",
			head, head->Size, head->File, head->Line);
		#endif
	}

	Log_Log("Heap", "%i blocks (0x%x bytes)", nBlocks, totalBytes);
	Log_Log("Heap", "%i free blocks (0x%x bytes)", nFree, freeBytes);
	frag = (nFree-1)*10000/nBlocks;
	Log_Log("Heap", "%i.%02i%% Heap Fragmentation", frag/100, frag%100);
	avgAlloc = (totalBytes-freeBytes)/(nBlocks-nFree);
	overhead = (sizeof(tHeapFoot)+sizeof(tHeapHead))*10000/avgAlloc;
	Log_Log("Heap", "Average allocation: %i bytes, Average Overhead: %i.%02i%%",
		avgAlloc, overhead/100, overhead%100
		);
	Log_Log("Heap", "Smallest Block: %i bytes, Largest: %i bytes", 
		minAlloc, maxAlloc);
	
	// Scan and get distribution
	#if 1
	{
		struct {
			Uint	Size;
			Uint	Count;
		}	sizeCounts[nBlocks];
		 int	i;
		
		memset(sizeCounts, 0, nBlocks*sizeof(sizeCounts[0]));
		
		for(head = gHeapStart;
			(Uint)head < (Uint)gHeapEnd;
			head = (void*)( (Uint)head + head->Size )
			)
		{
			for( i = 0; i < nBlocks; i ++ ) {
				if( sizeCounts[i].Size == 0 )
					break;
				if( sizeCounts[i].Size == head->Size )
					break;
			}
			// Should never reach this part (in a non-concurrent case)
			if( i == nBlocks )	continue;
			sizeCounts[i].Size = head->Size;
			sizeCounts[i].Count ++;
			#if 1
			//Log("Heap_Stats: %i %p - 0x%x bytes (%s) (%i)", nBlocks, head,
			//	head->Size, (head->Magic==MAGIC_FREE?"FREE":"used"), i
			//	);
			//Log("Heap_Stats: sizeCounts[%i] = {Size:0x%x, Count: %i}", i,
			//	sizeCounts[i].Size, sizeCounts[i].Count);
			#endif
		}
		
		for( i = 0; i < nBlocks && sizeCounts[i].Count; i ++ )
		{
			Log("Heap_Stats: 0x%x - %i blocks",
				sizeCounts[i].Size, sizeCounts[i].Count
				);
		}
	}
	#endif
}
#endif

// === EXPORTS ===
EXPORT(Heap_Allocate);
EXPORT(Heap_AllocateZero);
EXPORT(Heap_Reallocate);
EXPORT(Heap_Deallocate);
EXPORT(Heap_IsHeapAddr);

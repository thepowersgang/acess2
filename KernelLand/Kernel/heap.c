/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * heap.c
 * - Dynamic memory allocation
 */
#include <acess.h>
#include <mm_virt.h>
#include <heap_int.h>

#define WARNINGS	1	// Warn and dump on heap errors
#define	DEBUG_TRACE	0	// Enable tracing of allocations
#define	VERBOSE_DUMP	0	// Set to 1 to enable a verbose dump when heap errors are encountered

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
void	*Heap_Extend(size_t Bytes);
void	*Heap_Merge(tHeapHead *Head);
//void	*Heap_Allocate(const char *File, int Line, size_t Bytes);
//void	*Heap_AllocateZero(const char *File, int Line, size_t Bytes);
//void	*Heap_Reallocate(const char *File, int Line, void *Ptr, size_t Bytes);
//void	Heap_Deallocate(void *Ptr);
void	Heap_Dump(int bVerbose);
void	Heap_Stats(void);

// === GLOBALS ===
tMutex	glHeap;
tHeapHead	*gHeapStart;
tHeapHead	*gHeapEnd;

// === CODE ===
void Heap_Install(void)
{
	gHeapStart = (void*)MM_KHEAP_BASE;
	gHeapEnd   = gHeapStart;
	Heap_Extend(HEAP_INIT_SIZE);
}

/**
 * \brief Extend the size of the heap
 */
void *Heap_Extend(size_t Bytes)
{
	tHeapHead	*head = gHeapEnd;
	tHeapFoot	*foot;
	
	// Bounds Check
	if( gHeapEnd == (tHeapHead*)MM_KHEAP_MAX )
		return NULL;
	
	if( Bytes == 0 ) {
		Log_Warning("Heap", "Heap_Extend called with Bytes=%i", Bytes);
		return NULL;
	}
	
	const Uint	pages = (Bytes + 0xFFF) >> 12;
	tHeapHead	*new_end = (void*)( (tVAddr)gHeapEnd + (pages << 12) );
	// Bounds Check
	if( new_end > (tHeapHead*)MM_KHEAP_MAX )
	{
		// TODO: Clip allocation to avaliable space, and have caller check returned block
		return NULL;
	}
	
	// Heap expands in pages
	for( Uint i = 0; i < pages; i ++ )
	{
		if( !MM_Allocate( (tVAddr)gHeapEnd+(i<<12) ) )
		{
			Warning("OOM - Heap_Extend");
			Heap_Dump(1);
			return NULL;
		}
	}
	
	// Increase heap end
	gHeapEnd = new_end;
	
	// Create Block
	head->Size = (Bytes+0xFFF)&~0xFFF;
	head->Magic = MAGIC_FREE;
	foot = (void*)( (Uint)gHeapEnd - sizeof(tHeapFoot) );
	foot->Head = head;
	foot->Magic = MAGIC_FOOT;
	
	return Heap_Merge(head);	// Merge with previous block
}

/**
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
 * \param File	Allocating source file
 * \param Line	Source line
 * \param __Bytes	Size of region to allocate
 */
void *Heap_Allocate(const char *File, int Line, size_t __Bytes)
{
	tHeapHead	*head, *newhead;
	tHeapFoot	*foot, *newfoot;
	tHeapHead	*best = NULL;
	Uint	bestSize = 0;	// Speed hack
	size_t	Bytes;

	if( __Bytes == 0 ) {
		return NULL;	// TODO: Return a known un-mapped range.
//		return INVLPTR;
	}
	
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
	for( head = gHeapStart; head < gHeapEnd; head = (void*)((Uint)head + head->Size) )
	{
		// Alignment Check
		#if POW2_SIZES
		if( head->Size != 1UUL << LOG2(head->Size) ) {
		#else
		if( head->Size & (MIN_SIZE-1) ) {
		#endif
			Mutex_Release(&glHeap);	// Release spinlock
			#if WARNINGS
			Log_Warning("Heap", "Size of heap address %p is invalid - not aligned (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_Dump(VERBOSE_DUMP);
			#endif
			return NULL;
		}
		if( head->Size < MIN_SIZE ) {
			Mutex_Release(&glHeap);
			Log_Warning("Heap", "Size of heap address %p is invalid - Too small (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_Dump(VERBOSE_DUMP);
			return NULL;
		}
		if( head->Size > (2<<30) ) {
			Mutex_Release(&glHeap);
			Log_Warning("Heap", "Size of heap address %p is invalid - Over 2GiB (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_Dump(VERBOSE_DUMP);
			return NULL;
		}
		
		// Check if allocated
		if(head->Magic == MAGIC_USED)	continue;
		// Error check
		if(head->Magic != MAGIC_FREE)	{
			Mutex_Release(&glHeap);	// Release spinlock
			#if WARNINGS
			Log_Warning("Heap", "Magic of heap address %p is invalid (%p = 0x%x)",
				head, &head->Magic, head->Magic);
			Heap_Dump(VERBOSE_DUMP);
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
			head->ValidSize = __Bytes;
			head->AllocateTime = now();
			Mutex_Release(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			Debug("[Heap   ] Malloc'd %p (%i bytes), returning to %p",
				head->Data, head->Size,  __builtin_return_address(0));
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
			best->ValidSize = __Bytes;
			best->AllocateTime = now();
			Mutex_Release(&glHeap);	// Release spinlock
			#if DEBUG_TRACE
			Debug("[Heap   ] Malloc'd %p (%i bytes), returning to %s:%i", best->Data, best->Size, File, Line);
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
	best->AllocateTime = now();
	
	Mutex_Release(&glHeap);	// Release spinlock
	#if DEBUG_TRACE
	Debug("[Heap   ] Malloc'd %p (0x%x bytes), returning to %s:%i",
		best->Data, best->Size, File, Line);
	#endif
	return best->Data;
}

/**
 * \brief Free an allocated memory block
 */
void Heap_Deallocate(void *Ptr)
{
	tHeapHead	*head = (void*)( (Uint)Ptr - sizeof(tHeapHead) );
	tHeapFoot	*foot;
	
	// INVLPTR is returned from Heap_Allocate when the allocation
	// size is zero.
	if( Ptr == INVLPTR )	return;
	
	#if DEBUG_TRACE
	Debug("[Heap   ] free: %p freed by %p (%i old)", Ptr, __builtin_return_address(0), now()-head->AllocateTime);
	#endif
	
	// Alignment Check
	if( (Uint)Ptr & (sizeof(Uint)-1) ) {
		Log_Warning("Heap", "free - Passed a non-aligned address (%p)", Ptr);
		return;
	}
	
	// Sanity check
	if((Uint)Ptr < (Uint)gHeapStart || (Uint)Ptr > (Uint)gHeapEnd)
	{
		Log_Warning("Heap", "free - Passed a non-heap address by %p (%p < %p < %p)",
			__builtin_return_address(0), gHeapStart, Ptr, gHeapEnd);
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
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
		return;
	}
	
	// Check memory block - Footer
	foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
	if(foot->Head != head) {
		Log_Warning("Heap", "free - Footer backlink is incorrect (%p, 0x%x)", head, foot->Head);
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
		return;
	}
	if(foot->Magic != MAGIC_FOOT) {
		Log_Warning("Heap", "free - Footer magic is invalid (%p, %p = 0x%x)", head, &foot->Magic, foot->Magic);
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
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
	if(!nextHead)	return NULL;
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
	// Call dump non-verbosely.
	// - If a heap error is detected, it'll print
	Heap_Dump(0);
}

void Heap_Dump(int bVerbose)
{
	tHeapHead	*head, *badHead;
	tHeapFoot	*foot = NULL;
	static int	in_heap_dump;
	
	if( in_heap_dump )	return;

	in_heap_dump = 1;

	head = gHeapStart;
	while( (Uint)head < (Uint)gHeapEnd )
	{		
		foot = (void*)( (Uint)head + head->Size - sizeof(tHeapFoot) );
		
		if( bVerbose )
		{
			Log_Log("Heap", "%p (0x%P): 0x%08x (%i) %4C",
				head, MM_GetPhysAddr(head), head->Size, head->ValidSize, &head->Magic);
			Log_Log("Heap", "%p %4C", foot->Head, &foot->Magic);
			if(head->File) {
				Log_Log("Heap", "%sowned by %s:%i",
					(head->Magic==MAGIC_FREE?"was ":""), head->File, head->Line);
			}
		}
		
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
		
		if( bVerbose )
		{
			Log_Log("Heap", "");
		}
		
		// All OK? Go to next
		head = foot->NextHead;
	}
	
	// If the heap is valid, ok!
	if( (tVAddr)head == (tVAddr)gHeapEnd ) {
		in_heap_dump = 0;
		return ;
	}
	
	// Check for a bad return
	if( (tVAddr)head >= (tVAddr)gHeapEnd ) {
		in_heap_dump = 0;
		return ;
	}

	// If not verbose, we need to dump the failing block
	if( !bVerbose )
	{
		Log_Log("Heap", "%p (%P): 0x%08lx %i %4C",
			head, MM_GetPhysAddr(head), head->Size, head->ValidSize, &head->Magic);
		if(foot)
			Log_Log("Heap", "Foot %p = {Head:%p,Magic:%4C}", foot, foot->Head, &foot->Magic);
		if(head->File) {
			Log_Log("Heap", "%sowned by %s:%i",
				(head->Magic==MAGIC_FREE?"was ":""), head->File, head->Line);
		}
		Log_Log("Heap", "");
	}
	
	
	badHead = head;
	
	// Work backwards
	foot = (void*)( (tVAddr)gHeapEnd - sizeof(tHeapFoot) );
	Log_Log("Heap", "==== Going Backwards ==== (from %p)", foot);
	head = foot->Head;
	while( (tVAddr)head >= (tVAddr)badHead )
	{
		Log_Log("Heap", "%p (%P): 0x%08lx %i %4C",
			head, MM_GetPhysAddr(head), head->Size, head->ValidSize, &head->Magic);
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

void Heap_Stats(void)
{
	 int	nBlocks = 0;
	 int	nFree = 0;
	 int	totalBytes = 0;
	 int	freeBytes = 0;
	 int	maxAlloc=0, minAlloc=-1;
	 int	avgAlloc, frag, overhead;
	
	for(tHeapHead *head = gHeapStart; head < gHeapEnd; head = (void*)( (tVAddr)head + head->Size ) )
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
		if( head->Magic == MAGIC_FREE )
			Log_Debug("Heap", "%p (%P) - 0x%x free",
				head->Data, MM_GetPhysAddr(&head->Data), head->Size);
		else
			Log_Debug("Heap", "%p (%P) - 0x%x (%i) Owned by %s:%i (%lli ms old)",
				head->Data, MM_GetPhysAddr(&head->Data), head->Size,
				head->ValidSize, head->File, head->Line,
				now() - head->AllocateTime
				);
		#endif
	}

	Log_Log("Heap", "%i blocks (0x%x bytes)", nBlocks, totalBytes);
	Log_Log("Heap", "%i free blocks (0x%x bytes)", nFree, freeBytes);
	if(nBlocks != 0)
		frag = (nFree-1)*10000/nBlocks;
	else
		frag = 0;
	Log_Log("Heap", "%i.%02i%% Heap Fragmentation", frag/100, frag%100);
	if(nBlocks <= nFree)
		avgAlloc = 0;
	else
		avgAlloc = (totalBytes-freeBytes)/(nBlocks-nFree);
	if(avgAlloc != 0)
		overhead = (sizeof(tHeapFoot)+sizeof(tHeapHead))*10000/avgAlloc;
	else
		overhead = 0;
	Log_Log("Heap", "Average allocation: %i bytes, Average Overhead: %i.%02i%%",
		avgAlloc, overhead/100, overhead%100
		);
	Log_Log("Heap", "Smallest Block: %i bytes, Largest: %i bytes", 
		minAlloc, maxAlloc);
	
	// Scan and get distribution
	#if 1
	if(nBlocks > 0)
	{
		struct {
			Uint	Size;
			Uint	Count;
		}	sizeCounts[nBlocks];
		 int	i;
		
		memset(sizeCounts, 0, nBlocks*sizeof(sizeCounts[0]));
		
		for(tHeapHead *head = gHeapStart; head < gHeapEnd; head = (void*)( (tVAddr)head + head->Size ) )
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
			#if 0
			Log("Heap_Stats: %i %p - 0x%x bytes (%s) (%i)", nBlocks, head,
				head->Size, (head->Magic==MAGIC_FREE?"FREE":"used"), i
				);
			Log("Heap_Stats: sizeCounts[%i] = {Size:0x%x, Count: %i}", i,
				sizeCounts[i].Size, sizeCounts[i].Count);
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

// === EXPORTS ===
EXPORT(Heap_Allocate);
EXPORT(Heap_AllocateZero);
EXPORT(Heap_Reallocate);
EXPORT(Heap_Deallocate);
EXPORT(Heap_IsHeapAddr);

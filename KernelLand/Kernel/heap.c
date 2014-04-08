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
#include <limits.h>
#include <debug_hooks.h>

#define WARNINGS	1	// Warn and dump on heap errors
#define	DEBUG_TRACE	0	// Enable tracing of allocations
#define	VERBOSE_DUMP	0	// Set to 1 to enable a verbose dump when heap errors are encountered
#define VALIDATE_ON_ALLOC	1	// Set to 1 to enable validation of the heap on all malloc() calls

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
static const size_t	Heap_int_GetBlockSize(size_t AllocSize);
//void	*Heap_Allocate(const char *File, int Line, size_t Bytes);
//void	*Heap_AllocateZero(const char *File, int Line, size_t Bytes);
//void	*Heap_Reallocate(const char *File, int Line, void *Ptr, size_t Bytes);
//void	Heap_Deallocate(const char *File, int Line, void *Ptr);
 int	Heap_int_ApplyWatchpont(void *Addr, bool Enabled);
void	Heap_int_WatchBlock(tHeapHead *Head, bool Enabled);
//void	Heap_Dump(void);
void	Heap_ValidateDump(int bVerbose);
//void	Heap_Stats(void);

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

static inline tHeapHead *Heap_NextHead(tHeapHead *Head) {
	return (void*)( (char*)Head + Head->Size );
}
static inline tHeapFoot *Heap_ThisFoot(tHeapHead *Head) {
	return (void*)( (char*)Head + Head->Size - sizeof(tHeapFoot) );
}
static inline tHeapFoot *Heap_PrevFoot(tHeapHead *Head) {
	//ASSERT(Head != gHeapStart);
	return (void*)( (char*)Head - sizeof(tHeapFoot) );
}

/**
 * \brief Extend the size of the heap
 */
void *Heap_Extend(size_t Bytes)
{
	//Debug("Heap_Extend(0x%x)", Bytes);
	
	// Bounds Check
	if( gHeapEnd == (tHeapHead*)MM_KHEAP_MAX ) {
		Log_Error("Heap", "Heap limit reached (%p)", (void*)MM_KHEAP_MAX);
		return NULL;
	}
	
	if( Bytes == 0 ) {
		Log_Warning("Heap", "Heap_Extend called with Bytes=%i", Bytes);
		return NULL;
	}
	
	const Uint	pages = (Bytes + 0xFFF) >> 12;
	tHeapHead	*new_end = (void*)( (tVAddr)gHeapEnd + (pages << 12) );
	// Bounds Check
	if( new_end > (tHeapHead*)MM_KHEAP_MAX )
	{
		Log_Error("Heap", "Heap limit exceeded (%p)", (void*)new_end);
		// TODO: Clip allocation to available space, and have caller check returned block
		return NULL;
	}
	
	// Heap expands in pages
	for( Uint i = 0; i < pages; i ++ )
	{
		if( !MM_Allocate( (tPage*)gHeapEnd + i ) )
		{
			Warning("OOM - Heap_Extend (%i bytes)");
			Heap_Dump();
			return NULL;
		}
	}
	
	// Increase heap end
	tHeapHead	*head = gHeapEnd;
	gHeapEnd = new_end;
	
	// Create Block
	head->Size = (Bytes+0xFFF)&~0xFFF;
	head->Magic = MAGIC_FREE;
	tHeapFoot *foot = Heap_ThisFoot(head);
	foot->Head = head;
	foot->Magic = MAGIC_FOOT;
	
	return Heap_Merge(head);	// Merge with previous block
}

/**
 * \brief Merges two adjacent heap blocks
 */
void *Heap_Merge(tHeapHead *Head)
{
	//Log("Heap_Merge: (Head=%p)", Head);
	tHeapFoot *thisFoot = Heap_ThisFoot(Head);
	
	ASSERT( Heap_NextHead(Head) <= gHeapEnd );
	
	// Merge Left (Down)
	tHeapFoot *foot = Heap_PrevFoot(Head);
	if( Head > gHeapStart && foot->Head->Magic == MAGIC_FREE)
	{
		foot->Head->Size += Head->Size;	// Increase size
		thisFoot->Head = foot->Head;	// Change backlink
		Head->Magic = 0;	// Clear old head
		Head->Size = 0;
		Head = foot->Head;	// Save new head address
		foot->Head = NULL;	// Clear central footer
		foot->Magic = 0;
	}
	
	// Merge Right (Upwards)
	tHeapHead *nexthead = Heap_NextHead(Head);
	if(nexthead < gHeapEnd && nexthead->Magic == MAGIC_FREE)
	{
		Head->Size += nexthead->Size;
		foot = Heap_ThisFoot(Head);
		foot->Head = Head;	// Update Backlink
		thisFoot->Head = NULL;	// Clear old footer
		thisFoot->Magic = 0;
		nexthead->Size = 0;		// Clear old header
		nexthead->Magic = 0;
	}
	
	// Return new address
	return Head;
}

static const size_t Heap_int_GetBlockSize(size_t AllocSize)
{
	size_t Bytes;
	#if POW2_SIZES
	Bytes = AllocSize + sizeof(tHeapHead) + sizeof(tHeapFoot);
	Bytes = 1UUL << LOG2(Bytes);
	#else
	Bytes = (AllocSize + sizeof(tHeapHead) + sizeof(tHeapFoot) + MIN_SIZE-1) & ~(MIN_SIZE-1);
	#endif
	return Bytes;
}

/**
 * \param File	Allocating source file
 * \param Line	Source line
 * \param __Bytes	Size of region to allocate
 */
void *Heap_Allocate(const char *File, int Line, size_t __Bytes)
{
	tHeapHead	*newhead;
	tHeapFoot	*foot, *newfoot;
	tHeapHead	*best = NULL;
	Uint	bestSize = UINT_MAX;	// Speed hack
	size_t	Bytes;

	if( __Bytes == 0 ) {
		return NULL;	// TODO: Return a known un-mapped range.
//		return INVLPTR;
	}

	#if VALIDATE_ON_ALLOC
	Heap_Validate();
	#endif
	
	// Get required size
	Bytes = Heap_int_GetBlockSize(__Bytes);
	
	// Lock Heap
	Mutex_Acquire(&glHeap);
	
	// Traverse Heap
	for( tHeapHead *head = gHeapStart; head < gHeapEnd; head = Heap_NextHead(head) )
	{
		// Alignment Check
		#if POW2_SIZES
		if( head->Size != 1UUL << LOG2(head->Size) ) {
		#else
		if( head->Size & (MIN_SIZE-1) ) {
		#endif
			Mutex_Release(&glHeap);	// Release spinlock
			#if WARNINGS
			Log_Warning("Heap", "Size of heap address %p is invalid"
				" - not aligned (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_ValidateDump(VERBOSE_DUMP);
			#endif
			return NULL;
		}
		if( head->Size < MIN_SIZE ) {
			Mutex_Release(&glHeap);
			#if WARNINGS
			Log_Warning("Heap", "Size of heap address %p is invalid"
				" - Too small (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_ValidateDump(VERBOSE_DUMP);
			#endif
			return NULL;
		}
		if( head->Size > (2<<30) ) {
			Mutex_Release(&glHeap);
			#if WARNINGS
			Log_Warning("Heap", "Size of heap address %p is invalid"
				" - Over 2GiB (0x%x) [at paddr 0x%x]",
				head, head->Size, MM_GetPhysAddr(&head->Size));
			Heap_ValidateDump(VERBOSE_DUMP);
			#endif
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
			Heap_ValidateDump(VERBOSE_DUMP);
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
			Log_Debug("Heap", "Malloc'd %p (0x%x bytes), returning to %s:%i",
				head->Data, head->Size, File, Line);
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
			Warning("OOM when allocating 0x%x bytes", Bytes);
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
			Log_Debug("Heap", "Malloc'd %p (0x%x bytes), returning to %s:%i",
				best->Data, best->Size, File, Line);
			#endif
			return best->Data;
		}
	}
	
	// Split Block
	// - Save size for new block
	size_t	newsize = best->Size - Bytes;
	// - Allocate beginning of old block
	best->Size = Bytes;		// Update size in old header
	best->ValidSize = __Bytes;
	best->Magic = MAGIC_USED;	// Mark block as used
	best->File = File;
	best->Line = Line;
	best->AllocateTime = now();
	// - Create a new foot on old block
	newfoot = Heap_ThisFoot(best);
	newfoot->Head = best;	// Create new footer
	newfoot->Magic = MAGIC_FOOT;
	// - Create a new header after resized old
	newhead = Heap_NextHead(best);
	newhead->Size = newsize;
	newhead->Magic = MAGIC_FREE;
	newhead->ValidSize = 0;
	newhead->File = NULL;
	newhead->Line = 0;
	// - Update footer
	foot = Heap_ThisFoot(newhead);
	foot->Head = newhead;
	
	Mutex_Release(&glHeap);	// Release spinlock
	#if DEBUG_TRACE
	Log_Debug("Heap", "Malloc'd %p (0x%x bytes), returning to %s:%i",
		best->Data, best->Size, File, Line);
	#endif
	return best->Data;
}

/**
 * \brief Free an allocated memory block
 */
void Heap_Deallocate(const char *File, int Line, void *Ptr)
{
	// INVLPTR is returned from Heap_Allocate when the allocation
	// size is zero.
	if( Ptr == INVLPTR )	return;
	
	// Alignment Check
	if( (tVAddr)Ptr % sizeof(void*) != 0 ) {
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
	tHeapHead *head = (tHeapHead*)Ptr - 1;
	if(head->Magic == MAGIC_FREE) {
		Log_Warning("Heap", "free - Passed a freed block (%p) by %s:%i (was freed by %s:%i)",
			head, File, Line,
			head->File, head->Line);
		Proc_PrintBacktrace();
		return;
	}
	if(head->Magic != MAGIC_USED) {
		Log_Warning("Heap", "free - Magic value is invalid (%p, 0x%x)", head, head->Magic);
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
		return;
	}
	
	// Check memory block - Footer
	tHeapFoot *foot = Heap_ThisFoot(head);
	if(foot->Head != head) {
		Log_Warning("Heap", "free - Footer backlink is incorrect (%p, 0x%x)", head, foot->Head);
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
		return;
	}
	if(foot->Magic != MAGIC_FOOT) {
		Log_Warning("Heap", "free - Footer magic is invalid (%p, %p = 0x%x)",
			head, &foot->Magic, foot->Magic);
		Log_Notice("Heap", "Allocated by %s:%i", head->File, head->Line);
		return;
	}
	
	#if DEBUG_TRACE
	Log_Debug("Heap", "free: %p freed by %s:%i (%i old)",
		Ptr, File, Line, now()-head->AllocateTime);
	#endif
	
	// Lock
	Mutex_Acquire( &glHeap );

	Heap_int_WatchBlock(head, false);

	// Mark as free
	head->Magic = MAGIC_FREE;
	head->File = File;
	head->Line = Line;
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
	tHeapHead	*head = (tHeapHead*)__ptr - 1;
	tHeapHead	*nextHead;
	tHeapFoot	*foot;
	Uint	newSize = Heap_int_GetBlockSize(__size);
	
	// Check for reallocating NULL
	if(__ptr == NULL)
		return Heap_Allocate(File, Line, __size);
	
	if( !Heap_IsHeapAddr(__ptr)) {
		Log_Error("Heap", "%s:%i passed non-heap address %p when reallocating to %zi",
			File, Line, __ptr, __size);
		return NULL;
	}
	
	// Check if resize is needed
	// TODO: Reduce size of block if needed
	if(newSize <= head->Size) {
		#if DEBUG_TRACE
		Log_Debug("Heap", "realloc maintain %p (0x%x >= 0x%x), returning to %s:%i",
			head->Data, head->Size, newSize, File, Line);
		#endif
		return __ptr;
	}

	#if VALIDATE_ON_ALLOC
	Heap_Validate();
	#endif
	
	Heap_int_WatchBlock(head, false);
	
	// Check if next block is free
	nextHead = Heap_NextHead(head);
	
	// Extend into next block
	if(nextHead->Magic == MAGIC_FREE && nextHead->Size+head->Size >= newSize)
	{
		#if DEBUG_TRACE
		Log_Debug("Heap", "realloc expand %p (0x%x to 0x%x), returning to %s:%i",
			head->Data, head->Size, newSize, File, Line);
		#endif
		Uint	size = nextHead->Size + head->Size;
		foot = Heap_ThisFoot(nextHead);
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
		// - Update old with new information
		head->Size = newSize;
		head->File = File;	// Kinda counts as a new allocation
		head->Line = Line;
		head->ValidSize = __size;
		// - Create new footer
		foot = Heap_ThisFoot(head);
		foot->Head = head;
		foot->Magic = MAGIC_FOOT;
		// - Create new header
		nextHead = Heap_NextHead(head);
		nextHead->Size = size - newSize;
		nextHead->Magic = MAGIC_FREE;
		// - Update old footer
		Heap_ThisFoot(nextHead)->Head = nextHead;
		return __ptr;
	}
	
	// Extend downwards?
	foot = Heap_PrevFoot(head);
	nextHead = foot->Head;
	if(nextHead->Magic == MAGIC_FREE && nextHead->Size+head->Size >= newSize)
	{
		Uint	size = nextHead->Size + head->Size;
		// Inexact fit, split things up
		if(size > newSize)
		{
			// TODO: Handle splitting of downwards blocks
			Warning("[Heap   ] TODO: Space efficient realloc when new size is smaller");
		}
		
		#if DEBUG_TRACE
		Log_Debug("Heap", "realloc expand down %p (0x%x to 0x%x), returning to %s:%i",
			head->Data, head->Size, newSize, File, Line);
		#endif
		
		// Exact fit
		Uint	oldDataSize;
		// Set 1st (new/lower) header
		nextHead->Magic = MAGIC_USED;
		nextHead->Size = newSize;
		nextHead->File = File;
		nextHead->Line = Line;
		nextHead->ValidSize = __size;
		// Get 2nd (old) footer
		foot = Heap_ThisFoot(nextHead);
		foot->Head = nextHead;
		// Save old data size
		oldDataSize = head->Size - sizeof(tHeapFoot) - sizeof(tHeapHead);
		// Clear old header
		head->Size = 0;
		head->Magic = 0;
		// Copy data
		memmove(nextHead->Data, __ptr, oldDataSize);
		// Return
		return nextHead->Data;
	}
	
	// Well, darn
	nextHead = Heap_Allocate( File, Line, __size );
	if(!nextHead)	return NULL;
	nextHead -= 1;
	nextHead->File = File;
	nextHead->Line = Line;
	nextHead->ValidSize = __size;
	
	ASSERTC(head->Size, <, nextHead->Size);
	ASSERTC(__ptr, ==, head->Data);
	memcpy(
		nextHead->Data,
		__ptr,
		head->Size - sizeof(tHeapFoot) - sizeof(tHeapHead)
		);
	
	free(__ptr);
	Heap_Validate();

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

int Heap_int_ApplyWatchpont(void *Word, bool enabled)
{
	#if ARCHDIR_IS_x86
	static void	*active_wps[4];
	unsigned int	dr;
	for( dr = 2; dr < 4; dr ++ )
	{
		if( (enabled && active_wps[dr] == NULL) || active_wps[dr] == Word)
			break;
	}
	if(dr == 4) {
		return 1;
	}
	if( enabled )
	{
		active_wps[dr] = Word;
		switch(dr)
		{
		//case 0:	ASM("mov %0, %%dr0" : : "r" (Word));	break;
		//case 1:	ASM("mov %0, %%dr1" : : "r" (Word));	break;
		case 2:	ASM("mov %0, %%dr2" : : "r" (Word));	break;
		case 3:	ASM("mov %0, %%dr3" : : "r" (Word));	break;
		default:	ASSERTC(dr,<,4);	return 1;
		}
	}
	else
	{
		active_wps[dr] = NULL;
	}
	Uint32	dr7flag;
	ASM("MOV %%dr7, %0" : "=r" (dr7flag));
	dr7flag &= ~(0x2 << (dr*2));
	dr7flag &= ~(0xF000 << (dr*4));
	if( enabled ) {
		dr7flag |= 0x2 << (dr*2);
		dr7flag |= 0xD000 << (dr*4);	// 4 bytes, write
		Debug("Heap_int_ApplyWatchpont: Watchpoint #%i %p ENABLED", dr, Word);
	}
	else {
		Debug("Heap_int_ApplyWatchpont: Watchpoint #%i %p disabled", dr, Word);
	}
	ASM("MOV %0, %%dr7" : : "r" (dr7flag));
	return 0;
	#else
	return 1;
	#endif
}

void Heap_int_WatchBlock(tHeapHead *Head, bool Enabled)
{
	int rv;
	rv = Heap_int_ApplyWatchpont( &Head->Size, Enabled );
	//rv = Heap_int_ApplyWatchpont( &Head->Magic, Enabled );
	rv = rv + Heap_int_ApplyWatchpont( &Heap_ThisFoot(Head)->Head, Enabled );
	if(rv && Enabled) {
		Warning("Can't apply watch on %p", Head);
	}
}

int Heap_WatchBlock(void *Ptr)
{
	//Heap_int_ApplyWatchpont();
	tHeapHead	*head;
	if((Uint)Ptr < (Uint)gHeapStart)	return 0;
	if((Uint)Ptr > (Uint)gHeapEnd)	return 0;
	if((Uint)Ptr & (sizeof(Uint)-1))	return 0;
	
	head = (tHeapHead*)Ptr - 1;
	
	Heap_int_WatchBlock( head, true );
	
	return 0;
}

/**
 */
void Heap_Validate(void)
{
	// Call dump non-verbosely.
	// - If a heap error is detected, it'll print
	Heap_ValidateDump(0);
}

void Heap_Dump(void)
{
	Heap_ValidateDump(1);
}

void Heap_ValidateDump(int bVerbose)
{
	tHeapHead	*head, *badHead;
	tHeapFoot	*foot = NULL;
	static int	in_heap_dump;
	
	if( in_heap_dump )	return;

	in_heap_dump = 1;

	head = gHeapStart;
	while( (Uint)head < (Uint)gHeapEnd )
	{		
		foot = Heap_ThisFoot(head);
		
		if( bVerbose )
		{
			Log_Log("Heap", "%p (%P): 0x%08x (%i) %4C",
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
	foot = Heap_PrevFoot(gHeapEnd);
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
		
		foot = Heap_PrevFoot(head);
		head = foot->Head;
		Log_Debug("Heap", "head=%p", head);
	}
	
	Panic("Heap_Dump - Heap is corrupted, kernel panic! (%p)", badHead);
}

void Heap_Stats(void)
{
	 int	nBlocks = 0;
	 int	nFree = 0;
	 int	totalBytes = 0;
	 int	freeBytes = 0;
	 int	maxAlloc=0, minAlloc=-1;
	 int	avgAlloc, frag, overhead;
	
	for( tHeapHead *head = gHeapStart; head < gHeapEnd; head = Heap_NextHead(head) )
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

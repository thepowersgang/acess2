/*
 */
#include <acess.h>
#include <mm_virt.h>
#include "include/vmem_layout.h"
#include <threads_int.h>
#include <stdbool.h>

extern Uint64	giPageCount;
extern tProcess	*gAllProcesses;
extern char	gKernelEnd[];

// === PROTOTYPES ===
void	Validate_VirtualMemoryUsage(void);
void	Validate_VirtualMemoryUsage_PT(const Uint32 *PTable, const tProcess *Proc, int PDIndex, int *RefCounts);
static bool	_should_skip_pde(const tProcess *Proc, int idx, Uint32 pde);
static void	_ref_mem(int *RefCounts, Uint PageNum, const tProcess *Proc, void *addr);
static void	_load_page(Uint32 *dst, tPAddr phys);

// === CODE ===
void Validate_VirtualMemoryUsage(void)
{
	 int	*refcount = calloc(giPageCount, sizeof(int));
	Uint32	*tmp_pd = malloc( PAGE_SIZE );
	Uint32	*tmp_pt = malloc( PAGE_SIZE );
	
	// Pass #1 - Count PT references
	for( tProcess *proc = gAllProcesses; proc; proc = proc->Next )
	{
		Uint	cr3 = proc->MemState.CR3;
		_ref_mem(refcount, cr3/PAGE_SIZE, proc, 0);

		_load_page(tmp_pd, cr3);
		const Uint32	*pdir = tmp_pd;
		for(int i = 0; i < 1024; i ++)
		{
			if( _should_skip_pde(proc, i, pdir[i]) )
				continue ;
			Uint32	ptaddr = pdir[i] & ~0xFFF;
			
			_ref_mem(refcount, ptaddr/PAGE_SIZE, proc, (void*)(i*1024*PAGE_SIZE));
		}
	}
	
	
	for( tProcess *proc = gAllProcesses; proc; proc = proc->Next )
	{
		Uint	cr3 = proc->MemState.CR3;
		
		_load_page(tmp_pd, cr3);
		const Uint32	*pdir = tmp_pd;
		for(int i = 0; i < 1024; i ++)
		{
			Uint32	pde = pdir[i];
			if( _should_skip_pde(proc, i, pde) )
				continue ;
			
			Uint32	ptaddr = pde & ~0xFFF;
			
			// 
			if( proc->PID == 0 || refcount[ ptaddr >> 12 ] == 1 )
			{
				_load_page(tmp_pt, ptaddr);
				Validate_VirtualMemoryUsage_PT(tmp_pt, proc, i, refcount);
			}
		}
	}
	
	for( int i = 0; i < giPageCount; i ++ )
	{
		 int	recorded_refc = MM_GetRefCount(i*PAGE_SIZE);
		
		void	*node;
		if( MM_GetPageNode(i*PAGE_SIZE, &node) == 0 && node != NULL )
			_ref_mem(refcount, i, NULL, 0);
		
		// 0x9F000 - BIOS memory area
		// 0xA0000+ - ROM area
		if( 0x9F <= i && i < 0x100 ) {
			_ref_mem(refcount, i, NULL, 0);
		}
		
		if( recorded_refc != refcount[i] ) {
			Debug("Refcount mismatch %P - recorded %i != checked %i",
				(tPAddr)i*PAGE_SIZE, recorded_refc, refcount[i]);
		}
	}
	
	// Output.
	for( tProcess *proc = gAllProcesses; proc; proc = proc->Next )
	{
		Debug("%p(%i %s)", proc, proc->PID, proc->FirstThread->ThreadName);
		Uint	cr3 = proc->MemState.CR3;
		
		_load_page(tmp_pd, cr3);
		const Uint32	*pdir = tmp_pd;
		for(int i = 0; i < 1024; i ++)
		{
			Uint32	pde = pdir[i];
			if( _should_skip_pde(proc, i, pde) )
				continue ;
			Uint32	paddr = pde & ~0xFFF;
			
			if( proc->PID != 0 && refcount[ paddr >> 12 ] > 1 )
				continue ;
			
			if( MM_GetRefCount(paddr) != refcount[paddr>>12] ) {
				Debug("Table %p(%i %s) @%p %P %i!=%i",
					proc, proc->PID, proc->FirstThread->ThreadName,
					i*1024*PAGE_SIZE, paddr,
					MM_GetRefCount(paddr), refcount[paddr>>12]
					);
			}
	
			_load_page(tmp_pt, paddr);
			const Uint32 *ptab = tmp_pt;
			for( int j = 0; j < 1024; j ++ ) 
			{
				if( !(ptab[j] & 1) )
					continue ;
				Uint32	pageaddr = ptab[j] & ~0xFFF;
				
				if( pageaddr/PAGE_SIZE >= giPageCount )
				{
					//Debug("PG %p(%i %s) @%p %P not in RAM",
					//	proc, proc->PID, proc->FirstThread->ThreadName,
					//	(i*1024+j)*PAGE_SIZE, pageaddr
					//	);
				}
				else if( MM_GetRefCount(pageaddr) != refcount[pageaddr>>12] ) {
					Debug("PG %p(%i %s) @%p %P %i!=%i",
						proc, proc->PID, proc->FirstThread->ThreadName,
						(i*1024+j)*PAGE_SIZE, pageaddr,
						MM_GetRefCount(pageaddr), refcount[pageaddr>>12]
						);
				}
			}
		}
	}
}

void Validate_VirtualMemoryUsage_PT(const Uint32 *PTable, const tProcess *Proc, int PDIndex, int *RefCounts)
{
	for( int i = 0; i < 1024; i ++ )
	{
		// Ignore the 3GB+1MB area
		if( PDIndex == 768 && i < 256 )
			continue ;
		// And ignore anything from the end of the kernel up to 3GB4MB
		if( PDIndex == 768 && i > ((tVAddr)gKernelEnd-KERNEL_BASE)/PAGE_SIZE )
			continue ;
		
		Uint32	pte = PTable[i];
		if( !(pte & 1) )
			continue ;
		
		Uint32	paddr = pte & ~0xFFF;
		Uint	page = paddr >> 12;
		
		_ref_mem(RefCounts, page, Proc, (void*)((PDIndex*1024+i)*PAGE_SIZE));
	}
}

bool _should_skip_pde(const tProcess *Proc, int idx, Uint32 pde)
{
	if( !(pde & 1) )
		return true;
	
	if( (pde & ~0xFFF) == Proc->MemState.CR3 ) {
		return true;
	}

	if( idx == PAGE_TABLE_ADDR>>22 ) {
		if( (pde & ~0xFFF) != Proc->MemState.CR3 ) {
			// Bad!
		}
	}	
	
	return false;
}

void _ref_mem(int *RefCounts, Uint PageNum, const tProcess *Proc, void *addr)
{
	if( PageNum == 0x1001000/PAGE_SIZE ) {
		Debug("frame #0x%x %i:%p", PageNum, Proc?Proc->PID:-1, addr);
	}
	if( PageNum >= giPageCount )
		;
	else
		RefCounts[ PageNum ] ++;
}

void _load_page(Uint32 *dst, tPAddr phys)
{
	const void *src = MM_MapTemp(phys);
	memcpy(dst, src, PAGE_SIZE);
	MM_FreeTemp( (void*)src);
}


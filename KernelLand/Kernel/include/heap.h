/**
 * Acess2 Kernel
 * heap.h
 * - Dynamic Memory Allocation exports
 */

#ifndef _HEAP_H_
#define _HEAP_H_

extern void	*Heap_Allocate(const char *File, int Line, size_t Bytes);
extern void	*Heap_AllocateZero(const char *File, int Line, size_t Bytes);
extern void	*Heap_Reallocate(const char *File, int Line, void *Ptr, size_t Bytes);
extern void	Heap_Deallocate(const char *File, int Line, void *Ptr);
extern int	Heap_IsHeapAddr(void *Ptr);
extern void	Heap_Validate(void);
/**
 * \brief Hint to the heap code to put a watchpoint on this block's memory
 *
 * Use sparingly, watchpoints are limited and/or very expensive (or not even implemented)
 */
extern int	Heap_WatchBlock(void *Ptr);

#define malloc(size)	Heap_Allocate(_MODULE_NAME_"/"__FILE__, __LINE__, (size))
#define calloc(num,size)	Heap_AllocateZero(_MODULE_NAME_"/"__FILE__, __LINE__, (num)*(size))
#define realloc(ptr,size)	Heap_Reallocate(_MODULE_NAME_"/"__FILE__, __LINE__, (ptr), (size))
#define	free(ptr)	Heap_Deallocate(_MODULE_NAME_"/"__FILE__,__LINE__,(ptr))
#define IsHeap(ptr)	Heap_IsHeapAddr((ptr))

#define strdup(Str)	_strdup(_MODULE_NAME_"/"__FILE__, __LINE__, (Str))

#endif

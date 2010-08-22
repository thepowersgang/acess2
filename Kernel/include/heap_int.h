/*
 * AcessOS Microkernel Version
 * heap_int.h
 * - Internal Heap Header
 */
#ifndef _HEAP_INT_H
#define _HEAP_INT_H

typedef struct {
	Uint	Size;
	 int	ValidSize;
	const char	*File;
	 int	Line;
	Uint	Magic;
	char	Data[];
} tHeapHead;

typedef struct {
	Uint	Magic;
	tHeapHead	*Head;
	tHeapHead	NextHead[];	// Array to make it act like an element, but have no size and refer to the next block
} tHeapFoot;

#endif

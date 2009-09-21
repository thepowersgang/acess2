/*
 * AcessOS Microkernel Version
 * heap.h
 */
#ifndef _HEAP_H
#define _HEAP_H

typedef struct {
	Uint	Size;
	Uint	Magic;
	char	Data[];
} tHeapHead;

typedef struct {
	Uint	Magic;
	tHeapHead	*Head;
	tHeapHead	NextHead[];	// Array to make it act like a pointer, but have no size
} tHeapFoot;

#endif

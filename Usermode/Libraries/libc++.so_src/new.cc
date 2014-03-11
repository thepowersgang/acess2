/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * new.cc
 * - new/delete
 */
#include <stddef.h>
#include <stdlib.h>

// === CODE ===
void *operator new( size_t size )
{
	return malloc( size );
}

void *operator new[]( size_t size )
{
	return malloc( size );
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[](void *ptr)
{
	free(ptr);
}


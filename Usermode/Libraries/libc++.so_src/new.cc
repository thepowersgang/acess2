/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * new.cc
 * - new/delete
 */
#include <stddef.h>
#include <stdlib.h>
#include <acess/sys.h>

// === CODE ===
void *operator new( size_t size )
{
	//_SysDebug("libc++ - operator new(%i)", size);
	return malloc( size );
}
void *operator new( size_t size, void* ptr )
{
	//_SysDebug("libc++ - operator new(%i, %p)", size, ptr);
	size = size;
	return ptr;
}

void *operator new[]( size_t size )
{
	//_SysDebug("libc++ - operator new[](%i)", size);
	return malloc( size );
}
void *operator new[]( size_t size, void* ptr )
{
	//_SysDebug("libc++ - operator new[](%i, %p)", size, ptr);
	size = size;
	return ptr;
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[](void *ptr)
{
	free(ptr);
}


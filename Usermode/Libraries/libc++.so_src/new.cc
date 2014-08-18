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
#include <new>

// === IMPORTS ===
extern "C" bool	_libc_free(void *mem);	// from libc.so, actual free.

// === CODE ===
void *operator new( size_t size )
{
	//_SysDebug("libc++ - operator new(%i)", size);
	return malloc( size );
}

void *operator new[]( size_t size )
{
	//_SysDebug("libc++ - operator new[](%i)", size);
	return malloc( size );
}

void operator delete(void *ptr)
{
	if( !_libc_free(ptr) ) {
		_SysDebug("delete of invalid by %p", __builtin_return_address(0));
		throw ::std::bad_alloc();
	}
}

void operator delete[](void *ptr)
{
	if( !_libc_free(ptr) ) {
		_SysDebug("delete[] of invalid by %p", __builtin_return_address(0));
		throw ::std::bad_alloc();
	}
}


::std::bad_alloc::bad_alloc() noexcept
{
}
::std::bad_alloc::~bad_alloc() noexcept
{
}

const char *::std::bad_alloc::what() const noexcept
{
	return "allocation failure";
}


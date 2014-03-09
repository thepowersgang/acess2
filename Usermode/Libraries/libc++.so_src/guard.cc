/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * guard.cc
 * - One-time construction API
 */
#include <stdint.h>

extern "C" int __cxa_guard_acquire ( int64_t *guard_object )
{
	// TODO: Mutex!
	if( *guard_object )
		return 1;
	*guard_object = 1;
	return 0;
}

extern "C" void __cxa_guard_release ( int64_t *guard_object )
{
	*guard_object = 0;
}

extern "C" void __cxa_guard_abort ( int64_t *guard_object )
{
	*guard_object = 0;
	// TODO: abort
}


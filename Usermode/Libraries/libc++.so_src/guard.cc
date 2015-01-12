/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * guard.cc
 * - One-time construction API
 */
#include <stdint.h>
#include <acess/sys.h>
#include <cstdlib>

#define FLAG_INIT_COMPLETE	(1<<0)
#define FLAG_INIT_LOCKED	(1<<1)

extern "C" int __cxa_guard_acquire ( int64_t *guard_object )
{
	// TODO: Mutex!
	if( *guard_object == FLAG_INIT_COMPLETE )
		return 0;
	if( *guard_object == FLAG_INIT_LOCKED ) {
		_SysDebug("ERROR: __cxa_guard_acquire - nested");
	}
	*guard_object = FLAG_INIT_LOCKED;
	return 1;
}

extern "C" void __cxa_guard_release ( int64_t *guard_object )
{
	*guard_object = FLAG_INIT_COMPLETE;
}

extern "C" void __cxa_guard_abort ( int64_t *guard_object )
{
	*guard_object = FLAG_INIT_COMPLETE;
	_SysDebug("__cxa_guard_abort");
	abort();
}


/*
 * Acess2 C++ Support Library
 * - By John Hodge (thePowersGang)
 *
 * exception_handling.cc
 * - Exception handling code (defined by C++ ABI)
 *
 * References:
 * - LLVM Exception handling
 *  > http://llvm.org/docs/ExceptionHandling.html
 * - Itanium C++ ABI
 *  >http://mentorembedded.github.io/cxx-abi/abi-eh.html
 * - HP's "aC++" Document, Ch 7 "Exception Handling Tables"
 *  > http://mentorembedded.github.io/cxx-abi/exceptions.pdf
 */
#include <typeinfo>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include "exception_handling_cxxabi.h"

#include <acess/sys.h>

#define DEBUG_ENABLED	1

#if DEBUG_ENABLED
# define	DEBUG(v...)	::_SysDebug(v)
#else
# define	DEBUG(v...)	do{}while(0)
#endif

/*__thread*/ struct __cxa_eh_globals {
	__cxa_exception	*caughtExceptions;
	unsigned  int	uncaughtExceptions;
} eh_globals;

/*__thread*/ struct {
	__cxa_exception	info;
	char	buf[32];
} emergency_exception;
/*__thread*/ bool	emergency_exception_used;

static bool TEST_AND_SET(bool& flag) {
	bool ret = flag;
	flag = true;
	return ret;
}

// === CODE ===
extern "C" __cxa_eh_globals *__cxa_get_globals(void)
{
	return &eh_globals;
}
extern "C" __cxa_eh_globals *__cxa_get_globals_fast(void)
{
	return &eh_globals;
}
extern "C" void __cxa_call_unexpected(void *)
{
	// An unexpected exception was thrown from a function that lists its possible exceptions
	_SysDebug("__cxa_call_unexpected - TODO");
	for(;;);
}

extern "C" void *__cxa_allocate_exception(size_t thrown_size)
{
	DEBUG("__cxa_allocate_exception(%i)", thrown_size);
	__cxa_exception	*ret = static_cast<__cxa_exception*>( malloc( sizeof(__cxa_exception) + thrown_size ) );
	if( !ret ) {
		if( thrown_size <= sizeof(emergency_exception.buf) && TEST_AND_SET(emergency_exception_used) )
		{
			ret = &emergency_exception.info;
		}
	}
	if( !ret ) {
		_SysDebug("__cxa_allocate_exception - No free space");
		::std::terminate();
	}
	DEBUG("__cxa_allocate_exception: return %p", ret+1);
	return ret + 1;
}

extern "C" void __cxa_free_exception(void *thrown_exception)
{
	DEBUG("__cxa_free_exception(%p)", thrown_exception);
	if(thrown_exception == &emergency_exception.buf) {
		//assert(emergency_exception_used);
		emergency_exception_used = false;
	}
	else {
		__cxa_exception	*except = static_cast<__cxa_exception*>( thrown_exception ) - 1;
		free(except);
	}
}

extern "C" void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void*))
{
	#if DEBUG_ENABLED
	DEBUG("__cxa_throw(%p,%p,%p) '%s' by %p",
		thrown_exception, tinfo, dest, tinfo->name(), __builtin_return_address(0)
		);
	{
		const ::std::exception* e = reinterpret_cast<const ::std::exception*>(thrown_exception);
		DEBUG("- e.what() = '%s'", e->what());
	}
	#endif

	__cxa_exception	*except = static_cast<__cxa_exception*>( thrown_exception ) - 1;
	
	except->unexpectedHandler = 0;
	except->terminateHandler = 0;
	except->exceptionType = tinfo;
	except->exceptionDestructor = dest;
	memcpy(&except->unwindHeader.exception_class, EXCEPTION_CLASS_ACESS, 8);
	__cxa_get_globals()->uncaughtExceptions ++;
	
	int rv = _Unwind_RaiseException( &except->unwindHeader );
	
	::_SysDebug("__cxa_throw(%p,%s) :: UNCAUGHT %i", thrown_exception, tinfo->name(), rv);
	::std::terminate();
}

extern "C" void *__cxa_begin_catch(_Unwind_Exception *exceptionObject)
{
	__cxa_exception	*except = reinterpret_cast<__cxa_exception*>( exceptionObject+1 )-1;
	DEBUG("__cxa_begin_catch(%p) - except=%p", exceptionObject, except);
	
	except->handlerCount ++;
	
	except->nextException = __cxa_get_globals()->caughtExceptions;
	__cxa_get_globals()->caughtExceptions = except;
	
	__cxa_get_globals_fast()->uncaughtExceptions --;
	
	return except+1;
}

extern "C" void __cxa_end_catch()
{
	struct __cxa_exception	*except = __cxa_get_globals()->caughtExceptions;
	DEBUG("__cxa_end_catch - %p", except);
	except->handlerCount --;
	__cxa_get_globals()->caughtExceptions = except->nextException;
	
	if( except->handlerCount == 0 ) {
		__cxa_free_exception(except+1);
	}
}



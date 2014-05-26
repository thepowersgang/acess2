/*
 */
#include <typeinfo>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include "exception_handling.h"

#include <acess/sys.h>

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
	::_SysDebug("__cxa_allocate_exception(%i)", thrown_size);
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
	::_SysDebug("__cxa_allocate_exception: return %p", ret+1);
	return ret + 1;
}

extern "C" void __cxa_free_exception(void *thrown_exception)
{
	::_SysDebug("__cxa_free_exception(%p)", thrown_exception);
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
	::_SysDebug("__cxa_throw(%p,%p,%p) '%s'", thrown_exception, tinfo, dest, tinfo->name());
	{
		const ::std::exception* e = reinterpret_cast<const ::std::exception*>(thrown_exception);
		::_SysDebug("- e.what() = '%s'", e->what());
	}
	::_SysDebug("- typeid(*tinfo) = %p", &typeid(*tinfo));

	__cxa_exception	*except = static_cast<__cxa_exception*>( thrown_exception ) - 1;
	
	except->unexpectedHandler = 0;
	except->terminateHandler = 0;
	except->exceptionType = tinfo;
	except->exceptionDestructor = dest;
	memcpy(&except->unwindHeader.exception_class, "Ac20C++\0", 8);
	__cxa_get_globals()->uncaughtExceptions ++;
	
	_Unwind_RaiseException(thrown_exception);
	
	::_SysDebug("__cxa_throw(%p,%s) :: UNCAUGHT", thrown_exception, tinfo->name());
	::std::terminate();
}

extern "C" void *__cxa_begin_catch(void *exceptionObject)
{
	::_SysDebug("__cxa_begin_catch(%p)", exceptionObject);
	__cxa_exception	*except = static_cast<__cxa_exception*>( exceptionObject ) - 1;
	
	except->handlerCount ++;
	
	except->nextException = __cxa_get_globals()->caughtExceptions;
	__cxa_get_globals()->caughtExceptions = except;
	
	__cxa_get_globals_fast()->uncaughtExceptions --;
	
	return except;
}

extern "C" void __cxa_end_catch()
{
	struct __cxa_exception	*except = __cxa_get_globals()->caughtExceptions;
	::_SysDebug("__cxa_end_catch - %p", except);
	except->handlerCount --;
	__cxa_get_globals()->caughtExceptions = except->nextException;
	
	if( except->handlerCount == 0 ) {
		__cxa_free_exception(except+1);
	}
}


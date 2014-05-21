/*
 */
#include <typeinfo>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include "exception_handling.h"


 int	uncaught_exception_count;
__cxa_exception	*uncaught_exception_top;

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
extern "C" void __cxa_call_unexpected(void *)
{
	// An unexpected exception was thrown from a function that lists its possible exceptions
	for(;;);
}

extern "C" void *__cxa_allocate_exception(size_t thrown_size)
{
	__cxa_exception	*ret = static_cast<__cxa_exception*>( malloc( sizeof(__cxa_exception) + thrown_size ) );
	if( !ret ) {
		if( thrown_size <= sizeof(emergency_exception.buf) && TEST_AND_SET(emergency_exception_used) )
		{
			ret = &emergency_exception.info;
		}
	}
	if( !ret ) {
		::std::terminate();
	}
	return ret;
}

extern "C" void __cxa_free_exception(void *thrown_exception)
{
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
	__cxa_exception	*except = static_cast<__cxa_exception*>( thrown_exception ) - 1;
	
	except->exceptionType = tinfo;
	except->exceptionDestructor = dest;
	memcpy(&except->unwindHeader.exception_class, "Ac20C++\0", 8);
	uncaught_exception_top ++;
	
	_Unwind_RaiseException(thrown_exception);
	
	::std::terminate();
}

extern "C" void *__cxa_begin_catch(void *exceptionObject)
{
	__cxa_exception	*except = static_cast<__cxa_exception*>( exceptionObject ) - 1;
	
	except->handlerCount ++;
	
	except->nextException = uncaught_exception_top;
	uncaught_exception_top = except;
	
	uncaught_exception_count --;
	
	return except;
}

extern "C" void __cxa_end_catch()
{
	struct __cxa_exception	*except = uncaught_exception_top;
	except->handlerCount --;
	uncaught_exception_top = except->nextException;
	
	if( except->handlerCount == 0 ) {
		__cxa_free_exception(except+1);
	}
}


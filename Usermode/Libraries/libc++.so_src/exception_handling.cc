/*
 */
#include <typeinfo>
#include <cstdint>

typedef void *(unexpected_handler)(void);
typedef void *(terminate_handler)(void);

struct _Unwind_Context;

typedef enum {
	_URC_NO_REASON = 0,
	_URC_FOREIGN_EXCEPTION_CAUGHT = 1,
	_URC_FATAL_PHASE2_ERROR = 2,
	_URC_FATAL_PHASE1_ERROR = 3,
	_URC_NORMAL_STOP = 4,
	_URC_END_OF_STACK = 5,
	_URC_HANDLER_FOUND = 6,
	_URC_INSTALL_CONTEXT = 7,
	_URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;

typedef void	(*_Unwind_Exception_Cleanup_Fn)(_Unwind_Reason_Code reason, struct _Unwind_Exception *exc);

struct _Unwind_Exception
{
	uint64_t	exception_class;
	_Unwind_Exception_Cleanup_Fn	exception_cleanup;
	uint64_t	private_1;
	uint64_t	private_2;
};

struct __cxa_exception
{
	std::type_info*	exceptionType;
	void (*exceptionDestructor)(void *);
	unexpected_handler	unexpectedHandler;
	terminate_handler	terminateHandler;
	__cxa_exception*	nextException;
	
	int	handlerCount;
	
	int	handlerSwitchValue;
	const char*	actionRecord;
	const char*	languageSpecificData;
	void*	catchTemp;
	void*	adjustedPtr;
	
	_Unwind_Exception	unwindHeader;
};

 int	uncaught_exception_count;
__cxa_exception	*uncaught_exception_top;

extern "C" void __cxa_call_unexpected(void *)
{
	// An unexpected exception was thrown from a function that lists its possible exceptions
	for(;;);
}

extern "C" void *__cxa_begin_catch(void *exceptionObject)
{
	struct __cxa_exception	*except = static_cast<__cxa_exception*>( exceptionObject );
	except --;
	
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
		//__cxa_free_exception(except+1);
	}
}


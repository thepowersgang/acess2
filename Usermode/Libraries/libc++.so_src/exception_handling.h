/*
 */
#ifndef _EXCEPTION_HANLDING_H_
#define _EXCEPTION_HANLDING_H_

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


extern "C" void __cxa_call_unexpected(void *);
extern "C" void *__cxa_allocate_exception(size_t thrown_size);
extern "C" void __cxa_free_exception(void *thrown_exception);
extern "C" void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void*));
extern "C" void *__cxa_begin_catch(void *exceptionObject);
extern "C" void __cxa_end_catch();

extern "C" void _Unwind_RaiseException(void *thrown_exception);

#endif


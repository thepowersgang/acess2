/*
 * Acess2 C++ Support Library
 * - By John Hodge (thePowersGang)
 *
 * exception_handling_cxxabi.h
 * - C++ Exception Handling definitions from the Itanium C++ ABI
 */
#ifndef _EXCEPTION_HANLDING_CXXABI_H_
#define _EXCEPTION_HANLDING_CXXABI_H_

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

typedef int	_Unwind_Action;
static const _Unwind_Action	_UA_SEARCH_PHASE  = 1;
static const _Unwind_Action	_UA_CLEANUP_PHASE = 2;
static const _Unwind_Action	_UA_HANDLER_FRAME = 4;
static const _Unwind_Action	_UA_FORCE_UNWIND  = 8;

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
	unexpected_handler*	unexpectedHandler;
	terminate_handler*	terminateHandler;
	__cxa_exception*	nextException;
	
	int	handlerCount;
	
	int	handlerSwitchValue;
	const char*	actionRecord;
	const char*	languageSpecificData;
	void*	catchTemp;
	void*	adjustedPtr;
	
	_Unwind_Exception	unwindHeader;
};

#define EXCEPTION_CLASS_ACESS	"Ac20C++\0"

typedef _Unwind_Reason_Code	(*_Unwind_Stop_Fn)(int version, _Unwind_Action actions, uint64_t exception_class, struct _Unwind_Exception *exceptionObject, struct _Unwind_Context *context, void *stop_parameter);

// Exports
extern "C" void __cxa_call_unexpected(void *);
extern "C" void *__cxa_allocate_exception(size_t thrown_size);
extern "C" void __cxa_free_exception(void *thrown_exception);
extern "C" void __cxa_throw(void *thrown_exception, std::type_info *tinfo, void (*dest)(void*));
extern "C" void *__cxa_begin_catch(_Unwind_Exception *exceptionObject);
extern "C" void __cxa_end_catch();

// Imports
extern "C" _Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception *exception_object);
extern "C" _Unwind_Reason_Code _Unwind_ForcedUnwind(struct _Unwind_Exception *exception_object, _Unwind_Stop_Fn stop, void *stop_parameter);
extern "C" void	_Unwind_DeleteException(struct _Unwind_Exception *exception_object);
extern "C" uint64_t	_Unwind_GetGR(struct _Unwind_Context *context, int index);
extern "C" void 	_Unwind_SetGR(struct _Unwind_Context *context, int index, uint64_t new_value);
extern "C" uint64_t	_Unwind_GetIP(struct _Unwind_Context *context);
extern "C" void 	_Unwind_SetIP(struct _Unwind_Context *context, uint64_t new_value);
extern "C" uint64_t	_Unwind_GetLanguageSpecificData(struct _Unwind_Context *context);
extern "C" uint64_t	_Unwind_GetRegionStart(struct _Unwind_Context *context);

#endif



/*
 * Acess2 C++ Support Library
 * - By John Hodge (thePowersGang)
 *
 * gxx_personality.cc
 * - Implementation of GNU C++ Exception handling "Personality"
 */
#include <typeinfo>
#include <cstdint>	// [u]intN_t
#include <cstdlib>	// abort
#include <cstring>	// memcmp
#include "exception_handling_acxx.h"
#include "exception_handling_cxxabi.h"
#include <acess/sys.h>
#include <cassert>
#include <cxxabi.h>	// __dynamic_cast

#define DEBUG_ENABLED	1

#if DEBUG_ENABLED
# define DEBUG(v...)	::_SysDebug(v)
#else
# define DEBUG(v...)	do{}while(0)
#endif

// === PROTOTYPES ===
extern "C" _Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions, uint64_t exceptionClass,
		struct _Unwind_Exception *exceptionObject, struct _Unwind_Context *context);
static int get_frame_action(const sLSDA_Header &header, _Unwind_Context *context, const std::type_info *throw_type,
	uint64_t &landingpad, int64_t &switch_value);
static bool exception_matches_single(const std::type_info *throw_type, const struct sLSDA_Header &header, int type_index);
static bool exception_matches_list(const std::type_info *throw_type, const struct sLSDA_Header &header, int list_index);
static void read_lsda_header(const void *&ptr, _Unwind_Context *context, struct sLSDA_Header *header);
static size_t _get_encoded_size(int encoding);
static uint64_t _get_base(uint8_t encoding, _Unwind_Context *context);
static uint64_t _read_encoded(const void *&ptr, _Unwind_Context *context, int encoding, uint64_t base);
static uint64_t _read_encoded(const void *&ptr, _Unwind_Context *context, int encoding);
template <typename T> static T read(const void *&ptr);
static uint64_t _read_leb128u(const void *&ptr);
static int64_t _read_leb128s(const void *&ptr);

// === CODE ===
extern "C" _Unwind_Reason_Code __gxx_personality_v0(int version, _Unwind_Action actions, uint64_t exceptionClass,
		struct _Unwind_Exception *exceptionObject, struct _Unwind_Context *context)
{
	//::_SysDebug("__gxx_personality_v0(%i, 0x%x, 0x%llx, ...)", version, actions, exceptionClass);
	//::_SysDebug("__gxx_personality_v0(..., %p, %p)", exceptionObject, context);
	
	//if( exceptionObject ) {
	//	::_SysDebug("exception_object: Class 0x%llx", exceptionObject->exception_class);
	//}
	//if( context )
	//{
	//	::_SysDebug("context: IP 0x%llx", _Unwind_GetIP(context));
	//	::_SysDebug("context: RegionStart 0x%llx", _Unwind_GetRegionStart(context));
	//	::_SysDebug("context: LangSpecData 0x%llx", _Unwind_GetLanguageSpecificData(context));
	//}
	
	if( version != 1 ) {
		::_SysDebug("%s: Unexpected version %i != exp 1", __func__, version);
		return _URC_FATAL_PHASE1_ERROR;
	}
	
	const void *lsda_ptr = (const void*)_Unwind_GetLanguageSpecificData(context);
	struct sLSDA_Header	header;
	read_lsda_header(lsda_ptr, context, &header);

	const void *exception_object = exceptionObject + 1;
	const __cxa_exception *exception_info = static_cast<const __cxa_exception*>(exception_object) - 1;
	std::type_info *throw_type = (
		memcmp(&exceptionClass,EXCEPTION_CLASS_ACESS,8) == 0 ? exception_info->exceptionType : NULL
		);

	uint64_t	landingpad = 0;
	 int64_t	switch_value = 0;
	int frame_action = get_frame_action(header, context, throw_type,  landingpad, switch_value);

	if( (actions & _UA_SEARCH_PHASE) && (actions & _UA_CLEANUP_PHASE) )
	{
		_SysDebug("__gxx_personality_v0: ERROR Both _UA_SEARCH_PHASE and _UA_CLEANUP_PHASE set");
		abort();
	}	

	// Search
	if( actions & _UA_SEARCH_PHASE )
	{
		// If a handler was found
		if( frame_action == 2 ) {
			// - return _URC_HANDLER_FOUND
			DEBUG("SEARCH: 0x%llx Handler %p(%i)",
				_Unwind_GetIP(context), landingpad, switch_value);
			return _URC_HANDLER_FOUND;
		}
		// - If no handler (either nothing, or cleanups), return _URC_CONTINUE_UNWIND
		DEBUG("SEARCH: 0x%llx no handler %p(%i)",
			_Unwind_GetIP(context), landingpad, switch_value);
		return _URC_CONTINUE_UNWIND;
	}

	// Cleanup	
	if( actions & _UA_CLEANUP_PHASE )
	{
		// No action, continue
		if( frame_action == 0 ) {
			return _URC_CONTINUE_UNWIND;
		}
		assert(landingpad);
	
		if( actions & _UA_FORCE_UNWIND )
		{
			// Unwind forced, override switch_value to 0
			// - Disables any attempt to handle exception
			switch_value = 0;
		}
	
		DEBUG("Install context IP=0x%x, R%i=%p/R%i=%i",
			(uintptr_t)landingpad,
			__builtin_eh_return_data_regno(0), exceptionObject,
			__builtin_eh_return_data_regno(1), switch_value
			);
		_Unwind_SetGR(context, __builtin_eh_return_data_regno(0), (uintptr_t)exceptionObject);
		_Unwind_SetGR(context, __builtin_eh_return_data_regno(1), switch_value);
		_Unwind_SetIP(context, landingpad);
		return _URC_INSTALL_CONTEXT;
	}
	
	_SysDebug("__gxx_personality_v0: Neither _UA_SEARCH_PHASE or _UA_CLEANUP_PHASE set");
	return _URC_FATAL_PHASE1_ERROR;
}

int get_frame_action(const sLSDA_Header &header, _Unwind_Context *context, const std::type_info* throw_type,
	uint64_t &landingpad, int64_t &switch_value)
{
	uint64_t ip = _Unwind_GetIP(context) - _Unwind_GetRegionStart(context);
	DEBUG("get_frame_action: IP = 0x%llx + 0x%llx", _Unwind_GetRegionStart(context), ip);
	// Check if there is a handler for this exception in this frame
	// - Search call site table for this return address (corresponds to a try block)
	uintptr_t	cs_ldgpad;
	uint64_t	cs_action;
	const void *lsda_ptr = header.CallSiteTable;
	while( lsda_ptr < header.ActionTable )
	{
		uintptr_t cs_start  = _read_encoded(lsda_ptr, context, header.CallSiteEncoding);
		uintptr_t cs_len    = _read_encoded(lsda_ptr, context, header.CallSiteEncoding);
		cs_ldgpad = _read_encoded(lsda_ptr, context, header.CallSiteEncoding);
		cs_action = _read_leb128u(lsda_ptr);
		
		// If IP falls below this entry, it's not on the list
		if( ip <= cs_start ) {
			lsda_ptr = header.ActionTable;
			break ;
		}
		if( ip > cs_start + cs_len )
			continue;
		
		break;
	}
	if( lsda_ptr > header.ActionTable ) {
		_SysDebug("__gxx_personality_v0: Malformed Call Site Table, reading overran the end (%p>%p)",
			lsda_ptr, header.ActionTable);
	}
	if( lsda_ptr >= header.ActionTable ) {
		// No match!
		DEBUG("__gxx_personality_v0: No entry for IP 0x%x", ip);
		return 0;
	}
	
	// Found it
	if( cs_ldgpad == 0 ) {
		DEBUG("No landingpad, hence no action");
		if( cs_action != 0 ) {
			_SysDebug("%s: NOTICE cs_ldgpad==0 but cs_action(0x%llx)!=0",
				__func__, cs_action);
		}
		return 0;
	}
	else if( cs_action == 0 ) {
		DEBUG("No action, cleanups only");
		switch_value = 0;
		landingpad = header.LPStart + cs_ldgpad;
		return 1;	// 1 = cleanup only
	}
	else {
		switch_value = 0;
		landingpad = header.LPStart + cs_ldgpad;
		// catch() handler
		bool	has_cleanups = false;	// set to true to override return value
		const void *action_list = (const char*)header.ActionTable + (cs_action-1);
		for(;;)	// broken by 0 length entry
		{
			leb128s_t filter = _read_leb128s(action_list);
			leb128s_t disp = _read_leb128s(action_list);
			if( filter == 0 )
			{
				// Cleanup
				has_cleanups = true;
			}
			else if( filter < 0 )
			{
				// Exception specifcation
				if( !exception_matches_list(throw_type, header, -filter) )
				{
					switch_value = filter;
					return 2;
				}
			}
			else
			{
				// Catch
				if( exception_matches_single(throw_type, header, filter) )
				{
					switch_value = filter;
					return 2;
				}
			}
			
			if( disp == 0 )
				break;
			action_list = (const char*)action_list + disp-1;
		}
		
		// If a cleanup request was seen, tell the caller that we want to handle it
		if( has_cleanups ) {
			return 1;	// 1 = cleanup only
		}
		// Else, there's nothing here to handle
		return 0;
	}
}

const ::std::type_info *get_type_info(const struct sLSDA_Header &header, int type_index)
{
	size_t	encoded_size = _get_encoded_size(header.TTEncoding);
	assert(encoded_size > 0);
	const void *ptr = (const void*)(header.TTBase - encoded_size * type_index);
	assert( header.TTBase );
	assert( ptr > header.ActionTable );
	
	uintptr_t type_ptr = _read_encoded(ptr, NULL, header.TTEncoding, header.TypePtrBase);
	return reinterpret_cast< ::std::type_info* >(type_ptr);
}

const ::std::type_info *get_exception_type(const void *exception_object)
{
	if( !exception_object )
		return NULL;
	const struct _Unwind_Exception	*u_execept = (const struct _Unwind_Exception*)exception_object;
	const struct __cxa_exception *except = (const struct __cxa_exception*)(u_execept + 1) - 1;
	if( memcmp(&except->unwindHeader.exception_class, EXCEPTION_CLASS_ACESS, 8) != 0 )
		return NULL;
	
	return except->exceptionType;
}

bool exception_matches_single(const std::type_info *throw_type, const struct sLSDA_Header &header, int type_index)
{
	const ::std::type_info *catch_type = get_type_info(header, type_index);
	DEBUG("catch_type = %p", catch_type);

	if( !catch_type )
	{
		DEBUG("catch(...)");
		return true;
	}
	else if( !throw_type )
	{
		DEBUG("threw UNK");
		return false;
	}
	else
	{
		DEBUG("catch(%s), throw %s", catch_type->name(), throw_type->name());
		size_t ofs = 0;
		if( !catch_type->__is_child(*throw_type, ofs) ) {
			_SysDebug("> No match");
			return false;
		}
		
		return true;
	}
}
bool exception_matches_list(const std::type_info *throw_type, const struct sLSDA_Header &header, int list_index)
{
	_SysDebug("TODO: exception_matches_list %i", list_index);
	abort();
	(void)throw_type;
	(void)header;
	return true;
}

template <typename T> T read(const void *&ptr)
{
	T rv = *reinterpret_cast<const T*>(ptr);
	ptr = (const char*)ptr + sizeof(T);
	return rv;
}

static void read_lsda_header(const void *&ptr, _Unwind_Context *context, struct sLSDA_Header *header)
{
	header->Base = ptr;
	
	uint8_t	start_encoding = read<uint8_t>(ptr);
	if( start_encoding == DW_EH_PE_omit )
		header->LPStart = _Unwind_GetRegionStart(context);
	else
		header->LPStart = _read_encoded(ptr, context, start_encoding);
	
	header->TTEncoding = read<uint8_t>(ptr);
	if( header->TTEncoding == DW_EH_PE_omit )
		header->TTBase = 0;
	else {
		ptrdiff_t	tt_ofs = _read_leb128u(ptr);
		header->TTBase = (uintptr_t)ptr + tt_ofs;
	}
	header->TypePtrBase = _get_base(header->TTEncoding, context);

	header->CallSiteEncoding = read<uint8_t>(ptr);
	uint64_t call_site_size = _read_leb128u(ptr);
	header->CallSiteTable = ptr;
	header->ActionTable = (const void*)( (uintptr_t)ptr + call_site_size );
	
	#if 0
	::_SysDebug("LSDA header:");
	::_SysDebug("->LPStart = 0x%lx", header->LPStart);
	::_SysDebug("->TTEncoding = 0x%02x", header->TTEncoding);
	::_SysDebug("->TTBase = 0x%lx", header->TTBase);
	::_SysDebug("->CallSiteEncoding = 0x%02x", header->CallSiteEncoding);
	::_SysDebug("->CallSiteTable = %p", header->CallSiteTable);
	::_SysDebug("->ActionTable = %p", header->ActionTable);
	#endif
}

static size_t _get_encoded_size(int encoding)
{
	switch(encoding & DW_EH_PE_fmtmask)
	{
	case DW_EH_PE_absptr:	// absolute
		return sizeof(void*);
	default:
		_SysDebug("_get_encoded_size: Unknown encoding 0x%02x", encoding);
		return 0;
	}
}

/*
 * \brief Read a DWARF encoded pointer from the stream
 */
static uint64_t _read_encoded(const void *&ptr, _Unwind_Context *context, int encoding, uint64_t base)
{
	(void)context;
	if( encoding == DW_EH_PE_aligned ) {
		void	**aligned = (void**)( ((uintptr_t)ptr + sizeof(void*) - 1) & -(sizeof(void*)-1) );
		ptr = (void*)( (uintptr_t)aligned + sizeof(void*) );
		return (uintptr_t)*aligned;
	}
	uint64_t	rv;
	uintptr_t	orig_ptr = (uintptr_t)ptr;
	switch(encoding & DW_EH_PE_fmtmask)
	{
	case DW_EH_PE_absptr:	// absolute
		rv = (uintptr_t)read<void*>(ptr);
		break;
	case DW_EH_PE_uleb128:
		rv = _read_leb128u(ptr);
		break;
	case DW_EH_PE_sleb128:
		rv = _read_leb128s(ptr);
		break;
	case DW_EH_PE_udata2:
		rv = read<uint16_t>(ptr);
		break;
	case DW_EH_PE_udata4:
		rv = read<uint32_t>(ptr);
		break;
	case DW_EH_PE_udata8:
		rv = read<uint64_t>(ptr);
		break;
	default:
		::_SysDebug("_read_encoded: Unknown encoding format 0x%x", encoding & DW_EH_PE_fmtmask);
		::abort();
	}
	if( rv != 0 )
	{
		if( (encoding & DW_EH_PE_relmask) == DW_EH_PE_pcrel ) {
			rv += orig_ptr;
		}
		else {
			rv += base;
		}
		
		if( encoding & DW_EH_PE_indirect ) {
			rv = (uintptr_t)*(void**)(uintptr_t)rv;
		}
		else {
			// nothing
		}
	}
	return rv;
}
static uint64_t _get_base(uint8_t encoding, _Unwind_Context *context)
{
	if( encoding == 0xFF )
		return 0;
	switch(encoding & DW_EH_PE_relmask)
	{
	case DW_EH_PE_absptr:
	case DW_EH_PE_pcrel:
	case DW_EH_PE_aligned:
		return 0;
	//case DW_EH_PE_textrel:
		//return _Unwind_GetTextRelBase(context);
	//case DW_EH_PE_datarel:
		//return _Unwind_GetDataRelBase(context);
	case DW_EH_PE_funcrel:
		return _Unwind_GetRegionStart(context);
	default:
		::_SysDebug("_get_base: Unknown encoding relativity 0x%x", (encoding & DW_EH_PE_relmask));
		::abort();
	}
}
static uint64_t _read_encoded(const void *&ptr, _Unwind_Context *context, int encoding)
{
	return _read_encoded(ptr, context, encoding, _get_base(encoding, context));
}

static uint64_t _read_leb128_raw(const void *&ptr, int *sizep)
{
	 int	size = 0;
	uint64_t val = 0;
	const uint8_t	*ptr8 = static_cast<const uint8_t*>(ptr);
	do
	{
		val |= (*ptr8 & 0x7F) << (size*7);
		size ++;
	} while( *ptr8++ & 0x80 );
	
	ptr = ptr8;
	*sizep = size;
	
	return val;
}

static uint64_t _read_leb128u(const void *&ptr)
{
	 int	unused_size;
	return _read_leb128_raw(ptr, &unused_size);
}
static int64_t _read_leb128s(const void *&ptr)
{
	 int	size;
	uint64_t val = _read_leb128_raw(ptr, &size);
	if( size*7 <= 64 && (val & (1 << (size*7-1))) )
	{
		val |= 0xFFFFFFFFFFFFFFFF << (size*7);
	}
	return val;
}


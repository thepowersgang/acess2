/**
 * \file logging.c
 * \author John Hodge (thePowersGang)
 *
 * \brief UDI Tracing, Logging and Debug
 */
#define DEBUG	1
#include <acess.h>
#include <udi.h>

void	__udi_assert(const char *expr, const char *file, int line);

// === EXPORTS ===
EXPORT(udi_trace_write);
EXPORT(udi_log_write);
EXPORT(__udi_assert);
//EXPORT(udi_assert);
EXPORT(udi_debug_break);
EXPORT(udi_debug_printf);

// === PROTOTYPES ===

// === CODE ===
void udi_trace_write(udi_init_context_t *init_context, udi_trevent_t trace_event, udi_index_t meta_idx, udi_ubit32_t msgnum, ...)
{
	ENTER("pinit_context itrace_event imeta_idx imsgnum",
		init_context, trace_event, meta_idx, msgnum);
//	const char *format = UDI_GetMessage(init_context, msgnum);
//	LOG("format = \"%s\"", format);
	LEAVE('-');
}

void udi_log_write( udi_log_write_call_t *callback, udi_cb_t *gcb,
	udi_trevent_t trace_event, udi_ubit8_t severity, udi_index_t meta_idx,
	udi_status_t original_status, udi_ubit32_t msgnum, ... )
{
	Log("UDI Log");
}

void __udi_assert(const char *expr, const char *file, int line)
{
	Log("UDI Assertion failure: %s:%i - %s", file, line, expr);
	UNIMPLEMENTED();
}

void udi_assert(udi_boolean_t bool)
{
	UNIMPLEMENTED();
}

void udi_debug_break( udi_init_context_t *init_context, const char *message)
{
	UNIMPLEMENTED();
}

void udi_debug_printf( const char *format, ... )
{
	va_list	args;
	va_start(args, format);
	LogF("udi_debug_printf: ");
	LogFV(format, args);
	va_end(args);
}

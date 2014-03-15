/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * debug_hooks.c
 * - Keyboard/serial kernel debug hooks
 */
#include <acess.h>
#include <debug_hooks.h>

// === CODE ===
void DebugHook_HandleInput(tDebugHook *HookHandle, size_t Length, const char *Input)
{
	switch(*Input)
	{
	case '0' ... '9':
		HookHandle->Value *= 10;
		HookHandle->Value += *Input - '0';
		break;
	// Instruction Tracing
	case 't':
		Log("Toggle instruction tracing on %i\n", HookHandle->Value);
		Threads_ToggleTrace( HookHandle->Value );
		HookHandle->Value = 0;
		break;
	
	// Thread List Dump
	case 'p':	Threads_Dump();	return;
	// Heap Statistics
	case 'h':	Heap_Stats();	return;
	// PMem Statistics
	case 'm':	MM_DumpStatistics();	return;
	// Dump Structure
	case 's':	return;
	
	// Validate structures
	//case 'v':
	//	Validate_VirtualMemoryUsage();
	//	return;
	}
}


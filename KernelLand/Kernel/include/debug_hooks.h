/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * debug_hooks.h
 * - Prototypes for methods used by keyboard/serial debugging
 */
#ifndef _DEBUG_HOOKS_H_
#define _DEBUG_HOOKS_H_


typedef struct sDebugHook {
	//tDebugHookOutput	Output;
	Uint	Value;
	// TODO: Console support?
} tDebugHook;

extern void	DebugHook_HandleInput(tDebugHook *HookHandle, size_t Length, const char *Input);

extern void	Heap_Dump(void);
extern void	Threads_Dump(void);
extern void	Threads_ToggleTrace(int TID);
extern void	Heap_Stats(void);
extern void	MM_DumpStatistics(void);

extern void	Proc_PrintBacktrace(void);

#endif

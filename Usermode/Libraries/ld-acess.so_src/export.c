/*
 * Acess2 Dynamic Linker
 */
#include "common.h"
#include <acess/sys.h>

extern uint64_t	__udivdi3(uint64_t Num, uint64_t Den);
extern uint64_t	__umoddi3(uint64_t Num, uint64_t Den);
extern int32_t	__divsi3(int32_t Num, int32_t Den);
extern int32_t	__modsi3(int32_t Num, int32_t Den);
extern uint32_t	__udivsi3(uint32_t Num, uint32_t Den);
extern uint32_t	__umodsi3(uint32_t Num, uint32_t Den);
extern void	ldacess_DumpLoadedLibraries(void);
extern void	_ZN4_sys5debugEPKcz(const char *,...);	// C++ "_sys::debug" used by STL debug

#define _STR(x)	#x
#define STR(x)	_STR(x)
#define EXP(sym)	{&sym, STR(sym)}

#define SYSCALL0(name,num)	EXP(name),
#define SYSCALL1(name,num)	EXP(name),
#define SYSCALL2(name,num)	EXP(name),
#define SYSCALL3(name,num)	EXP(name),
#define SYSCALL4(name,num)	EXP(name),
#define SYSCALL5(name,num)	EXP(name),
#define SYSCALL6(name,num)	EXP(name),

// === CONSTANTS ===
const struct {
	void	*Value;
	char	*Name;
}	caLocalExports[] = {
	EXP(gLoadedLibraries),
	EXP(_errno),
	EXP(ldacess_DumpLoadedLibraries),
	
	#define __ASSEMBLER__
	#include "arch/syscalls.s.h"
	#undef __ASSEMBLER__
	
	#ifdef ARCHDIR_is_armv7
	{0, "__gnu_Unwind_Find_exidx"},
	{0, "__cxa_call_unexpected"},
	{0, "__cxa_type_match"},
	{0, "__cxa_begin_cleanup"},
	#endif
#if 0
	EXP(__umoddi3),
	EXP(__udivdi3),
	EXP(__divsi3),
	EXP(__modsi3),
	EXP(__udivsi3),
	EXP(__umodsi3)
#endif
};

const int	ciNumLocalExports = sizeof(caLocalExports)/sizeof(caLocalExports[0]);

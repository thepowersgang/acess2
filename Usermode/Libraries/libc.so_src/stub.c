/*
 * AcessOS Basic C Library
 */
#include "stdio_int.h"
#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <acess/sys.h>

#define USE_CPUID	0

// === TYPES ===
typedef struct {
	intptr_t	Base;
	char	*Name;
}	tLoadedLib;

// === PROTOTYPES ===
#if USE_CPUID
static void	cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX);
#endif
 int	ErrorHandler(int Fault);

// === IMPORTS ===
extern tLoadedLib	gLoadedLibraries[64];
extern void	*_crt0_exit_handler;
extern void	_stdio_init(void);
extern void	_call_atexit_handlers(void);

// === GLOBALS ===
extern char **environ;
// --- CPU Features ---
#if USE_CPUID
tCPUID	gCPU_Features;
#endif

// === CODE ===
/**
 * \fn int SoMain()
 * \brief Stub Entrypoint
 * \param BaseAddress	Unused - Load Address of libc
 * \param argc	Unused - Argument Count (0 for current version of ld-acess)
 * \param argv	Unused - Arguments (NULL for current version of ld-acess)
 * \param envp	Environment Pointer
 */
int SoMain(UNUSED(uintptr_t, BaseAddress), UNUSED(int, argc), UNUSED(char **, argv), char **envp)
{
	// Init for env.c
	environ = envp;

	#if 0	
	{
		 int	i = 0;
		char	**tmp;
		_SysDebug("envp = %p", envp);
		for(tmp = envp; *tmp; tmp++,i++)
		{
			_SysDebug("envp[%i] = '%s'", i, *tmp);
		}
	}
	#endif

	_stdio_init();	
	
	#if USE_CPUID
	{
		uint32_t	ecx, edx;
		cpuid(1, NULL, NULL, &edx, &ecx);
		gCPU_Features.SSE  = edx & (1 << 25);	// SSE
		gCPU_Features.SSE2 = edx & (1 << 26);	// SSE2
	}
	#endif
	
	_crt0_exit_handler = _call_atexit_handlers;

	// Set Error handler
	_SysSetFaultHandler(ErrorHandler);
	
	return 1;
}

int ErrorHandler(int Fault)
{
	 int	i;

	extern void ldacess_DumpLoadedLibraries(void);	
	ldacess_DumpLoadedLibraries();

	fprintf(stderr, "ErrorHandler: (Fault = %i)\n", Fault);
	fprintf(stderr, "Loaded Libraries:\n");
	for( i = 0; i < 64; i ++ )
	{
		//if(gLoadedLibraries[i].Base == 0)	continue;
	//	fprintf(stderr, "%02i: %p  %s\n", i, gLoadedLibraries[i].Base, gLoadedLibraries[i].Name);
	}
	fprintf(stderr, "\n");
	exit(-1);
	return -1;
}

#if USE_CPUID
/**
 * \brief Call the CPUID opcode
 */
static void cpuid(uint32_t Num, uint32_t *EAX, uint32_t *EBX, uint32_t *EDX, uint32_t *ECX)
{
	uint32_t	eax, ebx, edx, ecx;
	
	__asm__ __volatile__ (
		"cpuid"
		: "=a"(eax), "=c"(ecx), "=d"(edx)
		: "a"(Num)
		);
	
	if(EAX)	*EAX = eax;
	if(EBX)	*EBX = ebx;
	if(EDX)	*EDX = edx;
	if(ECX)	*ECX = ecx;
}
#endif

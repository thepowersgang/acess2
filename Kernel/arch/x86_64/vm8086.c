/*
 */
#include <acess.h>
#include <vm8086.h>
#include <modules.h>

// === CONSTANTS ===
#define VM8086_STACK_SEG	0x9F00
#define VM8086_STACK_OFS	0x0AFE
#define VM8086_PAGES_PER_INST	4

// === PROTOTYPES ===
 int	VM8086_Install(char **Arguments);
tVM8086	*VM8086_Init(void);
void	VM8086_Free(tVM8086 *State);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, VM8086, VM8086_Install, NULL, NULL);
tSpinlock	glVM8086_Process;
tPID	gVM8086_WorkerPID;
tTID	gVM8086_CallingThread;
tVM8086	volatile * volatile gpVM8086_State = (void*)-1;	// Set to -1 to avoid race conditions

// === CODE ===
int VM8086_Install(char **Arguments)
{
	return MODULE_ERR_OK;
}

tVM8086 *VM8086_Init(void)
{
	return NULL;
}

void VM8086_Free(tVM8086 *State)
{
	
}

void *VM8086_Allocate(tVM8086 *State, int Size, Uint16 *Segment, Uint16 *Offset)
{
	return NULL;
}

void *VM8086_GetPointer(tVM8086 *State, Uint16 Segment, Uint16 Offset)
{
	return NULL;
}

void VM8086_Int(tVM8086 *State, Uint8 Interrupt)
{
	
}

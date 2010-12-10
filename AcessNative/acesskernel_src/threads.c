/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */
#include <acess.h>

// === STRUCTURES ===
typedef struct sThread
{
	struct sThread	*Next;
	tTID	TID, PID;
	tUID	UID, GID;

	struct sThread	*Parent;

	char	*ThreadName;

	// Config?
	Uint	Config[NUM_CFG_ENTRIES];
}	tThread;

// === GLOBALS ===
tThread	*gpThreads;
__thread tThread	*gpCurrentThread;

// === CODE ===
tUID Threads_GetUID() { return gpCurrentThread->UID; }
tGID Threads_GetGID() { return gpCurrentThread->GID; }
tTID Threads_GetTID() { return gpCurrentThread->TID; }
tPID Threads_GetPID() { return gpCurrentThread->PID; }

Uint *Threads_GetCfgPtr(int Index)
{
	return &gpCurrentThread->Config[Index];
}

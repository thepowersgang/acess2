/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads.c
 * - Threads handling
 */
#include <acess.h>
#include <threads.h>

// === CODE ===
tThread *Proc_GetCurThread(void)
{
	return NULL;
}

void Threads_PostEvent(tThread *Thread, Uint32 Events)
{
	
}

Uint32 Threads_WaitEvents(Uint32 Events)
{
	Log_KernelPanic("Threads", "Can't use _WaitEvents in DiskTool");
	return 0;
}

void Threads_ClearEvent(Uint32 Mask)
{
	
}

tUID Threads_GetUID(void) { return 0; }
tGID Threads_GetGID(void) { return 0; }

tTID Threads_GetTID(void) { return 0; }

int *Threads_GetMaxFD(void) { static int max_fd=32; return &max_fd; }
char **Threads_GetCWD(void) { static char *cwd; return &cwd; }
char **Threads_GetChroot(void) { static char *chroot; return &chroot; }

void Threads_Yield(void)
{
	Log_Warning("Threads", "Threads_Yield DEFINITELY shouldn't be used");
}

void Threads_Sleep(void)
{
	Log_Warning("Threads", "Threads_Sleep shouldn't be used");
}

int Threads_SetName(const char *Name)
{
	Log_Notice("Threads", "TODO: Threads_SetName('%s')", Name);
	return 0;
}

int *Threads_GetErrno(void) __attribute__ ((weak));

int *Threads_GetErrno(void)// __attribute__ ((weak))
{
	static int a_errno;
	return &a_errno;
}


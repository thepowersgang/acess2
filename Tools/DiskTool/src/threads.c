/*
 * 
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

int *Threads_GetMaxFD(void) { static int max_fd=32; return &max_fd; }
char **Threads_GetCWD(void) { static char *cwd; return &cwd; }
char **Threads_GetChroot(void) { static char *chroot; return &chroot; }


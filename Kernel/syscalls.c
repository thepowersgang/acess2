/*
 * AcessOS Microkernel Version
 * syscalls.c
 */
#define DEBUG	0

#include <common.h>
#include <syscalls.h>
#include <proc.h>
#include <errno.h>

// === IMPORTS ===
extern int	Proc_Clone(Uint *Err, Uint Flags);
extern Uint	Proc_SendMessage(Uint *Err, Uint Dest, Uint Length, void *Data);
extern int	Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer);
extern int	Proc_Execve(char *File, char **ArgV, char **EnvP);
extern Uint	Binary_Load(char *file, Uint *entryPoint);

// === CODE ===
void SyscallHandler(tSyscallRegs *Regs)
{
	Uint	ret=0, err=0;
	#if DEBUG
	ENTER("iThread iNum", gCurrentThread->TID, Regs->Num);
	if(Regs->Num < NUM_SYSCALLS)
		LOG("Syscall %s", cSYSCALL_NAMES[Regs->Num]);
	LOG("Arg1: 0x%x, Arg2: 0x%x, Arg3: 0x%x", Regs->Arg1, Regs->Arg2, Regs->Arg3);
	#endif
	switch(Regs->Num)
	{
	// -- Exit the current thread
	case SYS_EXIT:	Proc_Exit();	break;
	
	// -- Put the current thread to sleep
	case SYS_SLEEP:	Proc_Sleep();	Log(" SyscallHandler: %i is alive", gCurrentThread->TID);	break;
	
	// -- Yield current timeslice
	case SYS_YIELD:	Proc_Yield();	break;
	
	// -- Clone the current thread
	case SYS_CLONE:
		// Call clone system call
		ret = Proc_Clone(&err, Regs->Arg1);
		// Change user stack if requested
		if(ret == 0 && Regs->Arg2)
			Regs->StackPointer = Regs->Arg2;
		break;
	
	// -- Send a signal
	case SYS_KILL:
		err = -ENOSYS;
		ret = -1;
		break;
	
	// -- Map an address
	case SYS_MAP:	MM_Map(Regs->Arg1, Regs->Arg2);	break;
	// -- Allocate an address
	case SYS_ALLOCATE:	ret = MM_Allocate(Regs->Arg1);	break;
	// -- Unmap an address
	case SYS_UNMAP:		MM_Deallocate(Regs->Arg1);	break;
	
	// -- Get Thread/Process IDs
	case SYS_GETTID:	ret = gCurrentThread->TID;	break;
	case SYS_GETPID:	ret = gCurrentThread->TGID;	break;
	// -- Get User/Group IDs
	case SYS_GETUID:	ret = gCurrentThread->UID;	break;
	case SYS_GETGID:	ret = gCurrentThread->GID;	break;
	
	// -- Send Message
	case SYS_SENDMSG:
		ret = Proc_SendMessage(&err, Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3);
		break;
	// -- Check for messages
	case SYS_GETMSG:
		ret = Proc_GetMessage(&err, (Uint*)Regs->Arg1, (void*)Regs->Arg2);
		break;
	
	// -- Set the thread's name
	case SYS_SETNAME:
		// Sanity Check
		if(!Regs->Arg1) {	ret = -1;	err = -EINVAL;	break;	}
		// Lock Process
		LOCK( &gCurrentThread->IsLocked );
		// Free old name
		if(gCurrentThread->ThreadName && (Uint)gCurrentThread->ThreadName > 0xE0800000)
			free(gCurrentThread->ThreadName);
		// Change name
		gCurrentThread->ThreadName = malloc( strlen( (char*)Regs->Arg1 ) + 1 );
		strcpy(gCurrentThread->ThreadName, (char*)Regs->Arg1);
		// Unlock
		RELEASE( &gCurrentThread->IsLocked );
		break;
	
	// ---
	// Binary Control
	// ---
	case SYS_EXECVE:
		ret = Proc_Execve((char*)Regs->Arg1, (char**)Regs->Arg2, (char**)Regs->Arg3);
		break;
	case SYS_LOADBIN:
		ret = Binary_Load((char*)Regs->Arg1, (Uint*)Regs->Arg2);
		break;
	
	// ---
	// Virtual Filesystem
	// ---
	case SYS_OPEN:
		ret = VFS_Open((char*)Regs->Arg1, Regs->Arg2 | VFS_OPENFLAG_USER);
		break;
	
	case SYS_CLOSE:
		VFS_Close( Regs->Arg1 );
		break;
	
	case SYS_WRITE:
		#if BITS < 64
		VFS_Write( Regs->Arg1, Regs->Arg2|((Uint64)Regs->Arg3<<32), (void*)Regs->Arg4 );
		#else
		VFS_Write( Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3 );
		#endif
		break;
	
	
	// -- Debug
	case SYS_DEBUG:
		Log((char*)Regs->Arg1,
			Regs->Arg2, Regs->Arg3, Regs->Arg4, Regs->Arg5, Regs->Arg6);
		break;
	
	// -- Default (Return Error)
	default:
		Warning("SyscallHandler: Unknown System Call %i", Regs->Num);
		if(Regs->Num < NUM_SYSCALLS)
			Warning(" Syscall %s", cSYSCALL_NAMES[Regs->Num]);
		err = -ENOSYS;
		ret = -1;
		break;
	}
	Regs->Return = ret;
	Regs->Error = err;
	#if DEBUG
	LOG("SyscallHandler: err = %i", err);
	LEAVE('x', ret);
	#endif
}


/*
 * AcessOS Microkernel Version
 * syscalls.c
 */
#define DEBUG	0

#include <acess.h>
#include <syscalls.h>
#include <proc.h>
#include <errno.h>

#define CHECK_NUM_NULLOK(v,size)	\
	if((v)&&!Syscall_Valid((size),(Uint)(v))){ret=-1;err=-EINVAL;break;}
#define CHECK_STR_NULLOK(v)	\
	if((v)&&!Syscall_ValidString((Uint)(v))){ret=-1;err=-EINVAL;break;}
#define CHECK_NUM_NONULL(v,size)	\
	if(!(v)||!Syscall_Valid((size),(Uint)(v))){ret=-1;err=-EINVAL;break;}
#define CHECK_STR_NONULL(v)	\
	if(!(v)||!Syscall_ValidString((Uint)(v))){ret=-1;err=-EINVAL;break;}

// === IMPORTS ===
extern int	Proc_Clone(Uint *Err, Uint Flags);
extern int	Threads_WaitTID(int TID, int *status);
extern Uint	Proc_SendMessage(Uint *Err, Uint Dest, Uint Length, void *Data);
extern int	Proc_GetMessage(Uint *Err, Uint *Source, void *Buffer);
extern int	Proc_Execve(char *File, char **ArgV, char **EnvP);
extern Uint	Binary_Load(char *file, Uint *entryPoint);
extern int	Threads_SetName(char *NewName);
extern int	Threads_GetPID();
extern int	Threads_GetTID();
extern tUID	Threads_GetUID();
extern int	Threads_SetUID(Uint *errno, tUID ID);
extern tGID	Threads_GetGID();
extern int	Threads_SetGID(Uint *errno, tGID ID);
extern int	Threads_SetFaultHandler(Uint Handler);

// === PROTOTYPES ===
 int	Syscall_ValidString(Uint Addr);
 int	Syscall_Valid(int Size, Uint Addr);

// === CODE ===
// TODO: Do sanity checking on arguments, ATM the user can really fuck with the kernel
void SyscallHandler(tSyscallRegs *Regs)
{
	Uint64	ret = 0;
	Uint	err = -EOK;
	
	ENTER("iThread iNum", Threads_GetTID(), Regs->Num);
	if(Regs->Num < NUM_SYSCALLS)
		LOG("Syscall %s", cSYSCALL_NAMES[Regs->Num]);
	LOG("Arg1: 0x%x, Arg2: 0x%x, Arg3: 0x%x, Arg4: 0x%x", Regs->Arg1, Regs->Arg2, Regs->Arg3, Regs->Arg4);
	
	switch(Regs->Num)
	{
	// -- Exit the current thread
	case SYS_EXIT:	Threads_Exit(0, Regs->Arg1);	break;
	
	// -- Put the current thread to sleep
	case SYS_SLEEP:	Threads_Sleep();	break;
	
	// -- Yield current timeslice
	case SYS_YIELD:	Threads_Yield();	break;
	
	// -- Set Error Handler
	case SYS_SETFAULTHANDLER:
		Threads_SetFaultHandler(Regs->Arg1);
		break;
	
	// -- Clone the current thread
	case SYS_CLONE:
		// Call clone system call
		ret = Proc_Clone(&err, Regs->Arg1);
		// Change user stack if a new stack address is passed
		if(ret == 0 && Regs->Arg2)
			Regs->StackPointer = Regs->Arg2;
		break;
	
	// -- Send a signal
	case SYS_KILL:
		err = -ENOSYS;
		ret = -1;
		break;
	
	// -- Wait for a thread
	case SYS_WAITTID:
		// Sanity Check (Status can be NULL)
		CHECK_NUM_NULLOK( Regs->Arg2, sizeof(int) );
		// TID, *Status
		ret = Threads_WaitTID(Regs->Arg1, (void*)Regs->Arg2);
		break;
	
	// -- Get the physical address of a page
	case SYS_GETPHYS:
		ret = MM_GetPhysAddr(Regs->Arg1);
		break;
	
	// -- Map an address
	case SYS_MAP:	MM_Map(Regs->Arg1, Regs->Arg2);	break;
	
	// -- Allocate an address
	case SYS_ALLOCATE:	ret = MM_Allocate(Regs->Arg1);	break;
	
	// -- Unmap an address
	case SYS_UNMAP:		MM_Deallocate(Regs->Arg1);	break;
	
	// -- Get Thread/Process IDs
	case SYS_GETTID:	ret = Threads_GetTID();	break;
	case SYS_GETPID:	ret = Threads_GetPID();	break;
	
	// -- Get User/Group IDs
	case SYS_GETUID:	ret = Threads_GetUID();	break;
	case SYS_GETGID:	ret = Threads_GetGID();	break;
	
	// -- Set User/Group IDs
	case SYS_SETUID:	ret = Threads_SetUID(&err, Regs->Arg1);	break;
	case SYS_SETGID:	ret = Threads_SetGID(&err, Regs->Arg1);	break;
	
	// -- Send Message
	case SYS_SENDMSG:
		CHECK_NUM_NONULL(Regs->Arg3, Regs->Arg2);
		// Destination, Size, *Data
		ret = Proc_SendMessage(&err, Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3);
		break;
	// -- Check for messages
	case SYS_GETMSG:
		CHECK_NUM_NULLOK(Regs->Arg1, sizeof(Uint));
		if( Regs->Arg2 && Regs->Arg2 != -1 && !MM_IsUser(Regs->Arg2) )
		{
			err = -EINVAL;	ret = -1;	break;
		}
		// *Source, *Data
		ret = Proc_GetMessage(&err, (Uint*)Regs->Arg1, (void*)Regs->Arg2);
		break;
	
	// -- Set the thread's name
	case SYS_SETNAME:
		CHECK_STR_NONULL(Regs->Arg1);
		Threads_SetName( (char*)Regs->Arg1 );
		break;
	
	// ---
	// Binary Control
	// ---
	case SYS_EXECVE:
		CHECK_STR_NONULL(Regs->Arg1);
		//Log(" Regs = {Arg2: %x, Arg3: %x}", Regs->Arg2, Regs->Arg3);
		{
			 int	i;
			char	**tmp = (char**)Regs->Arg2;
			// Check ArgV (traverse array checking all string pointers)
			CHECK_NUM_NONULL( tmp, sizeof(char**) );
			//Log("tmp = %p", tmp);
			for(i=0;tmp[i];i++) {
				CHECK_NUM_NONULL( &tmp[i], sizeof(char*) );
				CHECK_STR_NONULL( tmp[i] );
			}
			// Check EnvP also
			// - EnvP can be NULL
			if( Regs->Arg3 )
			{
				tmp = (char**)Regs->Arg3;
				//Log("tmp = %p", tmp);
				for(i=0;tmp[i];i++) {
					CHECK_NUM_NULLOK( &tmp[i], sizeof(char*) );
					CHECK_STR_NONULL( tmp[i] );
				}
			}
		}
		LEAVE('s', "Assuming 0");
		// Path, **Argv, **Envp
		ret = Proc_Execve((char*)Regs->Arg1, (char**)Regs->Arg2, (char**)Regs->Arg3);
		break;
	case SYS_LOADBIN:
		if( !Syscall_ValidString(Regs->Arg1)
		||  !Syscall_Valid(sizeof(Uint), Regs->Arg2) ) {
			err = -EINVAL;
			ret = 0;
			break;
		}
		// Path, *Entrypoint
		ret = Binary_Load((char*)Regs->Arg1, (Uint*)Regs->Arg2);
		break;
	
	// ---
	// Virtual Filesystem
	// ---
	case SYS_OPEN:
		if( !Syscall_ValidString(Regs->Arg1) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_Open((char*)Regs->Arg1, Regs->Arg2 | VFS_OPENFLAG_USER);
		break;
	
	case SYS_CLOSE:
		VFS_Close( Regs->Arg1 );
		break;
	
	case SYS_SEEK:
		ret = VFS_Seek( Regs->Arg1, Regs->Arg2, Regs->Arg3 );
		break;
		
	case SYS_TELL:
		ret = VFS_Tell( Regs->Arg1 );
		break;
	
	case SYS_WRITE:
		CHECK_NUM_NONULL( Regs->Arg3, Regs->Arg2 );
		ret = VFS_Write( Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3 );
		break;
	
	case SYS_READ:
		CHECK_NUM_NONULL( Regs->Arg3, Regs->Arg2 );
		ret = VFS_Read( Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3 );
		break;
	
	case SYS_FINFO:
		CHECK_NUM_NONULL( Regs->Arg2, sizeof(tFInfo) + Regs->Arg3*sizeof(tVFS_ACL) );
		// FP, Dest, MaxACLs
		ret = VFS_FInfo( Regs->Arg1, (void*)Regs->Arg2, Regs->Arg3 );
		break;
	
	// Get ACL Value
	case SYS_GETACL:
		if( !Syscall_Valid(sizeof(tVFS_ACL), Regs->Arg1) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_GetACL( Regs->Arg1, (void*)Regs->Arg2 );
		break;
	
	// Read Directory
	case SYS_READDIR:
		if( !Syscall_Valid(8, Regs->Arg2) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_ReadDir( Regs->Arg1, (void*)Regs->Arg2 );
		break;
	
	// Open a file that is a entry in an open directory
	case SYS_OPENCHILD:
		if( !Syscall_ValidString(Regs->Arg2) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_OpenChild( &err, Regs->Arg1, (char*)Regs->Arg2, Regs->Arg3);
		break;
	
	// Change Directory
	case SYS_CHDIR:
		if( !Syscall_ValidString(Regs->Arg1) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_ChDir( (void*)Regs->Arg1 );
		break;
	
	// IO Control
	case SYS_IOCTL:
		// All sanity checking should be done by the driver
		ret = VFS_IOCtl( Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3 );
		break;
	
	// Mount a filesystem
	case SYS_MOUNT:
		// Only root can mount filesystems
		if(Threads_GetUID() != 0) {
			err = -EACCES;
			ret = -1;
			break;
		}
		// Sanity check the paths
		if(!Syscall_ValidString(Regs->Arg1)
		|| !Syscall_ValidString(Regs->Arg2)
		|| !Syscall_ValidString(Regs->Arg3)
		|| !Syscall_ValidString(Regs->Arg4) ) {
			err = -EINVAL;
			ret = -1;
			break;
		}
		ret = VFS_Mount(
			(char*)Regs->Arg1,	// Device
			(char*)Regs->Arg2,	// Mount point
			(char*)Regs->Arg3,	// Filesystem
			(char*)Regs->Arg4	// Options
			);
		break;
	
	// -- Debug
	//#if DEBUG_BUILD
	case SYS_DEBUG:
		Log((char*)Regs->Arg1,
			Regs->Arg2, Regs->Arg3, Regs->Arg4, Regs->Arg5, Regs->Arg6);
		break;
	//#endif
	
	// -- Default (Return Error)
	default:
		Warning("SyscallHandler: Unknown System Call %i", Regs->Num);
		if(Regs->Num < NUM_SYSCALLS)
			Warning(" Syscall '%s'", cSYSCALL_NAMES[Regs->Num]);
		err = -ENOSYS;
		ret = -1;
		break;
	}
	
	if(err != 0) {
		LOG("ID: %i, Return errno = %i", Regs->Num, err);
	}
	
	#if BITS < 64
	Regs->Return = ret&0xFFFFFFFF;
	Regs->RetHi = ret >> 32;
	#else
	Regs->Return = ret;
	#endif
	Regs->Error = err;
	#if DEBUG
	LOG("err = %i", err);
	if(Regs->Num != SYS_EXECVE)
		LEAVE('x', ret);
	else
		LOG("Actual %i", ret);
	#endif
}

/**
 * \fn int Syscall_ValidString(Uint Addr)
 * \brief Checks if a memory address contains a valid string
 */
int Syscall_ValidString(Uint Addr)
{
	// Check if the memory is user memory
	if(!MM_IsUser(Addr))	return 0;
	
	return CheckString( (char*)Addr );
}

/**
 * \fn int Syscall_Valid(int Size, Uint Addr)
 * \brief Checks if a memory address is valid
 */
int Syscall_Valid(int Size, Uint Addr)
{
	if(!MM_IsUser(Addr))	return 0;
	
	return CheckMem( (void*)Addr, Size );
}

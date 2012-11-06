/*
 * AcessOS Microkernel Version
 * syscalls.c
 */
#define DEBUG	0

#include <acess.h>
#include <syscalls.h>
#include <proc.h>
#include <hal_proc.h>
#include <errno.h>
#include <threads.h>
#include <events.h>

#define CHECK_NUM_NULLOK(v,size)	\
	if((v)&&!Syscall_Valid((size),(v))){ret=-1;err=-EINVAL;break;}
#define CHECK_STR_NULLOK(v)	\
	if((v)&&!Syscall_ValidString((v))){ret=-1;err=-EINVAL;break;}
#define CHECK_NUM_NONULL(v,size)	\
	if(!(v)||!Syscall_Valid((size),(v))){ret=-1;err=-EINVAL;break;}
#define CHECK_STR_NONULL(v)	\
	if(!(v)||!Syscall_ValidString((v))){ret=-1;err=-EINVAL;break;}
#define CHECK_STR_ARRAY(arr)	do {\
	 int	i;\
	char	**tmp = (char**)arr; \
	CHECK_NUM_NONULL( tmp, sizeof(char**) ); \
	for(i=0;tmp[i];i++) { \
		CHECK_STR_NONULL( tmp[i] ); \
		CHECK_NUM_NONULL( &tmp[i+1], sizeof(char*) ); \
	}\
	if(tmp[i]) break;\
} while(0)

// === IMPORTS ===
extern Uint	Binary_Load(const char *file, Uint *entryPoint);

// === PROTOTYPES ===
void	SyscallHandler(tSyscallRegs *Regs);
 int	Syscall_ValidString(const char *Addr);
 int	Syscall_Valid(int Size, const void *Addr);
 int	Syscall_MM_SetFlags(const void *Addr, Uint Flags, Uint Mask);

// === CODE ===
// TODO: Do sanity checking on arguments, ATM the user can really fuck with the kernel
void SyscallHandler(tSyscallRegs *Regs)
{
	Uint64	ret = 0;
	Uint	err = -EOK;
	 int	callNum = Regs->Num;
	
	#if DEBUG < 2
	if(callNum != SYS_READ && callNum != SYS_WRITE) {
	#endif
	ENTER("iThread iNum", Threads_GetTID(), callNum);
	if(callNum < NUM_SYSCALLS)
		LOG("Syscall %s", cSYSCALL_NAMES[callNum]);
	LOG("Arg1: 0x%x, Arg2: 0x%x, Arg3: 0x%x, Arg4: 0x%x", Regs->Arg1, Regs->Arg2, Regs->Arg3, Regs->Arg4);
	#if DEBUG < 2
	}
	#endif
	
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
		ret = Proc_Clone(Regs->Arg1);
		break;
	
	// -- Send a signal
	case SYS_KILL:
		err = -ENOSYS;
		ret = -1;
		break;

	// -- Wait fr an event	
	case SYS_WAITEVENT:
		// Message mask
		ret = Threads_WaitEvents(Regs->Arg1);
		break;

	// -- Wait for a thread
	case SYS_WAITTID:
		// Sanity Check (Status can be NULL)
		CHECK_NUM_NULLOK( (int*)Regs->Arg2, sizeof(int) );
		// TID, *Status
		ret = Threads_WaitTID(Regs->Arg1, (int*)Regs->Arg2);
		break;
	
	// -- Get the physical address of a page
	case SYS_GETPHYS:
		ret = MM_GetPhysAddr( (void*)Regs->Arg1 );
		break;
	
	// -- Map an address
	case SYS_MAP:	MM_Map(Regs->Arg1, Regs->Arg2);	break;
	
	// -- Allocate an address
	case SYS_ALLOCATE:	ret = MM_Allocate(Regs->Arg1);	break;
	
	// -- Unmap an address
	case SYS_UNMAP:		MM_Deallocate(Regs->Arg1);	break;
	
	// -- Change the protection on an address
	case SYS_SETFLAGS:
		ret = Syscall_MM_SetFlags((void*)Regs->Arg1, Regs->Arg2, Regs->Arg3);
		break;

	// -- Get Thread/Process IDs
	case SYS_GETTID:	ret = Threads_GetTID();	break;
	case SYS_GETPID:	ret = Threads_GetPID();	break;
	
	// -- Get User/Group IDs
	case SYS_GETUID:	ret = Threads_GetUID();	break;
	case SYS_GETGID:	ret = Threads_GetGID();	break;
	
	// -- Set User/Group IDs
	case SYS_SETUID:	ret = Threads_SetUID(Regs->Arg1);	break;
	case SYS_SETGID:	ret = Threads_SetGID(Regs->Arg1);	break;
	
	// -- Send Message
	case SYS_SENDMSG:
		CHECK_NUM_NONULL( (void*)Regs->Arg3, Regs->Arg2 );
		// Destination, Size, *Data
		ret = Proc_SendMessage(Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3);
		break;
	// -- Check for messages
	case SYS_GETMSG:
		CHECK_NUM_NULLOK( (Uint*)Regs->Arg1, sizeof(Uint) );
		if( Regs->Arg3 != -1 )
			CHECK_NUM_NULLOK((void*)Regs->Arg3, Regs->Arg2);
		// *Source, *Data
		ret = Proc_GetMessage((Uint*)Regs->Arg1, Regs->Arg2, (void*)Regs->Arg3);
		break;
	
	// -- Get the current timestamp
	case SYS_GETTIME:
		ret = now();
		break;
	
	// -- Set the thread's name
	case SYS_SETNAME:
		CHECK_STR_NONULL( (char*) Regs->Arg1);
		Threads_SetName( (char*)Regs->Arg1 );
		break;
	
	// ---
	// Binary Control
	// ---
	// -- Create a new process
	case SYS_SPAWN:
		CHECK_STR_NONULL((const char*)Regs->Arg1);
		CHECK_STR_ARRAY((const char**)Regs->Arg2);
		CHECK_STR_ARRAY((const char**)Regs->Arg3);
		CHECK_NUM_NONULL((void*)Regs->Arg5, Regs->Arg4*sizeof(int));
		ret = Proc_SysSpawn(
			(const char*)Regs->Arg1, (const char**)Regs->Arg2, (const char**)Regs->Arg3,
			Regs->Arg4, (int*)Regs->Arg5
			);
		break;
	// -- Replace the current process with another
	case SYS_EXECVE:
		CHECK_STR_NONULL((char*)Regs->Arg1);
		CHECK_STR_ARRAY( (char**)Regs->Arg2 );
		if( Regs->Arg3 )
			CHECK_STR_ARRAY( (char**)Regs->Arg3 );
		LEAVE('s', "Assuming 0");
		// Path, **Argv, **Envp, DataSize (=0 to tell it to create a copy)
		ret = Proc_Execve(
			(const char*)Regs->Arg1, (const char**)Regs->Arg2, (const char**)Regs->Arg3,
			0
			);
		break;
	// -- Load a binary into the current process
	case SYS_LOADBIN:
		CHECK_STR_NONULL( (char*)Regs->Arg1 );
		CHECK_NUM_NONULL( (Uint*)Regs->Arg2, sizeof(Uint) );
		// Path, *Entrypoint
		ret = Binary_Load((char*)Regs->Arg1, (Uint*)Regs->Arg2);
		break;
	
	// ---
	// Virtual Filesystem
	// ---
	case SYS_OPEN:
		CHECK_STR_NONULL( (char*)Regs->Arg1 );
		LOG("VFS_Open(\"%s\", 0x%x)", (char*)Regs->Arg1, Regs->Arg2 | VFS_OPENFLAG_USER);
		ret = VFS_Open((char*)Regs->Arg1, Regs->Arg2 | VFS_OPENFLAG_USER);
		break;
	
	case SYS_CLOSE:
		LOG("VFS_Close(%i)", Regs->Arg1);
		VFS_Close( Regs->Arg1 );
		break;
	
	case SYS_SEEK:
		#if BITS == 64
		ret = VFS_Seek( Regs->Arg1, Regs->Arg2, Regs->Arg3 );
		#else
		ret = VFS_Seek( Regs->Arg1, Regs->Arg2|(((Uint64)Regs->Arg3)<<32), Regs->Arg4 );
		#endif
		break;
		
	case SYS_TELL:
		ret = VFS_Tell( Regs->Arg1 );
		break;
	
	case SYS_WRITE:
		CHECK_NUM_NONULL( (void*)Regs->Arg2, Regs->Arg3 );
		ret = VFS_Write( Regs->Arg1, Regs->Arg3, (void*)Regs->Arg2 );
		break;
	
	case SYS_READ:
		CHECK_NUM_NONULL( (void*)Regs->Arg2, Regs->Arg3 );
		ret = VFS_Read( Regs->Arg1, Regs->Arg3, (void*)Regs->Arg2 );
		break;
	
	case SYS_FINFO:
		CHECK_NUM_NONULL( (void*)Regs->Arg2, sizeof(tFInfo) + Regs->Arg3*sizeof(tVFS_ACL) );
		// FP, Dest, MaxACLs
		ret = VFS_FInfo( Regs->Arg1, (void*)Regs->Arg2, Regs->Arg3 );
		break;
	
	// Get ACL Value
	case SYS_GETACL:
		CHECK_NUM_NONULL( (void*)Regs->Arg2, sizeof(tVFS_ACL) );
		ret = VFS_GetACL( Regs->Arg1, (void*)Regs->Arg2 );
		break;
	
	// Read Directory
	case SYS_READDIR:
		// TODO: What if the filename is longer?
		// Maybe force it to be a 256 byte buffer
		CHECK_NUM_NONULL( (void*)Regs->Arg2, 256 );
		ret = VFS_ReadDir( Regs->Arg1, (void*)Regs->Arg2 );
		break;
	
	// Open a file that is a entry in an open directory
	case SYS_OPENCHILD:
		CHECK_STR_NONULL( (char*)Regs->Arg2 );
		ret = VFS_OpenChild( Regs->Arg1, (char*)Regs->Arg2, Regs->Arg3 | VFS_OPENFLAG_USER);
		break;
	
	// Change Directory
	case SYS_CHDIR:
		CHECK_STR_NONULL( (const char*)Regs->Arg1 );
		ret = VFS_ChDir( (const char*)Regs->Arg1 );
		break;
	
	// IO Control
	case SYS_IOCTL:
		// All sanity checking should be done by the driver
		if( Regs->Arg3 && !MM_IsUser(Regs->Arg3) ) {
			err = -EINVAL;	ret = -1;	break;
		}
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
		
		if( !Regs->Arg1 )
		{
			if( !Syscall_ValidString((char*)Regs->Arg2) ) {
				err = -EINVAL;
				ret = -1;
				break;
			}
			
			ret = VFS_Unmount((char*)Regs->Arg2);
		}
		else
		{
			// Sanity check the paths
			if(!Syscall_ValidString((char*)Regs->Arg1)
			|| !Syscall_ValidString((char*)Regs->Arg2)
			|| (Regs->Arg3 && !Syscall_ValidString((char*)Regs->Arg3))
			|| !Syscall_ValidString((char*)Regs->Arg4) ) {
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
		}
		break;
		
	// Wait on a set of handles
	case SYS_SELECT:
		// Sanity checks
		if( (Regs->Arg2 && !Syscall_Valid(sizeof(fd_set), (void*)Regs->Arg2))
		 || (Regs->Arg3 && !Syscall_Valid(sizeof(fd_set), (void*)Regs->Arg3))
		 || (Regs->Arg4 && !Syscall_Valid(sizeof(fd_set), (void*)Regs->Arg4))
		 || (Regs->Arg5 && !Syscall_Valid(sizeof(tTime), (void*)Regs->Arg5)) )
		{
			err = -EINVAL;
			ret = -1;
			break;
		}
		// Perform the call
		ret = VFS_Select(
			Regs->Arg1,	// Max handle
			(fd_set *)Regs->Arg2,	// Read
			(fd_set *)Regs->Arg3,	// Write
			(fd_set *)Regs->Arg4,	// Errors
			(tTime *)Regs->Arg5,	// Timeout
			(Uint32)Regs->Arg6,	// Extra wakeup events
			0	// User handles
			);
		break;
	
	// -- Debug
	//#if DEBUG_BUILD
	case SYS_DEBUG:
		CHECK_STR_NONULL( (char*)Regs->Arg1 );
		LogF("Log: %08lli [%i] ", now(), Threads_GetTID());
		LogF((const char*)Regs->Arg1,
			Regs->Arg2, Regs->Arg3, Regs->Arg4, Regs->Arg5, Regs->Arg6);
		LogF("\r\n");
		break;
	//#endif
	
	// -- Default (Return Error)
	default:
		Log_Warning("Syscalls", "Unknown System Call %i", Regs->Num);
		if(Regs->Num < NUM_SYSCALLS)
			Log_Warning("Syscall", " named '%s'", cSYSCALL_NAMES[Regs->Num]);
		err = -ENOSYS;
		ret = -1;
		break;
	}

	if(err == 0)	err = errno;
	
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
	# if DEBUG < 2
	if( callNum != SYS_READ && callNum != SYS_WRITE ) {
	# endif
	LOG("err = %i", err);
	if( callNum == SYS_EXECVE )
		LOG("Actual %i", ret);
	else
		LEAVE('x', ret);
	# if DEBUG < 2
	}
	# endif
	#endif
}

/**
 * \fn int Syscall_ValidString(const char *Addr)
 * \brief Checks if a memory address contains a valid string
 */
int Syscall_ValidString(const char *Addr)
{
	// Check if the memory is user memory
	if(!MM_IsUser( (tVAddr) Addr))	return 0;
	
	return CheckString( Addr );
}

/**
 * \fn int Syscall_Valid(int Size, const void *Addr)
 * \brief Checks if a memory address is valid
 */
int Syscall_Valid(int Size, const void *Addr)
{
	if(!MM_IsUser( (tVAddr)Addr ))	return 0;
	
	return CheckMem( Addr, Size );
}

int Syscall_MM_SetFlags(const void *Addr, Uint Flags, Uint Mask)
{
	tPAddr	paddr = MM_GetPhysAddr(Addr);
	Flags &= MM_PFLAG_RO|MM_PFLAG_EXEC;
	Mask &= MM_PFLAG_RO|MM_PFLAG_EXEC;

	//Log_Debug("Syscalls", "SYS_SETFLAGS: %p %x %x", Addr, Flags, Mask);	

	// Enable write?
	if( (Mask & MM_PFLAG_RO) && !(Flags & MM_PFLAG_RO) ) {
		void	*node;
		// HACK - Assume that RO mmap'd files are immutable
		if( MM_GetPageNode(paddr, &node) == 0 && node ) {
			Flags |= MM_PFLAG_COW;
			Mask |= MM_PFLAG_COW;
		}
	}
	MM_SetFlags((tVAddr)Addr, Flags, Mask);
	return 0;
}

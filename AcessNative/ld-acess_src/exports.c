/*
 * AcessNative
 *
 * exports.c
 * - Exported functions
 */
#define DONT_INCLUDE_SYSCALL_NAMES 1
#include "../../Usermode/Libraries/ld-acess.so_src/include_exp/acess/sys.h"
#include "../syscalls.h"
#include "exports.h"
#include <stdarg.h>
#include <stddef.h>

#define DEBUG(v...)	do{if(gbSyscallDebugEnabled)Debug(v);}while(0)
#define PAGE_SIZE	4096

#define TODO()	Warning("TODO: %s", __func__)

typedef struct sFILE	FILE;

extern void	exit(int) __attribute__ ((noreturn));
extern int	printf(const char *, ...);
extern int	fprintf(FILE *,const char *, ...);
extern int	sprintf(char *,const char *, ...);
extern int	vprintf(const char *, va_list);
extern int	strncmp(const char *, const char *, size_t);

extern int	gSocket;
extern int	giSyscall_ClientID;	// Needed for execve
extern void	_InitSyscalls(void);
extern void	_CloseSyscalls(void);

extern void	Warning(const char *Format, ...);
extern void	Debug(const char *Format, ...);
extern int	AllocateMemory(uintptr_t VirtAddr, size_t ByteCount);

// === CONSTANTS ===
#define NATIVE_FILE_MASK	0x40000000

// === GLOBALS ===
int	acess__errno;
 int	gbSyscallDebugEnabled = 0;
char	*gsExecutablePath = "./ld-acess";

// === CODE ===
// --- VFS Calls
int acess__SysChdir(const char *Path)
{
	return _Syscall(SYS_CHDIR, ">s", Path);
}

int acess__SysOpen(const char *Path, unsigned int Flags)
{
	if( strncmp(Path, "$$$$", 4) == 0 )
	{
		return native_open(Path, Flags) | NATIVE_FILE_MASK;
	}
	SYSTRACE("open(\"%s\", 0x%x)", Path, Flags);
	return _Syscall(SYS_OPEN, ">s >i", Path, Flags);
}

void acess__SysClose(int FD)
{
	if(FD & NATIVE_FILE_MASK) {
		return native_close(FD & (NATIVE_FILE_MASK-1));
	}
	SYSTRACE("close(%i)", FD);
	_Syscall(SYS_CLOSE, ">i", FD);
}

int acess__SysReopen(int FD, const char *Path, int Flags) {
	SYSTRACE("reopen(0x%x, \"%s\", 0x%x)", FD, Path, Flags);
	return _Syscall(SYS_REOPEN, ">i >s >i", FD, Path, Flags);
}

int acess__SysCopyFD(int srcfd, int dstfd) {
	SYSTRACE("_SysCopyFD(%i, %i)", srcfd, dstfd);
	return _Syscall(SYS_COPYFD, ">i >i", srcfd, dstfd);
}

int acess__SysFDFlags(int fd, int mask, int newflags) {
	return _Syscall(SYS_FDFLAGS, ">i >i >i", fd, mask, newflags);
}

size_t acess__SysRead(int FD, void *Dest, size_t Bytes) {
	if(FD & NATIVE_FILE_MASK)
		return native_read(FD & (NATIVE_FILE_MASK-1), Dest, Bytes);
	SYSTRACE("_SysRead(0x%x, 0x%x, *%p)", FD, Bytes, Dest);
	return _Syscall(SYS_READ, ">i >i <d", FD, Bytes, Bytes, Dest);
}

size_t acess__SysWrite(int FD, const void *Src, size_t Bytes) {
	if(FD & NATIVE_FILE_MASK)
		return native_write(FD & (NATIVE_FILE_MASK-1), Src, Bytes);
	SYSTRACE("_SysWrite(0x%x, 0x%x, %p\"%.*s\")", FD, Bytes, Src, Bytes, (char*)Src);
	return _Syscall(SYS_WRITE, ">i >i >d", FD, Bytes, Bytes, Src);
}
uint64_t acess__SysTruncate(int fd, uint64_t size) {
	TODO();
	return 0;
}

int acess__SysSeek(int FD, int64_t Ofs, int Dir)
{
	if(FD & NATIVE_FILE_MASK) {
		return native_seek(FD & (NATIVE_FILE_MASK-1), Ofs, Dir);
	}
	SYSTRACE("_SysSeek(0x%x, 0x%llx, %i)", FD, Ofs, Dir);
	return _Syscall(SYS_SEEK, ">i >I >i", FD, Ofs, Dir);
}

uint64_t acess__SysTell(int FD)
{
	if(FD & NATIVE_FILE_MASK)
		return native_tell( FD & (NATIVE_FILE_MASK-1) );
	SYSTRACE("_SysTell(0x%x)", FD);
	return _Syscall(SYS_TELL, ">i", FD);
}

int acess__SysIOCtl(int fd, int id, void *data) {
	 int	len;
	SYSTRACE("_SysIOCtl(%i, %i, %p)", fd, id, data);
	// NOTE: The length here is hacky and could break
	len = (data == NULL ? 0 : PAGE_SIZE - ((uintptr_t)data % PAGE_SIZE));
	return _Syscall(SYS_IOCTL, ">i >i ?d", fd, id, len, data);
}
int acess__SysFInfo(int fd, t_sysFInfo *info, int maxacls) {
	SYSTRACE("_SysFInfo(%i, %p, %i)", fd, info, maxacls);
	return _Syscall(SYS_FINFO, ">i <d >i",
		fd,
		sizeof(t_sysFInfo)+maxacls*sizeof(t_sysACL), info,
		maxacls
		);
}

int acess__SysReadDir(int fd, char *dest) {
	SYSTRACE("_SysReadDir(%i, %p)", fd, dest);
	return _Syscall(SYS_READDIR, ">i <d", fd, 256, dest);
}

int acess__SysSelect(int nfds, fd_set *read, fd_set *write, fd_set *error, int64_t *timeout, uint32_t events)
{
	SYSTRACE("_SysSelect(%i, %p, %p, %p, %p, 0x%x)", nfds, read, write, error, timeout, events);
	return _Syscall(SYS_SELECT, ">i ?d ?d ?d >d >i", nfds,
		read ? (nfds+7)/8 : 0, read,
		write ? (nfds+7)/8 : 0, write,
		error ? (nfds+7)/8 : 0, error,
		sizeof(*timeout), timeout,
		events
		);
}
int acess__SysMkDir(const char *pathname)
{
	TODO();
	return 0;
}
int acess__SysUnlink(const char *pathname)
{
	// TODO:
	TODO();
	return 0;
}
void* acess__SysMMap(void *addr, size_t length, unsigned int _flags, int fd, uint64_t offset)
{
	TODO();
	return NULL;
}
int acess__SysMUnMap(void *addr, size_t length)
{
	TODO();
	return 0;
}
uint64_t acess__SysMarshalFD(int FD)
{
	TODO();
	return 0;
}
int acess__SysUnMarshalFD(uint64_t Handle)
{
	TODO();
	return -1;
}

int acess__SysOpenChild(int fd, char *name, int flags) {
	SYSTRACE("_SysOpenChild(0x%x, '%s', 0x%x)", fd, name, flags);
	return _Syscall(SYS_OPENCHILD, ">i >s >i", fd, name, flags);
}

int acess__SysGetACL(int fd, t_sysACL *dest) {
	SYSTRACE("%s(0x%x, %p)", __func__, fd, dest);
	return _Syscall(SYS_GETACL, ">i <d", fd, sizeof(t_sysACL), dest);
}

int acess__SysMount(const char *Device, const char *Directory, const char *Type, const char *Options) {
	SYSTRACE("_SysMount('%s', '%s', '%s', '%s')", Device, Directory, Type, Options);
	return _Syscall(SYS_MOUNT, ">s >s >s >s", Device, Directory, Type, Options);
}


// --- Error Handler
int acess__SysSetFaultHandler(int (*Handler)(int)) {
	printf("TODO: Set fault handler (asked to set to %p)\n", Handler);
	return 0;
}

void acess__SysSetName(const char *Name)
{
	// TODO:
	TODO();
}

int acess__SysGetName(char *NameDest)
{
	// TODO: 
	TODO();
	return 0;
}

int acess__SysSetPri(int Priority)
{
	// TODO:
	TODO();
	return 0;
}

// --- Binaries? ---
void *acess_SysLoadBin(const char *path, void **entry)
{
	// ERROR!
	TODO();
	return NULL;
}

int acess__SysUnloadBin(void *base)
{
	// ERROR!
	TODO();
	return -1;
}

// --- System ---
int acess__SysLoadModule(const char *Path)
{
	TODO();
	return -1;
}

// --- Timekeeping ---
int64_t acess__SysTimestamp(void)
{
	// TODO: Better impl
	TODO();
//	return now()*1000;
	return 0;
}

// --- Memory Management ---
uint64_t acess__SysGetPhys(uintptr_t vaddr)
{
	// TODO:
	TODO();
	return 0;
}

uint64_t acess__SysAllocate(uintptr_t vaddr)
{
	if( AllocateMemory(vaddr, 0x1000) == -1 )	// Allocate a page
		return 0;
		
	return vaddr;	// Just ignore the need for paddrs :)
}

// --- Process Management ---
int acess__SysClone(int flags, void *stack)
{
	#ifdef __WIN32__
	Warning("Win32 does not support anything like fork(2), cannot emulate");
	exit(-1);
	#else
	extern int fork(void);
	if(flags & CLONE_VM) {
		 int	ret, newID, kernel_tid=0;
		Debug("USERSIDE fork()");
		
		newID = _Syscall(SYS_AN_FORK, "<d", sizeof(int), &kernel_tid);
		ret = fork();
		if(ret < 0) {
			return ret;
		}
		
		if(ret == 0)
		{
			_CloseSyscalls();
			giSyscall_ClientID = newID;
			_InitSyscalls();
			return 0;
		}
		
		// Return the acess TID instead
		return kernel_tid;
	}
	else
	{
		Warning("ERROR: Threads currently unsupported\n");
		exit(-1);
	}
	#endif
}

int acess__SysKill(int pid, int sig)
{
	// TODO: Impliment SysKill
	return -1;
}

int acess__SysExecVE(char *path, char **argv, const char **envp)
{
	 int	i, argc;
	
	DEBUG("acess_execve: (path='%s', argv=%p, envp=%p)", path, argv, envp);
	
	// Get argument count
	for( argc = 0; argv[argc]; argc ++ ) ;
	DEBUG(" acess_execve: argc = %i", argc);

	const char	*new_argv[7+argc+1];
	char	client_id_str[11];
	char	socket_fd_str[11];
	sprintf(client_id_str, "%i", giSyscall_ClientID);
	sprintf(socket_fd_str, "%i", gSocket);
	new_argv[0] = "ld-acess";	// TODO: Get path to ld-acess executable
	new_argv[1] = "--key";  	// Set client ID for Request.c
	new_argv[2] = client_id_str;
	new_argv[3] = "--socket";	// Socket
	new_argv[4] = socket_fd_str;
	new_argv[5] = "--binary";	// Set the binary path (instead of using argv[0])
	new_argv[6] = path;
	for( i = 0; i < argc; i ++ )	new_argv[7+i] = argv[i];
	new_argv[7+i] = NULL;
	
	#if 1
	argc += 7;
	for( i = 0; i < argc; i ++ )
		printf("\"%s\" ", new_argv[i]);
	printf("\n");
	if(envp)
	{
		printf("envp = %p\n", envp);
		for( i = 0; envp[i]; i ++ )
			printf("%i: \"%s\"\n", i, envp[i]);
		printf("envc = %i\n", i);
	}
	#endif
	
	// Call actual execve
	return native_execve("./ld-acess", new_argv, envp);
}

int acess__SysSpawn(const char *binary, const char **argv, const char **envp, int nfd, int fds[], struct s_sys_spawninfo *info)
{
	 int	argc = 0;
	while( argv[argc++] );

	Debug("_SysSpawn('%s', %p (%i), %p, %i, %p, %p)",
		binary, argv, argc, envp, nfd, fds, info);

	 int	kernel_tid;
	 int	newID;
	newID = _Syscall(SYS_AN_SPAWN, "<d >d >d",
		sizeof(int), &kernel_tid,
		nfd*sizeof(int), fds,
		info ? sizeof(*info) : 0, info);

	const char	*new_argv[5+argc+1];
	 int	new_argc = 0, i;
	char	client_id_str[11];
	sprintf(client_id_str, "%i", newID);
	new_argv[new_argc++] = gsExecutablePath;       // TODO: Get path to ld-acess executable
	new_argv[new_argc++] = "--key";
	new_argv[new_argc++] = client_id_str;
	new_argv[new_argc++] = "--binary";
	new_argv[new_argc++] = binary;
	for( i = 0; argv[i]; i ++)
		new_argv[new_argc++] = argv[i];
	new_argv[new_argc++] = NULL;
	
	// TODO: Debug output?
	
	native_spawn(gsExecutablePath, new_argv, envp);

	return kernel_tid;
}

//void acess_sleep(void)
//{
//	DEBUG("%s()", __func__);
//	_Syscall(SYS_SLEEP, "");
//}

void acess__SysTimedSleep(int64_t Delay)
{
	DEBUG("%s(%lli)", __func__, Delay);
	// Not accurate, but fuck it
	//if( Delay > 1000 )	sleep(Delay / 1000);
	//if( Delay % 1000 )	usleep( (Delay % 1000) * 1000 );
	//_Syscall(SYS_TIMEDSLEEP, ">I", Delay);
}

int acess__SysWaitTID(int TID, int *ExitStatus)
{
	DEBUG("%s(%i, %p)", __func__, TID, ExitStatus);
	return _Syscall(SYS_WAITTID, ">i <d", TID, sizeof(int), &ExitStatus);
}

int acess_setuid(int ID) { return _Syscall(SYS_SETUID, ">i", ID); }
int acess_setgid(int ID) { return _Syscall(SYS_SETGID, ">i", ID); }
int acess_gettid(void) { return _Syscall(SYS_GETTID, ""); }
int acess__SysGetPID(void) { return _Syscall(SYS_GETPID, ""); }
int acess__SysGetUID(void) { return _Syscall(SYS_GETUID, ""); }
int acess__SysGetGID(void) { return _Syscall(SYS_GETGID, ""); }
int acess_getgid(void) { return _Syscall(SYS_GETGID, ""); }

int acess__SysSendMessage(int DestTID, int Length, void *Data)
{
	DEBUG("%s(%i, 0x%x, %p)", __func__, DestTID, Length, Data);
	return _Syscall(SYS_SENDMSG, ">i >d", DestTID, Length, Data);
}

int acess__SysGetMessage(int *SourceTID, int BufLen, void *Data)
{
	DEBUG("%s(%p, %p)", __func__, SourceTID, Data);
	return _Syscall(SYS_GETMSG, "<d <d",
		SourceTID ? sizeof(uint32_t) : 0, SourceTID,
		BufLen, Data
		);
}

int acess__SysWaitEvent(int Mask)
{
	DEBUG("%s(%x)", __func__, Mask);
	return _Syscall(SYS_WAITEVENT, ">i", Mask);
}

// --- Logging
void acess__SysDebug(const char *Format, ...)
{
	va_list	args;
	
	va_start(args, Format);
	
	printf("[_SysDebug %i] ", giSyscall_ClientID);
	vprintf(Format, args);
	printf("\n");
	
	va_end(args);
}

void acess__exit(int Status)
{
	DEBUG("_exit(%i)", Status);
	_Syscall(SYS_EXIT, ">i", Status);
	exit(Status);
}

uint32_t acess__SysSetMemFlags(uintptr_t vaddr, uint32_t flags, uint32_t mask)
{
	// TODO: Impliment acess__SysSetMemFlags?
	return 0;
}


// === Symbol List ===
#ifndef DEFSYM
# define DEFSYM(name)	{#name, &acess_##name}
#endif
const tSym	caBuiltinSymbols[] = {
	DEFSYM(_exit),
	
	DEFSYM(_SysChdir),
	DEFSYM(_SysOpen),
	DEFSYM(_SysOpenChild),
	DEFSYM(_SysReopen),
	DEFSYM(_SysClose),
	DEFSYM(_SysRead),
	DEFSYM(_SysWrite),
	DEFSYM(_SysSeek),
	DEFSYM(_SysTell),
	DEFSYM(_SysIOCtl),
	DEFSYM(_SysFInfo),
	DEFSYM(_SysReadDir),
	DEFSYM(_SysGetACL),
	DEFSYM(_SysMount),
	DEFSYM(_SysSelect),
	DEFSYM(_SysMkDir),
	DEFSYM(_SysUnlink),
	
	DEFSYM(_SysClone),
	DEFSYM(_SysExecVE),
	DEFSYM(_SysSpawn),
//	DEFSYM(sleep),
	
	DEFSYM(_SysWaitTID),
	DEFSYM(gettid),
	DEFSYM(_SysGetPID),
	DEFSYM(setuid),
	DEFSYM(setgid),
	DEFSYM(_SysGetUID),
	DEFSYM(getgid),

	DEFSYM(_SysSendMessage),
	DEFSYM(_SysGetMessage),
	
	DEFSYM(_SysAllocate),
	DEFSYM(_SysSetMemFlags),
	DEFSYM(_SysDebug),
	{"_ZN4_sys5debugEPKcz", &acess__SysDebug},
	DEFSYM(_SysSetFaultHandler),
	DEFSYM(_SysWaitEvent),
	
	DEFSYM(_errno)
};

const int	ciNumBuiltinSymbols = sizeof(caBuiltinSymbols)/sizeof(caBuiltinSymbols[0]);


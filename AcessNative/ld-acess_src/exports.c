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

#define DEBUG(v...)	Debug(v)
#define PAGE_SIZE	4096

typedef struct sFILE	FILE;

extern FILE	*stderr;
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

extern void	Debug(const char *Format, ...);
extern int	AllocateMemory(uintptr_t VirtAddr, size_t ByteCount);

// === CONSTANTS ===
#define NATIVE_FILE_MASK	0x40000000

// === GLOBALS ===
int	acess__errno;

// === CODE ===
// --- VFS Calls
int acess_chdir(const char *Path)
{
	return _Syscall(SYS_CHDIR, ">s", Path);
}

int acess_open(const char *Path, int Flags)
{
	if( strncmp(Path, "$$$$", 4) == 0 )
	{
		return native_open(Path, Flags) | NATIVE_FILE_MASK;
	}
	DEBUG("open(\"%s\", 0x%x)", Path, Flags);
	return _Syscall(SYS_OPEN, ">s >i", Path, Flags);
}

void acess_close(int FD) {
	if(FD & NATIVE_FILE_MASK) {
		return native_close(FD & (NATIVE_FILE_MASK-1));
	}
	DEBUG("close(%i)", FD);
	_Syscall(SYS_CLOSE, ">i", FD);
}

int acess_reopen(int FD, const char *Path, int Flags) {
	DEBUG("reopen(0x%x, \"%s\", 0x%x)", FD, Path, Flags);
	return _Syscall(SYS_REOPEN, ">i >s >i", FD, Path, Flags);
}

size_t acess_read(int FD, void *Dest, size_t Bytes) {
	if(FD & NATIVE_FILE_MASK)
		return native_read(FD & (NATIVE_FILE_MASK-1), Dest, Bytes);
//	if( FD > 2 )
		DEBUG("read(0x%x, 0x%x, *%p)", FD, Bytes, Dest);
	return _Syscall(SYS_READ, ">i >i <d", FD, Bytes, Bytes, Dest);
}

size_t acess_write(int FD, const void *Src, size_t Bytes) {
	if(FD & NATIVE_FILE_MASK)
		return native_write(FD & (NATIVE_FILE_MASK-1), Src, Bytes);
//	if( FD > 2 )
		DEBUG("write(0x%x, 0x%x, %p\"%.*s\")", FD, Bytes, Src, Bytes, (char*)Src);
	return _Syscall(SYS_WRITE, ">i >i >d", FD, Bytes, Bytes, Src);
}

int acess_seek(int FD, int64_t Ofs, int Dir) {
	if(FD & NATIVE_FILE_MASK) {
		return native_seek(FD & (NATIVE_FILE_MASK-1), Ofs, Dir);
	}
	DEBUG("seek(0x%x, 0x%llx, %i)", FD, Ofs, Dir);
	return _Syscall(SYS_SEEK, ">i >I >i", FD, Ofs, Dir);
}

uint64_t acess_tell(int FD) {
	if(FD & NATIVE_FILE_MASK)
		return native_tell( FD & (NATIVE_FILE_MASK-1) );
	return _Syscall(SYS_TELL, ">i", FD);
}

int acess_ioctl(int fd, int id, void *data) {
	 int	len;
	DEBUG("ioctl(%i, %i, %p)", fd, id, data);
	// NOTE: The length here is hacky and could break
	if( data == NULL )
		len = 0;
	else
		len = PAGE_SIZE - ((uintptr_t)data % PAGE_SIZE);
	return _Syscall(SYS_IOCTL, ">i >i ?d", fd, id, len, data);
}
int acess_finfo(int fd, t_sysFInfo *info, int maxacls) {
//	DEBUG("offsetof(size, t_sysFInfo) = %i", offsetof(t_sysFInfo, size));
	DEBUG("finfo(%i, %p, %i)", fd, info, maxacls);
	return _Syscall(SYS_FINFO, ">i <d >i",
		fd,
		sizeof(t_sysFInfo)+maxacls*sizeof(t_sysACL), info,
		maxacls
		);
}

int acess_SysReadDir(int fd, char *dest) {
	DEBUG("SysReadDir(%i, %p)", fd, dest);
	return _Syscall(SYS_READDIR, ">i <d", fd, 256, dest);
}

int acess__SysSelect(int nfds, fd_set *read, fd_set *write, fd_set *error, time_t *timeout, uint32_t events)
{
	DEBUG("_SysSelect(%i, %p, %p, %p, %p, 0x%x)", nfds, read, write, error, timeout, events);
	return _Syscall(SYS_SELECT, ">i ?d ?d ?d >d >i", nfds,
		read ? (nfds+7)/8 : 0, read,
		write ? (nfds+7)/8 : 0, write,
		error ? (nfds+7)/8 : 0, error,
		sizeof(*timeout), timeout,
		events
		);
}

int acess_select(int nfds, fd_set *read, fd_set *write, fd_set *error, time_t *timeout)
{
	return acess__SysSelect(nfds, read, write, error, timeout, 0);
}


int acess__SysOpenChild(int fd, char *name, int flags) {
	return _Syscall(SYS_OPENCHILD, ">i >s >i", fd, name, flags);
}

int acess__SysGetACL(int fd, t_sysACL *dest) {
	return _Syscall(SYS_GETACL, ">i <d", fd, sizeof(t_sysACL), dest);
}

int acess__SysMount(const char *Device, const char *Directory, const char *Type, const char *Options) {
	return _Syscall(SYS_MOUNT, ">s >s >s >s", Device, Directory, Type, Options);
}


// --- Error Handler
int acess__SysSetFaultHandler(int (*Handler)(int)) {
	printf("TODO: Set fault handler (asked to set to %p)\n", Handler);
	return 0;
}

// --- Memory Management ---
uint64_t acess__SysAllocate(uint vaddr)
{
	if( AllocateMemory(vaddr, 0x1000) == -1 )	// Allocate a page
		return 0;
		
	return vaddr;	// Just ignore the need for paddrs :)
}

// --- Process Management ---
int acess_clone(int flags, void *stack)
{
	extern int fork(void);
	if(flags & CLONE_VM) {
		 int	ret, newID, kernel_tid=0;
		printf("USERSIDE fork()\n");
		
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
		fprintf(stderr, "ERROR: Threads currently unsupported\n");
		exit(-1);
	}
}

int acess_execve(char *path, char **argv, char **envp)
{
	 int	i, argc;
	
	DEBUG("acess_execve: (path='%s', argv=%p, envp=%p)", path, argv, envp);
	
	// Get argument count
	for( argc = 0; argv[argc]; argc ++ ) ;
	DEBUG(" acess_execve: argc = %i", argc);

	char	*new_argv[7+argc+1];
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

void acess_sleep(void)
{
	_Syscall(SYS_SLEEP, "");
}

int acess_waittid(int TID, int *ExitStatus)
{
	return _Syscall(SYS_WAITTID, ">i <d", TID, sizeof(int), &ExitStatus);
}

int acess_setuid(int ID) { return _Syscall(SYS_SETUID, ">i", ID); }
int acess_setgid(int ID) { return _Syscall(SYS_SETGID, ">i", ID); }
int acess_gettid(void) { return _Syscall(SYS_GETTID, ""); }
int acess_getpid(void) { return _Syscall(SYS_GETPID, ""); }
int acess_getuid(void) { return _Syscall(SYS_GETUID, ""); }
int acess_getgid(void) { return _Syscall(SYS_GETGID, ""); }

int acess_SysSendMessage(int DestTID, int Length, void *Data)
{
	return _Syscall(SYS_SENDMSG, ">i >d", DestTID, Length, Data);
}

int acess_SysGetMessage(int *SourceTID, void *Data)
{
//	static __thread int lastlen = 1024;
	int lastlen;

	lastlen = _Syscall(SYS_GETMSG, "<d <d",
		SourceTID ? sizeof(uint32_t) : 0, SourceTID,
		Data ? 1024 : 0, Data
		);
	return lastlen;
}

int acess__SysWaitEvent(int Mask)
{
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


// === Symbol List ===
#define DEFSYM(name)	{#name, &acess_##name}
const tSym	caBuiltinSymbols[] = {
	DEFSYM(_exit),
	
	DEFSYM(chdir),
	DEFSYM(open),
	DEFSYM(close),
	DEFSYM(reopen),
	DEFSYM(read),
	DEFSYM(write),
	DEFSYM(seek),
	DEFSYM(tell),
	DEFSYM(ioctl),
	DEFSYM(finfo),
	DEFSYM(SysReadDir),
	DEFSYM(select),
	DEFSYM(_SysOpenChild),
	DEFSYM(_SysGetACL),
	DEFSYM(_SysMount),
	DEFSYM(_SysSelect),
	
	DEFSYM(clone),
	DEFSYM(execve),
	DEFSYM(sleep),
	
	DEFSYM(waittid),
	DEFSYM(setuid),
	DEFSYM(setgid),
	DEFSYM(gettid),

	DEFSYM(SysSendMessage),
	DEFSYM(SysGetMessage),
	
	DEFSYM(_SysAllocate),
	DEFSYM(_SysDebug),
	DEFSYM(_SysSetFaultHandler),
	DEFSYM(_SysWaitEvent),
	
	DEFSYM(_errno)
};

const int	ciNumBuiltinSymbols = sizeof(caBuiltinSymbols)/sizeof(caBuiltinSymbols[0]);


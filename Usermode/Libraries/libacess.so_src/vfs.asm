;
; Acess2 System Interface
;
%include "syscalls.inc.asm"

[BITS 32]
[extern _errno]

[section .text]
SYSCALL2	open, SYS_OPEN	; char*, int
SYSCALL3	reopen, SYS_REOPEN	; int, char*, int
SYSCALL1	close, SYS_CLOSE	; int
SYSCALL4	read, SYS_READ	; int, int64_t, void*
SYSCALL4	write, SYS_WRITE	; int, int64_t, void*
SYSCALL4	seek, SYS_SEEK		; int, int64_t, int
SYSCALL1	tell, SYS_TELL		; int
SYSCALL3	finfo, SYS_FINFO	; int, void*, int
SYSCALL2	readdir, SYS_READDIR	; int, char*
SYSCALL2	_SysGetACL, SYS_GETACL	; int, void*
SYSCALL1	chdir, SYS_CHDIR	; char*
SYSCALL3	ioctl, SYS_IOCTL	; int, int, void*
SYSCALL4	_SysMount, SYS_MOUNT	; char*, char*, char*, char*

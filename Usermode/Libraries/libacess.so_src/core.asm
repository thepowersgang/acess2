;
; Acess2 System Interface
;
%include "syscalls.inc.asm"

[BITS 32]
[section .data]
[global _errno]
_errno:
	dd	0

[section .text]
[global SoMain:func]
SoMain:
	ret

; --- Process Controll ---
SYSCALL1	_exit, SYS_EXIT
SYSCALL2	clone, SYS_CLONE
SYSCALL2	kill, SYS_KILL
SYSCALL2	signal, SYS_SIGNAL
SYSCALL0	yield, SYS_YIELD
SYSCALL0	sleep, SYS_SLEEP
SYSCALL1	wait, SYS_WAIT

SYSCALL0	gettid, SYS_GETTID
SYSCALL0	getpid, SYS_GETPID
SYSCALL0	getuid, SYS_GETUID
SYSCALL0	getgid, SYS_GETGID

SYSCALL0	setuid, SYS_SETUID
SYSCALL0	setgid, SYS_SETGID

SYSCALL1	SysSetName, SYS_SETNAME
SYSCALL2	SysGetName, SYS_GETNAME

SYSCALL1	SysSetPri, SYS_SETPRI

SYSCALL3	SysSendMessage, SYS_SENDMSG
SYSCALL3	SysGetMessage, SYS_GETMSG

SYSCALL3	SysSpawn, SYS_SPAWN
SYSCALL3	execve, SYS_EXECVE
SYSCALL2	SysLoadBin, SYS_LOADBIN


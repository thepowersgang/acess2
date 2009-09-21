;
; Acess2 System Interface
;
%include "syscalls.inc.asm"

[BITS 32]
[extern _errno]

[section .text]
SYSCALL2	open, SYS_OPEN
SYSCALL3	reopen, SYS_REOPEN
SYSCALL1	close, SYS_CLOSE
SYSCALL3	read, SYS_READ
SYSCALL4	write, SYS_WRITE	; int, int64_t, void*
SYSCALL4	seek, SYS_SEEK		; int, int64_t, int
SYSCALL2	finfo, SYS_FINFO

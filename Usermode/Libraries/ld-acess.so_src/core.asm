;
; Acess2 System Interface
;
%include "syscalls.inc.asm"

[BITS 32]
[section .data]
[global _errno:data (4)]
_errno:
	dd	0

[section .text]
; DEST
; SRC
_memcpy:
	push ebp
	mov ebp, esp
	push edi
	push esi	; DI and SI must be maintained, CX doesn't
	
	mov ecx, [ebp+16]
	mov esi, [ebp+12]
	mov edi, [ebp+8]
	rep movsb
	
	pop esi
	pop edi
	pop ebp
	ret

; --- Process Control ---
SYSCALL1	_exit, SYS_EXIT

%if 0
SYSCALL2	clone, SYS_CLONE
%else
[global clone:func]
clone:
	push ebp
	mov ebp, esp
	push ebx
	
	mov ebx, [ebp+12]	; Get new stack pointer
	
	; Check if the new stack is being used
	test ebx, ebx
	jz .doCall
	; Modify it to include the calling function (and this)
	%if 0
	mov eax, [ebp]	; Get old stack frame
	sub eax, ebp	; Get size
	sub ebx, eax	; Alter new stack pointer
	push eax	; < Size
	push DWORD [ebp]	; < Source
	push ebx	; < Dest
	call _memcpy
	add esp, 4*3	; Restore stack
	; EBX should still be the new stack pointer
	mov eax, [ebp]	; Save old stack frame pointer in new stack
	mov [ebx-4], eax
	mov eax, [ebp-4]	; Save EBX there too
	mov [ebx-8], eax
	sub ebx, 8	; Update stack pointer for system
	%else
	; Quick hack, just this stack frame
	mov eax, [ebp+4]
	mov [ebx-4], eax	; Return
	mov [ebx-8], ebx	; EBP
	mov DWORD [ebx-12], 0	; EBX
	sub ebx, 12
	%endif
.doCall:
	mov eax, SYS_CLONE
	mov ecx, ebx	; Stack
	mov ebx, [ebp+8]	; Flags
	SYSCALL_OP
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret
%endif

SYSCALL2	kill, SYS_KILL
SYSCALL0	yield, SYS_YIELD
SYSCALL0	sleep, SYS_SLEEP
SYSCALL2	waittid, SYS_WAITTID

SYSCALL0	gettid, SYS_GETTID
SYSCALL0	getpid, SYS_GETPID
SYSCALL0	getuid, SYS_GETUID
SYSCALL0	getgid, SYS_GETGID

SYSCALL1	setuid, SYS_SETUID
SYSCALL1	setgid, SYS_SETGID

SYSCALL1	SysSetName, SYS_SETNAME
SYSCALL2	SysGetName, SYS_GETNAME

SYSCALL1	SysSetPri, SYS_SETPRI

SYSCALL3	SysSendMessage, SYS_SENDMSG
SYSCALL3	SysGetMessage, SYS_GETMSG

SYSCALL3	SysSpawn, SYS_SPAWN
SYSCALL3	execve, SYS_EXECVE
SYSCALL2	SysLoadBin, SYS_LOADBIN
SYSCALL1	SysUnloadBin, SYS_UNLOADBIN

SYSCALL1	_SysSetFaultHandler, SYS_SETFAULTHANDLER

SYSCALL6	_SysDebug, 0x100

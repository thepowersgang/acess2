;
; Acess2 System Interface
;
%include "syscalls.inc.asm"

[BITS 32]
[extern _errno]

[section .text]
SYSCALL0	_SysGetPhys, SYS_GETPHYS	; uint64_t _SysGetPhys(uint addr)
SYSCALL1	_SysAllocate, SYS_ALLOCATE	; uint64_t _SysAllocate(uint addr)

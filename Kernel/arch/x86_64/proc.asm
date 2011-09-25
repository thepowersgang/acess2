;
;
;
%include "arch/x86_64/include/common.inc.asm"
[BITS 64]
[section .text]

[extern Threads_Exit]

[global GetRIP]
GetRIP:
	mov rax, [rsp]
	ret

[global NewTaskHeader]
NewTaskHeader:
	mov rax, [rsp]
	mov dr0, rax
	
	sti
	mov al, 0x20
	mov dx, 0x20
	out dx, al

	mov rdi, [rsp+0x18]
	dec QWORD [rsp+0x10]
	jz .call
	mov rsi, [rsp+0x20]
	dec QWORD [rsp+0x10]
	jz .call
	mov rdx, [rsp+0x28]
	dec QWORD [rsp+0x10]
	jz .call
	mov rcx, [rsp+0x30]
	dec QWORD [rsp+0x10]
	jz .call
.call:
	mov rax, [rsp+0x8]
;	xchg bx, bx
	call rax
	
	; Quit thread with RAX as the return code
	xor rdi, rdi
	mov rsi, rax
	call Threads_Exit

.hlt:
	jmp .hlt

[global SaveState]
SaveState:
	; Save regs to RSI
	xchg bx, bx
	add rsi, 0x80
	SAVE_GPR rsi
	sub rsi, 0x80
	; Return RSI as the RSP value
	mov [rdi], rsi
	call GetRIP
	cmp eax, 0x80000000
	ja .fastret
.restore:
	; RSP = RSI from call
	xchg bx, bx
	POP_GPR
	mov rsp, [rsp-0x60]	; Restore RSP from the saved value
	xor eax, eax
	ret
.fastret:
	; RAX is still the return addr
	ret
	

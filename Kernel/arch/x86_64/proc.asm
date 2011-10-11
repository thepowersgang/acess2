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

[extern MM_Clone]
[extern MM_DumpTables]
[global Proc_CloneInt]
Proc_CloneInt:
	PUSH_GPR
	; Save RSP
	mov [rdi], rsp
	call MM_Clone
	; Save CR3
	mov rsi, [rsp+0x30]	; Saved version of RSI
	mov [rsi], rax
	; Undo the PUSH_GPR
	add rsp, 0x80
	mov rax, .newTask
	ret
.newTask:
;	mov rdi, 0
;	mov rsi, 0x800000000000
;	call MM_DumpTables
	POP_GPR
	xor eax, eax
	ret

[global SaveState]
SaveState:
	; Save regs to RSI
	add rsi, 0x80
	SAVE_GPR rsi
	; Save return addr
	mov rax, [rsp]
	mov [rsi], rax
	; Return RSI as the RSP value
	sub rsi, 0x80
	mov [rdi], rsi
	; Check for 
	mov rax, .restore
	ret
.restore:
	; RSP = RSI now
	POP_GPR
	mov rax, [rsp]
	mov rsp, [rsp-0x60]	; Restore RSP from the saved value
	mov [rsp], rax	; Restore return address
	xor eax, eax
	ret

[global SwitchTasks]
; rdi = New RSP
; rsi = Old RSP save loc
; rdx = New RIP
; rcx = Old RIP save loc
; r8 = CR3
SwitchTasks:
	PUSH_GPR
	
	; Save state RIP and RSP
	lea rax, [rel .restore]
	mov [rcx], rax
	mov [rsi], rsp

	; Change CR3 if requested
	test r8, r8
	jz .setState
	mov cr3, r8
	
	; Make sure the stack is valid before jumping
	invlpg [rdi]
	invlpg [rdi+0x1000]
	
	; Go to new state
.setState:
	mov rsp, rdi
	jmp rdx

	; Restore point for saved state
.restore:
	POP_GPR
	xor eax, eax	; Return zero
	ret

; vim: ft=nasm

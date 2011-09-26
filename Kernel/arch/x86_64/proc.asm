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
[global Proc_CloneInt]
Proc_CloneInt:
	PUSH_GPR
	; Save RSP
	mov [rdi], rsp
	call MM_Clone
	; Save CR3
	mov rsi, [rsp+0x30]
	mov [rsi], rax
	; Undo the PUSH_GPR
	add rsp, 0x80
	mov rax, .newTask
	ret
.newTask:
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
	
	lea rax, [rel .restore]
	mov QWORD [rcx], rax
	mov [rsi], rsp

	test r8, r8
	jz .setState
	mov cr3, r8
	invlpg [rdi]
	invlpg [rdi+0x1000]
.setState:
	mov rsp, rdi
	jmp rdx

.restore:
	POP_GPR
	xor eax, eax
	ret


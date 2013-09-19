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
	; [rsp+0x00]: Thread
	; [rsp+0x08]: Function
	; [rsp+0x10]: Argument

	mov rdi, [rsp+0x10]
	mov rax, [rsp+0x8]
	add rsp, 0x10	; Reclaim stack space (thread/fcn)
	;xchg bx, bx
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
	; Call MM_Clone (with bNoUserCopy flag)
	mov rdi, rdx
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
	invlpg [rdi-0x1000]
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

[global Proc_InitialiseSSE]
Proc_InitialiseSSE:
	mov rax, cr4
	or ax, (1 << 9)|(1 << 10)	; Set OSFXSR and OSXMMEXCPT
	mov cr4, rax
	mov rax, cr0
	and ax, ~(1 << 2)	; Clear EM
	or rax, (1 << 1)	; Set MP
	mov rax, cr0
	ret
[global Proc_DisableSSE]
Proc_DisableSSE:
	mov rax, cr0
	or ax, 1 << 3	; Set TS
	mov cr0, rax
	ret
[global Proc_EnableSSE]
Proc_EnableSSE:
	mov rax, cr0
	and ax, ~(1 << 3)	; Clear TS
	mov cr0, rax
	ret

[global Proc_SaveSSE]
Proc_SaveSSE:
	fxsave [rdi]
	ret
[global Proc_RestoreSSE]
Proc_RestoreSSE:
	fxrstor [rdi]
	ret

[section .usertext]

[global User_Signal_Kill]
User_Signal_Kill:
	xor rax, rax
	mov bx, di
	mov bh, 0x02
	int 0xAC
	jmp $


; vim: ft=nasm

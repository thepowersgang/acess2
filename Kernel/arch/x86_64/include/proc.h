/*
 * Acess2 x86_64 Port
 * 
 * proc.h - Process/Thread management code
 */
#ifndef _PROC_H_
#define _PROC_H_

#include <arch.h>

// Register Structure
// TODO: Rebuild once IDT code is done
typedef struct {
	Uint	rax, rcx, rdx, rbx;
    Uint	krsp, rbp, rsi, rdi;
	Uint	r8, r9, r10, r11;
	Uint	r12, r13, r14, r15;
    Uint	int_num, err_code;
    Uint	rip, cs;
	Uint	rflags, rsp, ss;
} tRegs;

/**
 * \brief Memory State for thread handler
 */
typedef struct sMemoryState
{
	tPAddr	CR3;
}	tMemoryState;

/**
 * \brief Task state for thread handler
 */
typedef struct sTaskState
{
	Uint	RIP, RSP, RBP;
}	tTaskState;

#endif


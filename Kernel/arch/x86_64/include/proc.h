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
	Uint	ds, es, fs, gs;
	Uint	r15, r14, r13, r12;
	Uint	r11, r10, r9, r8;
    Uint	rdi, rsi, rbp, krsp;
	Uint	rbx, rdx, rcx, rax;
    Uint	int_num, err_code;
    Uint	eip, cs;
	Uint	eflags, esp, ss;
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


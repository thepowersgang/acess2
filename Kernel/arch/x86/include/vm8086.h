/*
 * Acess2 VM8086 BIOS Interface
 * - By John Hodge (thePowersGang)
 *
 * vm8086.h
 * - Core Header
 */
#ifndef _VM80806_H_
#define _VM80806_H_

// === TYPES ===
typedef struct sVM8086
{
	Uint16	AX, CX, DX, BX;
	Uint16	BP, SP, SI, DI;
	
	Uint16	CS, SS, DS, ES;
	
	Uint16	IP;
}	tVM8086;

// === FUNCTIONS ===
extern tVM8086	*VM8086_Init(void);
extern void	VM8086_Free(tVM8086 *State);
extern void	*VM8086_Allocate(tVM8086 *State, int Size, Uint16 *Segment, Uint16 *Ofs);
extern void *VM8086_GetPointer(tVM8086 *State, Uint16 Segment, Uint16 Ofs);
extern void	VM8086_Int(tVM8086 *State, Uint8 Interrupt);

#endif

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
/**
 * \note Semi-opaque - Past \a .IP, the implementation may add any data
 *       it needs to the state.
 */
typedef struct sVM8086
{
	Uint16	AX, CX, DX, BX;
	Uint16	BP, SP, SI, DI;
	
	Uint16	SS, DS, ES;
	
	Uint16	CS, IP;
	
	struct sVM8086_InternalData	*Internal;
}	tVM8086;

// === FUNCTIONS ===
/**
 * \brief Create an instance of the VM8086 Emulator
 * \note Do not free this pointer with ::free, instead use ::VM8086_Free
 * \return Pointer to a tVM8086 structure, this structure may be larger than
 *         tVM8086 due to internal data.
 */
extern tVM8086	*VM8086_Init(void);
/**
 * \brief Free an allocated tVM8086 structure
 * \param State	Emulator state to free
 */
extern void	VM8086_Free(tVM8086 *State);
/**
 * \brief Allocate a piece of memory in the emulated address space and
 *        return a host and emulated pointer to it.
 * \param State	Emulator state
 * \param Size	Size of memory block
 * \param Segment	Pointer to location to store the allocated memory's segment
 * \param Offset	Pointet to location to store the allocated memory's offset
 * \return Host pointer to the allocated memory
 */
extern void	*VM8086_Allocate(tVM8086 *State, int Size, Uint16 *Segment, Uint16 *Offset);
/**
 * \brief Gets a pointer to a piece of emulated memory
 * \todo Only 1 machine page is garenteed to be contiguous
 * \param State	Emulator State
 * \param Segment	Source Segment
 * \param Offset	Source Offset
 * \return Host pointer to the emulated memory
 */
extern void *VM8086_GetPointer(tVM8086 *State, Uint16 Segment, Uint16 Ofs);
/**
 * \brief Calls a real-mode interrupt described by the current state of the IVT.
 * \param State	Emulator State
 * \param Interrupt	BIOS Interrupt to call
 */
extern void	VM8086_Int(tVM8086 *State, Uint8 Interrupt);

#endif

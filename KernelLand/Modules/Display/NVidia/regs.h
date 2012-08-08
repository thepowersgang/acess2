/*
 * Acess2 NVidia Graphics Driver
 * - By John Hodge (thePowersGang)
 * 
 * regs.h
 * - Register definitions
 */
#ifndef _NVIDIA__REGS_H_
#define _NVIDIA__REGS_H_

// CRT Controller Registers
enum eNVRegs_CRTC
{
	NUM_CRTC_REGS
};

// Attribute Controller registers
enum eNVRegs_ATC
{
	NUM_ATC_REGS
};

enum eNVRegs_GRC
{
	NUM_GRC_REGS
};

// Sequencer registers
enum eNVRegs_SEQ
{
	NUM_SEQ_REGS
};

struct sNVRegDump
{
	Uint8	atc_regs[NUM_ATC_REGS];
	Uint8	crtc_regs[NUM_CRTC_REGS];
	Uint8	gra_regs[NUM_GRC_REGS];
	Uint8	seq_regs[NUM_SEQ_REGS];
};

#endif


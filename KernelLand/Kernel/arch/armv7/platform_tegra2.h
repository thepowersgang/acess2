/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * platform_tegra2.c
 * - Tegra2 Core code
 */
#ifndef _PLATFORM__TEGRA2_H_
#define _PLATFORM__TEGRA2_H_

struct sTimerRegs
{
	Uint32	PTV_0;	// Control / Target value
	Uint32	PCR_0;	// Current value / IRQ clear
};
struct sTimersMap
{
	struct sTimerRegs	TMR0;
	struct sTimerRegs	TMR1;
	// TMRUS
	char _padding[ 0x50-0x10 ];

	struct sTimerRegs	TMR2;
	struct sTimerRegs	TMR3;
};

#if 0
struct sIRQRegs
{
	Uint32	VIRQ_CPU;
	Uint32	VIRQ_COP;
	Uint32	VFIQ_CPU;
	Uint32	VFIQ_COP;
	Uint32	ISR;
	Uint32	FIR;	// Force interrupt status
	Uint32	FIR_SET;	// Set bit in FIR
	Uint32	FIR_CLR;	// Clear bit in FIR
	Uint32	CPU_IER;	// RO - Interrupt Enable register
	Uint32	CPU_IER_SET;
	Uint32	CPU_IER_CLR;
	Uint32	CPU_IEP;	// 1 = FIQ
	Uint32	COP_IER;	// RO - Interrupt Enable register
	Uint32	COP_IER_SET;
	Uint32	COP_IER_CLR;
	Uint32	COP_IEP;	// 1 = FIQ
};
struct sArbGntRegs
{
	Uint32	CPU_Status;
	Uint32	CPU_Enable;
	Uint32	COP_Status;
	Uint32	COP_Enable;
};
struct sIRQMap
{
	struct sIRQRegs	Pri;
	struct sArbGntRegs	Arb;
	char	_pad1[0x100-sizeof(struct sIRQRegs)-sizeof(struct sIRQRegs)];
	struct sIRQRegs	Sec;
	char	_pad2[0x100-sizeof(struct sIRQRegs)];
	struct sIRQRegs	Tri;
	char	_pad3[0x100-sizeof(struct sIRQRegs)];
	struct sIRQRegs	Quad;
};
#endif

#endif


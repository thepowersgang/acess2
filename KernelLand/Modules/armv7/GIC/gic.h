/*
 * ARMv7 GIC Support
 * - By John Hodge (thePowersGang)
 * 
 * gic.h
 * - GIC Core Definitions
 */
#ifndef _ARM7_GIC_H_
#define _ARM7_GIC_H_

enum eGICD_Registers
{
	GICD_CTLR       = 0x000/4,	// Distributor Control Register
	GICD_TYPER      = 0x004/4,	// Interrupt Controller Type
	GICD_IIDR       = 0x008/4,	// Distributor Implementer Identifcation
	
	GICD_IGROUPR0   = 0x080/4,	// Interrupt Group Register (#0)
	GICD_ISENABLER0 = 0x100/4,	// Interrupt Set-Enable Register #0 (128*8=1024)
	GICD_ICENABLER0 = 0x180/4,	// Interrupt Clear-Enable Register #0
	GICD_ISPENDR0   = 0x200/4,	// Interrupt Set-Pending Register #0
	GICD_ICPENDR0   = 0x280/4,	// Interrupt Clear-Pending Register #0
	GICD_ISACTIVER0 = 0x300/4,	// Interrupt Set-Active Register (GICv2)
	GICD_ICACTIVER0 = 0x380/4,	// Interrupt Clear-Active Register (GICv2)
	
	GICD_IPRIORITYR0 = 0x400/4,	// Interrupt priority registers (254*4 = )
	
	GICD_ITARGETSR0 = 0x800/4,	// Interrupt Processor Targets Register (8*4)

	GICD_ICFGR0     = 0xC00/4,	// Interrupt Configuration Register (64*4)
	GICD_NSACR0     = 0xE00/4,	// Non-secure Access Control Register (64*4)
	GICD_SIGR       = 0xF00/4,	// Software Generated Interrupt Register (Write Only)
	GICD_CPENDSGIR0 = 0xF10/4,	// SGI Clear-Pending Registers (4*4)
	GICD_SPENDSGIR0 = 0xF20/4,	// SGI Set-Pending Registers (4*4)
};

enum eGICC_Registers
{
	GICC_CTLR   = 0x000/4,	// CPU Interface Control Register
	GICC_PMR    = 0x004/4,	// Interrupt Priority Mask Register
	GICC_BPR    = 0x008/4,	// Binary Point Register
	GICC_IAR    = 0x00C/4,	// Interrupt Acknowledge Register
	GICC_EOIR   = 0x010/4,	// End of Interrupt Register
	GICC_RPR    = 0x014/4,	// Running Priority Register
	GICC_HPPIR  = 0x018/4,	// Highest Priority Pending Interrupt Register
	GICC_ABPR   = 0x01C/4,	// Aliased Binary Point Register
	GICC_AIAR   = 0x020/4,	// Aliased Interrupt Acknowledge Register,
	GICC_AEOIR  = 0x024/4,	// Aliased End of Interrupt Register
	GICC_AHPPIR = 0x028/4,	// Aliased Highest Priority Pending Interrupt Register

	GICC_APR0   = 0x0D0/4,	// Active Priorities Registers (4*4)
	GICC_NSAPR0 = 0x0E0/4,	// Non-secure Active Priorities Registers (4*4)
	
	GICC_IIDR   = 0x0FC/4,	// CPU Interface Identifcation Register
	GICC_DIR    = 0x0FC/4,	// Deactivate Interrupt Register (Write Only)
};

#endif

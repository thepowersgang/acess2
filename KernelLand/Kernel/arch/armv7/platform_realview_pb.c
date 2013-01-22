/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * platform_realviewpb.c
 * - RealviewPB core code
 */
#include <acess.h>

// === PROTOTYPES ===
void	Time_Setup(void);

// === GLOBALS ===
tPAddr	gGIC_DistributorAddr = 0x1e001000;
tPAddr	gGIC_InterfaceAddr = 0x1e000000;

// === CODE ===
void Time_Setup(void)
{
	// TODO: 
}

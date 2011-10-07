/*
 * ARMv7 GIC Support
 * - By John Hodge (thePowersGang)
 * 
 * gic.c
 * - GIC Core
 */
#include <acess.h>
#include <modules.h>

// === PROTOTYPES ===
 int	GIC_Install(char **Arguments);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, armv7_GIC, GIC_Install, NULL, NULL);

// === CODE ===
int GIC_Install(char **Arguments)
{
	return MODULE_ERR_OK;
}

int IRQ_AddHandler(int IRQ, void (*Handler)(int, void*), void *Ptr)
{
	Log_Warning("GIC", "TODO: Implement IRQ_AddHandler");
	return 0;
}


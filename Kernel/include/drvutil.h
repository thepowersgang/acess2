/*
 * Acess2
 * Common Driver/Filesystem Helper Functions
 */

#ifndef _DRVUTIL_H_
#define _DRVUTIL_H_

#include <common.h>

// === TYPES ===
typedef Uint	(*tDrvUtil_Callback)(Uint64 Address, Uint Count, void *Buffer, Uint Argument);

// === PROTOTYPES ===
// --- Block IO Helpers ---
extern Uint64 DrvUtil_ReadBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, Uint64 BlockSize, Uint Argument);

extern Uint64 DrvUtil_WriteBlock(Uint64 Start, Uint64 Length, void *Buffer,
	tDrvUtil_Callback ReadBlocks, tDrvUtil_Callback WriteBlocks,
	Uint64 BlockSize, Uint Argument);

#endif

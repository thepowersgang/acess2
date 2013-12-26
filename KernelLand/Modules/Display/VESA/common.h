/**
 * Acess2 Kernel VBE Driver
 * - By John Hodge (thePowersGang)
 *
 * common.h
 * - Driver-internal definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include "vbe.h"

/**
 * \name Mode Flags
 * \{
 */
#define	FLAG_LFB	0x01	//!< Framebuffer is a linear framebufer
#define FLAG_POPULATED	0x02	//!< Mode information is populated
#define FLAG_VALID	0x04	//!< Mode is valid (usable)
#define FLAG_GRAPHICS	0x08	//!< Graphics mode
/**
 * \}
 */

// === TYPES ===
typedef struct sVesa_Mode
{
	Uint16	code;
	Uint16	width, height;
	Uint16	pitch, bpp;
	Uint16	flags;
	Uint32	fbSize;
	Uint32	framebuffer;
}	tVesa_Mode;
#endif

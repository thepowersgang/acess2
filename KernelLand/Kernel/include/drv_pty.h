/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv_pty.h
 * - Pseudo Terminals
 */
#ifndef _KERNEL_DRV_PTY_H_
#define _KERNEL_DRV_PTY_H_

// Userland
#include <acess/devices/pty.h>

typedef struct sPTY	tPTY;
typedef void	(*tPTY_OutputFcn)(void *Handle, size_t Length, const void *Data);
typedef int	(*tPTY_ReqResize)(void *Handle, const struct ptydims *Dims);
typedef int	(*tPTY_ModeSet)(void *Handle, const struct ptymode *Mode);

extern tPTY	*PTY_Create(const char *Name, void *Handle, tPTY_OutputFcn OutputFcn, tPTY_ReqResize ReqResize, tPTY_ModeSet ModeSet, const struct ptydims *InitialDims, const struct ptymode *InitialMode);
extern int	PTY_SetAttrib(tPTY *PTY, const struct ptydims *Dims, const struct ptymode *Mode, int WasClient);
extern void	PTY_Close(tPTY *PTY);
extern size_t	PTY_SendInput(tPTY *PTY, const char *InputString, size_t InputLength);

#endif


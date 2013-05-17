/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv_pty.h
 * - Pseudo Terminals
 */
#ifndef _KERNEL_DRV_PTY_H_
#define _KERNEL_DRV_PTY_H_

#include "../../../Usermode/Libraries/ld-acess.so_src/include_exp/acess/devices/pty.h"

typedef struct sPTY	tPTY;
typedef void	(*tPTY_OutputFcn)(void *Handle, const void *Data, size_t Length, unsigned int OutMode);
typedef int	(*tPTY_ReqResize)(void *Handle, const struct ptydims *Dims);

extern tPTY	*PTY_Create(const char *Name, void *Handle, tPTY_OutputFcn OutputFcn, tPTY_ReqResize ReqResize);
extern int	PTY_SetAttrib(tPTY *PTY, const struct ptydims *Dims, const struct ptymode *Mode, int WasClient);
extern void	PTY_Close(tPTY *PTY);
extern size_t	PTY_SendInput(tPTY *PTY, const char *InputString, size_t InputLength);

#endif


/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/lowlevel.h
 * - Definitions for system-dependent code
 */
#ifndef _LOWLEVEL_H_
#define _LOWLEVEL_H_

#include <acess/sys.h>

// === GLOBALS ===
extern int 	giTerminalFD;
extern const char	*gsTerminalDevice;

// === FUNCTIONS ===
// --- Input ---
extern int	Input_Init(void);
extern void	Input_FillSelect(int *nfds, fd_set *set);
extern void	Input_HandleSelect(fd_set *set);
// --- IPC ---
extern void	IPC_Init(void);
extern void	IPC_FillSelect(int *nfds, fd_set *set);
extern void	IPC_HandleSelect(fd_set *set);

#endif


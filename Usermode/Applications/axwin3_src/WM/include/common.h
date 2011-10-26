/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/common.h
 * - Common definitions and functions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <acess/sys.h>

// === FUNCTIONS ===
// --- Input ---
 int	Input_Init(void);
void	Input_FillSelect(int *nfds, fd_set *set);
void	Input_HandleSelect(fd_set *set);
// --- IPC ---
 int	IPC_Init(void);
void	IPC_FillSelect(int *nfds, fd_set *set);
void	IPC_HandleSelect(fd_set *set);

#endif


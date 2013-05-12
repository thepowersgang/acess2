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

#define TODO(str)	

#define ASSERT(expr)	do{if(!(expr)){_SysDebug("%s:%i - ASSERTION FAILED: "#expr, __FILE__, __LINE__);exit(-1);}}while(0)

#define UNIMPLEMENTED()	do{_SysDebug("TODO: Implement %s", __func__); for(;;);}while(0)

#define	AXWIN_VERSION	0x300

static inline int MIN(int a, int b)	{ return (a < b) ? a : b; }
static inline int MAX(int a, int b)	{ return (a > b) ? a : b; }

// === GLOBALS ===
extern int 	giTerminalFD;
extern const char	*gsTerminalDevice;

extern int	giScreenWidth, giScreenHeight;

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


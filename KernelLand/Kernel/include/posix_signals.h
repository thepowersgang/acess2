/*
 * Acess2 Kernel
 * Signal List
 */
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#define SIG_DFL	NULL

#include "../../../Usermode/Libraries/libc.so_src/include_exp/signal_list.h"

extern void	Threads_PostSignal(int SigNum);
extern void	Threads_SignalGroup(tPGID PGID, int SignalNum);

#endif

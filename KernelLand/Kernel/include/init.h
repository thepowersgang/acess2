/*
 * AcessOS Microkernel Version
 * init.h
 */
#ifndef _INIT_H
#define _INIT_H

extern void	Arch_LoadBootModules(void);
extern void	StartupPrint(const char *String);
extern void	System_Init(char *Commandline);
extern void	Threads_Init(void);
extern void	Heap_Install(void);

#endif

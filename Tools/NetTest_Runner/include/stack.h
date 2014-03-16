/*
 * stack.h
 * - Functions for interacting with the IPStack program
 */
#ifndef _STACK_H_
#define _STACK_H_

#include <stdint.h>

extern void	Stack_AddDevice(const char *Ident, const void *MacAddr);
extern void	Stack_AddInterface(const char *Name, int AddrType, const void *Addr, int MaskBits);
extern void	Stack_AddRoute(int Type, const void *Network, int MaskBits, const void *NextHop);

extern void	Stack_AddArg(const char *Fmt, ...);
extern int	Stack_Start(const char *Subcommand);
extern void	Stack_Kill(void);

extern int	Stack_SendCommand(const char *CommandString);

#endif


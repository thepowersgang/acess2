/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include "pseudo_curses.h"

typedef struct sServer	tServer;

extern void	_SysDebug(const char *format, ...);

extern int	writef(int FD, const char *Format, ...);
extern int	OpenTCP(const char *AddressString, short PortNumber);
extern char	*GetValue(char *Src, int *Ofs);

extern void	Redraw_Screen(void);
extern void	Exit(const char *Reason) __attribute__((noreturn));

#endif


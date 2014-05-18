/*
 */
#ifndef _INPUT_H_
#define _INPUT_H_

#include <acess/sys.h>

extern void	Input_FillSelect(int *nfds, fd_set *rfds);
extern void	Input_HandleSelect(int nfds, const fd_set *rfds);

#endif


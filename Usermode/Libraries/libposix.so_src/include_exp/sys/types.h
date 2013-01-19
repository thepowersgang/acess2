/*
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stdint.h>

//typedef signed int	ssize_t;
//#ifdef  __USE_BSD
typedef unsigned int	u_int;
//#endif

typedef struct stat	t_fstat;


typedef unsigned int	id_t;
typedef unsigned long	pid_t;
typedef unsigned long	tid_t;
typedef signed long long int	time_t;
typedef long long int	off_t;

typedef unsigned int	uint;

#include "stat.h"
#include <acess/fd_set.h>

#endif

/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * utime.h
 * - 
 */
#ifndef _LIBPOSIX__UTIME_H_
#define _LIBPOSIX__UTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>	// time_t

struct utimbuf {
	time_t	actime; 	// access time
	time_t	modtime;	// modifcation time
};

extern int	utime(const char *filename, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif

#endif


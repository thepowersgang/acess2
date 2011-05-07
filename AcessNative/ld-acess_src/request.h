/*
 * Acess2 - AcessNative
 * ld-acess
 *
 * request.h - IPC Request common header
 */

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "../syscalls.h"

extern int	SendRequest(tRequestHeader *Request, int RequestSize, int ResponseSize);

#endif

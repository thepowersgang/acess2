/*
 * Acess2 DiskTool
 * - By John Hodge (thePowersGang)
 *
 * include/modules.h
 * - Reimplimentation of kernel module interface for POSIX userland
 */
#ifndef _INCLUDE__MODULES_H_
#define _INCLUDE__MODULES_H_

enum
{
	MODULE_ERR_OK,
};

#define MODULE_DEFINE(flags, version, name, init, deinit, deps...) \
void __init_##init(void) __attribute__((constructor(200))); void __init_##init(void){init(NULL);}

#endif


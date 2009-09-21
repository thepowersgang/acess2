/*
 * AcessOS Microkernel Version
 * init.h
 */
#ifndef _INIT_H
#define _INIT_H

#define INIT_SRV_MAGIC	(0xE6|('S'<<8)|('R'<<16)|('V'<<24))

typedef struct sInitServ {
	Uint32	Magic;
	Uint32	LoadBase;
	Uint32	MemSize;
	 int	(*Entrypoint)(char **Args);
} tInitServ;

#endif

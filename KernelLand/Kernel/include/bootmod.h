/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * include/bootmod.h
 * - Common boot modules type
 */
#ifndef _BOOTMOD_H_
#define _BOOTMOD_H_

typedef struct sBootModule	tBootModule;

struct sBootModule {
	tPAddr	PBase;
	void	*Base;
	Uint	Size;
	char	*ArgString;
};

#endif


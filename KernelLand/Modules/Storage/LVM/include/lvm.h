/*
 * Acess2 Logical Volume Manager
 * - By John Hodge (thePowersGang)
 *
 * lvm.h
 * - LVM Exports
 */
#ifndef _LVM__LVM_H_
#define _LVM__LVM_H_

#include <acess.h>

typedef struct sLVM_VolType	tLVM_VolType;

struct sLVM_VolType
{
	const char *Name;

	int	(*Read)(void *, Uint64, size_t, void *);
	int	(*Write)(void *, Uint64, size_t, const void *);
};


extern int	LVM_AddVolume(const tLVM_VolType *Type, const char *Name, void *Ptr, size_t BlockSize, size_t BlockCount);

#endif


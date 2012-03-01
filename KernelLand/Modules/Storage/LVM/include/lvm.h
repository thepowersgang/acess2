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

typedef int (*tLVM_ReadFcn)(void *, Uint64, size_t, void *);
typedef int (*tLVM_WriteFcn)(void *, Uint64, size_t, const void *);

extern int	LVM_AddVolume(const char *Name, void *Ptr, tLVM_ReadFcn Read, tLVM_WriteFcn Write);

#endif


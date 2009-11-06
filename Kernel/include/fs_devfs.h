/*
 * Acess 2
 * Device Filesystem (DevFS)
 * - vfs/fs/devfs.c
 */
/**
 * \file fs_devfs.h
 * \brief Acess Device Filesystem interface
 */
#ifndef _FS_DEVFS_H
#define _FS_DEVFS_H
#include <vfs.h>

// === TYPES ===
/**
 * \brief DevFS Driver
 */
typedef struct sDevFS_Driver {
	struct sDevFS_Driver	*Next;	//!< Set to NULL by drivers (used internally)
	char	*Name;	//!< Name of the driver file/folder
	tVFS_Node	RootNode;	//!< Root node of driver
} tDevFS_Driver;

// === FUNCTIONS ===
/**
 * \fn int DevFS_AddDevice(tDevFS_Driver *Dev)
 * \brief Registers a device in the Device filesystem
 * \param Dev	Pointer to a persistant structure that represents the driver
 */
extern int	DevFS_AddDevice(tDevFS_Driver *Dev);

#endif

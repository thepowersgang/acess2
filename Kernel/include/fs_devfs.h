/**
 * \file fs_devfs.h
 * \brief Acess Device Filesystem interface
 * \author John Hodge (thePowersGang)
 */
#ifndef _FS_DEVFS_H
#define _FS_DEVFS_H
#include <vfs.h>

// === TYPES ===
/**
 * \brief DevFS driver definition
 */
typedef struct sDevFS_Driver
{
	struct sDevFS_Driver	*Next;	//!< Set to NULL by drivers (used internally)
	const char	*Name;	//!< Name of the driver file/folder (must be unique)
	tVFS_Node	RootNode;	//!< Root node of driver
} tDevFS_Driver;

// === FUNCTIONS ===
/**
 * \fn int DevFS_AddDevice(tDevFS_Driver *Device)
 * \brief Registers a device in the Device Filesystem
 * \param Device	Pointer to a persistant structure that represents the driver
 * \return Boolean success
 */
extern int	DevFS_AddDevice(tDevFS_Driver *Device);

/**
 * \brief Unregisters a device with the Device Filesystem
 */
extern void	DevFS_DelDevice(tDevFS_Driver *Device);

#endif

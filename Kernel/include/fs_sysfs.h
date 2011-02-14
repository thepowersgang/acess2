/*
 * Acess2
 * - SysFS Export Header
 */
/**
 * \file fs_sysfs.h
 * \brief ProcDev/SysFS Interface
 * \author John Hodge (thePowersGang)
 * 
 * 
 */
#ifndef _FS_SYSFS_H_
#define _FS_SYSFS_H_

/**
 * \brief Registers a file in the SysFS tree
 * \param Path	Path relative to the SysFS root (no .. or .)
 * \param Data	File buffer address
 * \param Length	Length of the file buffer
 * \return An ID number to refer to the file, or -1 on error
 * \note \a Data must be maintained until ::SysFS_UpdateFile is called
 *       with a different buffer, or ::SysFS_RemoveFile is called.
 */
extern int	SysFS_RegisterFile(const char *Path, const char *Data, int Length);

/**
 * \brief Updates the size/pointer associated with a SysFD file
 * \param ID	Number returned by ::SysFS_Register
 * \param Data	New buffer address
 * \param Length	New length of the file
 * \return Boolean Success
 */
extern int	SysFS_UpdateFile(int ID, const char *Data, int Length);

/**
 * \brief Removes a file from the SysFS tree
 * \param ID	Number returned by ::SysFS_Register
 */
extern int	SysFS_RemoveFile(int ID);

#endif

/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * include/vfs_threads.h
 * - Handle maintainance functions for the VFS used by threading code
 */
#ifndef _VFS_THREADS_H_
#define _VFS_THREADS_H_

// === FUNCTIONS ===
extern void	VFS_ReferenceUserHandles(void);
extern void	VFS_CloseAllUserHandles(struct sProcess *Process);

extern void	*VFS_SaveHandles(int NumFDs, int *FDs);
extern void	VFS_RestoreHandles(int NumFDs, void *Handles);
extern void	VFS_FreeSavedHandles(int NumFDs, void *Handles);

#endif

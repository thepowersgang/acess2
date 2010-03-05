/*
 * Acess2 - NFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * common.h - Common definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct sNFS_Connection
{
	 int	FD;
	tIPAddr	Host;
	char	*Base;
	tVFS_Node	Node;
}	tNFS_Connection;

#endif

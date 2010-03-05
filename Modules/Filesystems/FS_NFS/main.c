/*
 * Acess2 - NFS Driver
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess licence. See the
 * file COPYING for details.
 *
 * main.c - Driver core
 */
#define DEBUG	1
#define VERBOSE	0
#include "common.h"
#include <modules.h>

// === PROTOTYPES ===
 int	NFS_Install(char **Arguments);
tVFS_Node	*NFS_InitDevice(char *Devices, char **Options);
void	NFS_Unmount(tVFS_Node *Node);

// === GLOBALS ===
MODULE_DEFINE(0, 0x32 /*v0.5*/, FS_NFS, NFS_Install, NULL);
tVFS_Driver	gNFS_FSInfo = {"nfs", 0, NFS_InitDevice, NFS_Unmount, NULL};

tNFS_Connection	*gpNFS_Connections;

// === CODE ===
/**
 * \brief Installs the NFS driver
 */
int NFS_Install(char **Arguments)
{
	VFS_AddDriver( &gNFS_FSInfo );
	return 1;
}

/**
 * \brief Mount a NFS share
 */
tVFS_Node *NFS_InitDevice(char *Device, char **Options)
{
	char	*path, *host;
	tNFS_Connection	*conn;
	
	path = strchr( Device, ':' ) + 1;
	host = strndup( Device, (int)(path-Device)-1 );
	
	conn = malloc( sizeof(tNFS_Connection) );
	
	if( !IPTools_GetAddress(host, &conn->IP) ) {
		free(conn);
		return NULL;
	}
	free(host);
	
	conn->FD = IPTools_OpenUdpClient( &conn->Host );
	if(conn->FD == -1) {
		free(conn);
		return NULL;
	}
	
	conn->Base = strdup( path );
	conn->RootNode.ImplPtr = conn;
	conn->RootNode.Flags = VFS_FFLAG_DIRECTORY;
	
	conn->RootNode.ReadDir = NFS_ReadDir;
	conn->RootNode.FindDir = NFS_FindDir;
	conn->RootNode.Close = NULL;
	
	return &conn->RootNode;
}

void NFS_Unmount(tVFS_Node *Node)
{
	
}

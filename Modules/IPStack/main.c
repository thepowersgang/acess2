/*
 * Acess2 IP Stack
 * - Address Resolution Protocol
 */
#include "ipstack.h"
#include <modules.h>
#include <fs_devfs.h>

// === IMPORTS ===
 int	ARP_Initialise();

// === PROTOTYPES ===
 int	IPStack_Install(char **Arguments);
char	*IPStack_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*IPStack_FindDir(tVFS_Node *Node, char *Name);

// === GLOBALS ===
MODULE_DEFINE(0, 0x000A, IPStack, IPStack_Install, NULL);
tDevFS_Driver	gIP_DriverInfo = {
	NULL, "ip",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = IPStack_ReadDir,
	.FindDir = IPStack_FindDir
	}
};
 int	giIP_NumInterfaces;
tInterface	*gIP_Interfaces = NULL;

// === CODE ===
/**
 * \fn int IPStack_Install(char **Arguments)
 * \brief Intialise the relevant parts of the stack and register with DevFS
 */
int IPStack_Install(char **Arguments)
{
	 int	i = 0;
	
	// Install Handlers
	ARP_Initialise();
	
	// Parse module arguments
	for( i = 0; Arguments[i]; i++ )
	{
		//if(strcmp(Arguments[i], "Device") == '=') {
		//	
		//}
	}
	
	return 1;
}

/**
 * \brief Read from the IP Stack's Device Directory
 */
char *IPStack_ReadDir(tVFS_Node *Node, int Pos)
{
	tInterface	*iface;
	char	name[5] = "ip0\0\0";
	
	// Create the name
	if(Pos < 10)
		name[2] = '0' + Pos;
	else {
		name[2] = '0' + Pos/10;
		name[3] = '0' + Pos%10;
	}
	
	// Traverse the list
	for( iface = gIP_Interfaces; iface && Pos--; iface = iface->Next ) ;
	
	// Did we run off the end?
	if(!iface)	return NULL;
	
	// Return the pre-generated name
	return strdup(name);
}

/**
 * \brief Get the node of an interface
 */
tVFS_Node *IPStack_FindDir(tVFS_Node *Node, char *Name)
{
	return NULL;
}

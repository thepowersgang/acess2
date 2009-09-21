/* 
 * Acess Micro VFS
 */
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"

// === GLOBALS ===
tVFS_ACL	gVFS_ACL_EveryoneRWX = { {0,-1}, {0,VFS_PERM_ALL} };
tVFS_ACL	gVFS_ACL_EveryoneRW = { {0,-1}, {0,VFS_PERM_ALL^VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRX = { {0,-1}, {0,VFS_PERM_READ|VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRO = { {0,-1}, {0,VFS_PERM_READ} };

// === CODE ===
/**
 * \fn int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
 * \brief Checks the permissions on a file
 */
int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
{
	 int	i;
	 int	uid = Proc_GetUID();
	 int	gid = Proc_GetGID();
	
	// Root can do anything
	if(uid == 0)	return 1;
	
	// Root only file?, fast return
	if( Node->NumACLs == 0 )	return 0;
	
	// Check Deny Permissions
	for(i=0;i<Node->NumACLs;i++)
	{
		if(!Node->ACLs[i].Inv)	continue;	// Ignore ALLOWs
		if(Node->ACLs[i].ID != -1)
		{
			if(!Node->ACLs[i].Group && Node->ACLs[i].ID != uid)	continue;
			if(Node->ACLs[i].Group && Node->ACLs[i].ID != gid)	continue;
		}
		
		if(Node->ACLs[i].Perms & Permissions)	return 0;
	}
	
	// Check for allow permissions
	for(i=0;i<Node->NumACLs;i++)
	{
		if(Node->ACLs[i].Inv)	continue;	// Ignore DENYs
		if(Node->ACLs[i].ID != -1)
		{
			if(!Node->ACLs[i].Group && Node->ACLs[i].ID != uid)	continue;
			if(Node->ACLs[i].Group && Node->ACLs[i].ID != gid)	continue;
		}
		
		if((Node->ACLs[i].Perms & Permissions) == Permissions)	return 1;
	}
	
	return 0;
}

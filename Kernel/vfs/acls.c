/* 
 * Acess Micro VFS
 */
#include <common.h>
#include "vfs.h"
#include "vfs_int.h"

// === GLOBALS ===
tVFS_ACL	gVFS_ACL_EveryoneRWX = { {1,-1}, {0,VFS_PERM_ALL} };
tVFS_ACL	gVFS_ACL_EveryoneRW = { {1,-1}, {0,VFS_PERM_ALL^VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRX = { {1,-1}, {0,VFS_PERM_READ|VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRO = { {1,-1}, {0,VFS_PERM_READ} };

// === CODE ===
/**
 * \fn int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
 * \brief Checks the permissions on a file
 */
int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
{
	 int	i;
	 int	uid = Threads_GetUID();
	 int	gid = Threads_GetGID();
	
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
/**
 * \fn int VFS_GetACL(int FD, tVFS_ACL *Dest)
 */
int VFS_GetACL(int FD, tVFS_ACL *Dest)
{
	 int	i;
	tVFS_Handle	*h = VFS_GetHandle(FD);
	
	// Error check
	if(!h)	return -1;
	
	// Root can do anything
	if(Dest->Group == 0 && Dest->ID == 0) {
		Dest->Inv = 0;
		Dest->Perms = -1;
		return 1;
	}
	
	// Root only file?, fast return
	if( h->Node->NumACLs == 0 ) {
		Dest->Inv = 0;
		Dest->Perms = 0;
		return 0;
	}
	
	// Check Deny Permissions
	for(i=0;i<h->Node->NumACLs;i++)
	{
		if(h->Node->ACLs[i].Group != Dest->Group)	continue;
		if(h->Node->ACLs[i].ID != Dest->ID)	continue;
		
		Dest->Inv = h->Node->ACLs[i].Inv;
		Dest->Perms = h->Node->ACLs[i].Perms;
		return 1;
	}
	
	
	Dest->Inv = 0;
	Dest->Perms = 0;
	return 0;
}

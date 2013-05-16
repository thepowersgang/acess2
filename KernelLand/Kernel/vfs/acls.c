/* 
 * Acess Micro VFS
 */
#include <acess.h>
#include "vfs.h"
#include "vfs_int.h"

// === PROTOTYPES ===
Uint	VFS_int_CheckACLs(tVFS_ACL *ACLs, int Num, int bDeny, Uint Perms, tUID UID, tGID GID);

// === GLOBALS ===
tVFS_ACL	gVFS_ACL_EveryoneRWX = { {1,-1}, {0,VFS_PERM_ALL} };
tVFS_ACL	gVFS_ACL_EveryoneRW = { {1,-1}, {0,VFS_PERM_ALL^VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRX = { {1,-1}, {0,VFS_PERM_READ|VFS_PERM_EXECUTE} };
tVFS_ACL	gVFS_ACL_EveryoneRO = { {1,-1}, {0,VFS_PERM_READ} };

// === CODE ===
Uint VFS_int_CheckACLs(tVFS_ACL *ACLs, int Num, int bDeny, Uint Perms, tUID UID, tGID GID)
{
	 int	i;
	for(i = 0; i < Num; i ++ )
	{
		if(ACLs[i].Perm.Inv)
			continue;	// Ignore ALLOWs
		// Check if the entry applies to this case
		if(ACLs[i].Ent.ID != 0x7FFFFFFF)
		{
			if(!ACLs[i].Ent.Group) {
				if(ACLs[i].Ent.ID != UID)	continue;
			}
			else {
				if(ACLs[i].Ent.ID != GID)	continue;
			}
		}
		
		//Log("Deny %x", Node->ACLs[i].Perms);
		
		if(bDeny && (ACLs[i].Perm.Perms & Perms) != 0 )
		{
			return ACLs[i].Perm.Perms & Perms;
		}
		if(!bDeny && (ACLs[i].Perm.Perms & Perms) == Perms)
		{
			return 0;	//(~ACLs[i].Perm.Perms) & Perms;
		}
	}
	return bDeny ? 0 : Perms;
}

/**
 * \fn int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
 * \brief Checks the permissions on a file
 */
int VFS_CheckACL(tVFS_Node *Node, Uint Permissions)
{
	Uint	rv;
	 int	uid = Threads_GetUID();
	 int	gid = Threads_GetGID();
	
	// Root can do anything
	if(uid == 0)	return 1;
	
	// Root only file?, fast return
	if( Node->NumACLs == 0 ) {
		Log("VFS_CheckACL - %p inaccesable, NumACLs = 0, uid=%i", Node, uid);
		return 0;
	}
	
	// Check Deny Permissions
	rv = VFS_int_CheckACLs(Node->ACLs, Node->NumACLs, 1, Permissions, uid, gid);
	if( !rv )
		Log("VFS_CheckACL - %p inaccesable, %x denied", Node, rv);
		return 0;
	rv = VFS_int_CheckACLs(Node->ACLs, Node->NumACLs, 0, Permissions, uid, gid);
	if( !rv ) {
		Log("VFS_CheckACL - %p inaccesable, %x not allowed", Node, rv);
		return 0;
	}

	return 1;
}
/**
 * \fn int VFS_GetACL(int FD, tVFS_ACL *Dest)
 */
int VFS_GetACL(int FD, tVFS_ACL *Dest)
{
	 int	i;
	tVFS_Handle	*h = VFS_GetHandle(FD);
	
	// Error check
	if(!h) {
		return -1;
	}
	
	// Root can do anything
	if(Dest->Ent.Group == 0 && Dest->Ent.ID == 0) {
		Dest->Perm.Inv = 0;
		Dest->Perm.Perms = -1;
		return 1;
	}
	
	// Root only file?, fast return
	if( h->Node->NumACLs == 0 ) {
		Dest->Perm.Inv = 0;
		Dest->Perm.Perms = 0;
		return 0;
	}
	
	// Check Deny Permissions
	for(i=0;i<h->Node->NumACLs;i++)
	{
		if(h->Node->ACLs[i].Ent.Group != Dest->Ent.Group)	continue;
		if(h->Node->ACLs[i].Ent.ID != Dest->Ent.ID)	continue;
		
		Dest->Perm.Inv = h->Node->ACLs[i].Perm.Inv;
		Dest->Perm.Perms = h->Node->ACLs[i].Perm.Perms;
		return 1;
	}
	
	
	Dest->Perm.Inv = 0;
	Dest->Perm.Perms = 0;
	return 0;
}

/**
 * \fn tVFS_ACL *VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group)
 * \brief Converts UNIX permissions to three Acess ACL entries
 */
tVFS_ACL *VFS_UnixToAcessACL(Uint Mode, Uint Owner, Uint Group)
{
	tVFS_ACL	*ret = malloc(sizeof(tVFS_ACL)*3);
	
	// Error Check
	if(!ret)	return NULL;
	
	// Owner
	ret[0].Ent.Group = 0; ret[0].Ent.ID = Owner;
	ret[0].Perm.Inv = 0;  ret[0].Perm.Perms = 0;
	if(Mode & 0400)	ret[0].Perm.Perms |= VFS_PERM_READ;
	if(Mode & 0200)	ret[0].Perm.Perms |= VFS_PERM_WRITE;
	if(Mode & 0100)	ret[0].Perm.Perms |= VFS_PERM_EXECUTE;
	
	// Group
	ret[1].Ent.Group = 1; ret[1].Ent.ID = Group;
	ret[1].Perm.Inv = 0;  ret[1].Perm.Perms = 0;
	if(Mode & 0040)	ret[1].Perm.Perms |= VFS_PERM_READ;
	if(Mode & 0020)	ret[1].Perm.Perms |= VFS_PERM_WRITE;
	if(Mode & 0010)	ret[1].Perm.Perms |= VFS_PERM_EXECUTE;
	
	// Global
	ret[2].Ent.Group = 1; ret[2].Ent.ID = -1;
	ret[2].Perm.Inv = 0;  ret[2].Perm.Perms = 0;
	if(Mode & 0004)	ret[2].Perm.Perms |= VFS_PERM_READ;
	if(Mode & 0002)	ret[2].Perm.Perms |= VFS_PERM_WRITE;
	if(Mode & 0001)	ret[2].Perm.Perms |= VFS_PERM_EXECUTE;
	
	// Return buffer
	return ret;
}

// === EXPORTS ===
// --- Variables ---
EXPORTV(gVFS_ACL_EveryoneRWX);
EXPORTV(gVFS_ACL_EveryoneRW);
EXPORTV(gVFS_ACL_EveryoneRX);
// --- Functions ---
EXPORT(VFS_UnixToAcessACL);

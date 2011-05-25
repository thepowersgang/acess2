/*
 * Acess2 VFS
 * - AllocHandle, GetHandle
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <vfs_int.h>
#include <vfs_ext.h>

// === CONSTANTS ===
#define MAX_KERNEL_FILES	128
#define MAX_USER_FILES	64

// === IMPORTS ===
extern int	Server_GetClientID(void);

// === PROTOTYPES ===
tVFS_Handle	*VFS_GetHandle(int FD);
 int	VFS_AllocHandle(int FD, tVFS_Node *Node, int Mode);

// === Types ===
typedef struct sUserHandles
{
	struct sUserHandles	*Next;
	 int	PID;
	tVFS_Handle	Handles[MAX_USER_FILES];
}	tUserHandles;

// === GLOBALS ===
tUserHandles	*gpUserHandles = NULL;
tVFS_Handle	gaKernelHandles[MAX_KERNEL_FILES];

// === CODE ===
tUserHandles *VFS_int_GetUserHandles(int PID, int bCreate)
{
	tUserHandles	*ent, *prev = NULL;
	for( ent = gpUserHandles; ent; prev = ent, ent = ent->Next ) {
		if( ent->PID == PID ) {
			Log_Warning("VFS", "Process %i already has a handle list", PID);
			return ent;
		}
		if( ent->PID > PID )	break;
	}
	
	if(!bCreate)
		return NULL;
	
	ent = calloc( 1, sizeof(tUserHandles) );
	ent->PID = PID;
	if( prev ) {
		ent->Next = prev->Next;
		prev->Next = ent;
	}
	else {
		ent->Next = gpUserHandles;
		gpUserHandles = ent;
	}
	Log_Notice("VFS", "Created handle list for process %i", PID);
	return ent;
}

/**
 * \brief Clone the handle list of the current process into another
 */
void VFS_CloneHandleList(int PID)
{
	tUserHandles	*ent;
	tUserHandles	*cur;
	 int	i;
	
	cur = VFS_int_GetUserHandles(Threads_GetPID(), 0);
	if(!cur)	return ;	// Don't need to do anything if the current list is empty
	
	ent = VFS_int_GetUserHandles(PID, 1);
	
	memcpy(ent->Handles, cur->Handles, CFGINT(CFG_VFS_MAXFILES)*sizeof(tVFS_Handle));
	
	for( i = 0; i < CFGINT(CFG_VFS_MAXFILES); i ++ )
	{
		if(!cur->Handles[i].Node)	continue;
		
		if(ent->Handles[i].Node->Reference)
			ent->Handles[i].Node->Reference(ent->Handles[i].Node);
	}
}

/**
 * \fn tVFS_Handle *VFS_GetHandle(int FD)
 * \brief Gets a pointer to the handle information structure
 */
tVFS_Handle *VFS_GetHandle(int FD)
{
	tVFS_Handle	*h;
	
	//Log_Debug("VFS", "VFS_GetHandle: (FD=0x%x)", FD);
	
	if(FD < 0) {
		LOG("FD (%i) < 0, RETURN NULL", FD);
		return NULL;
	}
	
	if(FD & VFS_KERNEL_FLAG) {
		FD &= (VFS_KERNEL_FLAG - 1);
		if(FD >= MAX_KERNEL_FILES) {
			LOG("FD (%i) > MAX_KERNEL_FILES (%i), RETURN NULL", FD, MAX_KERNEL_FILES);
			return NULL;
		}
		h = &gaKernelHandles[ FD ];
	}
	else
	{
		tUserHandles	*ent;
		 int	pid = Threads_GetPID();
		
		ent = VFS_int_GetUserHandles(pid, 0);
		if(!ent) {
			Log_Error("VFS", "Client %i does not have a handle list (>)", pid);
			return NULL;
		}
		
		if(FD >= CFGINT(CFG_VFS_MAXFILES)) {
			LOG("FD (%i) > Limit (%i), RETURN NULL", FD, CFGINT(CFG_VFS_MAXFILES));
			return NULL;
		}
		h = &ent->Handles[ FD ];
		LOG("FD (%i) -> %p (Mode:0x%x,Node:%p)", FD, h, h->Mode, h->Node);
	}
	
	if(h->Node == NULL) {
		LOG("FD (%i) Unused", FD);
		return NULL;
	}
	//Log_Debug("VFS", "VFS_GetHandle: RETURN %p", h);
	return h;
}

int VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode)
{
	 int	i;
	
	// Check for a user open
	if(bIsUser)
	{
		tUserHandles	*ent;
		// Find the PID's handle list
		ent = VFS_int_GetUserHandles(Threads_GetPID(), 1);
		// Get a handle
		for(i=0;i<CFGINT(CFG_VFS_MAXFILES);i++)
		{
			if(ent->Handles[i].Node)	continue;
			ent->Handles[i].Node = Node;
			ent->Handles[i].Position = 0;
			ent->Handles[i].Mode = Mode;
			return i;
		}
	}
	else
	{
		// Get a handle
		for(i=0;i<MAX_KERNEL_FILES;i++)
		{
			if(gaKernelHandles[i].Node)	continue;
			gaKernelHandles[i].Node = Node;
			gaKernelHandles[i].Position = 0;
			gaKernelHandles[i].Mode = Mode;
			return i|VFS_KERNEL_FLAG;
		}
	}
	
	return -1;
}

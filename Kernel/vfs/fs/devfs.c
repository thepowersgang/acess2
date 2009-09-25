/*
 * Acess 2
 * Device Filesystem (DevFS)
 * - vfs/fs/devfs.c
 */
#include <common.h>
#include <vfs.h>
#include <fs_devfs.h>

// === PROTOTYPES ===
 int	DevFS_AddDevice(tDevFS_Driver *Dev);
tVFS_Node	*DevFS_InitDevice(char *Device, char *Options);
char	*DevFS_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*DevFS_FindDir(tVFS_Node *Node, char *Name);

// === GLOBALS ===
tVFS_Driver	gDevFS_Info = {
	"devfs", 0, DevFS_InitDevice, NULL, NULL
	};
tVFS_Node	gDevFS_RootNode = {
	.NumACLs = 1,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.ReadDir = DevFS_ReadDir,
	.FindDir = DevFS_FindDir
	};
tDevFS_Driver	*gDevFS_Drivers = NULL;
 int	giDevFS_NextID = 1;

// === CODE ===
/**
 * \fn int DevFS_AddDevice(tDevFS_Driver *Dev)
 */
int DevFS_AddDevice(tDevFS_Driver *Dev)
{
	Dev->Next = gDevFS_Drivers;
	gDevFS_Drivers = Dev;
	
	return giDevFS_NextID++;
}

/**
 * \fn tVFS_Node *DevFS_InitDevice(char *Device, char *Options)
 * \brief Initialise the DevFS and detect double-mounting, or just do nothing
 * \stub
 */
tVFS_Node *DevFS_InitDevice(char *Device, char *Options)
{
	return &gDevFS_RootNode;
}

/**
 * \fn char *DevFS_ReadDir(tVFS_Node *Node, int Pos)
 */
char *DevFS_ReadDir(tVFS_Node *Node, int Pos)
{
	tDevFS_Driver	*dev;
	
	if(Pos < 0)	return NULL;
	
	for(dev = gDevFS_Drivers;
		dev && Pos--;
		dev = dev->Next
		);
	
	return dev->Name;
}

/**
 * \fn tVFS_Node *DevFS_FindDir(tVFS_Node *Node, char *Name)
 * \brief Get an entry from the devices directory
 */
tVFS_Node *DevFS_FindDir(tVFS_Node *Node, char *Name)
{
	tDevFS_Driver	*dev;
	
	//ENTER("pNode sName", Node, Name);
	
	for(dev = gDevFS_Drivers;
		dev;
		dev = dev->Next
		)
	{
		//LOG("dev = %p", dev);
		//LOG("dev->Name = '%s'", dev->Name);
		if(strcmp(dev->Name, Name) == 0) {
			//LEAVE('p', &dev->RootNode);
			return &dev->RootNode;
		}
	}
	
	//LEAVE('n');
	return NULL;
}

// --- EXPORTS ---
EXPORT(DevFS_AddDevice);

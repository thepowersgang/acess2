/*
 * Acess 2
 * Device Filesystem (DevFS)
 * - vfs/fs/devfs.c
 */
#include <acess.h>
#include <vfs.h>
#include <fs_devfs.h>

// === PROTOTYPES ===
 int	DevFS_AddDevice(tDevFS_Driver *Device);
void	DevFS_DelDevice(tDevFS_Driver *Device);
tVFS_Node	*DevFS_InitDevice(char *Device, char **Options);
char	*DevFS_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*DevFS_FindDir(tVFS_Node *Node, const char *Name);

// === GLOBALS ===
tVFS_Driver	gDevFS_Info = {
	"devfs", 0, DevFS_InitDevice, NULL, NULL
	};
tVFS_Node	gDevFS_RootNode = {
	.Size = 0,
	.Flags = VFS_FFLAG_DIRECTORY,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.ReadDir = DevFS_ReadDir,
	.FindDir = DevFS_FindDir
	};
tDevFS_Driver	*gDevFS_Drivers = NULL;
 int	giDevFS_NextID = 1;
tShortSpinlock	glDevFS_ListLock;

// === CODE ===
/**
 * \fn int DevFS_AddDevice(tDevFS_Driver *Device)
 */
int DevFS_AddDevice(tDevFS_Driver *Device)
{
	 int	ret = 0;
	tDevFS_Driver	*dev;
	
	SHORTLOCK( &glDevFS_ListLock );
	
	// Check if the device is already registered or the name is taken
	for( dev = gDevFS_Drivers; dev; dev = dev->Next )
	{
		if(dev == Device)	break;
		if(strcmp(dev->Name, Device->Name) == 0)	break;
	}
	
	if(dev) {
		if(dev == Device)
			Log_Warning("DevFS", "Device %p '%s' attempted to register itself twice",
				dev, dev->Name);
		else
			Log_Warning("DevFS", "Device %p attempted to register '%s' which was owned by %p",
				Device, dev->Name, dev);
		ret = 0;	// Error
	}
	else {
		Device->Next = gDevFS_Drivers;
		gDevFS_Drivers = Device;
		gDevFS_RootNode.Size ++;
		ret = giDevFS_NextID ++;
	}
	SHORTREL( &glDevFS_ListLock );
	
	return ret;
}

/**
 * \brief Delete a device from the DevFS folder
 */
void DevFS_DelDevice(tDevFS_Driver *Device)
{
	tDevFS_Driver	*prev = NULL, *dev;
	
	SHORTLOCK( &glDevFS_ListLock );
	// Search list for device
	for(dev = gDevFS_Drivers;
		dev && dev != Device;
		prev = dev, dev = dev->Next
		);
	
	// Check if it was found
	if(dev)
	{
		if(prev)
			prev->Next = Device->Next;
		else
			gDevFS_Drivers = Device->Next;
	}
	else
		Log_Warning("DevFS", "Attempted to unregister device %p '%s' which was not registered",
			Device, Device->Name);
	
	SHORTREL( &glDevFS_ListLock );
}

/**
 * \fn tVFS_Node *DevFS_InitDevice(char *Device, char **Options)
 * \brief Initialise the DevFS and detect double-mounting, or just do nothing
 * \note STUB
 */
tVFS_Node *DevFS_InitDevice(char *Device, char **Options)
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
	
	if(dev)
		return strdup(dev->Name);
	else
		return NULL;
}

/**
 * \fn tVFS_Node *DevFS_FindDir(tVFS_Node *Node, const char *Name)
 * \brief Get an entry from the devices directory
 */
tVFS_Node *DevFS_FindDir(tVFS_Node *Node, const char *Name)
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
EXPORT(DevFS_DelDevice);

/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/zero-one.c
 * - /Devices/{null,zero,one}
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>

// === PROTOTYPES ===
 int	CoreDevs_Install(char **Arguments);
size_t	CoreDevs_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
size_t	CoreDevs_Read_Zero(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	CoreDevs_Read_One (tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
size_t	CoreDevs_Read_Null(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, CoreDevs, CoreDevs_Install, NULL, NULL);
tVFS_NodeType	gCoreDevs_NT_Null = {
	.TypeName = "CoreDevs-null",
	.Read  = CoreDevs_Read_Null,
	.Write = CoreDevs_Write
};
tVFS_NodeType	gCoreDevs_NT_Zero = {
	.TypeName = "CoreDevs-zero",
	.Read  = CoreDevs_Read_Zero,
	.Write = CoreDevs_Write
};
tVFS_NodeType	gCoreDevs_NT_One = {
	.TypeName = "CoreDevs-one",
	.Read  = CoreDevs_Read_One,
	.Write = CoreDevs_Write
};
tDevFS_Driver	gCoreDevs_Null = {
	NULL, "null",
	{
	.Size = 0,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_Null
	}
};
tDevFS_Driver	gCoreDevs_Zero = {
	NULL, "zero",
	{
	.Size = 0,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_Zero
	}
};
tDevFS_Driver	gCoreDevs_One = {
	NULL, "one",
	{
	.Size = 0,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_One
	}
};

// === CODE ===
/**
 * \brief Installs the CoreDevs driver
 */
int CoreDevs_Install(char **Options)
{
	DevFS_AddDevice( &gCoreDevs_Null );
	DevFS_AddDevice( &gCoreDevs_Zero );
	DevFS_AddDevice( &gCoreDevs_One  );
	return MODULE_ERR_OK;
}

size_t CoreDevs_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	return Length;	// Ignore
}

size_t CoreDevs_Read_Zero(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	memset(Buffer, 0, Length);
	return Length;
}

size_t CoreDevs_Read_One (tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	Uint8	*ptr = Buffer;
	size_t	rem;
	for( rem = Length; rem --; ptr ++ )
		*ptr = 0xFF;
	return Length;
}

size_t CoreDevs_Read_Null(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	return 0;
}

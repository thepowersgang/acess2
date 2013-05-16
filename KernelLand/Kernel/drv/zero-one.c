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
size_t	CoreDevs_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
size_t	CoreDevs_Read_Zero(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	CoreDevs_Read_One (tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	CoreDevs_Read_Null(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	CoreDevs_Read_FRandom(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	CoreDevs_Read_GRandom(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);

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
tVFS_NodeType	gCoreDevs_NT_FRandom = {
	.TypeName = "CoreDevs-frandom",
	.Read  = CoreDevs_Read_FRandom,
	.Write = CoreDevs_Write
};
tVFS_NodeType	gCoreDevs_NT_GRandom = {
	.TypeName = "CoreDevs-grandom",
	.Read  = CoreDevs_Read_GRandom,
	.Write = CoreDevs_Write
};
tDevFS_Driver	gCoreDevs_Null = {
	NULL, "null",
	{
	.Size = 0, .NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_Null
	}
};
tDevFS_Driver	gCoreDevs_Zero = {
	NULL, "zero",
	{
	.Size = 0, .NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_Zero
	}
};
tDevFS_Driver	gCoreDevs_One = {
	NULL, "one",
	{
	.Size = 0, .NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_One
	}
};
tDevFS_Driver	gCoreDevs_FRandom = {
	NULL, "frandom",
	{
	.Size = 0, .NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_FRandom,
	.DataAvaliable = 1
	}
};
tDevFS_Driver	gCoreDevs_GRandom = {
	NULL, "grandom",
	{
	.Size = 0, .NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Type = &gCoreDevs_NT_GRandom,
	.DataAvaliable = 1
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
	DevFS_AddDevice( &gCoreDevs_FRandom  );
	//DevFS_AddDevice( &gCoreDevs_GRandom  );
	return MODULE_ERR_OK;
}

size_t CoreDevs_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	return Length;	// Ignore
}

size_t CoreDevs_Read_Zero(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	memset(Buffer, 0, Length);
	return Length;
}

size_t CoreDevs_Read_One (tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	Uint8	*ptr = Buffer;
	size_t	rem;
	for( rem = Length; rem --; ptr ++ )
		*ptr = 0xFF;
	return Length;
}

size_t CoreDevs_Read_Null(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	return 0;
}

//! Fast random number generator
size_t CoreDevs_Read_FRandom(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	Uint8	*cbuf = Buffer;
	for( int i = 0; i < Length; i ++ )
		*cbuf++ = rand();
	return Length;
}

size_t CoreDevs_Read_GRandom(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	// TODO: VFS_IOFLAG_NOBLOCK
	Log_Error("CoreDevs", "GRandom is unimplimented");
	return -1;
}


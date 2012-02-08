/*
 * Acess2 EDI Layer
 */
#define DEBUG	0
#define VERSION	((0<<8)|1)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#define	IMPLEMENTING_EDI	1
#include "edi/edi.h"

#define	VALIDATE_PTR(_ptr,_err)	do{if(!(_ptr))return _err; }while(0)
#define	GET_DATA(_ptr)	(Object)

#include "edi_io.inc.c"
#include "edi_int.inc.c"

// === STRUCTURES ===
typedef struct sAcessEdiDriver {
	struct sAcessEdiDriver	*Next;
	tDevFS_Driver	Driver;
	 int	FileCount;
	struct {
		char	*Name;
		tVFS_Node	Node;
	}	*Files;
	edi_object_metadata_t	*Objects;
	edi_initialization_t	Init;
	driver_finish_t	Finish;
} tAcessEdiDriver;

// === PROTOTYPES ===
 int	EDI_Install(char **Arguments);
 int	EDI_DetectDriver(void *Base);
 int	EDI_LoadDriver(void *Base);
vfs_node *EDI_FS_ReadDir(vfs_node *Node, int Pos);
vfs_node *EDI_FS_FindDir(vfs_node *Node, char *Name);
 int	EDI_FS_CharRead(vfs_node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	EDI_FS_CharWrite(vfs_node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	EDI_FS_IOCtl(vfs_node *Node, int Id, void *Data);
data_pointer EDI_GetInternalClass(edi_string_t ClassName);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, EDI, EDI_Install, NULL, NULL);
tModuleLoader	gEDI_Loader = {
	NULL, "EDI", EDI_DetectDriver, EDI_LoadDriver, NULL
};
tSpinlock	glEDI_Drivers;
tAcessEdiDriver	*gEdi_Drivers = NULL;
edi_class_declaration_t	*gcEdi_IntClasses[] = {
	&scEdi_Int_Class_IO, &scEdi_Int_Class_IRQ
};
#define	NUM_INT_CLASSES	(sizeof(gcEdi_IntClasses)/sizeof(gcEdi_IntClasses[0]))
char *csCharNumbers[] = {"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9"};
char *csBlockNumbers[] = {"blk0", "blk1", "blk2", "blk3", "blk4", "blk5", "blk6", "blk7", "blk8", "blk9"};

// === CODE ===
/**
 * \fn int EDI_Install(char **Arguments)
 * \brief Stub intialisation routine
 */
int EDI_Install(char **Arguments)
{
	Module_RegisterLoader( &gEDI_Loader );
	return 1;
}

/**
 * \brief Detects if a driver should be loaded by the EDI subsystem
 */
int EDI_DetectDriver(void *Base)
{
	if( Binary_FindSymbol(BASE, "driver_init", NULL) == 0 )
		return 0;
	
	return 1;
}

/**
 * \fn int Edi_LoadDriver(void *Base)
 * \brief Load an EDI Driver from a loaded binary
 * \param Base	Binary Handle
 * \return 0 on success, non zero on error
 */
int EDI_LoadDriver(void *Base)
{
	driver_init_t	init;
	driver_finish_t	finish;
	tAcessEdiDriver	*info;
	 int	i, j;
	 int	devfsId;
	edi_class_declaration_t	*classes;

	ENTER("pBase", Base);
	
	// Get Functions
	if( !Binary_FindSymbol(Base, "driver_init", (Uint*)&init)
	||	!Binary_FindSymbol(Base, "driver_finish", (Uint*)&finish) )
	{
		Warning("[EDI  ] Driver %p does not provide both `driver_init` and `driver_finish`\n", Base);
		Binary_Unload(Base);
		return 0;
	}
	
	// Allocate Driver Information
	info = malloc( sizeof(tAcessEdiDriver) );
	info->Finish = finish;
	
	// Initialise Driver
	info->Init = init( 0, NULL );	// TODO: Implement Resources
	
	LOG("info->Init.driver_name = '%s'", info->Init.driver_name);
	LOG("info->Init.num_driver_classes = %i", info->Init.num_driver_classes);
	
	// Count mappable classes
	classes = info->Init.driver_classes;
	info->FileCount = 0;
	info->Objects = NULL;
	for( i = 0, j = 0; i < info->Init.num_driver_classes; i++ )
	{
		if( strncmp(classes[i].name, "EDI-CHARACTER-DEVICE", 32) == 0 )
		{
			data_pointer	*obj;
			// Initialise Object Instances
			for( ; (obj = classes[i].constructor()); j++ ) {
				LOG("%i - Constructed '%s'", j, classes[i].name);
				info->FileCount ++;
				info->Objects = realloc(info->Objects, sizeof(*info->Objects)*info->FileCount);
				info->Objects[j].object = obj;
				info->Objects[j].object_class = &classes[i];
			}
		}
		else
			LOG("%i - %s", i, classes[i].name);
	}
	
	if(info->FileCount)
	{
		 int	iNumChar = 0;
		// Create VFS Nodes
		info->Files = malloc( info->FileCount * sizeof(*info->Files) );
		memset(info->Files, 0, info->FileCount * sizeof(*info->Files));
		j = 0;
		for( j = 0; j < info->FileCount; j++ )
		{
			classes = info->Objects[j].object_class;
			if( strncmp(classes->name, "EDI-CHARACTER-DEVICE", 32) == 0 )
			{
				LOG("%i - %s", j, csCharNumbers[iNumChar]);
				info->Files[j].Name = csCharNumbers[iNumChar];
				info->Files[j].Node.NumACLs = 1;
				info->Files[j].Node.ACLs = &gVFS_ACL_EveryoneRW;
				info->Files[j].Node.ImplPtr = &info->Objects[j];
				info->Files[j].Node.Read = EDI_FS_CharRead;
				info->Files[j].Node.Write = EDI_FS_CharWrite;
				info->Files[j].Node.IOCtl = EDI_FS_IOCtl;
				info->Files[j].Node.CTime =
					info->Files[j].Node.MTime =
					info->Files[j].Node.ATime = now();
				
				iNumChar ++;
				continue;
			}
		}
		
		// Create DevFS Driver
		info->Driver.ioctl = EDI_FS_IOCtl;
		memsetda(&info->Driver.rootNode, 0, sizeof(vfs_node) / 4);
		info->Driver.Name = info->Init.driver_name;
		info->Driver.RootNode.Flags = VFS_FFLAG_DIRECTORY;
		info->Driver.RootNode.NumACLs = 1;
		info->Driver.RootNode.ACLs = &gVFS_ACL_EveryoneRX;
		info->Driver.RootNode.Length = info->FileCount;
		info->Driver.RootNode.ImplPtr = info;
		info->Driver.RootNode.ReadDir = EDI_FS_ReadDir;
		info->Driver.RootNode.FindDir = EDI_FS_FindDir;
		info->Driver.RootNode.IOCtl = EDI_FS_IOCtl;
		
		// Register
		devfsId = dev_addDevice( &info->Driver );
		if(devfsId == -1) {
			free(info->Files);	// Free Files
			info->Finish();	// Clean up driver
			free(info);		// Free info structure
			Binary_Unload(iDriverBase);	// Unload library
			return -3;	// Return error
		}
	}
	
	// Append to loaded list;
	LOCK(&glEDI_Drivers);
	info->Next = gEDI_Drivers;
	gEDI_Drivers = info;
	RELEASE(&glEDI_Drivers);
	
	LogF("[EDI ] Loaded Driver '%s' (%s)\n", info->Init.driver_name, Path);
	LEAVE('i', 1);
	return 1;
}

// --- Filesystem Interaction ---
/**
 * \brief Read from a drivers class list
 * \param Node	Driver's Root Node
 * \param Pos	Index of file to get
 */
char *EDI_FS_ReadDir(tVFS_Node *Node, int Pos)
{
	tAcessEdiDriver	*info;
	
	// Sanity Check
	if(!Node)	return NULL;
	
	// Get Information Structure
	info = (void *) Node->ImplPtr;
	if(!info)	return NULL;
	
	// Check Position
	if(Pos < 0)	return NULL;
	if(Pos >= info->FileCount)	return NULL;
	
	return strdup( info->Files[Pos].Name );
}

/**
 * \fn tVFS_Node *EDI_FS_FindDir(tVFS_Node *Node, char *Name)
 * \brief Find a named file in a driver
 * \param Node	Driver's Root Node
 * \param Name	Name of file to find
 */
tVFS_Node *EDI_FS_FindDir(tVFS_Node *Node, char *Name)
{
	tAcessEdiDriver	*info;
	 int	i;
	
	// Sanity Check
	if(!Node)	return NULL;
	if(!Name)	return NULL;
	
	// Get Information Structure
	info = (void *) Node->ImplPtr;
	if(!info)	return NULL;
	
	for( i = 0; i < info->FileCount; i++ )
	{
		if(strcmp(info->Files[i].name, Name) == 0)
			return &info->Files[i].Node;
	}
	
	return NULL;
}

/**
 * \fn Uint64 EDI_FS_CharRead(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read from an EDI Character Device
 * \param Node	File Node
 * \param Offset	Offset into file (ignored)
 * \param Length	Number of characters to read
 * \param Buffer	Destination for data
 * \return	Number of characters read
 */
Uint64 EDI_FS_CharRead(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	edi_object_metadata_t	*meta;
	edi_class_declaration_t	*class;
	
	// Sanity Check
	if(!Node || !Buffer)	return 0;
	if(Length <= 0)	return 0;
	// Get Object Metadata
	meta = (void *) Node->ImplPtr;
	if(!meta)	return 0;
	
	// Get Class
	class = meta->object_class;
	if(!class)	return 0;
	
	// Read from object
	if( ((tAnyFunction) class->methods[0].code)( meta->object, Buffer, Length ))
		return Length;

	return 0;
}

/**
 * \fn Uint64 EDI_FS_CharWrite(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write to an EDI Character Device
 * \param Node	File Node
 * \param Offset	Offset into file (ignored)
 * \param Length	Number of characters to write
 * \param Buffer	Source for data
 * \return	Number of characters written
 */
Uint64 EDI_FS_CharWrite(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	edi_object_metadata_t	*meta;
	edi_class_declaration_t	*class;
	
	// Sanity Check
	if(!Node || !Buffer)	return 0;
	if(Length <= 0)	return 0;
	// Get Object Metadata
	meta = (void *) Node->ImplPtr;
	if(!meta)	return 0;
	
	// Get Class
	class = meta->object_class;
	if(!class)	return 0;
	
	// Write to object
	if( ((tAnyFunction) class->methods[1].code)( meta->object, Buffer, Length ))
		return Length;

	return 0;
}

/**
 * \fn int EDI_FS_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Perfom an IOCtl call on the object
 */
int EDI_FS_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	return 0;
}

// --- EDI Functions ---
/**
 * \fn data_pointer EDI_GetDefinedClass(edi_string_t ClassName)
 * \brief Gets the structure of a driver defined class
 * \param ClassName	Name of class to find
 * \return Class definition or NULL
 */
data_pointer EDI_GetDefinedClass(edi_string_t ClassName)
{
	 int	i;
	tAcessEdiDriver	*drv;
	edi_class_declaration_t	*classes;
	
	for(drv = gEdi_Drivers;
		drv;
		drv = drv->Next	)
	{
		classes = drv->Init.driver_classes;
		for( i = 0; i < drv->Init.num_driver_classes; i++ )
		{
			if( strncmp(classes[i].name, ClassName, 32) == 0 )
				return &classes[i];
		}
	}
	return NULL;
}

/**
 * \fn int32_t EDI_CheckClassExistence(edi_string_t ClassName)
 * \brief Checks if a class exists
 * \param ClassName	Name of class
 * \return 1 if the class exists, -1 otherwise
 */
int32_t EDI_CheckClassExistence(edi_string_t ClassName)
{
	//LogF("check_class_existence: (ClassName='%s')\n", ClassName);
	if(EDI_GetInternalClass(ClassName))
		return 1;
		
	if(EDI_GetDefinedClass(ClassName))	// Driver Defined
		return 1;
	
	return -1;
}

/**
 * \fn edi_object_metadata_t EDI_ConstructObject(edi_string_t ClassName)
 * \brief Construct an instance of an class (an object)
 * \param ClassName	Name of the class to construct
 */
edi_object_metadata_t EDI_ConstructObject(edi_string_t ClassName)
{
	edi_object_metadata_t	ret = {0, 0};
	edi_class_declaration_t	*class;
	
	//LogF("EDI_ConstructObject: (ClassName='%s')\n", ClassName);
	
	// Get class definition
	if( !(class = EDI_GetInternalClass(ClassName)) )	// Internal
		if( !(class = EDI_GetDefinedClass(ClassName)) )	// Driver Defined
			return ret;		// Return ERROR
	
	// Initialise
	ret.object = class->constructor();
	if( !ret.object )
		return ret;	// Return ERROR
	
	// Set declaration pointer
	ret.object_class = class;
	
	//LogF("EDI_ConstructObject: RETURN {0x%x,0x%x}\n", ret.object, ret.object_class);
	return ret;
}

/**
 * \fn void EDI_DestroyObject(edi_object_metadata_t Object)
 * \brief Destroy an instance of a class
 * \param Object	Object to destroy
 */
void EDI_DestroyObject(edi_object_metadata_t Object)
{
	if( !Object.object )	return;
	if( !Object.object_class )	return;
	
	((edi_class_declaration_t*)(Object.object_class))->destructor( &Object );
}

/**
 * \fn function_pointer EDI_GetMethodByName(data_pointer ObjectClass, edi_string_t MethodName)
 * \brief Get a method of a class by it's name
 * \param ObjectClass	Pointer to a ::edi_object_metadata_t of the object
 * \param MethodName	Name of the desired method
 * \return Function address or NULL
 */
function_pointer EDI_GetMethodByName(data_pointer ObjectClass, edi_string_t MethodName)
{
	edi_class_declaration_t	*dec = ObjectClass;
	 int	i;
	
	//LogF("get_method_by_name: (ObjectClass=0x%x, MethodName='%s')\n", ObjectClass, MethodName);
	
	if(!ObjectClass)	return NULL;
	
	for(i = 0; i < dec->num_methods; i++)
	{
		if(strncmp(MethodName, dec->methods[i].name, 32) == 0)
			return dec->methods[i].code;
	}
	return NULL;
}

#if 0
function_pointer get_method_by_declaration(data_pointer Class, edi_function_declaration_t Declaration);
#endif

/**
 * \fn edi_string_ptr_t EDI_GetClassParent(edi_string_t ClassName)
 * \brief Get the parent of the named class
 * \todo Implement
 */
edi_string_ptr_t EDI_GetClassParent(edi_string_t ClassName)
{
	WarningEx("EDI", "`get_class_parent` is unimplemented");
	return NULL;
}

/**
 * \fn data_pointer EDI_GetInternalClass(edi_string_t ClassName)
 * \brief Get a builtin class
 * \param ClassName	Name of class to find
 * \return Pointer to the ::edi_class_declaration_t of the class
 */
data_pointer EDI_GetInternalClass(edi_string_t ClassName)
{
	 int	i;
	//LogF("get_internal_class: (ClassName='%s')\n", ClassName);
	for( i = 0; i < NUM_INT_CLASSES; i++ )
	{
		if( strncmp( gcEdi_IntClasses[i]->name, ClassName, 32 ) == 0 ) {
			return gcEdi_IntClasses[i];
		}
	}
	//LogF("get_internal_class: RETURN NULL\n");
	return NULL;
}

/**
 * \fn edi_string_ptr_t EDI_GetObjectClass(data_pointer Object)
 * \brief Get the name of the object of \a Object
 * \param Object	Object to get name of
 * \return Pointer to the class name
 */
edi_string_ptr_t EDI_GetObjectClass(data_pointer ObjectClass)
{
	edi_object_metadata_t	*Metadata = ObjectClass;
	// Sanity Check
	if(!ObjectClass)	return NULL;
	if(!(edi_class_declaration_t*) Metadata->object_class)	return NULL;
	
	// Return Class Name
	return ((edi_class_declaration_t*) Metadata->object_class)->name;
}

// === EXPORTS ===
EXPORTAS(EDI_CheckClassExistence, check_class_existence);
EXPORTAS(EDI_ConstructObject, construct_object);
EXPORTAS(EDI_DestroyObject, destroy_object);
EXPORTAS(EDI_GetMethodByName, get_method_by_name);
EXPORTAS(EDI_GetClassParent, get_class_parent);
EXPORTAS(EDI_GetInternalClass, get_internal_class);
EXPORTAS(EDI_GetObjectClass, get_object_class);

/*
 * Acess2
 * Common Binary Loader
 */
#define DEBUG	0
#include <acess.h>
#include <binary.h>
#include <mm_virt.h>
#include <hal_proc.h>
#include <vfs_threads.h>

// === CONSTANTS ===
#define BIN_LOWEST	MM_USER_MIN		// 1MiB
#define BIN_GRANUALITY	0x10000		// 64KiB
#define BIN_HIGHEST	(USER_LIB_MAX-BIN_GRANUALITY)		// Just below the kernel
#define	KLIB_LOWEST	MM_MODULE_MIN
#define KLIB_GRANUALITY	0x10000		// 32KiB
#define	KLIB_HIGHEST	(MM_MODULE_MAX-KLIB_GRANUALITY)

// === TYPES ===
typedef struct sKernelBin {
	struct sKernelBin	*Next;
	void	*Base;
	tBinary	*Info;
} tKernelBin;

// === IMPORTS ===
extern char	*Threads_GetName(int ID);
extern tKernelSymbol	gKernelSymbols[];
extern tKernelSymbol	gKernelSymbolsEnd[];
extern tBinaryType	gELF_Info;

// === PROTOTYPES ===
size_t	Binary_int_CacheArgs(const char **Path, const char ***ArgV, const char ***EnvP, void *DestBuffer);
 int	Proc_int_Execve(const char *File, const char **ArgV, const char **EnvP, int DataSize, bool bClearUser);
tVAddr	Binary_Load(const char *Path, tVAddr *EntryPoint);
tBinary	*Binary_GetInfo(tMount MountID, tInode InodeID);
tVAddr	Binary_MapIn(tBinary *Binary, const char *Path, tVAddr LoadMin, tVAddr LoadMax);
tVAddr	Binary_IsMapped(tBinary *Binary);
tBinary *Binary_DoLoad(tMount MountID, tInode Inode, const char *Path);
void	Binary_Dereference(tBinary *Info);
#if 0
Uint	Binary_Relocate(void *Base);
#endif
Uint	Binary_GetSymbolEx(const char *Name, Uint *Value);
#if 0
Uint	Binary_FindSymbol(void *Base, const char *Name, Uint *Val);
#endif
 int	Binary_int_CheckMemFree( tVAddr _start, size_t _len );

// === GLOBALS ===
tShortSpinlock	glBinListLock;
tBinary	*glLoadedBinaries = NULL;
char	**gsaRegInterps = NULL;
 int	giRegInterps = 0;
tShortSpinlock	glKBinListLock;
tKernelBin	*glLoadedKernelLibs;
tBinaryType	*gRegBinTypes = &gELF_Info;
 
// === FUNCTIONS ===
/**
 * \brief Registers a binary type
 */
int Binary_RegisterType(tBinaryType *Type)
{
	Type->Next = gRegBinTypes;
	gRegBinTypes = Type;
	return 1;
}

/**
 * \fn int Proc_Spawn(const char *Path)
 */
int Proc_Spawn(const char *Path)
{
	char	stackPath[strlen(Path)+1];
	ENTER("sPath", Path);
	
	strcpy(stackPath, Path);
	
	LOG("stackPath = '%s'", stackPath);
	
	if(Proc_Clone(CLONE_VM|CLONE_NOUSER) == 0)
	{
		// CHILD
		const char	*args[2] = {stackPath, NULL};
		LOG("stackPath = '%s'", stackPath);
		Proc_Execve(stackPath, args, &args[1], 0);
		for(;;);
	}
	LEAVE('i', 0);
	return 0;
}

/**
 * \todo Document
 */
size_t Binary_int_CacheArgs(const char **Path, const char ***ArgV, const char ***EnvP, void *DestBuffer)
{
	size_t	size;
	 int	argc=0, envc=0;
	 int	i;
	char	*strbuf;
	const char	**arrays;
	
	// Calculate size
	size = 0;
	if( ArgV && *ArgV )
	{
		const char	**argv = *ArgV;
		for( argc = 0; argv[argc]; argc ++ )
			size += strlen( argv[argc] ) + 1;
	}
	if( EnvP && *EnvP )
	{
		const char	**envp = *EnvP;
		for( envc = 0; envp[envc]; envc ++ )
			size += strlen( envp[envc] ) + 1;
	}
	size = (size + sizeof(void*)-1) & ~(sizeof(void*)-1);	// Word align
	size += (argc+1+envc+1)*sizeof(void*);	// Arrays
	if( Path )
	{
		size += strlen( *Path ) + 1;
	}

	if( DestBuffer )	
	{
		arrays = DestBuffer;
		strbuf = (void*)&arrays[argc+1+envc+1];
	
		// Fill ArgV
		if( ArgV && *ArgV )
		{
			const char	**argv = *ArgV;
			for( i = 0; argv[i]; i ++ )
			{
				arrays[i] = strbuf;
				strcpy(strbuf, argv[i]);
				strbuf += strlen( argv[i] ) + 1;
			}
			*ArgV = arrays;
			arrays += i;
		}
		*arrays++ = NULL;
		// Fill EnvP
		if( EnvP && *EnvP )
		{
			const char	**envp = *EnvP;
			for( i = 0; envp[i]; i ++ )
			{
				arrays[i] = strbuf;
				strcpy(strbuf, envp[i]);
				strbuf += strlen( envp[i] ) + 1;
			}
			*EnvP = arrays;
			arrays += i;
		}
		*arrays++ = NULL;
		// Fill path
		if( Path )
		{
			strcpy(strbuf, *Path);
			*Path = strbuf;
		}
	}
	
	return size;
}

/**
 * \brief Create a new process with the specified set of file descriptors
 */
int Proc_SysSpawn(const char *Binary, const char **ArgV, const char **EnvP, int nFD, int *FDs)
{
	
	// --- Save File, ArgV and EnvP
	size_t size = Binary_int_CacheArgs( &Binary, &ArgV, &EnvP, NULL );
	void *cachebuf = malloc( size );
	Binary_int_CacheArgs( &Binary, &ArgV, &EnvP, cachebuf );

	// Cache the VFS handles	
	void *handles = VFS_SaveHandles(nFD, FDs);

	// Create new process	
	tPID ret = Proc_Clone(CLONE_VM|CLONE_NOUSER);
	if( ret == 0 )
	{
		VFS_RestoreHandles(nFD, handles);
		VFS_FreeSavedHandles(nFD, handles);
		// Frees cachebuf
		Proc_int_Execve(Binary, ArgV, EnvP, size, 0);
		for(;;);
	}
	if( ret == -1 )
	{
		VFS_FreeSavedHandles(nFD, handles);
		free(cachebuf);
	}
	
	return ret;
}

/**
 * \brief Replace the current user image with another
 * \param File	File to load as the next image
 * \param ArgV	Arguments to pass to user
 * \param EnvP	User's environment
 * \note Called Proc_ for historical reasons
 */
int Proc_Execve(const char *File, const char **ArgV, const char **EnvP, int DataSize)
{
	return Proc_int_Execve(File, ArgV, EnvP, DataSize, 1);
}

int Proc_int_Execve(const char *File, const char **ArgV, const char **EnvP, int DataSize, bool bClearUser)
{
	void	*cachebuf;
	tVAddr	entry;
	Uint	base;	// Uint because Proc_StartUser wants it
	 int	argc;
	
	ENTER("sFile pArgV pEnvP", File, ArgV, EnvP);
	
	// --- Save File, ArgV and EnvP
	if( DataSize == 0 )
	{
		DataSize = Binary_int_CacheArgs( &File, &ArgV, &EnvP, NULL );
		cachebuf = malloc( DataSize );
		Binary_int_CacheArgs( &File, &ArgV, &EnvP, cachebuf );
	}

	// --- Get argc	
	for( argc = 0; ArgV && ArgV[argc]; argc ++ )
		;
	
	// --- Set Process Name
	Threads_SetName(File);
	
	// --- Clear User Address space
	if( bClearUser )
	{
		// MM_ClearUser should preserve handles
		MM_ClearUser();
		// - NOTE: Not a reliable test, but helps for now
		ASSERTC( VFS_IOCtl(0, 0, NULL), !=, -1 );
	}
	
	// --- Load new binary
	base = Binary_Load(File, &entry);
	if(base == 0)
	{
		Log_Warning("Binary", "Proc_Execve - Unable to load '%s' [errno=%i]", File, errno);
		LEAVE('-');
		Threads_Exit(0, -10);
		for(;;);
	}
	
	LOG("entry = 0x%x, base = 0x%x", entry, base);
	LEAVE('-');
	// --- And... Jump to it
	Proc_StartUser(entry, base, argc, ArgV, DataSize);
	for(;;);	// Tell GCC that we never return
}

/**
 * \brief Load a binary into the current address space
 * \param Path	Path to binary to load
 * \param EntryPoint	Pointer for exectuable entry point
 * \return Virtual address where the binary has been loaded
 */
tVAddr Binary_Load(const char *Path, tVAddr *EntryPoint)
{
	tMount	mount_id;
	tInode	inode;
	tBinary	*pBinary;
	tVAddr	base = -1;

	ENTER("sPath pEntryPoint", Path, EntryPoint);
	
	// Sanity Check Argument
	if(Path == NULL) {
		LEAVE('x', 0);
		return 0;
	}

	// Check if this path has been loaded before.
	#if 0
	// TODO: Implement a list of string/tBinary pairs for loaded bins
	#endif

	// Get Inode
	{
		int fd;
		tFInfo	info;
		fd = VFS_Open(Path, VFS_OPENFLAG_READ|VFS_OPENFLAG_EXEC);
		if( fd == -1 ) {
			LOG("%s does not exist", Path);
			LEAVE_RET('x', 0);
		}
		VFS_FInfo(fd, &info, 0);
		VFS_Close(fd);
		mount_id = info.mount;
		inode = info.inode;
		LOG("mount_id = %i, inode = %i", mount_id, inode);
	}

	// TODO: Also get modifcation time?

	// Check if the binary has already been loaded
	if( !(pBinary = Binary_GetInfo(mount_id, inode)) )
		pBinary = Binary_DoLoad(mount_id, inode, Path);	// Else load it
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('x', 0);
		return 0;
	}
	
	// Map into process space
	base = Binary_MapIn(pBinary, Path, BIN_LOWEST, BIN_HIGHEST);
	
	// Check for errors
	if(base == 0) {
		LEAVE('x', 0);
		return 0;
	}
	
	// Interpret
	if(pBinary->Interpreter) {
		tVAddr	start;
		if( Binary_Load(pBinary->Interpreter, &start) == 0 ) {
			Log_Error("Binary", "Can't load interpeter '%s' for '%s'",
				pBinary->Interpreter, Path);
			LEAVE('x', 0);
			return 0;
		}
		*EntryPoint = start;
	}
	else
		*EntryPoint = pBinary->Entry - pBinary->Base + base;
	
	// Return
	LOG("*EntryPoint = 0x%x", *EntryPoint);
	LEAVE('x', base);
	return base;	// Pass the base as an argument to the user if there is an interpreter
}

/**
 * \brief Finds a matching binary entry
 * \param MountID	Mountpoint ID of binary file
 * \param InodeID	Inode ID of the file
 * \return Pointer to the binary definition (if already loaded)
 */
tBinary *Binary_GetInfo(tMount MountID, tInode InodeID)
{
	tBinary	*pBinary;
	for(pBinary = glLoadedBinaries; pBinary; pBinary = pBinary->Next)
	{
		if(pBinary->MountID == MountID && pBinary->Inode == InodeID)
			return pBinary;
	}
	return NULL;
}

/**
 * \brief Maps an already-loaded binary into an address space.
 * \param Binary	Pointer to globally stored binary definition
 * \param Path	Path to the binary's file (for debug)
 * \param LoadMin	Lowest location to map to
 * \param LoadMax	Highest location to map to
 * \return Base load address
 */
tVAddr Binary_MapIn(tBinary *Binary, const char *Path, tVAddr LoadMin, tVAddr LoadMax)
{
	tVAddr	base;
	 int	i, fd;

	ENTER("pBinary sPath pLoadMin pLoadMax", Binary, Path, LoadMin, LoadMax);

	// Reference Executable (Makes sure that it isn't unloaded)
	Binary->ReferenceCount ++;
	
	// Get Binary Base
	base = Binary->Base;
	
	// Check if base is free
	if(base != 0)
	{
		LOG("Checking base %p", base);
		for( i = 0; i < Binary->NumSections; i ++ )
		{
			if( Binary_int_CheckMemFree( Binary->LoadSections[i].Virtual, Binary->LoadSections[i].MemSize ) )
			{
				base = 0;
				LOG("Address 0x%x is taken\n", Binary->LoadSections[i].Virtual);
				break;
			}
		}
	}
	
	// Check if the executable has no base or it is not free
	if(base == 0)
	{
		// If so, give it a base
		base = LoadMax;
		while(base >= LoadMin)
		{
			for( i = 0; i < Binary->NumSections; i ++ )
			{
				tVAddr	addr = Binary->LoadSections[i].Virtual - Binary->Base + base;
				size_t	size = Binary->LoadSections[i].MemSize;
				if( addr + size > LoadMax )
					break;
				if( Binary_int_CheckMemFree( addr, size ) )
					break;
			}
			// If space was found, break
			if(i == Binary->NumSections)		break;
			// Else decrement pointer and try again
			base -= BIN_GRANUALITY;
		}
		LOG("Allocated base %p", base);
	}
	
	// Error Check
	if(base < LoadMin) {
		Log_Warning("Binary", "Executable '%s' cannot be loaded, no space", Path);
		LEAVE('i', 0);
		return 0;
	}
	
	// Map Executable In
	if( Binary->MountID )
		fd = VFS_OpenInode(Binary->MountID, Binary->Inode, VFS_OPENFLAG_READ);
	else
		fd = VFS_Open(Path, VFS_OPENFLAG_READ);
	for( i = 0; i < Binary->NumSections; i ++ )
	{
		tBinarySection	*sect = &Binary->LoadSections[i];
		Uint	protflags, mapflags;
		tVAddr	addr = sect->Virtual - Binary->Base + base;
		LOG("%i - %p, 0x%x bytes from offset 0x%llx (%x)", i, addr, sect->FileSize, sect->Offset, sect->Flags);

		protflags = MMAP_PROT_READ;
		mapflags = MMAP_MAP_FIXED;

		if( sect->Flags & BIN_SECTFLAG_EXEC )
			protflags |= MMAP_PROT_EXEC;
		// Read only pages are COW
		if( sect->Flags & BIN_SECTFLAG_RO  ) {
			VFS_MMap( (void*)addr, sect->FileSize, protflags, MMAP_MAP_SHARED|mapflags, fd, sect->Offset );
		}
		else {
			protflags |= MMAP_PROT_WRITE;
			VFS_MMap( (void*)addr, sect->FileSize, protflags, MMAP_MAP_PRIVATE|mapflags, fd, sect->Offset );
		}
		
		// Apply anonymous memory for BSS
		if( sect->FileSize < sect->MemSize ) {
			mapflags |= MMAP_MAP_ANONYMOUS;
			VFS_MMap(
				(void*)(addr + sect->FileSize), sect->MemSize - sect->FileSize,
				protflags, MMAP_MAP_PRIVATE|mapflags,
				0, 0
				);
		}
	}
	
	Log_Debug("Binary", "PID %i - Mapped '%s' to %p", Threads_GetPID(), Path, base);
	VFS_Close(fd);
	
	LEAVE('p', base);
	return base;
}

#if 0
/**
 * \fn Uint Binary_IsMapped(tBinary *binary)
 * \brief Check if a binary is already mapped into the address space
 * \param binary	Binary information to check
 * \return Current Base or 0
 */
Uint Binary_IsMapped(tBinary *binary)
{
	Uint	iBase;
	
	// Check prefered base
	iBase = binary->Base;
	if(MM_GetPage( iBase ) == (binary->Pages[0].Physical & ~0xFFF))
		return iBase;
	
	for(iBase = BIN_HIGHEST;
		iBase >= BIN_LOWEST;
		iBase -= BIN_GRANUALITY)
	{
		if(MM_GetPage( iBase ) == (binary->Pages[0].Physical & ~0xFFF))
			return iBase;
	}
	
	return 0;
}
#endif

/**
 * \fn tBinary *Binary_DoLoad(char *truePath)
 * \brief Loads a binary file into memory
 * \param truePath	Absolute filename of binary
 */
tBinary *Binary_DoLoad(tMount MountID, tInode Inode, const char *Path)
{
	tBinary	*pBinary;
	 int	fp;
	Uint32	ident;
	tBinaryType	*bt = gRegBinTypes;
	
	ENTER("iMountID XInode sPath", MountID, Inode, Path);
	
	// Open File
	if( MountID )
	{
		fp = VFS_OpenInode(MountID, Inode, VFS_OPENFLAG_READ);
	}
	else
	{
		fp = VFS_Open(Path, VFS_OPENFLAG_READ);
	}
	if(fp == -1) {
		LOG("Unable to load file, access denied");
		LEAVE('n');
		return NULL;
	}

	LOG("fp = 0x%x", fp);
	
	// Read File Type
	VFS_Read(fp, 4, &ident);
	VFS_Seek(fp, 0, SEEK_SET);

	LOG("ident = 0x%x", ident);

	// Determine the type	
	for(; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) != (Uint32)bt->Ident )
			continue;
		LOG("bt = %p (%s)", bt, bt->Name);
		pBinary = bt->Load(fp);
		break;
	}

	// Close File
	VFS_Close(fp);
	
	// Catch errors
	if(!bt) {
		Log_Warning("Binary", "'%s' is an unknown file type. (%02x %02x %02x %02x)",
			Path, ident&0xFF, (ident>>8)&0xFF, (ident>>16)&0xFF, (ident>>24)&0xFF);
		LEAVE('n');
		return NULL;
	}
	
	LOG("pBinary = %p", pBinary);
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('n');
		return NULL;
	}
	
	// Initialise Structure
	pBinary->ReferenceCount = 0;
	pBinary->MountID = MountID;
	pBinary->Inode = Inode;
	
	// Debug Information
	LOG("Interpreter: '%s'", pBinary->Interpreter);
	LOG("Base: 0x%x, Entry: 0x%x", pBinary->Base, pBinary->Entry);
	LOG("NumSections: %i", pBinary->NumSections);
	
	// Add to the list
	SHORTLOCK(&glBinListLock);
	pBinary->Next = glLoadedBinaries;
	glLoadedBinaries = pBinary;
	SHORTREL(&glBinListLock);

	// TODO: Register the path with the binary	

	// Return
	LEAVE('p', pBinary);
	return pBinary;
}

/**
 * \fn void Binary_Unload(void *Base)
 * \brief Unload / Unmap a binary
 * \param Base	Loaded Base
 * \note Currently used only for kernel libaries
 */
void Binary_Unload(void *Base)
{
	tKernelBin	*pKBin;
	tKernelBin	*prev = NULL;
	 int	i;
	
	if((Uint)Base < 0xC0000000)
	{
		// TODO: User Binaries
		Log_Warning("BIN", "Unloading user binaries is currently unimplemented");
		return;
	}
	
	// Kernel Libraries
	for(pKBin = glLoadedKernelLibs;
		pKBin;
		prev = pKBin, pKBin = pKBin->Next)
	{
		// Check the base
		if(pKBin->Base != Base)	continue;
		// Deallocate Memory
		for(i = 0; i < pKBin->Info->NumSections; i++)
		{
			// TODO: VFS_MUnmap();
		}
		// Dereference Binary
		Binary_Dereference( pKBin->Info );
		// Remove from list
		if(prev)	prev->Next = pKBin->Next;
		else		glLoadedKernelLibs = pKBin->Next;
		// Free Kernel Lib
		free(pKBin);
		return;
	}
}

/**
 * \fn void Binary_Dereference(tBinary *Info)
 * \brief Dereferences and if nessasary, deletes a binary
 * \param Info	Binary information structure
 */
void Binary_Dereference(tBinary *Info)
{
	// Decrement reference count
	Info->ReferenceCount --;
	
	// Check if it is still in use
	if(Info->ReferenceCount)	return;
	
	/// \todo Implement binary freeing
}

/**
 * \fn char *Binary_RegInterp(char *Path)
 * \brief Registers an Interpreter
 * \param Path	Path to interpreter provided by executable
 */
char *Binary_RegInterp(char *Path)
{
	 int	i;
	// NULL Check Argument
	if(Path == NULL)	return NULL;
	// NULL Check the array
	if(gsaRegInterps == NULL)
	{
		giRegInterps = 1;
		gsaRegInterps = malloc( sizeof(char*) );
		gsaRegInterps[0] = malloc( strlen(Path) );
		strcpy(gsaRegInterps[0], Path);
		return gsaRegInterps[0];
	}
	
	// Scan Array
	for( i = 0; i < giRegInterps; i++ )
	{
		if(strcmp(gsaRegInterps[i], Path) == 0)
			return gsaRegInterps[i];
	}
	
	// Interpreter is not in list
	giRegInterps ++;
	gsaRegInterps = malloc( sizeof(char*)*giRegInterps );
	gsaRegInterps[i] = malloc( strlen(Path) );
	strcpy(gsaRegInterps[i], Path);
	return gsaRegInterps[i];
}

// ============
// Kernel Binary Handling
// ============
/**
 * \fn void *Binary_LoadKernel(const char *File)
 * \brief Load a binary into kernel space
 * \note This function shares much with #Binary_Load, but does it's own mapping
 * \param File	File to load into the kernel
 */
void *Binary_LoadKernel(const char *File)
{
	tBinary	*pBinary;
	tKernelBin	*pKBinary;
	tVAddr	base = -1;
	tMount	mount_id;
	tInode	inode;

	ENTER("sFile", File);
	
	// Sanity Check Argument
	if(File == NULL) {
		LEAVE('n');
		return 0;
	}

	{
		int fd = VFS_Open(File, VFS_OPENFLAG_READ);
		tFInfo	info;
		if(fd == -1) {
			LOG("Opening failed");
			LEAVE('n');
			return NULL;
		}
		VFS_FInfo(fd, &info, 0);
		mount_id = info.mount;
		inode = info.inode;
		VFS_Close(fd);
		LOG("Mount %i, Inode %lli", mount_id, inode);
	}
	
	// Check if the binary has already been loaded
	if( (pBinary = Binary_GetInfo(mount_id, inode)) )
	{
		for(pKBinary = glLoadedKernelLibs;
			pKBinary;
			pKBinary = pKBinary->Next )
		{
			if(pKBinary->Info == pBinary) {
				LOG("Already loaded");
				LEAVE('p', pKBinary->Base);
				return pKBinary->Base;
			}
		}
	}
	else
		pBinary = Binary_DoLoad(mount_id, inode, File);	// Else load it
	
	// Error Check
	if(pBinary == NULL) {
		LEAVE('n');
		return NULL;
	}
	
	LOG("Loaded as %p", pBinary);
	// --------------
	// Now pBinary is valid (either freshly loaded or only user mapped)
	// So, map it into kernel space
	// --------------
	
	// Reference Executable (Makes sure that it isn't unloaded)
	pBinary->ReferenceCount ++;

	base = Binary_MapIn(pBinary, File, KLIB_LOWEST, KLIB_HIGHEST);
	if( base == 0 ) {
		LEAVE('n');
		return 0;
	}

	// Add to list
	// TODO: Could this cause race conditions if a binary isn't fully loaded when used
	pKBinary = malloc(sizeof(*pKBinary));
	pKBinary->Base = (void*)base;
	pKBinary->Info = pBinary;
	SHORTLOCK( &glKBinListLock );
	pKBinary->Next = glLoadedKernelLibs;
	glLoadedKernelLibs = pKBinary;
	SHORTREL( &glKBinListLock );

	LEAVE('p', base);
	return (void*)base;
}

/**
 * \fn Uint Binary_Relocate(void *Base)
 * \brief Relocates a loaded binary (used by kernel libraries)
 * \param Base	Loaded base address of binary
 * \return Boolean Success
 */
Uint Binary_Relocate(void *Base)
{
	Uint32	ident = *(Uint32*) Base;
	tBinaryType	*bt = gRegBinTypes;
	
	for( ; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) == (Uint)bt->Ident )
			return bt->Relocate( (void*)Base);
	}
	
	Log_Warning("BIN", "%p is an unknown file type. (%02x %02x %02x %02x)",
		Base, ident&0xFF, (ident>>8)&0xFF, (ident>>16)&0xFF, (ident>>24)&0xFF);
	return 0;
}

/**
 * \fn int Binary_GetSymbol(char *Name, Uint *Val)
 * \brief Get a symbol value
 * \return Value of symbol or -1 on error
 * 
 * Gets the value of a symbol from either the currently loaded
 * libraries or the kernel's exports.
 */
int Binary_GetSymbol(const char *Name, Uint *Val)
{
	if( Binary_GetSymbolEx(Name, Val) )	return 1;
	return 0;
}

/**
 * \fn Uint Binary_GetSymbolEx(char *Name, Uint *Value)
 * \brief Get a symbol value
 * 
 * Gets the value of a symbol from either the currently loaded
 * libraries or the kernel's exports.
 */
Uint Binary_GetSymbolEx(const char *Name, Uint *Value)
{
	tKernelBin	*pKBin;
	 int	numKSyms = ((Uint)&gKernelSymbolsEnd-(Uint)&gKernelSymbols)/sizeof(tKernelSymbol);
	
	// Scan Kernel
	for( int i = 0; i < numKSyms; i++ )
	{
		if(strcmp(Name, gKernelSymbols[i].Name) == 0) {
			LOG("KSym %s = %p", gKernelSymbols[i].Name, gKernelSymbols[i].Value);
			*Value = gKernelSymbols[i].Value;
			return 1;
		}
	}
	
	
	// Scan Loaded Libraries
	for(pKBin = glLoadedKernelLibs;
		pKBin;
		pKBin = pKBin->Next )
	{
		if( Binary_FindSymbol(pKBin->Base, Name, Value) ) {
			return 1;
		}
	}
	
	Log_Warning("BIN", "Unable to find symbol '%s'", Name);
	return 0;
}

/**
 * \fn Uint Binary_FindSymbol(void *Base, char *Name, Uint *Val)
 * \brief Get a symbol from the specified library
 * \param Base	Base address
 * \param Name	Name of symbol to find
 * \param Val	Pointer to place final value
 */
Uint Binary_FindSymbol(void *Base, const char *Name, Uint *Val)
{
	Uint32	ident = *(Uint32*) Base;
	tBinaryType	*bt = gRegBinTypes;
	
	for(; bt; bt = bt->Next)
	{
		if( (ident & bt->Mask) == (Uint)bt->Ident )
			return bt->GetSymbol(Base, Name, Val);
	}
	
	Log_Warning("BIN", "Binary_FindSymbol - %p is an unknown file type. (%02x %02x %02x %02x)",
		Base, ident&0xFF, ident>>8, ident>>16, ident>>24);
	return 0;
}

/**
 * \brief Check if a range of memory is fully free
 * \return Inverse boolean free (0 if all pages are unmapped)
 */
int Binary_int_CheckMemFree( tVAddr _start, size_t _len )
{
	ENTER("p_start x_len", _start, _len);

	_len += _start & (PAGE_SIZE-1);
	_len = (_len + PAGE_SIZE - 1) & ~(PAGE_SIZE-1);
	_start &= ~(PAGE_SIZE-1);
	LOG("_start = %p, _len = 0x%x", _start, _len);
	for( ; _len > PAGE_SIZE; _len -= PAGE_SIZE, _start += PAGE_SIZE ) {
		if( MM_GetPhysAddr( (void*)_start ) != 0 ) {
			LEAVE('i', 1);
			return 1;
		}
	}
	if( _len == PAGE_SIZE && MM_GetPhysAddr( (void*)_start ) != 0 ) {
		LEAVE('i', 1);
		return 1;
	}
	LEAVE('i', 0);
	return 0;
}


// === EXPORTS ===
EXPORT(Binary_FindSymbol);
EXPORT(Binary_Unload);

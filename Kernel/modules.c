/*
 * Acess2
 * - Module Loader
 */
#include <acess.h>
#include <modules.h>

#define	USE_EDI	0
#define	USE_UDI	1

// === PROTOTYPES ===
 int	Modules_LoadBuiltins();
 int	Module_LoadMem(void *Buffer, Uint Length, char *ArgString);
 int	Module_LoadFile(char *Path, char *ArgString);
 int	Module_int_ResolveDeps(tModule *Info);
 int	Module_IsLoaded(char *Name);

// === IMPORTS ===
#if USE_UDI
extern int	UDI_LoadDriver(void *Base);
#endif
extern void	StartupPrint(char *Str);
extern tModule	gKernelModules[];
extern void	gKernelModulesEnd;

// === GLOBALS ===
 int	giNumBuiltinModules = 0;
 int	giModuleSpinlock = 0;
tModule	*gLoadedModules = NULL;
tModuleLoader	*gModule_Loaders = NULL;

// === CODE ===
int Modules_LoadBuiltins()
{
	 int	i, j, k;
	 int	ret;
	 int	numToInit = 0;
	Uint8	*baIsLoaded;
	char	**deps;
	
	// Count modules
	giNumBuiltinModules = (Uint)&gKernelModulesEnd - (Uint)&gKernelModules;
	giNumBuiltinModules /= sizeof(tModule);
	
	// Allocate loaded array
	baIsLoaded = calloc( giNumBuiltinModules, sizeof(*baIsLoaded) );
	
	// Pass 1 - Check for dependencies
	for( i = 0; i < giNumBuiltinModules; i++ )
	{
		deps = gKernelModules[i].Dependencies;
		if(deps)
		{
			for( j = 0; deps[j]; j++ )
			{
				for( k = 0; k < giNumBuiltinModules; k++ ) {
					if(strcmp(deps[j], gKernelModules[k].Name) == 0)
						break;
				}
				if(k == giNumBuiltinModules) {
					Warning(
						"[MOD  ] Dependency '%s' for module '%s' was not compiled in",
						deps[j], gKernelModules[i].Name
						);
					
					baIsLoaded[i] = -1;	// Don't Load
					break;
				}
			}
		}
		numToInit ++;
	}
	
	// Pass 2 - Intialise modules in order
	while(numToInit)
	{
		for( i = 0; i < giNumBuiltinModules; i++ )
		{
			if( baIsLoaded[i] )	continue;	// Ignore already loaded modules
		
			deps = gKernelModules[i].Dependencies;
			
			if( deps )
			{
				for( j = 0; deps[j]; j++ )
				{
					for( k = 0; k < giNumBuiltinModules; k++ ) {
						if(strcmp(deps[j], gKernelModules[k].Name) == 0)
							break;
					}
					// `k` is assumed to be less than `giNumBuiltinModules`
					// We checked this in pass 1
					
					// If a dependency failed, skip and mark as failed
					if( baIsLoaded[k] == -1 ) {
						baIsLoaded[i] = -1;
						numToInit --;
						break;
					}
					// If a dependency is not intialised, skip this module
					// and come back later
					if( !baIsLoaded[k] )	break;
				}
				// Check for breakouts
				if( deps[j] )	continue;
			}
			
			// All Dependencies OK? Initialise
			StartupPrint(gKernelModules[i].Name);
			Log("[MOD  ] Initialising %p '%s' v%i.%i...",
				&gKernelModules[i],
				gKernelModules[i].Name,
				gKernelModules[i].Version>>8, gKernelModules[i].Version & 0xFF
				);
			
			ret = gKernelModules[i].Init(NULL);
			if( ret != MODULE_ERR_OK ) {
				Log("[MOD  ] Loading Failed, all modules that depend on this will also fail");
				switch(ret)
				{
				case MODULE_ERR_MISC:
					Log("[MOD  ] Reason: Miscelanious");
					break;
				case MODULE_ERR_NOTNEEDED:
					Log("[MOD  ] Reason: Module not needed (probably hardware not found)");
					break;
				case MODULE_ERR_MALLOC:
					Log("[MOD  ] Reason: Error in malloc/realloc/calloc, probably not good");
					break;
				default:
					Log("[MOD  ] Reason - Unknown code %i", ret);
					break;
				}
				baIsLoaded[i] = -1;
			}
			// Mark as loaded
			else
				baIsLoaded[i] = 1;
			numToInit --;
		}
	}
	
	free(baIsLoaded);
	
	return 0;
}

/**
 * \brief Registers a tModuleLoader with the kernel
 * \param Loader	Pointer to loader structure (must be persistent)
 */
int Module_RegisterLoader(tModuleLoader *Loader)
{
	if(!Loader)	return 1;
	
	Loader->Next = gModule_Loaders;
	gModule_Loaders = Loader;
	
	return 0;
}

/**
 * \fn int Module_LoadMem(void *Buffer, Uint Length, char *ArgString)
 * \brief Load a module from a memory location
 */
int Module_LoadMem(void *Buffer, Uint Length, char *ArgString)
{
	char	path[VFS_MEMPATH_SIZE];
	
	VFS_GetMemPath(path, Buffer, Length);
	
	return Module_LoadFile( path, ArgString );
}

/**
 * \fn int Module_LoadFile(char *Path, char *ArgString)
 * \brief Load a module from a file
 */
int Module_LoadFile(char *Path, char *ArgString)
{
	void	*base;
	tModule	*info;
	
	// Load Binary
	base = Binary_LoadKernel(Path);
	
	// Error check
	if(base == NULL) {
		Warning("Module_LoadFile: Unable to load '%s'", Path);
		return 0;
	}
	
	// Check for Acess Driver
	if( Binary_FindSymbol(base, "DriverInfo", (Uint*)&info ) == 0 )
	{
		tModuleLoader	*tmp;
		for( tmp = gModule_Loaders; tmp; tmp = tmp->Next)
		{
			if( tmp->Detector(base) == 0 )	continue;
			
			return tmp->Loader(base);
		}
		
		#if USE_EDI
		// Check for EDI Driver
		if( Binary_FindSymbol(base, "driver_init", NULL ) != 0 )
		{
			return Module_InitEDI( base );	// And intialise
		}
		#endif
		
		// Unknown module type?, return error
		Binary_Unload(base);
		#if USE_EDI
		Warning("Module_LoadFile: Module has neither a Module Info struct, nor an EDI entrypoint");
		#else
		Warning("Module_LoadFile: Module does not have a Module Info struct");
		#endif
		return 0;
	}
	
	// Check magic number
	if(info->Magic != MODULE_MAGIC)
	{
		Warning("Module_LoadFile: Module's magic value is invalid (0x%x != 0x%x)", info->Magic, MODULE_MAGIC);
		return 0;
	}
	
	// Check Architecture
	if(info->Arch != MODULE_ARCH_ID)
	{
		Warning("Module_LoadFile: Module is for a different architecture");
		return 0;
	}
	
	// Resolve Dependencies
	if( !Module_int_ResolveDeps(info) ) {
		Binary_Unload(base);
		return 0;
	}
	
	Log("Initialising %p '%s' v%i.%i...",
				info,
				info->Name,
				info->Version>>8, info->Version & 0xFF
				);
	
	// Call Initialiser
	//if( info->Init( ArgString ) != 0 )
	if( info->Init( NULL ) == 0 )
	{
		Binary_Unload(base);
		return 0;
	}
	
	// Add to list
	LOCK( &giModuleSpinlock );
	info->Next = gLoadedModules;
	gLoadedModules = info;
	RELEASE( &giModuleSpinlock );
	
	return 1;
}

/**
 * \fn int Module_int_ResolveDeps(tModule *Info)
 * \brief Resolves the dependencies
 * \todo Implement
 * \note Currently does not resolve the dependencies, just checks them
 */
int Module_int_ResolveDeps(tModule *Info)
{
	char	**names = Info->Dependencies;
	
	// Walk dependencies array
	for( ; *names; names++ )
	{
		// Check if the module is loaded
		if( !Module_IsLoaded(*names) ) {
			Warning("Module `%s' requires `%s', which is not loaded\n", Info->Name, *names);
			return 0;
		}
	}
	return 1;
}

/**
 * \fn int Module_IsLoaded(char *Name)
 * \brief Checks if a module is loaded
 * \param Name	Name of module to find
 */
int Module_IsLoaded(char *Name)
{
	tModule	*mod = gLoadedModules;
	
	// Scan loaded list
	for( ; mod; mod = mod->Next )
	{
		// If found, return true
		if(strcmp(mod->Name, Name) == 0)
			return 1;
	}
	// not found - return false
	return 0;
}

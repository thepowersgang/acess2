/*
 * Acess2
 * - Module Loader
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>

#define	USE_EDI	0
#define	USE_UDI	0

// === PROTOTYPES ===
 int	Module_int_Initialise(tModule *Module, char *ArgString);
void	Modules_LoadBuiltins(void);
void	Modules_SetBuiltinParams(char *Name, char *ArgString);
 int	Module_RegisterLoader(tModuleLoader *Loader);
 int	Module_LoadMem(void *Buffer, Uint Length, char *ArgString);
 int	Module_LoadFile(char *Path, char *ArgString);
 int	Module_int_ResolveDeps(tModule *Info);
 int	Module_IsLoaded(const char *Name);

// === EXPORTS ===
EXPORT(Module_RegisterLoader);

// === IMPORTS ===
#if USE_UDI
extern int	UDI_LoadDriver(void *Base);
#endif
extern void	StartupPrint(char *Str);
extern void	gKernelModules;
extern void	gKernelModulesEnd;

// === GLOBALS ===
 int	giNumBuiltinModules = 0;
tShortSpinlock	glModuleSpinlock;
tModule	*gLoadedModules = NULL;
tModuleLoader	*gModule_Loaders = NULL;
tModule	*gLoadingModules = NULL;
tModule	**gapBuiltinModules = NULL;
char	**gasBuiltinModuleArgs;

// === CODE ===
/**
 * \brief Initialises a module
 * \param Module	Pointer to the module header
 * \param ArgString	Comma separated list of module arguments
 * \return Zero on success, eModuleErrors or -1 on error
 * \retval -1	Returned if a dependency fails, or a circular dependency
 *              exists.
 * \retval 0	Returned on success
 * \retval >0	Error code form the module's initialisation function
 */
int Module_int_Initialise(tModule *Module, char *ArgString)
{
	 int	i, j;
	 int	ret;
	char	**deps;
	char	**args;
	tModule	*mod;
	
	ENTER("pModule", Module);
	LOG("Module->Magic = 0x%x", Module->Magic);
	if(Module->Magic != MODULE_MAGIC) {
		Log_Warning(
			"Module",
			"Module %p is no a valid Acess2 module (0x%08x != 0x%08x)",
			Module, Module->Magic, MODULE_MAGIC
			);
		LEAVE('i', MODULE_ERR_BADMODULE);
		return MODULE_ERR_BADMODULE;
	}
	LOG("Module->Name = %p \"%s\"", Module->Name, Module->Name);
	
	if(Module->Arch != MODULE_ARCH_ID) {
		Log_Warning(
			"Module",
			"Module %p (%s) is for another architecture (%i)",
			Module, Module->Name, Module->Arch
			);
	}
	
	deps = Module->Dependencies;
	
	// Check if the module has been loaded
	for( mod = gLoadedModules; mod; mod = mod->Next )
	{
		if(mod == Module)	LEAVE_RET('i', 0);
	}
	
	// Add to the "loading" (prevents circular deps)
	Module->Next = gLoadingModules;
	gLoadingModules = Module;
	
	// Scan dependency list
	for( j = 0; deps && deps[j]; j++ )
	{
		// Check if the module is already loaded
		for( mod = gLoadedModules; mod; mod = mod->Next )
		{
			if(strcmp(deps[j], mod->Name) == 0)
				break;
		}
		if( mod )	continue;	// Dependency is loaded, check the rest
		
		// Ok, check if it's loading
		for( mod = gLoadingModules->Next; mod; mod = mod->Next )
		{
			if(strcmp(deps[j], mod->Name) == 0)
				break;
		}
		if( mod ) {
			Log_Warning("Module", "Circular dependency detected (%s and %s)",
				mod->Name, Module->Name);
			LEAVE_RET('i', -1);
		}
		
		// So, if it's not loaded, we better load it then
		for( i = 0; i < giNumBuiltinModules; i ++ )
		{
			if( strcmp(deps[j], gapBuiltinModules[i]->Name) == 0 )
				break;
		}
		if( i == giNumBuiltinModules ) {
			Log_Warning("Module", "Dependency '%s' for module '%s' failed",
				deps[j], Module->Name);
			return -1;
		}
		
		// Dependency is not loaded, so load it
		ret = Module_int_Initialise(
			gapBuiltinModules[i],
			gasBuiltinModuleArgs ? gasBuiltinModuleArgs[i] : NULL
			);
		if( ret )
		{
			// The only "ok" error is NOTNEEDED
			if(ret != MODULE_ERR_NOTNEEDED)
				LEAVE_RET('i', -1);
		}
	}
	
	// All Dependencies OK? Initialise
	StartupPrint(Module->Name);
	Log_Log("Module", "Initialising %p '%s' v%i.%i...",
		Module, Module->Name,
		Module->Version >> 8, Module->Version & 0xFF
		);
	
	if( ArgString )
		args = str_split( ArgString, ',' );
	else
		args = NULL;
	
	ret = Module->Init(args);
	
	if(args)	free(args);
	
	// Remove from loading list
	gLoadingModules = gLoadingModules->Next;
	
	if( ret != MODULE_ERR_OK ) {
		switch(ret)
		{
		case MODULE_ERR_MISC:
			Log_Warning("Module", "Unable to load, reason: Miscelanious");
			break;
		case MODULE_ERR_NOTNEEDED:
			Log_Warning("Module", "Unable to load, reason: Module not needed");
			break;
		case MODULE_ERR_MALLOC:
			Log_Warning("Module", "Unable to load, reason: Error in malloc/realloc/calloc, probably not good");
			break;
		default:
			Log_Warning("Module", "Unable to load reason - Unknown code %i", ret);
			break;
		}
		LEAVE_RET('i', ret);
		return ret;
	}
	LOG("ret = %i", ret);
	
	// Add to loaded list
	SHORTLOCK( &glModuleSpinlock );
	Module->Next = gLoadedModules;
	gLoadedModules = Module;
	SHORTREL( &glModuleSpinlock );
	
	LEAVE_RET('i', 0);
}

/**
 * \brief Scans the builtin modules and creates an array of them
 */
void Modules_int_GetBuiltinArray(void)
{
	 int	i;
	tModule	*module;
	
	// Count
	module = &gKernelModules;
	i = 0;
	while( (tVAddr)module < (tVAddr)&gKernelModulesEnd )
	{
		if(module->Magic == MODULE_MAGIC) {
			i ++;
			module ++;
		}
		else
			module = (void*)( (tVAddr)module + 4 );
	}
	
	// Create
	giNumBuiltinModules = i;
	gasBuiltinModuleArgs = calloc( giNumBuiltinModules, sizeof(char*) );
	gapBuiltinModules = malloc( giNumBuiltinModules * sizeof(tModule*) );
	
	
	// Fill
	module = &gKernelModules;
	i = 0;
	while( (tVAddr)module < (tVAddr)&gKernelModulesEnd )
	{
		if(module->Magic == MODULE_MAGIC) {
			gapBuiltinModules[i] = module;
			i ++;
			module ++;
		}
		else
			module = (void*)( (tVAddr)module + 4 );
	}
}

/**
 * \brief Initialises builtin modules
 */
void Modules_LoadBuiltins()
{
	 int	i;
	
	if( !gapBuiltinModules )
		Modules_int_GetBuiltinArray();
	
	for( i = 0; i < giNumBuiltinModules; i++ )
	{
		Module_int_Initialise(
			gapBuiltinModules[i],
			(gasBuiltinModuleArgs ? gasBuiltinModuleArgs[i] : NULL)
			);
	}
	
	if( gasBuiltinModuleArgs != NULL )
		free(gasBuiltinModuleArgs);
}

/**
 * \brief Initialise a builtin module given it's name
 * 
 * E.g. Used by VTerm to load an alternate video driver at runtime
 */
int Modules_InitialiseBuiltin(const char *Name)
{
	 int	i;
	
	// Check if it's loaded
	if( Module_IsLoaded(Name) )
		return 0;
	
	if( !gapBuiltinModules )
		Modules_int_GetBuiltinArray();
	
	for( i = 0; i < giNumBuiltinModules; i++ )
	{
		if( strcmp(gapBuiltinModules[i]->Name, Name) == 0 ) {
			return Module_int_Initialise(gapBuiltinModules[i],
				(gasBuiltinModuleArgs ? gasBuiltinModuleArgs[i] : NULL)
				);
		}
	}
	return -1;
}

/**
 * \brief Sets the parameters for a builtin module
 */
void Modules_SetBuiltinParams(char *Name, char *ArgString)
{
	 int	i;
	
	if( gasBuiltinModuleArgs == NULL )
	{
		Modules_int_GetBuiltinArray();
	}
	
	// I hate expensive scans
	for( i = 0; i < giNumBuiltinModules; i++ )
	{
		if(strcmp( gapBuiltinModules[i]->Name, Name ) == 0) {
			gasBuiltinModuleArgs[i] = ArgString;
			return ;
		}
	}
	
	Log_Warning("Modules", "Unknown builtin kernel module '%s'", Name);
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
		Log_Warning("Module", "Module_LoadFile - Unable to load '%s'", Path);
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
		Log_Warning("Module", "Module '%s' has neither a Module Info struct, nor an EDI entrypoint", Path);
		#else
		Log_Warning("Module", "Module '%s' does not have a Module Info struct", Path);
		#endif
		return 0;
	}
	
	// Initialise (and register)
	if( Module_int_Initialise( info, ArgString ) )
	{
		Binary_Unload(base);
		return 0;
	}
	
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
			Log_Warning("Module", "Module `%s' requires `%s', which is not loaded\n", Info->Name, *names);
			return 0;
		}
	}
	return 1;
}

/**
 * \fn int Module_IsLoaded(const char *Name)
 * \brief Checks if a module is loaded
 * \param Name	Name of module to find
 */
int Module_IsLoaded(const char *Name)
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

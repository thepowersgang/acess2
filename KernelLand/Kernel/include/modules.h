/*
 * AcessOS 2
 * - Module Loader
 */
/**
 * \file modules.h
 * \brief Module Handling and Loader Definitions
 * \author John Hodge (thePowersGang)
 * 
 * This file serves two pourposes. First it defines the format for native
 * Acess2 modules and the functions to create them.
 * Second, it defines the structure and register function for new module
 * loaders, allowing Acess to understand many different module / driver
 * formats.
 * 
 * Modules are defined by including this file in the module's main source
 * file and using the ::MODULE_DEFINE macro to create the module header.
 * 
 * To register a new module loader with the kernel, the loader module must
 * create and populate an instance of ::tModuleLoader then pass it to
 * ::Module_RegisterLoader
 */
#ifndef _MODULE_H
#define _MODULE_H

/**
 * \brief Module header magic value
 */
#define MODULE_MAGIC	('A'|('M'<<8)|('D'<<16)|('\2'<<24))

/**
 * \def MODULE_ARCH_ID
 * \brief Architecture ID
 */
// IA32 - Architecture 1
#if ARCHDIR == x86
# define MODULE_ARCH_ID	1
// IA64 - Architecture 2
#elif ARCHDIR == x86_64
# define MODULE_ARCH_ID	2
#else
# error "Unknown architecture when determining MODULE_ARCH_ID ('" #ARCHDIR "')"
#endif

/**
 * \brief Define a module
 * \param _flags	Module Flags
 * \param _ver	Module Version
 * \param _ident	Unique Module Name
 * \param _entry	Module initialiser / entrypoint
 * \param _deinit	Module cleanup / unloader
 * \param _deps	NULL terminated list of this's module's dependencies
 *             	Contains the identifiers of the required modules.
 */
#define MODULE_DEFINE(_flags,_ver,_ident,_entry,_deinit,_deps...) \
	const char *EXPAND_CONCAT(_DriverDeps_,_ident)[]={_deps};\
	tModule __attribute__ ((section ("KMODULES"),unused))\
	EXPAND_CONCAT(_DriverInfo_,_ident)=\
	{MODULE_MAGIC,MODULE_ARCH_ID,_flags,_ver,NULL,EXPAND_STR(_ident),\
	_entry,_deinit,EXPAND_CONCAT(_DriverDeps_,_ident)}

/**
 * \brief Module header
 * \note There is no reason for a module to touch this structure beyond
 *       using ::MODULE_DEFINE to create it.
 */
typedef struct sModule 
{
	Uint32	Magic;	//!< Identifying magic value (See ::MODULE_MAGIC)
	Uint8	Arch;	//!< Achitecture ID (See ::MODULE_ARCH_ID)
	Uint8	Flags;	//!< Module Flags
	Uint16	Version;	//!< Module Version in Major.Minor 8.8 form
	struct sModule	*Next;	//!< Next module in list (not to be touched by the driver)
	const char	*Name;	//!< Module Name/Identifier
	 int	(*Init)(char **Arguments);	//!< Module initialiser / entrypoint
	void	(*Deinit)(void);	//!< Cleanup Function
	const char	**Dependencies;	//!< NULL terminated list of dependencies
} PACKED tModule;

/**
 * \brief Return values for tModule.Init
 */
enum eModuleErrors
{
	MODULE_ERR_OK,	//!< No Error
	MODULE_ERR_MISC,	//!< Misc Error
	MODULE_ERR_NOTNEEDED,	//!< Module not needed
	MODULE_ERR_MALLOC,	//!< Error with malloc/realloc/calloc
	
	MODULE_ERR_BADMODULE,	//!< Bad module (only used by loader)
	MODULE_ERR_MAX	//!< Maximum defined error code
};

/**
 * \brief Module Loader definition
 * 
 * Allows a module to extend the loader to recognise other module types
 * E.g. EDI, UDI, Windows, Linux, ...
 */
typedef struct sModuleLoader
{
	struct sModuleLoader	*Next;	//!< Kernel Only - Next loader in list
	char	*Name;	//!< Friendly name for the loader
	 int	(*Detector)(void *Base);	//!< Simple detector function
	 int	(*Loader)(void *Base);	//!< Initialises the module
	 int	(*Unloader)(void *Base);	//!< Calls module's cleanup
} PACKED tModuleLoader;

/**
 * \brief Registers a tModuleLoader with the kernel
 * \param Loader	Pointer to loader structure (must be persistent)
 * \return Boolean Success
 */
extern int	Module_RegisterLoader(tModuleLoader *Loader);

/**
 * \brief Initialises (if needed) a named module
 * \param Name	Module name to initialise
 * \return -1 on not existing, 0 if the module initialised (or if it was already initialised)
 */
extern int	Module_EnsureLoaded(const char *Name);

#endif

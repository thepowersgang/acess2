/*
 * AcessOS 2
 * - Module Loader
 */
#ifndef _MODULE_H
#define _MODULE_H

#define MODULE_MAGIC	('A'|('M'<<8)|('D'<<16)|('\2'<<24))

// IA32 - Architecture 1
#if ARCHDIR == x86
# define MODULE_ARCH_ID	1
// IA64 - Architecture 2
#elif ARCHDIR == x64
# define MODULE_ARCH_ID	2
#else
# error "Unknown architecture when determining MODULE_ARCH_ID ('" #ARCHDIR "')"
#endif

#define MODULE_DEFINE(_flags,_ver,_ident,_entry,_deinit,_deps...) \
	char *EXPAND_CONCAT(_DriverDeps_,_ident)[]={_deps};\
	tModule __attribute__ ((section ("KMODULES"),unused))\
	EXPAND_CONCAT(_DriverInfo_,_ident)=\
	{MODULE_MAGIC,MODULE_ARCH_ID,_flags,_ver,NULL,EXPAND_STR(_ident),\
	_entry,_deinit,EXPAND_CONCAT(_DriverDeps_,_ident)}

typedef struct sModule {
	Uint32	Magic;
	Uint8	Arch;
	Uint8	Flags;
	Uint16	Version;
	struct sModule	*Next;
	char	*Name;
	 int	(*Init)(char **Arguments);
	void	(*Deinit)();
	char	**Dependencies;	// NULL Terminated List
} __attribute__((packed)) tModule;

#define MODULE_INIT_SUCCESS	1
#define MODULE_INIT_FAILURE	0

/**
 * \brief Module Loader definition
 * 
 * Allows a module to extend the loader to recognise other module types
 * E.g. EDI, UDI, Windows, Linux, ...
 */
typedef struct sModuleLoader {
	struct sModuleLoader	*Next;	//!< Kernel Only - Next loader in list
	char	*Name;	//!< Friendly name for the loader
	 int	(*Detector)(void *Base);	//!< Simple detector function
	 int	(*Loader)(void *Base);	//!< Initialises the module
	 int	(*Unloader)(void *Base);	//!< Calls module's cleanup
} PACKED tModuleLoader;

/**
 * \brief Registers a tModuleLoader with the kernel
 * \param Loader	Pointer to loader structure (must be persistent)
 */
extern int	Module_RegisterLoader(tModuleLoader *Loader);

#endif

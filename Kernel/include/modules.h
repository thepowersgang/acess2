/*
 * AcessOS 2
 * - Module Loader
 */
#ifndef _MODULE_H
#define _MODULE_H

#define MODULE_MAGIC	('A'|('M'<<8)|('D'<<16)|('\2'<<24))

// IA32 - Architecture 1
#if ARCH == i386 || ARCH == i586
# define MODULE_ARCH_ID	1
// IA64 - Architecture 2
#elif ARCH == x64 || ARCH == x86_64
# define MODULE_ARCH_ID	2
#else
# error "Unknown architecture when determining MODULE_ARCH_ID ('" #ARCH "')"
#endif

#define MODULE_DEFINE(_flags,_ver,_ident,_entry,_deinit,_deps...)	char *_DriverDeps_##_ident[]={_deps};\
	tModule __attribute__ ((section ("KMODULES"),unused)) _DriverInfo_##_ident=\
	{MODULE_MAGIC,MODULE_ARCH_ID,_flags,_ver,NULL,#_ident,_entry,_deinit,_DriverDeps_##_ident}

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

#endif

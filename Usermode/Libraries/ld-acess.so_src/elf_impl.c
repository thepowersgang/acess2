
#ifndef ELFTYPE
# error "ELFTYPE must be defined to either ELf32 or Elf64 to use this file"
#endif

#define __PREF(t,s)	t##_##s
#define _PREF(t,s)	__PREF(t,s)
#define PREF(sym)	_PREF(ELFTYPE,sym)

typedef struct
{
	void	*Base;
	intptr_t	iBaseDiff;
	const char	*strtab;
	const PREF(Sym)	*symtab;
} PREF(RelocInfo);

typedef int PREF(RelocFcn)(const PREF(RelocInfo)* Info, PREF(Xword) t_info, PREF(Xword)* ptr, PREF(Addr) addend, bool bRela);

// - Extern
static PREF(RelocFcn)*	PREF(GetRelocFcn)(unsigned Machine);
// - Local
static PREF(RelocFcn) PREF(doRelocate_unk);
static void PREF(int_GetBaseDyntab)(void* Base, const PREF(Phdr)* phtab, unsigned phentcount, intptr_t* iBaseDiff_p, PREF(Dyn)** dynamicTab_p);
static int PREF(GetSymbolVars)(void *Base, const PREF(Sym)** symtab, const PREF(Word)** pBuckets, const char **dynstrtab, uintptr_t* piBaseDiff);
static int PREF(GetSymbolInfo)(void *Base, const char *Name, void **Addr, size_t *Size, int* Section, int *Binding, int *Type);

// - Relocate
void *PREF(Relocate)(void *Base, char **envp, const char *Filename)
{
	TRACE("(Base=0x%x)", Base);
	const PREF(Ehdr)	*hdr = Base;
	
	// Check magic header
	// TODO: Validate header?
	
	
	// Parse Program Header to get Dynamic Table
	// - Determine the linked base of the executable
	const PREF(Phdr)	*phtab = (void*)( (uintptr_t)Base + hdr->e_phoff );
	
	intptr_t	iBaseDiff = 0;
	PREF(Dyn)*	dynamicTab = NULL;
	
	PREF(int_GetBaseDyntab)(Base, phtab, hdr->e_phnum, &iBaseDiff, &dynamicTab);
	
	// Check if a PT_DYNAMIC segement was found
	if(!dynamicTab)
	{
		SysDebug(" elf_relocate: No PT_DYNAMIC segment in image %p, returning", Base);
		return (void *)(intptr_t)(hdr->e_entry + iBaseDiff);
	}

	// Allow writing to read-only segments, just in case they need to be relocated
	// - Will be reversed at the end of the function
	for( unsigned i = 0; i < hdr->e_phnum; i ++ )
	{
		if(phtab[i].p_type == PT_LOAD && !(phtab[i].p_flags & PF_W) )
		{
			uintptr_t	addr = phtab[i].p_vaddr + iBaseDiff;
			uintptr_t	end = addr + phtab[i].p_memsz;
			for( ; addr < end; addr += PAGE_SIZE )
				_SysSetMemFlags(addr, 0, 1);	// Unset RO
		}
	}
	
	// === Get Symbol table and String Table ===
	char	*dynstrtab = NULL;	// .dynamic String Table
	PREF(Sym)	*dynsymtab = NULL;
	PREF(Word)	*hashtable = NULL;
	unsigned	iSymCount = 0;
	for( unsigned j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		const PREF(Dyn)	*dt = &dynamicTab[j];
		switch(dt->d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			TRACE("DYNAMIC Symbol Table 0x%x (0x%x)", dt->d_val, dt->d_val + iBaseDiff);
			dynsymtab = (void*)((intptr_t)dt->d_val + iBaseDiff);
			break;
		// --- String Table ---
		case DT_STRTAB:
			TRACE("DYNAMIC String Table 0x%x (0x%x)", dt->d_val, dt->d_val + iBaseDiff);
			dynstrtab = (void*)((intptr_t)dt->d_val + iBaseDiff);
			break;
		// --- Hash Table --
		case DT_HASH:
			TRACE("DYNAMIC Hash table %p (%p)", dt->d_val, dt->d_val + iBaseDiff);
			hashtable = (void*)((intptr_t)dt->d_val + iBaseDiff);
			iSymCount = hashtable[1];
			break;
		}
	}

	if(dynsymtab == NULL) {
		SysDebug("ld-acess.so - WARNING: No Dynamic Symbol table in %p, returning", hdr);
		return (void *)(intptr_t) (hdr->e_entry + iBaseDiff);
	}
	
	// Apply base offset to locally defined symbols
	// - #0 is defined as ("" SHN_UNDEF), so skip it
	for( unsigned i = 1; i < iSymCount; i ++ )
	{
		PREF(Sym)	*sym = &dynsymtab[i];
		const char *name = dynstrtab + sym->st_name;
		(void)name;
		if( sym->st_shndx == SHN_UNDEF )
		{
			TRACE("Sym %i'%s' deferred (SHN_UNDEF)", i, name);
		}
		else if( sym->st_shndx == SHN_ABS )
		{
			// Leave as is
			TRACE("Sym %i'%s' untouched", i, name);
		}
		else
		{
			void *newval;
			size_t	newsize;
			if( ELF32_ST_BIND(sym->st_info) != STB_WEAK )
			{
				TRACE("Sym %i'%s' = %p (local)", i, name, sym->st_value + iBaseDiff);
				sym->st_value += iBaseDiff;
			}
			// If GetSymbol doesn't return a strong/global symbol value
			else if( GetSymbol(name, &newval, &newsize, Base) != 1 )
			{
				TRACE("Sym %i'%s' = %p (Local weak)", i, name, sym->st_value + iBaseDiff);
				sym->st_value += iBaseDiff;
			}
			else
			{
				TRACE("Sym %i'%s' = %p+0x%x (Extern weak)", i, name, newval, newsize);
				sym->st_value = (uintptr_t)newval;
				sym->st_size = newsize;
			}
		}
	}

	// === Add to loaded list (can be imported now) ===
	AddLoaded( Filename, Base );

	// === Parse Relocation Data ===
	PREF(Rel)	*rel = NULL;
	PREF(Rela)	*rela = NULL;
	void	*plt = NULL;
	 int	relSz=0, relEntSz=8;
	 int	relaSz=0, relaEntSz=8;
	 int	pltSz=0, pltType=0;
	TRACE("dynamicTab = 0x%x", dynamicTab);
	for( int j = 0; dynamicTab[j].d_tag != DT_NULL; j++)
	{
		const PREF(Dyn)	*dt = &dynamicTab[j];
		switch(dt->d_tag)
		{
		// --- Shared Library Name ---
		case DT_SONAME:
			TRACE(".so Name '%s'", dynstrtab + dt->d_val);
			break;
		// --- Needed Library ---
		case DT_NEEDED: {
			//assert(dt->d_val < sizeof_dynstrtab);	// Disabled, no sizeof_dynstrtab
			const char	*libPath = dynstrtab + dt->d_val;
			TRACE(" Required Library '%s'", libPath);
			if(LoadLibrary(libPath, NULL, envp) == 0) {
				SysDebug("Unable to load required library '%s'", libPath);
				return 0;
			}
			TRACE(" Lib loaded");
			break; }
		// --- PLT/GOT ---
//		case DT_PLTGOT:	pltgot = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_JMPREL:	plt = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_PLTREL:	pltType = dt->d_val;	break;
		case DT_PLTRELSZ:	pltSz = dt->d_val;	break;
		
		// --- Relocation ---
		case DT_REL:	rel = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_RELSZ:	relSz = dt->d_val;	break;
		case DT_RELENT:	relEntSz = dt->d_val;	break;
		case DT_RELA:	rela = (void*)(iBaseDiff + dt->d_val);	break;
		case DT_RELASZ:	relaSz = dt->d_val;	break;
		case DT_RELAENT:	relaEntSz = dt->d_val;	break;
		
		// --- Symbol Table ---
		case DT_SYMTAB:
		// --- Hash Table ---
		case DT_HASH:
		// --- String Table ---
		case DT_STRTAB:
			break;
		
		// --- Unknown ---
		default:
			if(dt->d_tag > DT_JMPREL)	continue;
			//DEBUGS(" elf_relocate: %i-%i = %s,0x%x",
			//	i,j, csaDT_NAMES[dynamicTab[j].d_tag],dynamicTab[j].d_val);
			break;
		}
	}
	
	// Resolve symbols (second pass)
	// - #0 is defined as ("" SHN_UNDEF), so skip it
	 int	fail = 0;
	for( int i = 1; i < iSymCount; i ++ )
	{
		PREF(Sym)	*sym = &dynsymtab[i];
		const char *name = dynstrtab + sym->st_name;
		if( sym->st_shndx == SHN_UNDEF )
		{
			void *newval;
			size_t	newsize;
			if( !GetSymbol(name, &newval, &newsize, Base) ) {
				if( ELF32_ST_BIND(sym->st_info) != STB_WEAK ) {
					// Not a weak binding, set fail and move on
					WARNING("%s: Can't find required symbol '%s' for '%s'", __func__, name, Filename);
					fail = 1;
					continue ;
				}
				// Leave the symbol value as-is
			}
			else {
				TRACE("Sym %i'%s' bound to %p+0x%x", i, name, newval, newsize);
				sym->st_value = (intptr_t)newval;
				sym->st_size = newsize;
			}
		}
		else if( sym->st_shndx == SHN_ABS )
		{
			// Leave as is
		}
		else
		{
			// Handled previously
			// TODO: What about weak locally-defined symbols?
			//assert( ELF32_ST_BIND(sym->st_info) != STB_WEAK );
		}
	}
	if( fail ) {
		WARNING("Relocation of '%s' failed", Filename);
		return NULL;
	}
	
	TRACE("Beginning Relocation on '%s'", Filename);


	PREF(RelocFcn)*	do_relocate = PREF(GetRelocFcn)(hdr->e_machine);
	if( do_relocate == 0 )
	{
		SysDebug("%s: Unknown machine type %i", __func__, hdr->e_machine);
		do_relocate = PREF(doRelocate_unk);
		fail = 1;
	}
	
	TRACE("do_relocate = %p", do_relocate);

	#define _doRelocate(r_info, ptr, bRela, addend)	\
		do_relocate(&reloc_info, r_info, ptr, addend, bRela);

	PREF(RelocInfo)	reloc_info = {
		.Base = Base,
		.iBaseDiff = iBaseDiff,
		.strtab = dynstrtab,
		.symtab = dynsymtab
	};

	// Parse Relocation Entries
	if(rel && relSz)
	{
		TRACE("rel=0x%x, relSz=0x%x, relEntSz=0x%x", rel, relSz, relEntSz);
		int max = relSz / relEntSz;
		for( int i = 0; i < max; i++ )
		{
			PREF(Xword) *ptr = (void*)(iBaseDiff + rel[i].r_offset);
			fail |= _doRelocate(rel[i].r_info, ptr, 0, *ptr);
		}
	}
	// Parse Relocation Entries
	if(rela && relaSz)
	{
		TRACE("rela=0x%x, relaSz=0x%x, relaEntSz=0x%x", rela, relaSz, relaEntSz);
		int count = relaSz / relaEntSz;
		for( int i = 0; i < count; i++ )
		{
			void *ptr = (void*)(iBaseDiff + rela[i].r_offset);
			fail |= _doRelocate(rela[i].r_info, ptr, 1, rela[i].r_addend);
		}
	}
	
	// === Process PLT (Procedure Linkage Table) ===
	if(plt && pltSz)
	{
		TRACE("Relocate PLT, plt=0x%x", plt);
		if(pltType == DT_REL)
		{
			PREF(Rel)	*pltRel = plt;
			int count = pltSz / sizeof(*pltRel);
			TRACE("PLT Reloc Type = Rel, %i entries", count);
			for(int i = 0; i < count; i ++)
			{
				PREF(Xword) *ptr = (void*)(iBaseDiff + pltRel[i].r_offset);
				fail |= _doRelocate(pltRel[i].r_info, ptr, 0, *ptr);
			}
		}
		else
		{
			PREF(Rela)	*pltRela = plt;
			int count = pltSz / sizeof(*pltRela);
			TRACE("PLT Reloc Type = Rela, %i entries", count);
			for(int i=0;i<count;i++)
			{
				void *ptr = (void*)(iBaseDiff + pltRela[i].r_offset);
				fail |= _doRelocate(pltRela[i].r_info, ptr, 1, pltRela[i].r_addend);
			}
		}
	}

	// Re-set readonly
	for( int i = 0; i < hdr->e_phnum; i ++ )
	{
		// If load and not writable
		if(phtab[i].p_type == PT_LOAD && !(phtab[i].p_flags & PF_W) )
		{
			uintptr_t	addr = phtab[i].p_vaddr + iBaseDiff;
			uintptr_t	end = addr + phtab[i].p_memsz;
			for( ; addr < end; addr += PAGE_SIZE )
				_SysSetMemFlags(addr, 1, 1);	// Unset RO
		}
	}

	if( fail ) {
		TRACE("ElfRelocate: Failure");
		return NULL;
	}	

	#undef _doRelocate

	TRACE("RETURN 0x%x to %p", hdr->e_entry + iBaseDiff, __builtin_return_address(0));
	return (void*)(intptr_t)( hdr->e_entry + iBaseDiff );
}

void PREF(int_GetBaseDyntab)(void* Base, const PREF(Phdr)* phtab, unsigned phentcount, intptr_t* iBaseDiff_p, PREF(Dyn)** dynamicTab_p)
{
	uintptr_t	iRealBase = UINTPTR_MAX;
	assert(dynamicTab_p);
	
	for(unsigned i = 0; i < phentcount; i ++)
	{
		switch(phtab[i].p_type)
		{
		case PT_LOAD:
			// Determine linked base address
			if( iRealBase > phtab[i].p_vaddr)
				iRealBase = phtab[i].p_vaddr;
			break;
		case PT_DYNAMIC:
			// Find Dynamic Section
			if(!*dynamicTab_p) {
				*dynamicTab_p = (void *) (intptr_t) phtab[i].p_vaddr;
			}
			else {
				WARNING("elf_relocate: Multiple PT_DYNAMIC segments");
			}
			break;
		}
	}
	
	// Page Align real base
	iRealBase &= ~0xFFF;
	
	// Adjust "Real" Base
	const intptr_t	iBaseDiff = (intptr_t)Base - iRealBase;
	*iBaseDiff_p = iBaseDiff;

	// Adjust Dynamic Table
	if( *dynamicTab_p )
	{
		*dynamicTab_p = (void *)( (intptr_t)*dynamicTab_p + iBaseDiff );
	}

	TRACE("True Base = 0x%x, Compiled Base = 0x%x, Difference = 0x%x", Base, iRealBase, *iBaseDiff_p);
}

int PREF(doRelocate_unk)(const PREF(RelocInfo)* Info, PREF(Xword) r_info, PREF(Xword)* ptr, PREF(Addr) addend, bool bRela)
{
	return 1;
}

int PREF(GetSymbolVars)(void *Base, const PREF(Sym)** symtab, const PREF(Word)** pBuckets, const char **dynstrtab, uintptr_t* piBaseDiff)
{
	const PREF(Ehdr)*	hdr = Base;
	PREF(Dyn)*	dynTab = NULL;
	intptr_t	iBaseDiff = -1;
	
	PREF(int_GetBaseDyntab)(Base, (void*)( (uintptr_t)Base + hdr->e_phoff ), hdr->e_phnum, &iBaseDiff, &dynTab);
	if( !dynTab ) {
		SysDebug("ERROR - Unable to find DYNAMIC segment in %p", Base);
		return 1;
	}
	
	for( int i = 0; dynTab[i].d_tag != DT_NULL; i++)
	{
		switch(dynTab[i].d_tag)
		{
		// --- Symbol Table ---
		case DT_SYMTAB:
			*symtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);	// Rebased in Relocate
			break;
		case DT_STRTAB:
			*dynstrtab = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		// --- Hash Table --
		case DT_HASH:
			*pBuckets = (void*)((intptr_t)dynTab[i].d_val + iBaseDiff);
			break;
		}
	}
	
	if( !*symtab ) {
		SysDebug("ERRO - No DT_SYMTAB in %p", Base);
		return 1;
	}
	if( !*pBuckets ) {
		SysDebug("ERRO - No DT_HASH in %p", Base);
		return 1;
	}
	if( !*dynstrtab ) {
		SysDebug("ERRO - No DT_STRTAB in %p", Base);
		return 1;
	}

	// ... ok... maybe they haven't been relocated
	if( (uintptr_t)*symtab < (uintptr_t)Base )
	{
		SysDebug("Executable not yet relocated (symtab,pBuckets,dynstrtab = %p,%p,%p + 0x%x)",
			*symtab,*pBuckets,*dynstrtab, iBaseDiff);
		*symtab    = (void*)( (uintptr_t)*symtab    + iBaseDiff );
		*pBuckets  = (void*)( (uintptr_t)*pBuckets  + iBaseDiff );
		*dynstrtab = (void*)( (uintptr_t)*dynstrtab + iBaseDiff );
	}
	*piBaseDiff = iBaseDiff;
	return 0;
}

int PREF(GetSymbolInfo)(void *Base, const char *Name, void **Addr, size_t *Size, int* Section, int *Binding, int *Type)
{
	// Locate the tables
	uintptr_t	iBaseDiff = -1;
	const PREF(Sym)*	symtab = NULL;
	const PREF(Word)*	pBuckets = NULL;
	const char	*dynstrtab = NULL;
	if( PREF(GetSymbolVars)(Base, &symtab, &pBuckets, &dynstrtab, &iBaseDiff) )
		return 1;

	unsigned nbuckets = pBuckets[0];
//	int iSymCount = pBuckets[1];
	pBuckets = &pBuckets[2];
	
	const PREF(Word)*	pChains = &pBuckets[ nbuckets ];
	assert(pChains);

	// Get hash
	unsigned iNameHash = ElfHashString(Name);
	iNameHash %= nbuckets;

	// Walk Chain
	unsigned idx = pBuckets[ iNameHash ];
	do {
		const PREF(Sym)*	sym = &symtab[idx];
		assert(sym);
		if( strcmp(dynstrtab + sym->st_name, Name) == 0 )
		{
			TRACE("*sym = {value:0x%x,size:0x%x,info:0x%x,other:0x%x,shndx:%i}",
				sym->st_value, sym->st_size, sym->st_info,
				sym->st_other, sym->st_shndx);
			if(Addr)	*Addr = (void*)(intptr_t)( sym->st_value );
			if(Size)	*Size = sym->st_size;
			if(Binding)	*Binding = ELF32_ST_BIND(sym->st_info);
			if(Type)	*Type = ELF32_ST_TYPE(sym->st_info);
			if(Section)	*Section = sym->st_shndx;
			return 0;
		}
	} while( (idx = pChains[idx]) != STN_UNDEF && idx != pBuckets[iNameHash] );
	
	TRACE("No symbol");
	return 1;
}

int PREF(GetSymbol)(void *Base, const char *Name, void **ret, size_t *Size)
{
	 int	section, binding;
	TRACE("%s(%p,%s,...)", __func__, Base, Name);
	if( PREF(GetSymbolInfo)(Base, Name, ret, Size, &section, &binding, NULL) )
		return 0;
	if( section == SHN_UNDEF ) {
		TRACE("%s: Undefined %p", __func__, *ret, (Size?*Size:0), section);
		return 0;
	}
	if( binding == STB_WEAK ) {
		TRACE("%s: Weak, return %p+0x%x,section=%i", __func__, *ret, (Size?*Size:0), section);
		return 2;
	}
	TRACE("%s: Found %p+0x%x,section=%i", __func__, *ret, (Size?*Size:0), section);
	return 1;
}

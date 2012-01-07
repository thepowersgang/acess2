ENTRY (_start)

_kernel_base = 0x80000000;
_usertext_vbase = 0xFFFFE000;

SECTIONS
{
	. = 0;
	.init :
	{
		*(.init)
	}
	. += _kernel_base;
	.text : AT( ADDR(.text) - _kernel_base )
	{
		*(.text*)
		*(.rodata*)
	}
			

	/* HACKS: User accesible .text section */
	. = ALIGN(0x1000);
	gUsertextPhysStart = . - _kernel_base;
	. = _usertext_vbase;
	.usertext : AT( gUsertextPhysStart )
	{
		*(.usertext)
	}
	. += gUsertextPhysStart + _kernel_base - _usertext_vbase;
	
	/* 0x4000 (4 pages) alignment needed for root table */
	.data ALIGN(0x4000) : AT( ADDR(.data) - _kernel_base )
	{
		*(.padata)
		*(.data*)
		
		gKernelSymbols = .;
		*(KEXPORT)
		gKernelSymbolsEnd = .;
		
		gKernelModules = .;
		*(KMODULES)
		gKernelModulesEnd = .;
	}
	.bss : AT( ADDR(.bss) - _kernel_base )
	{
		bss_start = .;
		*(.bss*)
		*(COMMON*)
		. = ALIGN(0x1000);
		*(.pabss)
		bss_end = .;
	}
	gKernelEnd = .;
}

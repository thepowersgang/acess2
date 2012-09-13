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

	/DISCARD/           : { *(.ARM.extab.init .ARM.exidx.init)      }
	.ARM.extab          : AT( ADDR(.ARM.extab) - _kernel_base)
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	}
	PROVIDE_HIDDEN ( __exidx_start = . );
	.ARM.exidx          : AT( ADDR(.ARM.exidx) - _kernel_base)
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	}
	PROVIDE_HIDDEN ( __exidx_end = . );
	.eh_frame_hdr       : AT( ADDR(.eh_frame_hdr) - _kernel_base) {
		*(.eh_frame_hdr)
	}
	.eh_frame           : AT( ADDR(.eh_frame) - _kernel_base) ONLY_IF_RO
	{
		KEEP (*(.eh_frame))
	}
	.gcc_except_table   : AT( ADDR(.gcc_except_table) - _kernel_base) ONLY_IF_RO
	{
		*(.gcc_except_table .gcc_except_table.*)
	}

	/* 0x4000 (4 pages) alignment needed for root table */
	. = ALIGN(0x4000);
	.data : AT( ADDR(.data) - _kernel_base )
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
	.bss ALIGN(0x1000) : AT( ADDR(.bss) - _kernel_base )
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

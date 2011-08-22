ENTRY (_start)

_kernel_base = 0x80000000;

SECTIONS
{
	. = 0;
	. += _kernel_base;
	.text : AT( ADDR(.text) - _kernel_base )
	{
		*(.text*)
		*(.rodata*)
	}
	.data ALIGN(0x1000) : AT( ADDR(.data) - _kernel_base )
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
		*(.bss*)
		*(COMMON*)
	}
	gKernelEnd = .;
}

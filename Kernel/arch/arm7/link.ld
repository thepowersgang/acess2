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
	.data : AT( ADDR(.text) - _kernel_base )
	{
		*(.padata)
		*(.data*)
	}
	.bss : AT( ADDR(.text) - _kernel_base )
	{
		*(.bss*)
		*(COMMON*)
	}
}

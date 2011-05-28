ENTRY (_start)

SECTIONS
{
	. = 0x80000000;
	.text : { *(.text*) *(.rodata*) }
	.data : { *(.data*) }
	.bss : { *(.bss*) *(COMMON*) }
}

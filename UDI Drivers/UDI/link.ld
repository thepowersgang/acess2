
ENTRY(start)

OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	. = 0;
	
	. += SIZEOF_HEADERS;
	
	.hash :	{	*(.hash) }
	.dynsym : {	*(.dynsym) }
	.dynstr : {	*(.dynstr) }
	.rel.dyn :{	*(.rel.dyn) }
	.dynamic :{	*(.dynamic) }
	
	. = (. + 0xFFF) & ~ 0xFFF;
	.text :{	*(.text) }
	
	.rodata : {	*(.rodata) }
	
	_udiprops = .;
	.udiprops : {
		*(.udiprops)
	}
	_udiprops_end = .;
	
	. = (. + 0xFFF) & ~ 0xFFF;
	
	.data :	{ *(.data) }
	.bss :	{ *(.bss) }
	.got.plt : { *(.got.plt) }
	
}

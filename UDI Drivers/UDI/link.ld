
ENTRY(start)

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
	_udiprops_size = . - _udiprops;
	
	. = (. + 0xFFF) & ~ 0xFFF;
	
	.data :	{ *(.data) }
	.bss :	{ *(.bss) }
	.got.plt : { *(.got.plt) }
	
}

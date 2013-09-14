OUTPUT_FORMAT(elf32-i386)

SECTIONS 
{
	. = 0 + SIZEOF_HEADERS;
	
	.text : AT(ADDR(.text)) {
		textzero = .;
		*(.text)
	}
	
	.rodata ALIGN(0x1000): AT(ADDR(.rodata)) {
		*(.rodata)
		*(.rdata)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data)) {
		*(.data)
		_udiprops = .;
		*(.udiprops)
		_udiprops_size = . - _udiprops;
	}

	.bss : AT(ADDR(.bss)) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
}


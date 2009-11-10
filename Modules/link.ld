/*
 * Acess2 Kernel Modules
 * Linker Script
 */

ENTRY(ModuleEntry)
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
		DriverInfo = .;
		*(KMODULES)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data)) {
		*(.data)
	}

	.bss : AT(ADDR(.bss)) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
}

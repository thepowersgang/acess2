/*
 * AcessMicro Kernel
 * Linker Script
 */

//lowStart = start - 0xC0000000;
ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	. = 0x100000;
	__load_addr = .;
	.multiboot : AT(ADDR(.multiboot)) {
		*(.multiboot)
	}
	
	. += 0xC0000000;
	
	.text ALIGN(0x1000): AT(ADDR(.text) - 0xC0000000) {
		*(.text)
	}
	
	.usertext ALIGN(0x1000): AT(ADDR(.usertext) - 0xC0000000) {
		_UsertextBase = .;
		*(.usertext)
	}
	
	.rodata ALIGN(0x1000): AT(ADDR(.rodata) - 0xC0000000) {
		*(.initpd)
		*(.rodata)
		*(.rdata)
		gKernelModules = .;
		*(KMODULES)
		gKernelModulesEnd = .;
		. = ALIGN(4);
		gKernelSymbols = .;
		*(KEXPORT)
		gKernelSymbolsEnd = .;


	}
	/*
	.debug_abbrev : { *(.debug_abbrev) }
	.debug_info : { *(.debug_info) }
	.debug_line : { *(.debug_line) }
	.debug_loc : { *(.debug_loc) }
	.debug_pubnames : { *(.debug_pubnames) }
	.debug_aranges : { *(.debug_aranges) }
	.debug_ranges : { *(.debug_ranges) }
	.debug_str : { *(.debug_str) }
	.debug_frame : { *(.debug_frame) }
	*/
	
	.padata ALIGN (0x1000) : AT(ADDR(.padata) - 0xC0000000) {
		*(.padata)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
		*(.data)
	}

	__bss_start = .;
	.bss : AT(ADDR(.bss) - 0xC0000000) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
	gKernelEnd = (. + 0xFFF)&0xFFFFF000;
}

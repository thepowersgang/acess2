/*
 * AcessMicro Kernel
 * Linker Script
 */

lowStart = start - 0xC0000000;
ENTRY(lowStart)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
	. = 0x100000;
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
		_gKernelModules = .;
		*(KMODULES)
		_gKernelModulesEnd = .;
		. = ALIGN(4);
		_gKernelSymbols = .;
		*(KEXPORT)
		_gKernelSymbolsEnd = .;

	}
	
	.padata ALIGN (0x1000) : AT(ADDR(.padata) - 0xC0000000) {
		*(.padata)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
		*(.data)
	}

	.bss : AT(ADDR(.bss) - 0xC0000000) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
	_gKernelEnd = (. + 0xFFF)&0xFFFFF000;
}

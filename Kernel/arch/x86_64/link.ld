/*
 * Acess2 x86_64 Kernel
 * Linker Script
 */

_kernel_base = 0xFFFF800000000000;

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:x86-64)
ENTRY (start)

SECTIONS {
	. = 0x100000;
	gKernelBase = .;
	. += SIZEOF_HEADERS;
	__load_addr = .;
	.multiboot : AT(ADDR(.multiboot)) {
		*(.multiboot)
	}
	
	. += _kernel_base;
	
	.text ALIGN(0x1000): AT(ADDR(.text) - _kernel_base) {
		*(.text)
	}
	
	.usertext ALIGN(0x1000): AT(ADDR(.usertext) - _kernel_base) {
		_UsertextBase = .;
		*(.usertext)
	}
	_UsertextEnd = .;
	
	.rodata ALIGN(0x1000): AT(ADDR(.rodata) - _kernel_base) {
		*(.initpd)
		*(.rodata)
		*(.rdata)
		
		. = ALIGN(0x10);
		gKernelModules = .;
		*(KMODULES)
		gKernelModulesEnd = .;
		. = ALIGN(0x10);
		gKernelSymbols = .;
		*(KEXPORT)
		gKernelSymbolsEnd = .;
	}
	
	.padata ALIGN (0x1000) : AT(ADDR(.padata) - _kernel_base) {
		*(.padata)
	}
	
	.data ALIGN (0x1000) : AT(ADDR(.data) - _kernel_base) {
		*(.data)
	}

	__bss_start = .;
	.bss : AT(ADDR(.bss) - _kernel_base) {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
	gKernelEnd = (. + 0xFFF)&0xFFFFFFFFFFFFF000;
}

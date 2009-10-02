ENTRY(_SoMain)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    . = 0xBBFF0000;
	_gLinkedBase = .;

   .text : AT(ADDR(.text)) {
        code = .;
        *(.text)
        *(.rodata*)
   }

   .data ALIGN (0x1000) : AT(ADDR(.data)) {
        data = .;
        *(.data)
   }

   .bss ALIGN (0x1000) : AT(ADDR(.bss)) {
        _sbss = .;
        *(COMMON)
        *(.bss)
        _ebss = .;
        bss = .;
   }
   _end = .;
}

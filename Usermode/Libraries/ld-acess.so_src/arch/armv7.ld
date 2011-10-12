ENTRY(_start)
OUTPUT_FORMAT(elf32-littlearm)

SECTIONS {
	. = 0x00100000;
	gLinkedBase = .;
	. += SIZEOF_HEADERS;
	.interp		 : { *(.interp) }
	.note.gnu.build-id : { *(.note.gnu.build-id) }
	.hash		   : { *(.hash) }
	.gnu.hash	   : { *(.gnu.hash) }
	.dynsym		 : { *(.dynsym) }
	.dynstr		 : { *(.dynstr) }
	.gnu.version	: { *(.gnu.version) }
	.gnu.version_d  : { *(.gnu.version_d) }
	.gnu.version_r  : { *(.gnu.version_r) }
	.rel.dyn		:
	{
		*(.rel.init)
		*(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
		*(.rel.fini)
		*(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
		*(.rel.data.rel.ro* .rel.gnu.linkonce.d.rel.ro.*)
		*(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
		*(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
		*(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)
		*(.rel.ctors)
		*(.rel.dtors)
		*(.rel.got)
		*(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
	}
	.rela.dyn	   :
	{
		*(.rela.init)
		*(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
		*(.rela.fini)
		*(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
		*(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
		*(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
		*(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
		*(.rela.ctors)
		*(.rela.dtors)
		*(.rela.got)
		*(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
	}

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

.section .init
.global _init
.type _init, @function
_init:
	/* gcc will nicely put the contents of crtbegin.o's .init section here. */

.section .fini
.global _fini
.type _fini, @function
_fini:
	/* gcc will nicely put the contents of crtbegin.o's .fini section here. */

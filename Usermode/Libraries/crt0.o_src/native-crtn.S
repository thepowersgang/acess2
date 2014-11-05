.section .init
	/* gcc will nicely put the contents of crtend.o's .init section here. */
	ret

.section .fini
	ret
	/* gcc will nicely put the contents of crtend.o's .fini section here. */

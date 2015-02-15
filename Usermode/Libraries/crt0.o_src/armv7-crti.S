.section .init
.global _init
.type _init, function
_init:
#ifdef __thumb__
	.thumb
	
	push	{r3, r4, r5, r6, r7, lr}
#else
	.arm
	mov	ip, sp
	stmdb	sp!, {r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
	sub	fp, ip, #4
#endif
	/* gcc will nicely put the contents of crtbegin.o's .init section here. */

.section .fini
.global _fini
.type _fini, function
_fini:
#ifdef __thumb__
	.thumb
	
	push	{r3, r4, r5, r6, r7, lr}
#else
	.arm
	mov	ip, sp
	stmdb	sp!, {r3, r4, r5, r6, r7, r8, r9, sl, fp, ip, lr, pc}
	sub	fp, ip, #4
#endif
	/* gcc will nicely put the contents of crtbegin.o's .fini section here. */

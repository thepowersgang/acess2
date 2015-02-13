.section .init
	/* gcc will nicely put the contents of crtend.o's .init section here. */
#ifdef __thumb__
	.thumb
	
	pop	{r3, r4, r5, r6, r7}
	pop	{r3}
	mov	lr, r3
#else
	.arm
	
	sub	sp, fp, #40
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
#endif
	
#if defined __THUMB_INTERWORK__ || defined __thumb__
	bx	lr
#else
	mov	pc, lr
#endif

.section .fini
	/* gcc will nicely put the contents of crtend.o's .fini section here. */
#ifdef __thumb__
	.thumb
	
	pop	{r3, r4, r5, r6, r7}
	pop	{r3}
	mov	lr, r3
#else
	.arm
	
	sub	sp, fp, #40
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, sl, fp, sp, lr}
#endif
	
#if defined __THUMB_INTERWORK__ || defined __thumb__
	bx	lr
#else
	mov	pc, lr
#endif

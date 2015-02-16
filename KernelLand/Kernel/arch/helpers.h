/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * arch/helpers.h
 * - Misc helper functions for the arch code
 */
#ifndef _ARCH_HELPERS_H_
#define _ARCH_HELPERS_H_

// Divide
// - Find what power of two times Den is > Num
// - Iterate down in bit significance
//  > If the `N` value is greater than `D`, we can set this bit
#define DEF_DIVMOD(s) Uint##s __divmod##s(Uint##s N, Uint##s D, Uint##s*Rem){\
	Uint##s ret=0,add=1;\
	while(N/2>=D&&add) {D<<=1;add<<=1;}\
	while(add>0){\
		if(N>=D){ret+=add;N-=D;}\
		add>>=1;D>>=1;\
	}\
	if(Rem)*Rem = N;\
	return ret;\
}

#endif


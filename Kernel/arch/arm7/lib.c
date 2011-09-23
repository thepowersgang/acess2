/*
 * Acess2 ARM7 Port
 *
 * lib.c - Library Functions
 */
#include <acess.h>

// === PROTOTYPES ===
Uint64	__udivdi3(Uint64 Num, Uint64 Den);
Uint64	__umoddi3(Uint64 Num, Uint64 Den);
Uint32	__udivsi3(Uint32 Num, Uint32 Den);
Uint32	__umodsi3(Uint32 Num, Uint32 Den);
Sint32	__divsi3(Sint32 Num, Sint32 Den);
Sint32	__modsi3(Sint32 Num, Sint32 Den);

// === CODE ===
void *memcpy(void *_dest, const void *_src, size_t _length)
{
	Uint32	*dst;
	const Uint32	*src;
	Uint8	*dst8 = _dest;
	const Uint8	*src8 = _src;

	// Handle small copies / Non-aligned
	if( _length < 4 || ((tVAddr)_dest & 3) != ((tVAddr)_src & 3) )
	{
		for( ; _length--; dst8++,src8++ )
			*dst8 = *src8;
		return _dest;
	}

	// Force alignment
	while( (tVAddr)dst8 & 3 ) *dst8 ++ = *src8++;
	dst = (void *)dst8;	src = (void *)src8;

	// DWORD copies
	for( ; _length > 3; _length -= 4)
		*dst++ = *src++;

	// Trailing bytes
	dst8 = (void*)dst;	src8 = (void*)src;
	for( ; _length; _length -- )
		*dst8 ++ = *src8 ++;
	
	return _dest;
}

int memcmp(const void *_m1, const void *_m2, size_t _length)
{
	const Uint32	*m1, *m2;
	const Uint8	*m1_8 = _m1, *m2_8 = _m2;

	// Handle small copies / Non-aligned
	if( _length < 4 || ((tVAddr)_m1 & 3) != ((tVAddr)_m1 & 3) )
	{
		for( ; _length--; m1_8++,m2_8++ ) {
			if(*m1_8 != *m2_8)	return *m1_8 - *m2_8;
		}
		return 0;
	}

	// Force alignment
	for( ; (tVAddr)m1_8 & 3; m1_8 ++, m2_8 ++) {
		if(*m1_8 != *m2_8)	return *m1_8 - *m2_8;
	}
	m1 = (void *)m1_8;	m2 = (void *)m2_8;

	// DWORD copies
	for( ; _length > 3; _length -= 4, m1++, m2++)
		if(*m1 != *m2)	return *m1 - *m2;

	// Trailing bytes
	m1_8 = (void*)m1;	m2_8 = (void*)m2;
	for( ; _length; _length --, m1_8++, m2_8++ )
		if(*m1_8 != *m2_8)	return *m1_8 - *m2_8;
	
	return 0;
}

void *memset(void *_dest, int _value, size_t _length)
{
	Uint32	*dst, val32;
	Uint8	*dst8 = _dest;

	_value = (Uint8)_value;

	// Handle small copies / Non-aligned
	if( _length < 4 )
	{
		for( ; _length--; dst8++ )
			*dst8 = _value;
		return _dest;
	}

	val32 = _value;
	val32 |= val32 << 8;
	val32 |= val32 << 16;
	
	// Force alignment
	while( (tVAddr)dst8 & 3 ) *dst8 ++ = _value;
	dst = (void *)dst8;

	// DWORD copies
	for( ; _length > 3; _length -= 4)
		*dst++ = val32;

	// Trailing bytes
	dst8 = (void*)dst;
	for( ; _length; _length -- )
		*dst8 ++ = _value;
	
	return _dest;
}

Uint64 DivMod64U(Uint64 Num, Uint64 Den, Uint64 *Rem)
{
	Uint64	ret;
	if(Den == 0)	return 0;	// TODO: #div0
	if(Num == 0) {
		if(Rem)	*Rem = 0;
		return 0;
	}
	if(Den == 1) {
		if(Rem)	*Rem = 0;
		return Num;
	}
	if(Den == 2) {
		if(Rem)	*Rem = Num & 1;
		return Num >> 1;
	}
	if(Den == 16) {
		if(Rem)	*Rem = Num & 0xF;
		return Num >> 4;
	}
	if(Den == 0x1000) {
		if(Rem)	*Rem = Num & 0xFFF;
		return Num >> 12;
	}

	#if 0
	{
		// http://www.tofla.iconbar.com/tofla/arm/arm02/index.htm
		Uint64	tmp = 1;
		__asm__ __volatile__(
			"1:"
			"cmpl %2,%1"
			"movls %2,%2,lsl#1"
			"movls %3,%3,lsl#1"
			"bls 1b"
			"2:"
			"cmpl %"
		while(Num > Den) {
			Den <<= 1;
			tmp <<= 1;
		}
		Den >>= 1; tmp >>= 1;
		while(
	}
	#else
	for( ret = 0; Num > Den; ret ++, Num -= Den) ;
	#endif
	if(Rem)	*Rem = Num;
	return ret;
}

// Unsigned Divide 64-bit Integer
Uint64 __udivdi3(Uint64 Num, Uint64 Den)
{
	return DivMod64U(Num, Den, NULL);
//	if( Den == 0 )	return 5 / (Uint32)Den;	// Force a #DIV0
	if( Den == 16 )	return Num >> 4;
	if( Den == 256 )	return Num >> 8;
	if( Den == 512 )	return Num >> 9;
	if( Den == 1024 )	return Num >> 10;
	if( Den == 2048 )	return Num >> 11;
	if( Den == 4096 )	return Num >> 12;
	if( Num < Den )	return 0;
	if( Num <= 0xFFFFFFFF && Den <= 0xFFFFFFFF )
		return (Uint32)Num / (Uint32)Den;

	#if 0
	if( Den <= 0xFFFFFFFF ) {
		(Uint32)(Num >> 32) / (Uint32)Den
	}
	#endif
	Uint64	ret = 0;
	for( ret = 0; Num > Den; ret ++, Num -= Den );
	return ret;
}

// Unsigned Modulus 64-bit Integer
Uint64 __umoddi3(Uint64 Num, Uint64 Den)
{
	if( Den == 0 )	return 5 / (Uint32)Den;	// Force a #DIV0
	if( Num < Den )	return Num;
	if( Den == 1 )	return 0;
	if( Den == 2 )	return Num & 1;
	if( Den == 16 )	return Num & 3;
	if( Den == 256 )	return Num & 0xFF;
	if( Den == 512 )	return Num & 0x1FF;
	if( Den == 1024 )	return Num & 0x3FF;
	if( Den == 2048 )	return Num & 0x7FF;
	if( Den == 4096 )	return Num & 0xFFF;
//	if( Num <= 0xFFFFFFFF && Den <= 0xFFFFFFFF )
//		return (Uint32)Num % (Uint32)Den;

	#if 0
	if( Den <= 0xFFFFFFFF ) {
		(Uint32)(Num >> 32) / (Uint32)Den
	}
	#endif
	for( ; Num > Den; Num -= Den );
	return Num;
}

#define _divide_s_32(Num, Den, rem)	__asm__ __volatile__ ( \
		"mov %0, #0\n" \
	"	adds %1, %1, %1\n" \
	"	.rept 32\n" \
	"	adcs %0, %2, %0, lsl #1\n" \
	"	subcc %0, %0, %3\n" \
	"	adcs %1, %1, %1\n" \
	"	.endr\n" \
		: "=r" (rem), "=r" (Num) \
		: "r" (Den) \
		: "cc" \
		)
Uint32 __udivsi3(Uint32 Num, Uint32 Den)
{
	register Uint32	ret;
	Uint64	P, D;
	 int	i;

	if( Num == 0 )	return 0;
	if( Den == 0 )	return 0xFFFFFFFF;	// TODO: Throw an error
	if( Den == 1 )	return Num;
	
	D = ((Uint64)Den) << 32;	

	for( i = 32; i --; )
	{
		P = 2*P - D;
		if( P >= 0 )
			ret |= 1;
		else
			P += D;
		ret <<= 1;
	}

//	_divide_s_32(Num, Den, rem);
	return Num;
}

Uint32 __umodsi3(Uint32 Num, Uint32 Den)
{
	return Num - __udivsi3(Num, Den)*Den;
}

Sint32 __divsi3(Sint32 Num, Sint32 Den)
{
	if( (Num < 0) && (Den < 0) )
		return __udivsi3(-Num, -Den);
	else if( Num < 0 )
		return __udivsi3(-Num, Den);
	else if( Den < 0 )
		return __udivsi3(Den, -Den);
	else
		return __udivsi3(Den, Den);
}

Sint32 __modsi3(Sint32 Num, Sint32 Den)
{
	//register Sint32	rem;
	//_divide_s_32(Num, Den, rem);
	return Num - __divsi3(Num, Den) * Den;
}

/*
 * Acess2 ARM7 Port
 *
 * lib.c - Library Functions
 */
#include <acess.h>
#include "../helpers.h"

// === IMPORTS ===
extern void	__memcpy_align4(void *_dest, const void *_src, size_t _length);
extern void	__memcpy_byte(void *_dest, const void *_src, size_t _length);
extern Uint32	__divmod32_asm(Uint32 Num, Uint32 Den, Uint32 *Rem);

// === PROTOTYPES ===
Uint64	__divmod64(Uint64 Num, Uint64 Den, Uint64 *Rem);
Uint32	__divmod32(Uint32 Num, Uint32 Den, Uint32 *Rem);
#if 0
Uint64	__udivdi3(Uint64 Num, Uint64 Den);
Uint64	__umoddi3(Uint64 Num, Uint64 Den);
Uint32	__udivsi3(Uint32 Num, Uint32 Den);
Uint32	__umodsi3(Uint32 Num, Uint32 Den);
Sint32	__divsi3(Sint32 Num, Sint32 Den);
Sint32	__modsi3(Sint32 Num, Sint32 Den);
#endif

// === CODE ===
void *memcpy(void *_dest, const void *_src, size_t _length)
{
	Uint8	*dst8 = _dest;
	const Uint8	*src8 = _src;

	if( ((tVAddr)_dest & 3) == 0 && ((tVAddr)_src & 3) == 0 )
	{
		__memcpy_align4(_dest, _src, _length);
		return _dest;
	}

	// Handle small copies / Non-aligned
	if( _length < 4 || ((tVAddr)_dest & 3) != ((tVAddr)_src & 3) )
	{
		__memcpy_byte(_dest, _src, _length);
		return _dest;
	}

	// Force alignment
	while( (tVAddr)dst8 & 3 ) *dst8 ++ = *src8++, _length --;

	__memcpy_align4(dst8, src8, _length);
	
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

	// Handle small copies
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

DEF_DIVMOD(64)
DEF_DIVMOD(32)

Uint64 DivMod64U(Uint64 Num, Uint64 Den, Uint64 *Rem)
{
	Uint64	ret;
	if(Den == 0)	return 0;	// TODO: #div0
	if(Num < Den) {
		if(Rem)	*Rem = Num;
		return 0;
	}
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
	if(Den == 32) {
		if(Rem)	*Rem = Num & 0x1F;
		return Num >> 5;
	}
	if(Den == 0x1000) {
		if(Rem)	*Rem = Num & 0xFFF;
		return Num >> 12;
	}
	
	if( !(Den >> 32) && !(Num >> 32) ) {
		if(Rem)	*Rem = 0;	// Clear high bits
		return __divmod32_asm(Num, Den, (Uint32*)Rem);
	}

	ret = __divmod64(Num, Den, Rem);
	return ret;
}

#if 0
// Unsigned Divide 64-bit Integer
Uint64 __udivdi3(Uint64 Num, Uint64 Den)
{
	return DivMod64U(Num, Den, NULL);
}

// Unsigned Modulus 64-bit Integer
Uint64 __umoddi3(Uint64 Num, Uint64 Den)
{
	Uint64	ret = 0;
	DivMod64U(Num, Den, &ret);
	return ret;
}

Uint32 __udivsi3(Uint32 Num, Uint32 Den)
{
	return __divmod32_asm(Num, Den, NULL);
}

Uint32 __umodsi3(Uint32 Num, Uint32 Den)
{
	Uint32	rem;
	__divmod32_asm(Num, Den, &rem);
	return rem;
}
#endif

static inline Sint32 DivMod32S(Sint32 Num, Sint32 Den, Sint32 *Rem)
{
	Sint32	ret = 1;
	if( Num < 0 ) {
		ret = -ret;
		Num = -Num;
	}
	if( Den < 0 ) {
		ret = -ret;
		Den = -Den;
	}
	if(ret < 0)
		ret = -__divmod32(Num, Den, (Uint32*)Rem);
	else
		ret = __divmod32(Num, Den, (Uint32*)Rem);
	return ret;
}

#if 0
Sint32 __divsi3(Sint32 Num, Sint32 Den)
{
	return DivMod32S(Num, Den, NULL);
}

Sint32 __modsi3(Sint32 Num, Sint32 Den)
{
	Sint32	rem;
	DivMod32S(Num, Den, &rem);
	return rem;
}
#endif


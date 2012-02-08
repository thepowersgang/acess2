/*
 * Acess2 M68K port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/lib.c
 * - Library functions
 */
#include <acess.h>
#include "../helpers.h"
#include <drv_pci_int.h>

Uint64	__divmod64(Uint64 Num, Uint64 Den, Uint64 *Rem);
Uint64	__udivdi3(Uint64 Num, Uint64 Den);
Uint64	__umoddi3(Uint64 Num, Uint64 Den);

// === CODE ===
void *memcpy(void *__dest, const void *__src, size_t __len)
{
	register Uint8	*dest = __dest;
	register const Uint8 *src = __src;

	while(__len --)
		*dest++ = *src++;

	return __dest;
}

void *memset(void *__dest, int __val, size_t __count)
{
	register Uint8	*dest = __dest;
	
	while(__count --)
		*dest = __val;
	
	return __dest;
}

int memcmp(const void *__p1, const void *__p2, size_t __maxlen)
{
	const char	*m1 = __p1, *m2 = __p2;
	while( __maxlen-- )
	{
		if(*m1 != *m2)	return *m1 - *m2;
	}
	return 0;
}

DEF_DIVMOD(64)

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
		if(Rem)	*Rem = (Uint32)Num % (Uint32)Den;	// Clear high bits
		return (Uint32)Num / (Uint32)Den;
	}

	ret = __divmod64(Num, Den, Rem);
	return ret;
}

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

// ---- PCI (stubbed)
Uint32 PCI_CfgReadDWord(Uint32 Addr)
{
	return 0xFFFFFFFF;
}
void PCI_CfgWriteDWord(Uint32 Addr, Uint32 Data)
{
}


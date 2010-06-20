/* Acess GCC Helper Library
 *
 */
#include <acess/sys.h>
#include <stdint.h>

// === CODE ===
int SoMain()
{
	return 0;
}

// --- Errors ---
void __stack_chk_fail()
{
	write(2, 32, "FATAL ERROR: Stack Check Failed\n");
	_exit(-1);
	for(;;);
}

// --- 64-Bit Math ---
/**
 * \fn uint64_t __udivdi3(uint64_t Num, uint64_t Den)
 * \brief Divide two 64-bit integers
 */
uint64_t __udivdi3(uint64_t Num, uint64_t Den)
{
	#if 0
	uint64_t	ret = 0;
	if(Den == 0)	// Call Div by Zero Error
		__asm__ __volatile__ ("int $0");
	
	if(Den == 1)	return Num;
	if(Den == 2)	return Num >> 1;
	if(Den == 4)	return Num >> 2;
	if(Den == 8)	return Num >> 3;
	if(Den == 16)	return Num >> 4;
	if(Den == 32)	return Num >> 5;
	if(Den == 64)	return Num >> 6;
	if(Den == 128)	return Num >> 7;
	if(Den == 256)	return Num >> 8;
	
	while(Num > Den) {
		ret ++;
		Num -= Den;
	}
	return ret;
	#else
	uint64_t	P[64], q, n;
	 int	i;
	
	if(Den == 0)	__asm__ __volatile__ ("int $0x0");
	// Common speedups
	if(Num <= 0xFFFFFFFF && Den <= 0xFFFFFFFF)
		return Num / Den;
	if(Den == 1)	return Num;
	if(Den == 2)	return Num >> 1;
	if(Den == 16)	return Num >> 4;
	if(Num < Den)	return 0;
	if(Num == Den)	return 1;
	if(Num == Den*2)	return 2;
	
	// Non-restoring division, from wikipedia
	// http://en.wikipedia.org/wiki/Division_(digital)
	P[0] = Num;
	for( i = 0; i < 64; i ++ )
	{
		if( P[i] >= 0 ) {
			q |= (uint64_t)1 << (63-i);
			P[i+1] = 2*P[i] - Den;
		}
		else {
			//q |= 0 << (63-i);
			P[i+1] = 2*P[i] + Den;
		}
	}
	
	n = ~q;
	n = -n;
	q += n;
	
	return q;
	#endif
}

/**
 * \fn uint64_t __umoddi3(uint64_t Num, uint64_t Den)
 * \brief Get the modulus of two 64-bit integers
 */
uint64_t __umoddi3(uint64_t Num, uint64_t Den)
{
	#if 0
	if(Den == 0)	__asm__ __volatile__ ("int $0");	// Call Div by Zero Error
	
	if(Den == 1)	return 0;
	if(Den == 2)	return Num & 0x01;
	if(Den == 4)	return Num & 0x03;
	if(Den == 8)	return Num & 0x07;
	if(Den == 16)	return Num & 0x0F;
	if(Den == 32)	return Num & 0x1F;
	if(Den == 64)	return Num & 0x3F;
	if(Den == 128)	return Num & 0x3F;
	if(Den == 256)	return Num & 0x7F;
	
	while(Num >= Den)	Num -= Den;
	
	return Num;
	#else
	if(Den == 0)	__asm__ __volatile__ ("int $0");	// Call Div by Zero Error
	
	// Speedups
	if(Num < Den)	return Num;
	if(Num == Den)	return 0;
	if(Num <= 0xFFFFFFFF && Den <= 0xFFFFFFFF)
		return Num % Den;
	
	// Speedups for common operations
	if(Den == 1)	return 0;
	if(Den == 2)	return Num & 0x01;
	if(Den == 8)	return Num & 0x07;
	if(Den == 16)	return Num & 0x0F;
	return Num - __udivdi3(Num, Den) * Den;
	#endif
}

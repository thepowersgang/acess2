/* Acess GCC Helper Library
 *
 */
#include <sys/sys.h>

typedef unsigned long long int uint64_t;

// === CODE ===
int SoMain()
{
	return 0;
}

// --- Errors ---
void __stack_chk_fail()
{
	write(1, 32, "FATAL ERROR: Stack Check Failed\n");
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
	uint64_t	ret = 0;
	if(Den == 0)	// Call Div by Zero Error
		__asm__ __volatile__ ("int $0");
	while(Num > Den) {
		ret ++;
		Num -= Den;
	}
	return ret;
}

/**
 * \fn uint64_t __umoddi3(uint64_t Num, uint64_t Den)
 * \brief Get the modulus of two 64-bit integers
 */
uint64_t __umoddi3(uint64_t Num, uint64_t Den)
{
	if(Den == 0)	// Call Div by Zero Error
		__asm__ __volatile__ ("int $0");
	while(Num > Den)
		Num -= Den;
	return Num;
}

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
	write(2, "FATAL ERROR: Stack Check Failed\n", 32);
	_exit(-1);
	for(;;);
}


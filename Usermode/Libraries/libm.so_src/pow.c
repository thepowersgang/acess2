/*
 * Acess2 C Library (Maths)
 * - By John Hodge (thePowersGang)
 *
 * pow.c
 * - pow(), powf() and powl()
 */
#include <math.h>

// === CODE ===
double pow(double x, double y)
{
	if( x == 1.0f )
		return 1.0f;
	if( y == 0.0f )
		return 1.0f;
	return __builtin_pow(x,y);
}

double exp(double y)
{
	if( y == 0.0f )
		return 1.0f;
	return __builtin_exp(y);
}

double log(double n)
{
	if(n == 1.0f)
		return 0.0f;
	return __builtin_log(n);
}

/*
 * Acess2 Lib C
 * - By John Hodge (thePowersGang)
 *
 * rand.c
 * - srand/rand/rand_p functions
 */
#include <stdlib.h>

// === GLOBALS ===
unsigned int	_rand_state_x = 123456789;
unsigned int	_rand_state_y = 362436069;
unsigned int	_rand_state_z = 521288629;
unsigned int	_rand_state_w = 88675123;

// === FUNCTIONS ===
int rand_p(unsigned int *seedp)
{
	const int const_a = 0x731ADE, const_c = 12345;
	// Linear Congruency
	*seedp = *seedp * const_a + const_c;
	return *seedp;
}

void srand(unsigned int seed)
{
	_rand_state_x = rand_p( &seed );
	_rand_state_y = rand_p( &seed );
	_rand_state_z = rand_p( &seed );
	_rand_state_w = rand_p( &seed );
}

int rand(void)
{
	unsigned int t;
	
	t = _rand_state_x ^ (_rand_state_x << 11);
	_rand_state_x = _rand_state_y; _rand_state_y = _rand_state_z; _rand_state_z = _rand_state_w;
	return _rand_state_w = _rand_state_w ^ (_rand_state_w >> 19) ^ t ^ (t >> 8); 
}


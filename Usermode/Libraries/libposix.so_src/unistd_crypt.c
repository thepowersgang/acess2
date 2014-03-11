/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * unistd_crypt.c
 * - crypt(3)
 */
#include <unistd.h>

// === CODE ===
char *crypt(const char *key, const char *salt)
{
	if( *salt == '$' )
	{
		// $x$salt$
		// x=1 : MD5
		// x=5 : SHA-256
		// x=6 : SHA-512
	}
	return "YIH14pBTDJvJ6";	// 'password' with the salt YI
}

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
	return "YIH14pBTDJvJ6";	// 'password' with the salt YI
}

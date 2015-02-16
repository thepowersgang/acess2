#include <stdlib.h>
#include <stdbool.h>

bool _libc_free(void *addr)
{
	free(addr);
	return true;
}

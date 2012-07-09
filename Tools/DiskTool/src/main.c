/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <disktool_common.h>

// === CODE ===
int main(int argc, char *argv[])
{
	// Parse arguments
	for( int i = 1; i < argc; i ++ )
	{
		if( strcmp("--image", argv[i]) == 0 || strcmp("-i", argv[i]) == 0 ) {
			// Mount an image
			if( argc - i < 3 ) {
				fprintf(stderr, "--image/-i takes 2 arguments (ident and path)\n");
				exit(-1);
			}

			if( DiskTool_MountImage(argv[i+1], argv[i+2]) ) {
				fprintf(stderr, "Unable to mount '%s' as '%s'\n", argv[i+2], argv[i+1]);
				exit(-1);
			}

			i += 2;
		}
	}
	return 0;
}

// NOTE: This is in a native compiled file because it needs access to the real errno macro
int *Threads_GetErrno(void)
{
	return &errno;
}

// TODO: Move into a helper lib?
void itoa(char *buf, uint64_t num, int base, int minLength, char pad)
{
	char fmt[] = "%0ll*x";
	switch(base)
	{
	case  8:	fmt[5] = 'o';	break;
	case 10:	fmt[5] = 'd';	break;
	case 16:	fmt[5] = 'x';	break;
	}
	if(pad != '0') {
		fmt[1] = '%';
		sprintf(buf, fmt+1, minLength, num);
	}
	else {
		sprintf(buf, fmt, minLength, num);
	}
}

int strpos(const char *Str, char Ch)
{
	const char *r = strchr(Str, Ch);
	if(!r)	return -1;
	return r - Str;
}

int strucmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

int64_t DivUp(int64_t value, int64_t divisor)
{
	return (value + divisor - 1) / divisor;
}

int64_t timestamp(int sec, int min, int hr, int day, int month, int year)
{
	return 0;
}


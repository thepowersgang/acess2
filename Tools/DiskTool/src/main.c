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
		if( strcmp("mount", argv[i]) == 0 || strcmp("-i", argv[i]) == 0 ) {
			// Mount an image
			if( argc - i < 3 ) {
				fprintf(stderr, "mount takes 2 arguments (image and mountpoint)\n");
				exit(-1);
			}

			if( DiskTool_MountImage(argv[i+2], argv[i+1]) ) {
				fprintf(stderr, "Unable to mount '%s' as '%s'\n", argv[i+1], argv[i+2]);
				break;
			}

			i += 2;
			continue ;
		}
		
		if( strcmp("mountlvm", argv[i]) == 0 || strcmp("lvm", argv[i]) == 0 ) {
			
			if( argc - i < 3 ) {
				fprintf(stderr, "mountlvm takes 2 arguments (iamge and ident)\n");
				exit(-1);
			}

			if( DiskTool_RegisterLVM(argv[i+2], argv[i+1]) ) {
				fprintf(stderr, "Unable to register '%s' as LVM '%s'\n", argv[i+1], argv[i+2]);
				break;
			}
			
			i += 2;
			continue ;
		}
		
		if( strcmp("ls", argv[i]) == 0 ) {
			if( argc - i < 2 ) {
				fprintf(stderr, "ls 1 argument (path)\n");
				break;
			}

			DiskTool_ListDirectory(argv[i+1]);
			i += 1;
			continue ;
		}
		
		if( strcmp("cp", argv[i]) == 0 ) {
			
			if( argc - i < 3 ) {
				fprintf(stderr, "cp takes 2 arguments (source and destination)\n");
				break;
			}

			DiskTool_Copy(argv[i+1], argv[i+2]);			

			i += 2;
			continue ;
		}
	
		fprintf(stderr, "Unknown command '%s'\n", argv[i]);
	}
	
	DiskTool_Cleanup();
	
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

uint64_t DivMod64U(uint64_t value, uint64_t divisor, uint64_t *remainder)
{
	if(remainder)
		*remainder = value % divisor;
	return value / divisor;
}


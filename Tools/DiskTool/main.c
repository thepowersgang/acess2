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
				fprintf(stderr, "lvm takes 2 arguments (iamge and ident)\n");
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
				fprintf(stderr, "ls takes 1 argument (path)\n");
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

		if( strcmp("cat", argv[i]) == 0 ) {

			if( argc - 1 < 2 ) {
				fprintf(stderr, "cat takes 1 argument (path)\n");
				break;
			}

			DiskTool_Cat(argv[i+1]);

			i += 1;
			continue;
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

size_t _fwrite_stdout(size_t bytes, const void *data)
{
	return fwrite(data, bytes, 1, stdout);
}

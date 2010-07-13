/*
 * Acess2 mount command
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>

#define	MOUNTABLE_FILE	"/Acess/Conf/Mountable"
#define	MOUNTED_FILE	"/Devices/System/VFS/Mounts"

// === PROTOTYPES ===
void	ShowUsage(char *ProgName);
 int	GetMountDefs(char *Ident, char **spDevice, char **spDir, char **spType, char **spOptions);

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	fd;
	 int	i;
	char	*arg;
	char	*sType = NULL;
	char	*sDevice = NULL;
	char	*sDir = NULL;
	char	*sOptions = NULL;

	// List mounted filesystems
	// - This is cheating, isn't it?
	if(argc == 1) {
		// Dump the contents of /Devices/system/VFS/Mounts
		FILE	*fp = fopen("/Devices/system/VFS/Mounts", "r");
		char	buf[1024];
		 int	len;
		while( (len = fread(buf, 1024, 1, fp)) )
			fwrite(buf, len, 1, stdout);
		printf("\n");
		return 0;
	}

	if(argc < 3) {
		ShowUsage(argv[0]);
		return EXIT_FAILURE;
	}
	
	// Parse Arguments
	for( i = 1; i < argc; i++ )
	{
		arg = argv[i];
		
		if(arg[0] == '-')
		{
			switch(arg[1])
			{
			// -t <driver> :: Filesystem driver to use
			case 't':	sType = argv[++i];	break;
			case '-':
				//TODO: Long Arguments
			default:
				ShowUsage(argv[0]);
				return EXIT_FAILURE;
			}
			continue;
		}
		
		// Device?
		if(sDevice == NULL) {
			sDevice = arg;
			continue;
		}
		
		// Directory?
		if(sDir == NULL) {
			sDir = arg;
			continue;
		}
		
		ShowUsage(argv[0]);
		return EXIT_FAILURE;
	}

	// Check if we even got a device/mountpoint
	if(sDevice == NULL) {
		ShowUsage(argv[0]);
		return EXIT_FAILURE;
	}

	// If no directory was passed (we want to use the mount list)
	// or we are not root (we need to use the mount list)
	// Check the mount list
	if(sDir == NULL || getuid() != 0)
	{
		// Check if it is defined in the mounts file
		// - At this point sDevice could be a device name or a mount point
		if(GetMountDefs(sDevice, &sDevice, &sDir, &sType, &sOptions) == 0)
		{
			if(sDir == NULL)
				fprintf(stderr, "Unable to find '%s' in '%s'\n",
					sDevice, MOUNTABLE_FILE
					);
			else
				fprintf(stderr, "You must be root to mount devices or directories not in '%s'\n",
					MOUNTABLE_FILE
					);
			return EXIT_FAILURE;
		}
	
		// We need to be root to mount a filesystem, so, let us be elevated!
		setuid(0);	// I hope I have the setuid bit implemented.
	}
	else
	{
		// Check that we were passed a filesystem type
		if(sType == NULL) {
			fprintf(stderr, "Please pass a filesystem type\n");
			return EXIT_FAILURE;
		}
	}
	
	// Check Device
	fd = open(sDevice, OPENFLAG_READ);
	if(fd == -1) {
		printf("Device '%s' cannot be opened for reading\n", sDevice);
		return EXIT_FAILURE;
	}
	close(fd);
	
	// Check Mount Point
	fd = open(sDir, OPENFLAG_EXEC);
	if(fd == -1) {
		printf("Directory '%s' does not exist\n", sDir);
		return EXIT_FAILURE;
	}
	close(fd);

	// Replace sOptions with an empty string if it is still NULL
	if(sOptions == NULL)	sOptions = "";

	// Let's Mount!
	_SysMount(sDevice, sDir, sType, sOptions);

	return 0;
}

void ShowUsage(char *ProgName)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s [-t <type>] <device> <directory>\n", ProgName);
	fprintf(stderr, "or  %s <device>\n", ProgName);
	fprintf(stderr, "or  %s <directory>\n", ProgName);
	fprintf(stderr, "or  %s\n", ProgName);
}

/**
 * \fn int GetMountDefs(char *Ident, char **spDevice, char **spDir, char **spType, char **spOptions)
 * \brief Reads the mountable definitions file and returns the corresponding entry
 * \param spDevice	Pointer to a string (pointer) determining the device (also is the input for this function)
 * \note STUB
 */
int GetMountDefs(char *Ident, char **spDevice, char **spDir, char **spType, char **spOptions)
{
	// TODO: Read the mounts file (after deciding what it will be)
	return 0;
}

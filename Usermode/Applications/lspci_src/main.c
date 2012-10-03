/*
 * Acess2 lspci
 * - By John Hodge (thePowersGang)
 *
 * main.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <acess/sys.h>

#define PCI_BASE	"/Devices/pci"
// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	show_device(int PFD, const char *File, int bVerbose);

// === CODE ===
int main(int argc, char *argv[])
{
	int fd = open(PCI_BASE, OPENFLAG_READ);

	char name[256];	

	while( SysReadDir(fd, name) )
	{
		if(name[0] == '.')	continue ;
		
		show_device(fd, name, 0);
	}

	return 0;
}

void show_device(int PFD, const char *File, int bVerbose)
{
	 int	fd;
	 int	rv;

	struct {
		uint16_t	vendor;
		uint16_t	device;
		uint32_t	_unused;
		uint32_t	revclass;
	} pciinfo;

	fd = _SysOpenChild(PFD, File, OPENFLAG_READ);
	if( fd == -1 ) {
		printf("%s - ERR (open failure)\n", File);
		return ;
	}
	rv = read(fd, &pciinfo, sizeof(pciinfo));
	if( rv != sizeof(pciinfo) ) {
		printf("%s - ERR (read %i < %i)\n", File, rv, sizeof(pciinfo));
		close(fd);
		return ;
	}
	uint32_t	class_if = pciinfo.revclass >> 8;
	uint8_t 	revision = pciinfo.revclass & 0xFF;
	printf("%s - %04x:%04x %06x:%02x\n",
		File,
		pciinfo.vendor, pciinfo.device,
		class_if, revision
		);

	if( bVerbose )
	{
		printf("\n");
	}

	close(fd);
}


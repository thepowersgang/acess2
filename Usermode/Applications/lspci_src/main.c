/*
 * Acess2 lspci
 * - By John Hodge (thePowersGang)
 *
 * main.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <acess/sys.h>
#include <string.h>

#define PCI_BASE	"/Devices/pci"
// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	ParseCommandline(int argc, char *argv[]);
void	show_device(int PFD, const char *File, int bVerbose);

// === GLOBALS ===
 int	gbEnableVerbose = 0;
const char	*gsDeviceName;

// === CODE ===
int main(int argc, char *argv[])
{
	 int	rv;
	
	rv = ParseCommandline(argc, argv);
	if(rv)	return rv;

	int fd = _SysOpen(PCI_BASE, OPENFLAG_READ);
	if( gsDeviceName )
	{
		show_device(fd, gsDeviceName, gbEnableVerbose);
	}
	else
	{
		char name[256];	
		while( _SysReadDir(fd, name) )
		{
			if(name[0] == '.')	continue ;
			
			show_device(fd, name, gbEnableVerbose);
		}
	}
	_SysClose(fd);

	return 0;
}

void ShowUsage(const char *progname)
{
	fprintf(stderr, "Usage: %s [-v|--verbose] [<pcidev>]\n", progname);
	fprintf(stderr,
		"\n"
		"-v | --verbose	: Enable dump of BARs\n"
		"\n"
		);
}

int ParseCommandline(int argc, char *argv[])
{
	for( int i = 1; i < argc; i ++ )
	{
		const char *arg = argv[i];
		if( arg[0] != '-' ) {
			// Show an individual device
			gsDeviceName = arg;
		}
		else if( arg[1] != '-' ) {
			// Short args
			switch(arg[1])
			{
			case 'v':
				gbEnableVerbose ++;
				break;
			default:
				ShowUsage(argv[0]);
				return 1;
			}
		}
		else {
			// Long args
			if(strcmp(arg, "--verbose") == 0) {
				gbEnableVerbose ++;
			}
			else {
				ShowUsage(argv[0]);
				return 1;
			}
		}
	}
	return 0;
}

const char *get_device_class(uint32_t revclass)
{
	uint8_t	class = revclass >> 24;
	uint8_t	subclass = revclass >> 16;
	switch(class)
	{
	case 0x00:
		switch( subclass )
		{
		case 0x01:	return "VGA-Compatible";
		}
		return "Pre Classcodes";
	case 0x01:
		switch( subclass )
		{
		case 0x00:	return "SCSI Bus Controller";
		case 0x01:	return "IDE Controller";
		case 0x02:	return "Floppy Disk Controller";
		}
		return "Mass Storage Controller";
	case 0x02:	return "Network Controller";
	case 0x03:	return "Display Controller";
	case 0x04:	return "Multimedia Controller";
	case 0x05:	return "Memory Controller";
	case 0x06:	return "Bridge Device";
	case 0x07:	return "Simple Communications Controller";
	case 0x08:	return "Base System Peripherals";
	case 0x09:	return "Input Device";
	case 0x0A:	return "Docing Station";
	case 0x0B:	return "Processor";
	case 0x0C:	return "Serial Bus Controller";
	}
	return "";
}

void show_device(int PFD, const char *File, int bVerbose)
{
	 int	fd;
	 int	rv;

	struct {
		uint16_t	vendor;
		uint16_t	device;
		uint16_t	command;
		uint16_t	status;
		uint32_t	revclass;
		uint32_t	_unused2;
		uint32_t	bar[6];
	} pciinfo;

	fd = _SysOpenChild(PFD, File, OPENFLAG_READ);
	if( fd == -1 ) {
		printf("%s - ERR (open failure)\n", File);
		return ;
	}
	rv = _SysRead(fd, &pciinfo, sizeof(pciinfo));
	if( rv != sizeof(pciinfo) ) {
		printf("%s - ERR (read %i < %i)\n", File, rv, sizeof(pciinfo));
		_SysClose(fd);
		return ;
	}
	uint32_t	class_if = pciinfo.revclass >> 8;
	uint8_t 	revision = pciinfo.revclass & 0xFF;
	printf("%s - %04x:%04x %06x:%02x %s\n",
		File,
		pciinfo.vendor, pciinfo.device,
		class_if, revision,
		get_device_class(pciinfo.revclass)
		);

	if( bVerbose )
	{
		printf("Command: ");
		if(pciinfo.command & (1 <<10))	printf("INTx# Disabled, ");
		if(pciinfo.command & (1 << 2))	printf("Bus Master, ");
		if(pciinfo.command & (1 << 1))	printf("MMIO Enabled, ");
		if(pciinfo.command & (1 << 0))	printf("IO Enabled, ");
		printf("\n");
		for( int i = 0; i < 6; i ++ )
		{
			uint32_t	bar = pciinfo.bar[i];
			if( bar == 0 )
				continue ;
			printf("BAR%i: ", i);
			if( bar & 1 ) {
				printf("IO  0x%02x", bar & ~3);
			}
			else {
				// Memory type
				switch( (bar & 6) >> 1 )
				{
				case 0:
					printf("Mem32 0x%08x", bar & ~15);
					break;
				case 2:
					printf("Mem20 0x%05x", bar & ~15);
					break;
				case 4:
					printf("Mem64 0x%08x%08x", pciinfo.bar[i+1], bar & ~15);
					i ++;
					break;
				case 6:
					printf("UNK %08x", bar & ~15);
					break;
				}
				printf(" %s", (bar & 8 ? "Prefetchable" : ""));
			}
			printf("\n");
		}
		printf("\n");
	}

	_SysClose(fd);
}


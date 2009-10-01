/*
 * AcessOS 0.1
 * Floppy Disk Access Code
 */
#define DEBUG	0
#include <common.h>
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <dma.h>

#define WARN	0

// Version Information
#define FDD_VER_MAJ	 0
#define FDD_VER_MIN	75

#define USE_CACHE	1	// Use Sector Cache
#define	CACHE_SIZE	32	// Number of cachable sectors
#define FDD_SEEK_TIMEOUT	10	// Timeout for a seek operation
#define MOTOR_ON_DELAY	500		// Miliseconds
#define MOTOR_OFF_DELAY	2000	// Miliseconds

// === TYPEDEFS ===
typedef struct {
	 int	type;
	volatile int	motorState;	//2 - On, 1 - Spinup, 0 - Off
	 int	track[2];
	 int	timer;
	char	Name[2];
	tVFS_Node	Node;
} t_floppyDevice;

typedef struct {
	Uint64	timestamp;
	Uint16	disk;
	Uint16	sector;	// Allows 32Mb of addressable space (Plenty for FDD)
	char	data[512];
} t_floppySector;

// === CONSTANTS ===
static const char *cFDD_TYPES[] = {"None", "360kB 5.25\"", "1.2MB 5.25\"", "720kB 3.5\"", "1.44MB 3.5\"", "2.88MB 3.5\"" };
static const int cFDD_SIZES[] = { 0, 360*1024, 1200*1024, 720*1024, 1440*1024, 2880*1024 };
static const short	cPORTBASE[] = {0x3F0, 0x370 };

enum FloppyPorts {
	PORT_STATUSA	= 0x0,
	PORT_STATUSB	= 0x1,
	PORT_DIGOUTPUT	= 0x2,
	PORT_MAINSTATUS	= 0x4,
	PORT_DATARATE	= 0x4,
	PORT_DATA		= 0x5,
	PORT_DIGINPUT	= 0x7,
	PORT_CONFIGCTRL	= 0x7
};

enum FloppyCommands {
	FIX_DRIVE_DATA	= 0x03,
	HECK_DRIVE_STATUS	= 0x04,
	CALIBRATE_DRIVE	= 0x07,
	CHECK_INTERRUPT_STATUS = 0x08,
	SEEK_TRACK		= 0x0F,
	READ_SECTOR_ID	= 0x4A,
	FORMAT_TRACK	= 0x4D,
	READ_TRACK		= 0x42,
	READ_SECTOR		= 0x66,
	WRITE_SECTOR	= 0xC5,
	WRITE_DELETE_SECTOR	= 0xC9,
	READ_DELETE_SECTOR	= 0xCC,
};

// === PROTOTYPES ===
char	*FDD_ReadDir(tVFS_Node *Node, int pos);
tVFS_Node	*FDD_FindDir(tVFS_Node *dirNode, char *Name);
static int	fdd_readSector(int disk, int lba, void *buf);
void	FDD_WaitIRQ();
void	FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl);
static void	FDD_AquireSpinlock();
static void	inline FDD_FreeSpinlock();
#if USE_CACHE
static inline void FDD_AquireCacheSpinlock();
static inline void FDD_FreeCacheSpinlock();
#endif
static void	sendbyte(int base, char byte);
static int	getbyte(int base);
static int	seekTrack(int disk, int head, int track);
static void	stop_motor(int disk);
static void	start_motor(int disk);
static int	get_dims(int type, int lba, int *c, int *h, int *s, int *spt);
 int	FDD_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	FDD_ReadFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
 int	FDD_Install(char **Arguments);

// === GLOBALS ===
//MODULE_DEFINE(0, 0x004B, FDD, FDD_Install, NULL, NULL);
static t_floppyDevice	fdd_devices[2];
static volatile int	fdd_inUse = 0;
static volatile int	fdd_irq6 = 0;
tDevFS_Driver	gFDD_DriverInfo = {
	NULL, "fdd",
	{
	.Size = -1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ReadDir = FDD_ReadDir,
	.FindDir = FDD_FindDir,
	.IOCtl = FDD_IOCtl
	}
};
#if USE_CACHE
static int	siFDD_CacheInUse = 0;
static int	siFDD_SectorCacheSize = CACHE_SIZE;
static t_floppySector	sFDD_SectorCache[CACHE_SIZE];
#endif

//=== CODE ===
/**
 * \fn char *FDD_ReadDir(tVFS_Node *Node, int pos)
 * \brief Read Directory
 */
char *FDD_ReadDir(tVFS_Node *Node, int pos)
{
	//Update Accessed Time
	//gFDD_DrvInfo.rootNode.atime = now();
	
	//Check for bounds
	if(pos >= 2 || pos < 0)
		return NULL;
	
	if(fdd_devices[pos].type == 0)
		return VFS_SKIP;
	
	//Return
	return fdd_devices[pos].Name;
}

/**
 * \fn tVFS_Node *FDD_FindDir(tVFS_Node *Node, char *filename);
 * \brief Find File Routine (for vfs_node)
 */
tVFS_Node *FDD_FindDir(tVFS_Node *Node, char *filename)
{
	int i;
	
	ENTER("sfilename", filename);
	
	if(filename == NULL)	return NULL;
	
	//Check string length (should be 1)
	if(filename[0] == '\0')	return NULL;
	if(filename[1] != '\0')	return NULL;
	
	//Get First char
	i = filename[0] - '0';
	
	// Check for 1st disk and if it is present return
	if(i == 0 && fdd_devices[0].type != 0)
		return &fdd_devices[0].Node;
	
	// Check for 2nd disk and if it is present return
	if(i == 1 && fdd_devices[1].type != 0)
		return &fdd_devices[1].Node;
	
	// Else return null
	return NULL;
}

/**
 * \fn Uint64 fdd_readFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
 * \brief Read Data from a disk
*/
Uint64 FDD_ReadFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer)
{
	int i = 0;
	int	disk;
	Uint32	buf[128];
	
	ENTER("xoff xlen pbuffer", off, len, buffer)
	
	if(node == NULL) {
		LEAVE('i', -1);
		return -1;
	}
	
	if(node->Inode != 0 && node->Inode != 1) {
		LEAVE('i', -1);
		return -1;
	}
	
	disk = node->Inode;
	
	// Update Accessed Time
	node->ATime = now();
	
	if((off & 0x1FF) || (len & 0x1FF))
	{
		// Un-Aligned Offset/Length
		 int	startOff = off>>9;
		 int	sectOff = off&0x1FF;
		 int	sectors = (len+0x1FF)>>9;
	
		LOG("Non-aligned Read");
		
		//Read Starting Sector
		if(!fdd_readSector(disk, startOff, buf))
			return 0;
		memcpy(buffer, (char*)(buf+sectOff), len>512-sectOff?512-sectOff:len);
		
		//If the data size is one sector or less
		if(len <= 512-sectOff) {
			LEAVE('X', len);
			return len;	//Return
		}
		buffer += 512-sectOff;
	
		//Read Middle Sectors
		for(i=1;i<sectors-1;i++)	{
			if(!fdd_readSector(disk, startOff+i, buf)) {
				LEAVE('i', -1);
				return -1;
			}
			memcpy(buffer, buf, 512);
			buffer += 512;
		}
	
		//Read End Sectors
		if(!fdd_readSector(disk, startOff+i, buf))
			return 0;
		memcpy(buffer, buf, (len&0x1FF)-sectOff);
		
		LEAVE('X', len);
		return len;
	}
	else
	{
		 int	count = len >> 9;
		 int	sector = off >> 9;
		LOG("Aligned Read");
		//Aligned Offset and Length - Simple Code
		for(i=0;i<count;i++)
		{
			fdd_readSector(disk, sector, buf);
			memcpy(buffer, buf, 512);
			buffer += 512;
			sector++;
		}
		LEAVE('i', len);
		return len;
	}
}

/**
 * \fn static int fdd_readSector(int disk, int lba, void *buf)
 * \fn Read a sector from disk
*/
int fdd_readSector(int disk, int lba, void *buf)
{
	 int	cyl, head, sec;
	 int	spt, base;
	 int	i;
	
	ENTER("idisk xlba pbuf", disk, lba, buf);
	
	#if USE_CACHE
	FDD_AquireCacheSpinlock();
	for(i=0;i<siFDD_SectorCacheSize;i++)
	{
		if(sFDD_SectorCache[i].timestamp == 0)	continue;
		if(sFDD_SectorCache[i].disk == disk
		&& sFDD_SectorCache[i].sector == lba) {
			LOG("Found %i in cache %i", lba, i);
			memcpy(buf, sFDD_SectorCache[i].data, 512);
			sFDD_SectorCache[i].timestamp = now();
			FDD_FreeCacheSpinlock();
			LEAVE('i', 1);
			return 1;
		}
	}
	LOG("Read %i from Disk", lba);
	FDD_FreeCacheSpinlock();
	#endif
	
	base = cPORTBASE[disk>>1];
	
	LOG("Calculating Disk Dimensions");
	//Get CHS position
	if(get_dims(fdd_devices[disk].type, lba, &cyl, &head, &sec, &spt) != 1) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Remove Old Timer
	Time_RemoveTimer(fdd_devices[disk].timer);
	// Check if Motor is on
	if(fdd_devices[disk].motorState == 0) {
		start_motor(disk);
	}
	
	LOG("Wait for Motor Spinup");
	
	// Wait for spinup
	while(fdd_devices[disk].motorState == 1)	Threads_Yield();
	
	LOG("C:%i,H:%i,S:%i", cyl, head, sec);
	LOG("Acquire Spinlock");
	
	FDD_AquireSpinlock();
	
	// Seek to track
	outb(base+CALIBRATE_DRIVE, 0);
	i = 0;
	while(seekTrack(disk, head, (Uint8)cyl) == 0 && i++ < FDD_SEEK_TIMEOUT )	Threads_Yield();
	//FDD_SensInt(base, NULL, NULL);	// Wait for IRQ
	
	LOG("Setting DMA for read");
	
	//Read Data from DMA
	DMA_SetChannel(2, 512, 1);	// Read 512 Bytes
	
	LOG("Sending read command");
	
	//Threads_Wait(100);	// Wait for Head to settle
	Time_Delay(100);
	sendbyte(base, READ_SECTOR);	// Was 0xE6
	sendbyte(base, (head << 2) | (disk&1));
	sendbyte(base, (Uint8)cyl);
	sendbyte(base, (Uint8)head);
	sendbyte(base, (Uint8)sec);
	sendbyte(base, 0x02);	// Bytes Per Sector (Real BPS=128*2^{val})
	sendbyte(base, spt);	// SPT
	sendbyte(base, 0x1B);	// Gap Length (27 is default)
	sendbyte(base, 0xFF);	// Data Length
	
	// Wait for IRQ
	LOG("Waiting for Data to be read");
	FDD_WaitIRQ();
	
	// Read Data from DMA
	LOG(" fdd_readSector: Reading Data");
	DMA_ReadData(2, 512, buf);
	
	// Clear Input Buffer
	LOG("Clearing Input Buffer");
	getbyte(base); getbyte(base); getbyte(base);
	getbyte(base); getbyte(base); getbyte(base); getbyte(base);
	
	LOG("Realeasing Spinlock and Setting motor to stop");
	// Release Spinlock
	FDD_FreeSpinlock();
	
	//Set timer to turn off motor affter a gap
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, stop_motor, (void*)disk);	//One Shot Timer

	#if USE_CACHE
	{
		FDD_AquireCacheSpinlock();
		int oldest = 0;
		for(i=0;i<siFDD_SectorCacheSize;i++)
		{
			if(sFDD_SectorCache[i].timestamp == 0) {
				oldest = i;
				break;
			}
			if(sFDD_SectorCache[i].timestamp < sFDD_SectorCache[oldest].timestamp)
				oldest = i;
		}
		sFDD_SectorCache[oldest].timestamp = now();
		sFDD_SectorCache[oldest].disk = disk;
		sFDD_SectorCache[oldest].sector = lba;
		memcpy(sFDD_SectorCache[oldest].data, buf, 512);
		FDD_FreeCacheSpinlock();
	}
	#endif

	LEAVE('i', 1);
	return 1;
}

/**
 * \fn static int seekTrack(int disk, int track)
 * \brief Seek disk to selected track
 */
static int seekTrack(int disk, int head, int track)
{
	Uint8	sr0, cyl;
	 int	base;
	
	base = cPORTBASE[disk>>1];
	
	// Check if seeking is needed
	if(fdd_devices[disk].track[head] == track)
		return 1;
	
	// - Seek Head 0
	sendbyte(base, SEEK_TRACK);
	sendbyte(base, (head<<2)|(disk&1));
	sendbyte(base, track);	// Send Seek command
	FDD_WaitIRQ();
	FDD_SensInt(base, &sr0, &cyl);	// Wait for IRQ
	if((sr0 & 0xF0) != 0x20) {
		LOG("sr0 = 0x%x", sr0);
		return 0;	//Check Status
	}
	if(cyl != track)	return 0;
	
	// Set Track in structure
	fdd_devices[disk].track[head] = track;
	return 1;
}

/**
 * \fn static int get_dims(int type, int lba, int *c, int *h, int *s, int *spt)
 * \brief Get Dimensions of a disk
 */
static int get_dims(int type, int lba, int *c, int *h, int *s, int *spt)
{
	switch(type) {
	case 0:
		return 0;
	
	// 360Kb 5.25"
	case 1:
		*spt = 9;
		*s = (lba % 9) + 1;
		*c = lba / 18;
		*h = (lba / 9) & 1;
		break;
	
	// 1220Kb 5.25"
	case 2:
		*spt = 15;
		*s = (lba % 15) + 1;
		*c = lba / 30;
		*h = (lba / 15) & 1;
		break;
	
	// 720Kb 3.5"
	case 3:
		*spt = 9;
		*s = (lba % 9) + 1;
		*c = lba / 18;
		*h = (lba / 9) & 1;
		break;
	
	// 1440Kb 3.5"
	case 4:
		*spt = 18;
		*s = (lba % 18) + 1;
		*c = lba / 36;
		*h = (lba / 18) & 1;
		break;
		
	// 2880Kb 3.5"
	case 5:
		*spt = 36;
		*s = (lba % 36) + 1;
		*c = lba / 72;
		*h = (lba / 32) & 1;
		break;
		
	default:
		return -2;
	}
	return 1;
}

/**
 * \fn int FDD_IOCtl(tVFS_Node *node, int id, void *data)
 * \brief Stub ioctl function
 */
int FDD_IOCtl(tVFS_Node *node, int id, void *data)
{
	switch(id)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_DISK;
	case DRV_IOCTL_IDENT:	memcpy(data, "FDD\0", 4);	return 1;
	case DRV_IOCTL_VERSION:	return (FDD_VER_MAJ<<8)|FDD_VER_MIN;
	default:	return 0;
	}
}

/**
 * \fn void fdd_handler(void)
 * \brief Handles IRQ6
 */
void fdd_handler(void)
{
    fdd_irq6 = 1;
}

/**
 * \fn FDD_WaitIRQ()
 * \brief Wait for an IRQ6
 */
void FDD_WaitIRQ()
{
	// Wait for IRQ
	while(!fdd_irq6)	Threads_Yield();
	fdd_irq6 = 0;
}

void FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl)
{
	sendbyte(base, CHECK_INTERRUPT_STATUS);
	if(sr0)	*sr0 = getbyte(base);
	else	getbyte(base);
	if(cyl)	*cyl = getbyte(base);
	else	getbyte(base);
}

void FDD_AquireSpinlock()
{
	while(fdd_inUse)
		Threads_Yield();
	fdd_inUse = 1;
}

inline void FDD_FreeSpinlock()
{
	fdd_inUse = 0;
}

#if USE_CACHE
inline void FDD_AquireCacheSpinlock()
{
	while(siFDD_CacheInUse)	Threads_Yield();
	siFDD_CacheInUse = 1;
}
inline void FDD_FreeCacheSpinlock()
{
	siFDD_CacheInUse = 0;
}
#endif

/**
 * void sendbyte(int base, char byte)
 * \brief Sends a command to the controller
 */
static void sendbyte(int base, char byte)
{
    volatile int state;
    int timeout = 128;
    for( ; timeout--; )
    {
        state = inb(base + PORT_MAINSTATUS);
        if ((state & 0xC0) == 0x80)
        {
            outb(base + PORT_DATA, byte);
            return;
        }
        inb(0x80);	//Delay
    }
	#if WARN
		Warning("FDD_SendByte - Timeout sending byte 0x%x to base 0x%x\n", byte, base);
	#endif
}

/**
 * int getbyte(int base, char byte)
 * \brief Receive data from fdd controller
 */
static int getbyte(int base)
{
    volatile int state;
    int timeout;
    for( timeout = 128; timeout--; )
    {
        state = inb((base + PORT_MAINSTATUS));
        if ((state & 0xd0) == 0xd0)
	        return inb(base + PORT_DATA);
        inb(0x80);
    }
    return -1;
}

void FDD_Recalibrate(int disk)
{
	ENTER("idisk", disk);
	
	LOG("Starting Motor");
	start_motor(disk);
	// Wait for Spinup
	while(fdd_devices[disk].motorState == 1)	Threads_Yield();
	
	LOG("Sending Calibrate Command");
	sendbyte(cPORTBASE[disk>>1], CALIBRATE_DRIVE);
	sendbyte(cPORTBASE[disk>>1], disk&1);
	
	LOG("Waiting for IRQ");
	FDD_WaitIRQ();
	FDD_SensInt(cPORTBASE[disk>>1], NULL, NULL);
	
	LOG("Stopping Motor");
	stop_motor(disk);
	LEAVE('-');
}

void FDD_Reset(int id)
{
	int base = cPORTBASE[id];
	
	ENTER("iID", id);
	
	outb(base + PORT_DIGOUTPUT, 0);	// Stop Motors & Disable FDC
	outb(base + PORT_DIGOUTPUT, 0x0C);	// Re-enable FDC (DMA and Enable)
	
	LOG("Awaiting IRQ");
	
	FDD_WaitIRQ();
	FDD_SensInt(base, NULL, NULL);
	
	LOG("Setting Driver Info");
	outb(base + PORT_DATARATE, 0);	// Set data rate to 500K/s
	sendbyte(base, FIX_DRIVE_DATA);	// Step and Head Load Times
	sendbyte(base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	sendbyte(base, 0x02);	// Head Load Time >> 1
	while(seekTrack(0, 0, 1) == 0);	// set track
	while(seekTrack(0, 1, 1) == 0);	// set track
	
	LOG("Recalibrating Disk");
	FDD_Recalibrate((id<<1)|0);
	FDD_Recalibrate((id<<1)|1);

	LEAVE('-');
}

/**
 * \fn void fdd_timer()
 * \brief Called by timer
 */
static void fdd_timer(int arg)
{
	ENTER("iarg", arg);
	if(fdd_devices[arg].motorState == 1)
		fdd_devices[arg].motorState = 2;
	Time_RemoveTimer(fdd_devices[arg].timer);
	fdd_devices[arg].timer = -1;
	LEAVE('-');
}

/**
 * \fn void start_motor(char disk)
 * \brief Starts FDD Motor
 */
static void start_motor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state |= 1 << (4+disk);
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
	fdd_devices[disk].motorState = 1;
	fdd_devices[disk].timer = Time_CreateTimer(MOTOR_ON_DELAY, fdd_timer, (void*)disk);	//One Shot Timer
}

/**
 * \fn void stop_motor(int disk)
 * \brief Stops FDD Motor
 */
static void stop_motor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state &= ~( 1 << (4+disk) );
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
    fdd_devices[disk].motorState = 0;
}

/**
 * \fn int FDD_Install(char **Arguments)
 * \brief Installs floppy driver
 */
int FDD_Install(char **Arguments)
{
	Uint8 data;
	
	// Determine Floppy Types (From CMOS)
	outb(0x70, 0x10);
	data = inb(0x71);
	fdd_devices[0].type = data >> 4;
	fdd_devices[1].type = data & 0xF;
	fdd_devices[0].track[0] = -1;
	fdd_devices[1].track[1] = -1;
	
	// Clear FDD IRQ Flag
	FDD_SensInt(0x3F0, NULL, NULL);
	// Install IRQ6 Handler
	IRQ_AddHandler(6, fdd_handler);
	// Reset Primary FDD Controller
	FDD_Reset(0);
	
	Log("[FDD ] Detected Disk 0: %s and Disk 1: %s\n", cFDD_TYPES[data>>4], cFDD_TYPES[data&0xF]);
	
	// Initialise Root Node
	gFDD_DriverInfo.RootNode.CTime = gFDD_DriverInfo.RootNode.MTime
		= gFDD_DriverInfo.RootNode.ATime = now();
	
	// Initialise Child Nodes
	fdd_devices[0].Name[0] = '0';	fdd_devices[0].Name[1] = '\0';
	fdd_devices[0].Node.Inode = 0;
	fdd_devices[0].Node.Flags = 0;
	fdd_devices[0].Node.NumACLs = 0;
	fdd_devices[0].Node.Read = FDD_ReadFS;
	fdd_devices[0].Node.Write = NULL;//fdd_writeFS;
	memcpy(&fdd_devices[1].Node, &fdd_devices[0].Node, sizeof(tVFS_Node));
	fdd_devices[1].Name[0] = '1';
	fdd_devices[1].Node.Inode = 1;
	
	// Set Lengths
	fdd_devices[0].Node.Size = cFDD_SIZES[data >> 4];
	fdd_devices[1].Node.Size = cFDD_SIZES[data & 0xF];
	
	// Create Sector Cache
	#if USE_CACHE
	//sFDD_SectorCache = malloc(sizeof(*sFDD_SectorCache)*CACHE_SIZE);
	//siFDD_SectorCacheSize = CACHE_SIZE;
	#endif
	
	// Register with devfs
	DevFS_AddDevice(&gFDD_DriverInfo);
	
	return 0;
}

/**
 * \fn void ModuleUnload()
 * \brief Prepare the module for removal
 */
void ModuleUnload()
{
	int i;
	FDD_AquireSpinlock();
	for(i=0;i<4;i++) {
		Time_RemoveTimer(fdd_devices[i].timer);
		stop_motor(i);
	}
	//IRQ_Clear(6);
}

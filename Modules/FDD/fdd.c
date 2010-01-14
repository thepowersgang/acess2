/*
 * AcessOS 0.1
 * Floppy Disk Access Code
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_disk.h>
#include <dma.h>
#include <iocache.h>

#define WARN	0

// === CONSTANTS ===
// --- Current Version
#define FDD_VERSION	 ((0<<8)|(75))

// --- Options
#define USE_CACHE	0	// Use Sector Cache
#define	CACHE_SIZE	32	// Number of cachable sectors
#define FDD_SEEK_TIMEOUT	10	// Timeout for a seek operation
#define MOTOR_ON_DELAY	500		// Miliseconds
#define MOTOR_OFF_DELAY	2000	// Miliseconds

// === TYPEDEFS ===
/**
 * \brief Representation of a floppy drive
 */
typedef struct {
	 int	type;
	volatile int	motorState;	//2 - On, 1 - Spinup, 0 - Off
	 int	track[2];
	 int	timer;
	tVFS_Node	Node;
	#if !USE_CACHE
	tIOCache	*CacheHandle;
	#endif
} t_floppyDevice;

/**
 * \brief Cached Sector
 */
typedef struct {
	Uint64	timestamp;
	Uint16	disk;
	Uint16	sector;	// Allows 32Mb of addressable space (Plenty for FDD)
	Uint8	data[512];
} t_floppySector;

// === CONSTANTS ===
static const char	*cFDD_TYPES[] = {"None", "360kB 5.25\"", "1.2MB 5.25\"", "720kB 3.5\"", "1.44MB 3.5\"", "2.88MB 3.5\"" };
static const int	cFDD_SIZES[] = { 0, 360*1024, 1200*1024, 720*1024, 1440*1024, 2880*1024 };
static const short	cPORTBASE[] = { 0x3F0, 0x370 };

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
// --- Filesystem
 int	FDD_Install(char **Arguments);
char	*FDD_ReadDir(tVFS_Node *Node, int pos);
tVFS_Node	*FDD_FindDir(tVFS_Node *dirNode, char *Name);
 int	FDD_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	FDD_ReadFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
// --- 1st Level Disk Access
Uint	FDD_ReadSectors(Uint64 SectorAddr, Uint Count, void *Buffer, Uint Disk);
// --- Raw Disk Access
 int	FDD_ReadSector(Uint32 disk, Uint64 lba, void *Buffer);
 int	FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer);
// --- Helpers
void	FDD_IRQHandler(int Num);
void	FDD_WaitIRQ();
void	FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl);
inline void	FDD_AquireSpinlock();
inline void	FDD_FreeSpinlock();
#if USE_CACHE
inline void FDD_AquireCacheSpinlock();
inline void FDD_FreeCacheSpinlock();
#endif
void	FDD_int_SendByte(int base, char byte);
 int	FDD_int_GetByte(int base);
void	FDD_Reset(int id);
void	FDD_Recalibrate(int disk);
 int	FDD_int_SeekTrack(int disk, int head, int track);
void	FDD_int_TimerCallback(int arg);
void	FDD_int_StopMotor(int disk);
void	FDD_int_StartMotor(int disk);
 int	FDD_int_GetDims(int type, int lba, int *c, int *h, int *s, int *spt);

// === GLOBALS ===
MODULE_DEFINE(0, FDD_VERSION, FDD, FDD_Install, NULL, NULL);
t_floppyDevice	gFDD_Devices[2];
volatile int	fdd_inUse = 0;
volatile int	fdd_irq6 = 0;
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
int	siFDD_CacheInUse = 0;
int	siFDD_SectorCacheSize = CACHE_SIZE;
t_floppySector	sFDD_SectorCache[CACHE_SIZE];
#endif

// === CODE ===
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
	gFDD_Devices[0].type = data >> 4;
	gFDD_Devices[1].type = data & 0xF;
	gFDD_Devices[0].track[0] = -1;
	gFDD_Devices[1].track[1] = -1;
	
	Log("[FDD ] Detected Disk 0: %s and Disk 1: %s", cFDD_TYPES[data>>4], cFDD_TYPES[data&0xF]);
	
	// Clear FDD IRQ Flag
	FDD_SensInt(0x3F0, NULL, NULL);
	// Install IRQ6 Handler
	IRQ_AddHandler(6, FDD_IRQHandler);
	// Reset Primary FDD Controller
	FDD_Reset(0);
	
	// Initialise Root Node
	gFDD_DriverInfo.RootNode.CTime = gFDD_DriverInfo.RootNode.MTime
		= gFDD_DriverInfo.RootNode.ATime = now();
	
	// Initialise Child Nodes
	gFDD_Devices[0].Node.Inode = 0;
	gFDD_Devices[0].Node.Flags = 0;
	gFDD_Devices[0].Node.NumACLs = 0;
	gFDD_Devices[0].Node.Read = FDD_ReadFS;
	gFDD_Devices[0].Node.Write = NULL;//fdd_writeFS;
	memcpy(&gFDD_Devices[1].Node, &gFDD_Devices[0].Node, sizeof(tVFS_Node));
	
	gFDD_Devices[1].Node.Inode = 1;
	
	// Set Lengths
	gFDD_Devices[0].Node.Size = cFDD_SIZES[data >> 4];
	gFDD_Devices[1].Node.Size = cFDD_SIZES[data & 0xF];
	
	// Create Sector Cache
	#if USE_CACHE
	//sFDD_SectorCache = malloc(sizeof(*sFDD_SectorCache)*CACHE_SIZE);
	//siFDD_SectorCacheSize = CACHE_SIZE;
	#else
	if( cFDD_SIZES[data >> 4] ) {
		gFDD_Devices[0].CacheHandle = IOCache_Create(
			FDD_WriteSector, 0, 512,
			gFDD_Devices[0].Node.Size / (512*4)
			);	// Cache is 1/4 the size of the disk
	}
	if( cFDD_SIZES[data & 15] ) {
		gFDD_Devices[1].CacheHandle = IOCache_Create(
			FDD_WriteSector, 0, 512,
			gFDD_Devices[1].Node.Size / (512*4)
			);	// Cache is 1/4 the size of the disk
	}
	#endif
	
	// Register with devfs
	DevFS_AddDevice(&gFDD_DriverInfo);
	
	return 1;
}

/**
 * \fn char *FDD_ReadDir(tVFS_Node *Node, int pos)
 * \brief Read Directory
 */
char *FDD_ReadDir(tVFS_Node *Node, int pos)
{
	char	name[2] = "0\0";
	//Update Accessed Time
	//gFDD_DrvInfo.rootNode.atime = now();
	
	//Check for bounds
	if(pos >= 2 || pos < 0)
		return NULL;
	
	if(gFDD_Devices[pos].type == 0)
		return VFS_SKIP;
	
	name[0] += pos;
	
	//Return
	return strdup(name);
}

/**
 * \fn tVFS_Node *FDD_FindDir(tVFS_Node *Node, char *filename);
 * \brief Find File Routine (for vfs_node)
 */
tVFS_Node *FDD_FindDir(tVFS_Node *Node, char *Filename)
{
	 int	i;
	
	ENTER("sFilename", Filename);
	
	// Sanity check string
	if(Filename == NULL) {
		LEAVE('n');
		return NULL;
	}
	
	// Check string length (should be 1)
	if(Filename[0] == '\0' || Filename[1] != '\0') {
		LEAVE('n');
		return NULL;
	}
	
	// Get First character
	i = Filename[0] - '0';
	
	// Check for 1st disk and if it is present return
	if(i == 0 && gFDD_Devices[0].type != 0) {
		LEAVE('p', &gFDD_Devices[0].Node);
		return &gFDD_Devices[0].Node;
	}
	
	// Check for 2nd disk and if it is present return
	if(i == 1 && gFDD_Devices[1].type != 0) {
		LEAVE('p', &gFDD_Devices[1].Node);
		return &gFDD_Devices[1].Node;
	}
	
	// Else return null
	LEAVE('n');
	return NULL;
}

static const char	*casIOCTLS[] = {DRV_IOCTLNAMES,DRV_DISK_IOCTLNAMES,NULL};
/**
 * \fn int FDD_IOCtl(tVFS_Node *Node, int id, void *data)
 * \brief Stub ioctl function
 */
int FDD_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	case DRV_IOCTL_TYPE:
		return DRV_TYPE_DISK;
	
	case DRV_IOCTL_IDENT:
		if(!CheckMem(Data, 4))	return -1;
		memcpy(Data, "FDD\0", 4);
		return 1;
	
	case DRV_IOCTL_VERSION:
		return FDD_VERSION;
	
	case DRV_IOCTL_LOOKUP:
		if(!CheckString(Data))	return -1;
		return LookupString((char**)casIOCTLS, Data);
	
	case DISK_IOCTL_GETBLOCKSIZE:
		return 512;	
	
	default:
		return 0;
	}
}

/**
 * \fn Uint64 FDD_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read Data from a disk
*/
Uint64 FDD_ReadFS(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	i = 0;
	 int	disk;
	//Uint32	buf[128];
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	if(Node == NULL) {
		LEAVE('i', -1);
		return -1;
	}
	
	if(Node->Inode != 0 && Node->Inode != 1) {
		LEAVE('i', -1);
		return -1;
	}
	
	disk = Node->Inode;
	
	// Update Accessed Time
	Node->ATime = now();
	
	#if 0
	if((Offset & 0x1FF) || (Length & 0x1FF))
	{
		// Un-Aligned Offset/Length
		 int	startOff = Offset >> 9;
		 int	sectOff = Offset & 0x1FF;
		 int	sectors = (Length + 0x1FF) >> 9;
	
		LOG("Non-aligned Read");
		
		//Read Starting Sector
		if(!FDD_ReadSector(disk, startOff, buf))
			return 0;
		memcpy(Buffer, (char*)(buf+sectOff), Length > 512-sectOff ? 512-sectOff : Length);
		
		// If the data size is one sector or less
		if(Length <= 512-sectOff)
		{
			LEAVE('X', Length);
			return Length;	//Return
		}
		Buffer += 512-sectOff;
	
		//Read Middle Sectors
		for( i = 1; i < sectors - 1; i++ )
		{
			if(!FDD_ReadSector(disk, startOff+i, buf)) {
				LEAVE('i', -1);
				return -1;
			}
			memcpy(Buffer, buf, 512);
			Buffer += 512;
		}
	
		//Read End Sectors
		if(!FDD_ReadSector(disk, startOff+i, buf))
			return 0;
		memcpy(Buffer, buf, (len&0x1FF)-sectOff);
		
		LEAVE('X', Length);
		return Length;
	}
	else
	{
		 int	count = Length >> 9;
		 int	sector = Offset >> 9;
		LOG("Aligned Read");
		//Aligned Offset and Length - Simple Code
		for( i = 0; i < count; i ++ )
		{
			FDD_ReadSector(disk, sector, buf);
			memcpy(buffer, buf, 512);
			buffer += 512;
			sector++;
		}
		LEAVE('i', Length);
		return Length;
	}
	#endif
	
	i = DrvUtil_ReadBlock(Offset, Length, Buffer, FDD_ReadSectors, 512, disk);
	LEAVE('i', i);
	return i;
}

/**
 * \fn Uint FDD_ReadSectors(Uint64 SectorAddr, Uint Count, void *Buffer, Uint32 Disk)
 * \brief Reads \a Count contiguous sectors from a disk
 * \param SectorAddr	Address of the first sector
 * \param Count	Number of sectors to read
 * \param Buffer	Destination Buffer
 * \param Disk	Disk Number
 * \return Number of sectors read
 * \note Used as a ::DrvUtil_ReadBlock helper
 */
Uint FDD_ReadSectors(Uint64 SectorAddr, Uint Count, void *Buffer, Uint Disk)
{
	Uint	ret = 0;
	while(Count --)
	{
		if( FDD_ReadSector(Disk, SectorAddr, Buffer) != 1 )
			return ret;
		
		Buffer = (void*)( (tVAddr)Buffer + 512 );
		SectorAddr ++;
		ret ++;
	}
	return ret;
}

/**
 * \fn int FDD_ReadSector(Uint32 Disk, Uint64 SectorAddr, void *Buffer)
 * \fn Read a sector from disk
*/
int FDD_ReadSector(Uint32 Disk, Uint64 SectorAddr, void *Buffer)
{
	 int	cyl, head, sec;
	 int	spt, base;
	 int	i;
	 int	lba = SectorAddr;
	
	ENTER("idisk Xlba pbuf", disk, lba, buf);
	
	#if USE_CACHE
	FDD_AquireCacheSpinlock();
	for( i = 0; i < siFDD_SectorCacheSize; i++ )
	{
		if(sFDD_SectorCache[i].timestamp == 0)	continue;
		if(sFDD_SectorCache[i].disk == Disk
		&& sFDD_SectorCache[i].sector == lba) {
			LOG("Found %i in cache %i", lba, i);
			memcpy(Buffer, sFDD_SectorCache[i].data, 512);
			sFDD_SectorCache[i].timestamp = now();
			FDD_FreeCacheSpinlock();
			LEAVE('i', 1);
			return 1;
		}
	}
	LOG("Read %i from Disk", lba);
	FDD_FreeCacheSpinlock();
	#else
	if( IOCache_Read( gFDD_Devices[Disk].CacheHandle, SectorAddr, Buffer ) == 1 ) {
		LEAVE('i', 1);
		return 1;
	}
	#endif
	
	base = cPORTBASE[Disk>>1];
	
	LOG("Calculating Disk Dimensions");
	// Get CHS position
	if(FDD_int_GetDims(gFDD_Devices[Disk].type, lba, &cyl, &head, &sec, &spt) != 1) {
		LEAVE('i', -1);
		return -1;
	}
	
	// Remove Old Timer
	Time_RemoveTimer(gFDD_Devices[Disk].timer);
	// Check if Motor is on
	if(gFDD_Devices[Disk].motorState == 0) {
		FDD_int_StartMotor(Disk);
	}
	
	LOG("Wait for Motor Spinup");
	
	// Wait for spinup
	while(gFDD_Devices[Disk].motorState == 1)	Threads_Yield();
	
	LOG("C:%i,H:%i,S:%i", cyl, head, sec);
	LOG("Acquire Spinlock");
	
	FDD_AquireSpinlock();
	
	// Seek to track
	outb(base+CALIBRATE_DRIVE, 0);
	i = 0;
	while(FDD_int_SeekTrack(Disk, head, (Uint8)cyl) == 0 && i++ < FDD_SEEK_TIMEOUT )	Threads_Yield();
	//FDD_SensInt(base, NULL, NULL);	// Wait for IRQ
	
	LOG("Setting DMA for read");
	
	//Read Data from DMA
	DMA_SetChannel(2, 512, 1);	// Read 512 Bytes
	
	LOG("Sending read command");
	
	//Threads_Wait(100);	// Wait for Head to settle
	Time_Delay(100);
	FDD_int_SendByte(base, READ_SECTOR);	// Was 0xE6
	FDD_int_SendByte(base, (head << 2) | (Disk&1));
	FDD_int_SendByte(base, (Uint8)cyl);
	FDD_int_SendByte(base, (Uint8)head);
	FDD_int_SendByte(base, (Uint8)sec);
	FDD_int_SendByte(base, 0x02);	// Bytes Per Sector (Real BPS=128*2^{val})
	FDD_int_SendByte(base, spt);	// SPT
	FDD_int_SendByte(base, 0x1B);	// Gap Length (27 is default)
	FDD_int_SendByte(base, 0xFF);	// Data Length
	
	// Wait for IRQ
	LOG("Waiting for Data to be read");
	FDD_WaitIRQ();
	
	// Read Data from DMA
	LOG(" FDD_ReadSector: Reading Data");
	DMA_ReadData(2, 512, Buffer);
	
	// Clear Input Buffer
	LOG("Clearing Input Buffer");
	FDD_int_GetByte(base); FDD_int_GetByte(base); FDD_int_GetByte(base);
	FDD_int_GetByte(base); FDD_int_GetByte(base); FDD_int_GetByte(base); FDD_int_GetByte(base);
	
	LOG("Realeasing Spinlock and Setting motor to stop");
	// Release Spinlock
	FDD_FreeSpinlock();
	
	//Set timer to turn off motor affter a gap
	gFDD_Devices[Disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, FDD_int_StopMotor, (void*)Disk);	//One Shot Timer

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
		sFDD_SectorCache[oldest].disk = Disk;
		sFDD_SectorCache[oldest].sector = lba;
		memcpy(sFDD_SectorCache[oldest].data, Buffer, 512);
		FDD_FreeCacheSpinlock();
	}
	#else
	IOCache_Add( gFDD_Devices[Disk].CacheHandle, SectorAddr, Buffer );
	#endif

	LEAVE('i', 1);
	return 1;
}

/**
 * \fn int FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer)
 * \brief Write a sector to the floppy disk
 * \note Not Implemented
 */
int FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer)
{
	Warning("[FDD  ] Read Only at the moment");
	return -1;
}

/**
 * \fn int FDD_int_SeekTrack(int disk, int track)
 * \brief Seek disk to selected track
 */
int FDD_int_SeekTrack(int disk, int head, int track)
{
	Uint8	sr0, cyl;
	 int	base;
	
	base = cPORTBASE[disk>>1];
	
	// Check if seeking is needed
	if(gFDD_Devices[disk].track[head] == track)
		return 1;
	
	// - Seek Head 0
	FDD_int_SendByte(base, SEEK_TRACK);
	FDD_int_SendByte(base, (head<<2)|(disk&1));
	FDD_int_SendByte(base, track);	// Send Seek command
	FDD_WaitIRQ();
	FDD_SensInt(base, &sr0, &cyl);	// Wait for IRQ
	if((sr0 & 0xF0) != 0x20) {
		LOG("sr0 = 0x%x", sr0);
		return 0;	//Check Status
	}
	if(cyl != track)	return 0;
	
	// Set Track in structure
	gFDD_Devices[disk].track[head] = track;
	return 1;
}

/**
 * \fn int FDD_int_GetDims(int type, int lba, int *c, int *h, int *s, int *spt)
 * \brief Get Dimensions of a disk
 */
int FDD_int_GetDims(int type, int lba, int *c, int *h, int *s, int *spt)
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
		//Log("1440k - lba=%i(0x%x), *s=%i,*c=%i,*h=%i", lba, lba, *s, *c, *h);
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
 * \fn void FDD_IRQHandler(int Num)
 * \brief Handles IRQ6
 */
void FDD_IRQHandler(int Num)
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
	FDD_int_SendByte(base, CHECK_INTERRUPT_STATUS);
	if(sr0)	*sr0 = FDD_int_GetByte(base);
	else	FDD_int_GetByte(base);
	if(cyl)	*cyl = FDD_int_GetByte(base);
	else	FDD_int_GetByte(base);
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
 * void FDD_int_SendByte(int base, char byte)
 * \brief Sends a command to the controller
 */
void FDD_int_SendByte(int base, char byte)
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
		Warning("FDD_int_SendByte - Timeout sending byte 0x%x to base 0x%x\n", byte, base);
	#endif
}

/**
 * int FDD_int_GetByte(int base, char byte)
 * \brief Receive data from fdd controller
 */
int FDD_int_GetByte(int base)
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

/**
 * \brief Recalibrate the specified disk
 */
void FDD_Recalibrate(int disk)
{
	ENTER("idisk", disk);
	
	LOG("Starting Motor");
	FDD_int_StartMotor(disk);
	// Wait for Spinup
	while(gFDD_Devices[disk].motorState == 1)	Threads_Yield();
	
	LOG("Sending Calibrate Command");
	FDD_int_SendByte(cPORTBASE[disk>>1], CALIBRATE_DRIVE);
	FDD_int_SendByte(cPORTBASE[disk>>1], disk&1);
	
	LOG("Waiting for IRQ");
	FDD_WaitIRQ();
	FDD_SensInt(cPORTBASE[disk>>1], NULL, NULL);
	
	LOG("Stopping Motor");
	FDD_int_StopMotor(disk);
	LEAVE('-');
}

/**
 * \brief Reset the specified FDD controller
 */
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
	FDD_int_SendByte(base, FIX_DRIVE_DATA);	// Step and Head Load Times
	FDD_int_SendByte(base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	FDD_int_SendByte(base, 0x02);	// Head Load Time >> 1
	while(FDD_int_SeekTrack(0, 0, 1) == 0);	// set track
	while(FDD_int_SeekTrack(0, 1, 1) == 0);	// set track
	
	LOG("Recalibrating Disk");
	FDD_Recalibrate((id<<1)|0);
	FDD_Recalibrate((id<<1)|1);

	LEAVE('-');
}

/**
 * \fn void FDD_int_TimerCallback()
 * \brief Called by timer
 */
void FDD_int_TimerCallback(int arg)
{
	ENTER("iarg", arg);
	if(gFDD_Devices[arg].motorState == 1)
		gFDD_Devices[arg].motorState = 2;
	Time_RemoveTimer(gFDD_Devices[arg].timer);
	gFDD_Devices[arg].timer = -1;
	LEAVE('-');
}

/**
 * \fn void FDD_int_StartMotor(char disk)
 * \brief Starts FDD Motor
 */
void FDD_int_StartMotor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state |= 1 << (4+disk);
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
	gFDD_Devices[disk].motorState = 1;
	gFDD_Devices[disk].timer = Time_CreateTimer(MOTOR_ON_DELAY, FDD_int_TimerCallback, (void*)disk);
}

/**
 * \fn void FDD_int_StopMotor(int disk)
 * \brief Stops FDD Motor
 */
void FDD_int_StopMotor(int disk)
{
	Uint8	state;
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state &= ~( 1 << (4+disk) );
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
    gFDD_Devices[disk].motorState = 0;
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
		Time_RemoveTimer(gFDD_Devices[i].timer);
		FDD_int_StopMotor(i);
	}
	//IRQ_Clear(6);
}

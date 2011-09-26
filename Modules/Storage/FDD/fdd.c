/*
 * AcessOS 0.1
 * Floppy Disk Access Code
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <api_drv_disk.h>
#include <dma.h>
#include <iocache.h>

#define WARN	0

// === CONSTANTS ===
// --- Current Version
#define FDD_VERSION	 ((0<<8)|(75))

// --- Options
#define FDD_SEEK_TIMEOUT	10	// Timeout for a seek operation
#define MOTOR_ON_DELAY	500		// Miliseconds
#define MOTOR_OFF_DELAY	2000	// Miliseconds
#define	FDD_MAX_READWRITE_ATTEMPTS	16

// === TYPEDEFS ===
/**
 * \brief Representation of a floppy drive
 */
typedef struct sFloppyDrive
{
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
#if DEBUG
static const char	*cFDD_STATUSES[] = {NULL, "Error", "Invalid command", "Drive not ready"};
#endif

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

#define CMD_FLAG_MULTI_TRACK	0x80	//!< Multitrack
#define CMD_FLAG_MFM_ENCODING	0x40	//!< MFM Encoding Mode (Always set for read/write/format/verify)
#define CMD_FLAG_SKIP_MODE  	0x20	//!< Skip Mode (don't use)
enum FloppyCommands {
	CMD_READ_TRACK      = 0x02,
	CMD_SPECIFY         = 0x03,
	CMD_SENSE_STATUS    = 0x04,
	CMD_WRITE_DATA      = 0x05,
	CMD_READ_DATA       = 0x06,
	CMD_RECALIBRATE     = 0x07,
	CMD_SENSE_INTERRUPT = 0x08,
	CMD_WRITE_DEL_DATA  = 0x09,
	CMD_READ_SECTOR_ID  = 0x0A,
	// 0x0B - ?
	CMD_READ_DEL_DATA	= 0x0C,
	CMD_FORMAT_TRACK    = 0x0D,
	// 0x0E - ?
	CMD_SEEK_TRACK      = 0x0F,
	CMD_VERSION         = 0x10,
	
	CMD_LOCK            = 0x14,	//!< Save controller parameters
	
	CMD_CONFIGURE       = 0x13
};

// === PROTOTYPES ===
// --- Filesystem
 int	FDD_Install(char **Arguments);
void	FDD_UnloadModule();
// --- VFS Methods
char	*FDD_ReadDir(tVFS_Node *Node, int pos);
tVFS_Node	*FDD_FindDir(tVFS_Node *dirNode, const char *Name);
 int	FDD_IOCtl(tVFS_Node *Node, int ID, void *Data);
Uint64	FDD_ReadFS(tVFS_Node *node, Uint64 off, Uint64 len, void *buffer);
// --- Functions for IOCache/DrvUtil
Uint	FDD_ReadSectors(Uint64 SectorAddr, Uint Count, void *Buffer, Uint Disk);
// --- Raw Disk Access
 int	FDD_ReadSector(Uint32 disk, Uint64 lba, void *Buffer);
 int	FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer);
// --- Helpers
void	FDD_IRQHandler(int Num);
inline void	FDD_WaitIRQ();
void	FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl);
 int	FDD_int_SendByte(int base, Uint8 Byte);
 int	FDD_int_GetByte(int base, Uint8 *Byte);
 int	FDD_Reset(int id);
void	FDD_Recalibrate(int disk);
 int	FDD_Reconfigure(int ID);
 int	FDD_int_SeekTrack(int disk, int head, int track);
void	FDD_int_TimerCallback(void *Arg);
void	FDD_int_StopMotor(void *Arg);
void	FDD_int_StartMotor(int Disk);
 int	FDD_int_GetDims(int type, int lba, int *c, int *h, int *s, int *spt);

// === GLOBALS ===
MODULE_DEFINE(0, FDD_VERSION, FDD, FDD_Install, NULL, "x86_ISADMA", NULL);
t_floppyDevice	gFDD_Devices[2];
tMutex	glFDD;
volatile int	gbFDD_IrqFired = 0;
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

// === CODE ===
/**
 * \fn int FDD_Install(char **Arguments)
 * \brief Installs floppy driver
 */
int FDD_Install(char **Arguments)
{
	Uint8 data;
	char	**args = Arguments;
	
	// Determine Floppy Types (From CMOS)
	outb(0x70, 0x10);
	data = inb(0x71);
	gFDD_Devices[0].type = data >> 4;
	gFDD_Devices[1].type = data & 0xF;
	gFDD_Devices[0].track[0] = -1;
	gFDD_Devices[1].track[1] = -1;
	
	if(args) {
		for(;*args;args++)
		{
			if(strcmp(*args, "disable")==0)
				return MODULE_ERR_NOTNEEDED;
		}
	}
	
	Log_Log("FDD", "Detected Disk 0: %s and Disk 1: %s", cFDD_TYPES[data>>4], cFDD_TYPES[data&0xF]);
	
	if( data == 0 ) {
		return MODULE_ERR_NOTNEEDED;
	}
	
	// Install IRQ6 Handler
	IRQ_AddHandler(6, FDD_IRQHandler);

	// Ensure the FDD version is 0x90
	{
		Uint8	tmp = 0;
		FDD_int_SendByte(cPORTBASE[0], CMD_VERSION);
		FDD_int_GetByte(cPORTBASE[0], &tmp);
		if( tmp != 0x90 ) {
			Log_Error("FDD", "Version(0x%2x) != 0x90", tmp);
			return MODULE_ERR_NOTNEEDED;
		}
	}

	// Configure
	FDD_Reconfigure(0);

	// Reset Primary FDD Controller
	if( FDD_Reset(0) != 0 ) {
		return MODULE_ERR_MISC;
	}
	
	// Initialise Root Node
	gFDD_DriverInfo.RootNode.CTime = gFDD_DriverInfo.RootNode.MTime
		= gFDD_DriverInfo.RootNode.ATime = now();
	
	// Initialise Child Nodes
	gFDD_Devices[0].Node.Inode = 0;
	gFDD_Devices[0].Node.Flags = 0;
	gFDD_Devices[0].Node.NumACLs = 0;
	gFDD_Devices[0].Node.Read = FDD_ReadFS;
	gFDD_Devices[0].Node.Write = NULL;//FDD_WriteFS;
	memcpy(&gFDD_Devices[1].Node, &gFDD_Devices[0].Node, sizeof(tVFS_Node));
	
	gFDD_Devices[1].Node.Inode = 1;
	
	// Set Lengths
	gFDD_Devices[0].Node.Size = cFDD_SIZES[data >> 4];
	gFDD_Devices[1].Node.Size = cFDD_SIZES[data & 0xF];
	
	// Create Sector Cache
	if( cFDD_SIZES[data >> 4] )
	{
		gFDD_Devices[0].CacheHandle = IOCache_Create(
			FDD_WriteSector, 0, 512,
			gFDD_Devices[0].Node.Size / (512*4)
			);	// Cache is 1/4 the size of the disk
	}
	if( cFDD_SIZES[data & 15] )
	{
		gFDD_Devices[1].CacheHandle = IOCache_Create(
			FDD_WriteSector, 0, 512,
			gFDD_Devices[1].Node.Size / (512*4)
			);	// Cache is 1/4 the size of the disk
	}
	
	// Register with devfs
	DevFS_AddDevice(&gFDD_DriverInfo);
	
	return MODULE_ERR_OK;
}

/**
 * \brief Prepare the module for removal
 */
void FDD_UnloadModule()
{
	 int	i;
	DevFS_DelDevice( &gFDD_DriverInfo );
	Mutex_Acquire(&glFDD);
	for(i=0;i<4;i++) {
		Time_RemoveTimer(gFDD_Devices[i].timer);
		FDD_int_StopMotor((void *)(Uint)i);
	}
	Mutex_Release(&glFDD);
	//IRQ_Clear(6);
}

/**
 * \fn char *FDD_ReadDir(tVFS_Node *Node, int pos)
 * \brief Read Directory
 */
char *FDD_ReadDir(tVFS_Node *UNUSED(Node), int Pos)
{
	char	name[2] = "0\0";

	if(Pos >= 2 || Pos < 0)	return NULL;
	
	if(gFDD_Devices[Pos].type == 0)	return VFS_SKIP;
	
	name[0] += Pos;
	
	return strdup(name);
}

/**
 * \fn tVFS_Node *FDD_FindDir(tVFS_Node *Node, const char *filename);
 * \brief Find File Routine (for vfs_node)
 */
tVFS_Node *FDD_FindDir(tVFS_Node *UNUSED(Node), const char *Filename)
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
int FDD_IOCtl(tVFS_Node *UNUSED(Node), int ID, void *Data)
{
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_DISK, "FDD", FDD_VERSION, casIOCTLS);
	
	case DISK_IOCTL_GETBLOCKSIZE:	return 512;	
	
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
	 int	ret;
	
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);
	
	if(Node == NULL) {
		LEAVE('i', -1);
		return -1;
	}
	
	if(Node->Inode != 0 && Node->Inode != 1) {
		LEAVE('i', -1);
		return -1;
	}
	
	ret = DrvUtil_ReadBlock(Offset, Length, Buffer, FDD_ReadSectors, 512, Node->Inode);
	LEAVE('i', ret);
	return ret;
}

/**
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

int FDD_int_ReadWriteSector(Uint32 Disk, Uint64 SectorAddr, int Write, void *Buffer)
{
	 int	cyl, head, sec;
	 int	spt, base;
	 int	i;
	 int	lba = SectorAddr;
	Uint8	st0=0, st1=0, st2=0, bps=0;	// Status Values
	
	ENTER("iDisk XSectorAddr pBuffer", Disk, SectorAddr, Buffer);
	
	base = cPORTBASE[Disk >> 1];
	
	LOG("Calculating Disk Dimensions");
	// Get CHS position
	if(FDD_int_GetDims(gFDD_Devices[Disk].type, lba, &cyl, &head, &sec, &spt) != 1)
	{
		LEAVE('i', -1);
		return -1;
	}
	LOG("Cyl=%i, Head=%i, Sector=%i", cyl, head, sec);
	
	Mutex_Acquire(&glFDD);	// Lock to stop the motor stopping on us
	Time_RemoveTimer(gFDD_Devices[Disk].timer);	// Remove Old Timer
	// Start motor if needed
	if(gFDD_Devices[Disk].motorState != 2)	FDD_int_StartMotor(Disk);
	Mutex_Release(&glFDD);
	
	LOG("Wait for the motor to spin up");
	
	// Wait for spinup
	while(gFDD_Devices[Disk].motorState == 1)	Threads_Yield();
	
	LOG("Acquire Spinlock");
	Mutex_Acquire(&glFDD);
	
	#if 0
	// Seek to track
	outb(base + CALIBRATE_DRIVE, 0);
	i = 0;
	while(FDD_int_SeekTrack(Disk, head, (Uint8)cyl) == 0 && i++ < FDD_SEEK_TIMEOUT )
		Threads_Yield();
	if( i > FDD_SEEK_TIMEOUT ) {
		Mutex_Release(&glFDD);
		LEAVE('i', 0);
		return 0;
	}
	//FDD_SensInt(base, NULL, NULL);	// Wait for IRQ
	#endif
		
	// Read Data from DMA
	LOG("Setting DMA for read");
	DMA_SetChannel(2, 512, !Write);	// Read/Write 512 Bytes from channel 2
	
	LOG("Sending command");
	
	for( i = 0; i < FDD_MAX_READWRITE_ATTEMPTS; i ++ )
	{
		if( Write )
			FDD_int_SendByte(base, CMD_WRITE_DATA|CMD_FLAG_MFM_ENCODING);
		else
			FDD_int_SendByte(base, CMD_READ_DATA|CMD_FLAG_MFM_ENCODING);
		FDD_int_SendByte(base, (head << 2) | (Disk&1));
		FDD_int_SendByte(base, (Uint8)cyl);
		FDD_int_SendByte(base, (Uint8)head);
		FDD_int_SendByte(base, (Uint8)sec);
		FDD_int_SendByte(base, 0x02);	// Bytes Per Sector (Real BPS=128*2^{val})
		FDD_int_SendByte(base, spt);	// SPT
		FDD_int_SendByte(base, 0x1B);	// Gap Length (27 is default)
		FDD_int_SendByte(base, 0xFF);	// Data Length
		
		// Wait for IRQ
		if( Write ) {
			LOG("Writing Data");
			DMA_WriteData(2, 512, Buffer);
			LOG("Waiting for Data to be written");
			FDD_WaitIRQ();
		}
		else {
			LOG("Waiting for data to be read");
			FDD_WaitIRQ();
			LOG("Reading Data");
			DMA_ReadData(2, 512, Buffer);
		}
		
		// Clear Input Buffer
		LOG("Clearing Input Buffer");
		// Status Values
		FDD_int_GetByte(base, &st0);
		FDD_int_GetByte(base, &st1);
		FDD_int_GetByte(base, &st2);
		
		// Cylinder, Head and Sector (mutilated in some way)
		FDD_int_GetByte(base, NULL);	// Cylinder
		FDD_int_GetByte(base, NULL);	// Head
		FDD_int_GetByte(base, NULL);	// Sector
		// Should be the BPS set above (0x02)
		FDD_int_GetByte(base, &bps);
		
		// Check Status
		// - Error Code
		if(st0 & 0xC0) {
			LOG("Error (st0 & 0xC0) \"%s\"", cFDD_STATUSES[st0 >> 6]);
			continue;
		}
		// - Status Flags
		if(st0 & 0x08) {	LOG("Drive not ready");	continue; 	}
		if(st1 & 0x80) {	LOG("End of Cylinder");	continue;	}
		if(st1 & 0x20) {	LOG("CRC Error");	continue;	}
		if(st1 & 0x10) {	LOG("Controller Timeout");	continue;	}
		if(st1 & 0x04) {	LOG("No Data Found");	continue;	}
		if(st1 & 0x01 || st2 & 0x01) {
			LOG("No Address mark found");
			continue;
		}
		if(st2 & 0x40) {	LOG("Deleted address mark");	continue;	}
		if(st2 & 0x20) {	LOG("CRC error in data");	continue;	}
		if(st2 & 0x10) {	LOG("Wrong Cylinder");	continue;	}
		if(st2 & 0x04) {	LOG("uPD765 sector not found");	continue;	}
		if(st2 & 0x02) {	LOG("Bad Cylinder");	continue;	}
		
		if(bps != 0x2) {
			LOG("Returned BPS = 0x%02x, not 0x02", bps);
			continue;
		}
		
		if(st1 & 0x02) {
			LOG("Floppy not writable");
			// Return error without triggering the attempt count check
			i = FDD_MAX_READWRITE_ATTEMPTS+1;
			break;
		}
		
		// Success!
		break;
	}
	
	// Release Spinlock
	LOG("Realeasing Spinlock and setting motor to stop");
	Mutex_Release(&glFDD);
	
	if(i == FDD_MAX_READWRITE_ATTEMPTS) {
		Log_Warning("FDD", "Exceeded %i attempts in %s the disk",
			FDD_MAX_READWRITE_ATTEMPTS,
			(Write ? "writing to" : "reading from")
			);
	}
	
	// Don't turn the motor off now, wait for a while
	gFDD_Devices[Disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, FDD_int_StopMotor, (void*)(tVAddr)Disk);

	// Error check
	if( i < FDD_MAX_READWRITE_ATTEMPTS ) {
		LEAVE('i', 0);
		return 0;
	}
	else {
		LEAVE('i', 1);
		return 1;
	}
}

/**
 * \fn int FDD_ReadSector(Uint32 Disk, Uint64 SectorAddr, void *Buffer)
 * \brief Read a sector from disk
 * \todo Make real-hardware safe (account for read errors)
*/
int FDD_ReadSector(Uint32 Disk, Uint64 SectorAddr, void *Buffer)
{
	 int	ret;
	
	ENTER("iDisk XSectorAddr pBuffer", Disk, SectorAddr, Buffer);
	
	if( IOCache_Read( gFDD_Devices[Disk].CacheHandle, SectorAddr, Buffer ) == 1 ) {
		LEAVE('i', 1);
		return 1;
	}
	
	// Pass to general function
	ret = FDD_int_ReadWriteSector(Disk, SectorAddr, 0, Buffer);

	if( ret == 0 ) {
		IOCache_Add( gFDD_Devices[Disk].CacheHandle, SectorAddr, Buffer );
		LEAVE('i', 1);
		return 1;
	}
	else {
		LOG("Reading failed");
		LEAVE('i', 0);
		return 0;
	}
}

/**
 * \fn int FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer)
 * \brief Write a sector to the floppy disk
 * \note Not Implemented
 */
int FDD_WriteSector(Uint32 Disk, Uint64 LBA, void *Buffer)
{
	Log_Warning("FDD", "Read Only at the moment");
	return -1;
}

/**
 * \fn int FDD_int_SeekTrack(int disk, int track)
 * \brief Seek disk to selected track
 */
int FDD_int_SeekTrack(int disk, int head, int track)
{
	Uint8	sr0=0, cyl=0;
	 int	base;
	
	base = cPORTBASE[disk>>1];
	
	// Check if seeking is needed
	if(gFDD_Devices[disk].track[head] == track)
		return 1;
	
	// - Seek Head 0
	FDD_int_SendByte(base, CMD_SEEK_TRACK);
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

	LOG("Time_Delay(100)");	
	// Wait for Head to settle
	Time_Delay(100);
	
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
	gbFDD_IrqFired = 1;
}

/**
 * \fn FDD_WaitIRQ()
 * \brief Wait for an IRQ6
 */
inline void FDD_WaitIRQ()
{
	// Wait for IRQ
	while(!gbFDD_IrqFired)	Threads_Yield();
	gbFDD_IrqFired = 0;
}

void FDD_SensInt(int base, Uint8 *sr0, Uint8 *cyl)
{
	FDD_int_SendByte(base, CMD_SENSE_INTERRUPT);
	FDD_int_GetByte(base, sr0);
	FDD_int_GetByte(base, cyl);
}

/**
 * void FDD_int_SendByte(int base, char byte)
 * \brief Sends a command to the controller
 */
int FDD_int_SendByte(int base, Uint8 byte)
{
	tTime	end = now() + 1000;	// 1s
	
	while( (inb(base + PORT_MAINSTATUS) & 0x80) != 0x80 && now() < end )
		Threads_Yield();	//Delay

	if( inb(base + PORT_MAINSTATUS) & 0x40 ) {
		Log_Warning("FDD", "FDD_int_SendByte: DIO set, is this ok?");
	}
	
	if( now() < end )
	{
		outb(base + PORT_DATA, byte);
//		Log_Debug("FDD", "FDD_int_SendByte: Sent 0x%02x to 0x%x", byte, base);
		return 0;
	}
	else
	{
		Log_Warning("FDD", "FDD_int_SendByte: Timeout sending byte 0x%x to base 0x%x", byte, base);
		return 1;
	}
}

/**
 * int FDD_int_GetByte(int base, char byte)
 * \brief Receive data from fdd controller
 */
int FDD_int_GetByte(int base, Uint8 *value)
{
	tTime	end = now() + 1000;	// 1s
	
	while( (inb(base + PORT_MAINSTATUS) & 0x80) != 0x80 && now() < end )
		Threads_Yield();
	
	if( !(inb(base + PORT_MAINSTATUS) & 0x40) ) {
		Log_Warning("FDD", "FDD_int_GetByte: DIO unset, is this ok?");
	}

	if( now() < end )
	{
		Uint8	tmp = inb(base + PORT_DATA);
		if(value)	*value = tmp;
//		Log_Debug("FDD", "FDD_int_GetByte: Read 0x%02x from 0x%x", *value, base);
		return 0;
	}
	else
	{
		Log_Warning("FDD", "FDD_int_GetByte: Timeout reading byte from base 0x%x", base);
		return -1;
	}
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
	FDD_int_SendByte(cPORTBASE[disk>>1], CMD_RECALIBRATE);
	FDD_int_SendByte(cPORTBASE[disk>>1], disk&1);
	
	LOG("Waiting for IRQ");
	FDD_WaitIRQ();
	FDD_SensInt(cPORTBASE[disk>>1], NULL, NULL);
	
	LOG("Stopping Motor");
	gFDD_Devices[disk].timer = Time_CreateTimer(MOTOR_OFF_DELAY, FDD_int_StopMotor, (void*)(Uint)disk);
	LEAVE('-');
}

/**
 * \brief Reconfigure the controller
 */
int FDD_Reconfigure(int ID)
{
	Uint16	base = cPORTBASE[ID];
	
	ENTER("iID", ID);
	
	FDD_int_SendByte(base, CMD_CONFIGURE);
	FDD_int_SendByte(base, 0);
	// Implied seek enabled, FIFO Enabled, Drive Polling Disabled, data buffer threshold 8 bytes
	FDD_int_SendByte(base, (1 << 6) | (0 << 5) | (0 << 4) | 7);
	FDD_int_SendByte(base, 0);	// Precompensation - use default
	
	// Commit
	FDD_int_SendByte(base, CMD_LOCK|CMD_FLAG_MULTI_TRACK);
	FDD_int_GetByte(base, NULL);
	
	LEAVE('i', 0);
	return 0;
}

/**
 * \brief Reset the specified FDD controller
 */
int FDD_Reset(int id)
{
	Uint16	base = cPORTBASE[id];
	 int	retries;
	
	ENTER("iID", id);

	// Reset the card	
	outb(base + PORT_DIGOUTPUT, 0);	// Disable FDC
	// Wait 4 microseconds - or use 1 thread delay
	Threads_Yield();
	Threads_Yield();
	outb(base + PORT_DIGOUTPUT, 8|4);	// Re-enable FDC (DMA and Enable)
	
	// Set the data rate
	outb(base + PORT_DATARATE, 0);	// Set data rate to 500K/s

	// Wait for IRQ
	LOG("Awaiting IRQ");
	
	FDD_WaitIRQ();
	LOG("4x SenseInterrupt");
	FDD_SensInt(base, NULL, NULL);
	FDD_SensInt(base, NULL, NULL);
	FDD_SensInt(base, NULL, NULL);
	FDD_SensInt(base, NULL, NULL);
	
	// Specify
	FDD_int_SendByte(base, CMD_SPECIFY);	// Step and Head Load Times
	FDD_int_SendByte(base, 0xDF);	// Step Rate Time, Head Unload Time (Nibble each)
	FDD_int_SendByte(base, 0x02);	// Head Load Time >> 1

	// Recalibrate disks
	LOG("Recalibrate disks (16x seek)");
	retries = 16;
	while(FDD_int_SeekTrack(0, 0, 1) == 0 && retries --);	// set track
	if(retries < 0)	LEAVE_RET('i', -1);

	retries = 16;
	while(FDD_int_SeekTrack(0, 1, 1) == 0 && retries --);	// set track
	if(retries < 0)	LEAVE_RET('i', -1);
	
	LOG("Recalibrating Disk");
	FDD_Recalibrate((id<<1)|0);
	FDD_Recalibrate((id<<1)|1);

	LEAVE_RET('i', 0);
}

/**
 * \fn void FDD_int_TimerCallback()
 * \brief Called by timer
 */
void FDD_int_TimerCallback(void *Arg)
{
	 int	disk = (Uint)Arg;
	ENTER("iarg", disk);
	if(gFDD_Devices[disk].motorState == 1)
		gFDD_Devices[disk].motorState = 2;
	Time_RemoveTimer(gFDD_Devices[disk].timer);
	gFDD_Devices[disk].timer = -1;
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
	gFDD_Devices[disk].timer = Time_CreateTimer(MOTOR_ON_DELAY, FDD_int_TimerCallback, (void*)(Uint)disk);
}

/**
 * \fn void FDD_int_StopMotor(int disk)
 * \brief Stops FDD Motor
 */
void FDD_int_StopMotor(void *Arg)
{
	Uint8	state, disk = (Uint)Arg;
	if( Mutex_IsLocked(&glFDD) )	return ;
	ENTER("iDisk", disk);
	
	state = inb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT );
	state &= ~( 1 << (4+disk) );
	outb( cPORTBASE[ disk>>1 ] + PORT_DIGOUTPUT, state );
	gFDD_Devices[disk].motorState = 0;
	LEAVE('-');
}


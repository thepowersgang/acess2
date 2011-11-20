/*
 * Acess2 Virtual Terminal Driver
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include <modules.h>
#include <api_drv_video.h>
#include <api_drv_keyboard.h>
#include <api_drv_terminal.h>
#include <errno.h>
#include <semaphore.h>

// === CONSTANTS ===
#define VERSION	((0<<8)|(50))

#define	NUM_VTS	8
#define MAX_INPUT_CHARS32	64
#define MAX_INPUT_CHARS8	(MAX_INPUT_CHARS32*4)
//#define DEFAULT_OUTPUT	"BochsGA"
#define DEFAULT_OUTPUT	"Vesa"
#define FALLBACK_OUTPUT	"x86_VGAText"
#define DEFAULT_INPUT	"PS2Keyboard"
#define	DEFAULT_WIDTH	640
#define	DEFAULT_HEIGHT	480
#define DEFAULT_SCROLLBACK	2	// 2 Screens of text + current screen
//#define DEFAULT_SCROLLBACK	0
#define	DEFAULT_COLOUR	(VT_COL_BLACK|(0xAAA<<16))

#define	VT_FLAG_HIDECSR	0x01
#define	VT_FLAG_ALTBUF	0x02	//!< Alternate screen buffer
#define VT_FLAG_RAWIN	0x04	//!< Don't handle ^Z/^C/^V
#define	VT_FLAG_HASFB	0x10	//!< Set if the VTerm has requested the Framebuffer
#define VT_FLAG_SHOWCSR	0x20	//!< Always show the text cursor

enum eVT_InModes {
	VT_INMODE_TEXT8,	// UTF-8 Text Mode (VT100/xterm Emulation)
	VT_INMODE_TEXT32,	// UTF-32 Text Mode (Acess Native)
	NUM_VT_INMODES
};

// === TYPES ===
typedef struct {
	 int	Mode;	//!< Current Mode (see ::eTplTerminal_Modes)
	 int	Flags;	//!< Flags (see VT_FLAG_*)
	
	short	NewWidth;	//!< Un-applied dimensions (Width)
	short	NewHeight;	//!< Un-applied dimensions (Height)
	short	Width;	//!< Virtual Width
	short	Height;	//!< Virtual Height
	short	TextWidth;	//!< Text Virtual Width
	short	TextHeight;	//!< Text Virtual Height
	
	Uint32	CurColour;	//!< Current Text Colour
	
	 int	ViewPos;	//!< View Buffer Offset (Text Only)
	 int	WritePos;	//!< Write Buffer Offset (Text Only)
	tVT_Char	*Text;
	
	tVT_Char	*AltBuf;	//!< Alternate Screen Buffer
	 int	AltWritePos;	//!< Alternate write position
	short	ScrollTop;	//!< Top of scrolling region (smallest)
	short	ScrollHeight;	//!< Length of scrolling region

	 int	VideoCursorX;
	 int	VideoCursorY;
	
	tMutex	ReadingLock;	//!< Lock the VTerm when a process is reading from it
	tTID	ReadingThread;	//!< Owner of the lock
	 int	InputRead;	//!< Input buffer read position
	 int	InputWrite;	//!< Input buffer write position
	char	InputBuffer[MAX_INPUT_CHARS8];
//	tSemaphore	InputSemaphore;
	
	Uint32		*Buffer;

	// TODO: Do I need to keep this about?
	// When should it be deallocated? on move to text mode, or some other time
	// Call set again, it's freed, and if NULL it doesn't get reallocated.
	tVideo_IOCtl_Bitmap	*VideoCursor;
	
	char	Name[2];	//!< Name of the terminal
	tVFS_Node	Node;
} tVTerm;

// === IMPORTS ===
extern void	Debug_SetKTerminal(const char *File);

// === PROTOTYPES ===
 int	VT_Install(char **Arguments);
void	VT_InitOutput(void);
void	VT_InitInput(void);
char	*VT_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*VT_FindDir(tVFS_Node *Node, const char *Name);
 int	VT_Root_IOCtl(tVFS_Node *Node, int Id, void *Data);
Uint64	VT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	VT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	VT_Terminal_IOCtl(tVFS_Node *Node, int Id, void *Data);
void	VT_SetResolution(int Width, int Height);
void	VT_SetMode(int Mode);
void	VT_SetTerminal(int ID);
void	VT_KBCallBack(Uint32 Codepoint);
void	VT_int_PutString(tVTerm *Term, Uint8 *Buffer, Uint Count);
void	VT_int_ClearLine(tVTerm *Term, int Num);
void	VT_int_ParseEscape_StandardLarge(tVTerm *Term, char CmdChar, int argc, int *args);
 int	VT_int_ParseEscape(tVTerm *Term, char *Buffer);
void	VT_int_PutChar(tVTerm *Term, Uint32 Ch);
void	VT_int_ScrollText(tVTerm *Term, int Count);
void	VT_int_ScrollFramebuffer( tVTerm *Term, int Count );
void	VT_int_UpdateCursor( tVTerm *Term, int bShow );
void	VT_int_UpdateScreen( tVTerm *Term, int UpdateAll );
void	VT_int_ChangeMode(tVTerm *Term, int NewMode, int NewWidth, int NewHeight);
void	VT_int_ToggleAltBuffer(tVTerm *Term, int Enabled);

// === CONSTANTS ===
const Uint16	caVT100Colours[] = {
		// Black, Red, Green, Yellow, Blue, Purple, Cyan, Gray
		// Same again, but bright
		VT_COL_BLACK, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0xAAA,
		VT_COL_GREY, 0xF00, 0x0F0, 0xFF0, 0x00F, 0xF0F, 0x0FF, VT_COL_WHITE
	};

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VTerm, VT_Install, NULL, DEFAULT_INPUT, NULL);
tDevFS_Driver	gVT_DrvInfo = {
	NULL, "VTerm",
	{
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = NUM_VTS,
	.Inode = -1,
	.NumACLs = 0,
	.ReadDir = VT_ReadDir,
	.FindDir = VT_FindDir,
	.IOCtl = VT_Root_IOCtl
	}
};
// --- Terminals ---
tVTerm	gVT_Terminals[NUM_VTS];
 int	giVT_CurrentTerminal = 0;
tVTerm	*gpVT_CurTerm = &gVT_Terminals[0];
// --- Video State ---
short	giVT_RealWidth  = DEFAULT_WIDTH;	//!< Screen Width
short	giVT_RealHeight = DEFAULT_HEIGHT;	//!< Screen Height
 int	giVT_Scrollback = DEFAULT_SCROLLBACK;
// --- Driver Handles ---
char	*gsVT_OutputDevice = NULL;
char	*gsVT_InputDevice = NULL;
 int	giVT_OutputDevHandle = -2;
 int	giVT_InputDevHandle = -2;
// --- Key States --- (Used for VT Switching/Magic Combos)
 int	gbVT_CtrlDown = 0;
 int	gbVT_AltDown = 0;
 int	gbVT_SysrqDown = 0;

// === CODE ===
/**
 * \fn int VT_Install(char **Arguments)
 * \brief Installs the Virtual Terminal Driver
 */
int VT_Install(char **Arguments)
{
	 int	i;
	
	// Scan Arguments
	if(Arguments)
	{
		char	**args;
		const char	*arg;
		for(args = Arguments; (arg = *args); args++ )
		{
			char	data[strlen(arg)+1];
			char	*opt = data;
			char	*val;
			
			val = strchr(arg, '=');
			strcpy(data, arg);
			if( val ) {
				data[ val - arg ] = '\0';
				val ++;
			}
			Log_Debug("VTerm", "Argument '%s'", arg);
			
			if( strcmp(opt, "Video") == 0 ) {
				if( !gsVT_OutputDevice )
					gsVT_OutputDevice = strdup(val);
			}
			else if( strcmp(opt, "Input") == 0 ) {
				if( !gsVT_InputDevice )
					gsVT_InputDevice = strdup(val);
			}
			else if( strcmp(opt, "Width") == 0 ) {
				giVT_RealWidth = atoi( val );
			}
			else if( strcmp(opt, "Height") == 0 ) {
				giVT_RealHeight = atoi( val );
			}
			else if( strcmp(opt, "Scrollback") == 0 ) {
				giVT_Scrollback = atoi( val );
			}
		}
	}
	
	// Apply Defaults
	if(!gsVT_OutputDevice)	gsVT_OutputDevice = (char*)DEFAULT_OUTPUT;
	else if( Module_EnsureLoaded( gsVT_OutputDevice ) )	gsVT_OutputDevice = (char*)DEFAULT_OUTPUT;
	if( Module_EnsureLoaded( gsVT_OutputDevice ) )	gsVT_OutputDevice = (char*)FALLBACK_OUTPUT;
	if( Module_EnsureLoaded( gsVT_OutputDevice ) ) {
		Log_Error("VTerm", "Fallback video '%s' is not avaliable, giving up", FALLBACK_OUTPUT);
		return MODULE_ERR_MISC;
	}
	
	if(!gsVT_InputDevice)	gsVT_InputDevice = (char*)DEFAULT_INPUT;
	else if( Module_EnsureLoaded( gsVT_InputDevice ) )	gsVT_InputDevice = (char*)DEFAULT_INPUT;
	
	// Create device paths
	{
		char	*tmp;
		tmp = malloc( 9 + strlen(gsVT_OutputDevice) + 1 );
		strcpy(tmp, "/Devices/");
		strcpy(&tmp[9], gsVT_OutputDevice);
		gsVT_OutputDevice = tmp;

		tmp = malloc( 9 + strlen(gsVT_InputDevice) + 1 );
		strcpy(tmp, "/Devices/");
		strcpy(&tmp[9], gsVT_InputDevice);
		gsVT_InputDevice = tmp;
	}
	
	Log_Log("VTerm", "Using '%s' as output", gsVT_OutputDevice);
	Log_Log("VTerm", "Using '%s' as input", gsVT_InputDevice);
	
	VT_InitOutput();
	VT_InitInput();
	
	// Create Nodes
	for( i = 0; i < NUM_VTS; i++ )
	{
		gVT_Terminals[i].Mode = TERM_MODE_TEXT;
		gVT_Terminals[i].Flags = 0;
//		gVT_Terminals[i].Flags = VT_FLAG_HIDECSR;	//HACK - Stop all those memcpy calls
		gVT_Terminals[i].CurColour = DEFAULT_COLOUR;
		gVT_Terminals[i].WritePos = 0;
		gVT_Terminals[i].AltWritePos = 0;
		gVT_Terminals[i].ViewPos = 0;
		gVT_Terminals[i].ReadingThread = -1;
		gVT_Terminals[i].ScrollHeight = 0;
		
		// Initialise
		VT_int_ChangeMode( &gVT_Terminals[i],
			TERM_MODE_TEXT, giVT_RealWidth, giVT_RealHeight );
		
		gVT_Terminals[i].Name[0] = '0'+i;
		gVT_Terminals[i].Name[1] = '\0';
		gVT_Terminals[i].Node.Inode = i;
		gVT_Terminals[i].Node.ImplPtr = &gVT_Terminals[i];
		gVT_Terminals[i].Node.NumACLs = 0;	// Only root can open virtual terminals
		
		gVT_Terminals[i].Node.Read = VT_Read;
		gVT_Terminals[i].Node.Write = VT_Write;
		gVT_Terminals[i].Node.IOCtl = VT_Terminal_IOCtl;
//		Semaphore_Init(&gVT_Terminals[i].InputSemaphore, 0, MAX_INPUT_CHARS8, "VTerm", gVT_Terminals[i].Name);
	}
	
	// Add to DevFS
	DevFS_AddDevice( &gVT_DrvInfo );
	
	// Set kernel output to VT0
	Debug_SetKTerminal("/Devices/VTerm/0");
	
	return MODULE_ERR_OK;
}

/**
 * \fn void VT_InitOutput()
 * \brief Initialise Video Output
 */
void VT_InitOutput()
{
	giVT_OutputDevHandle = VFS_Open(gsVT_OutputDevice, VFS_OPENFLAG_WRITE);
	if(giVT_OutputDevHandle == -1) {
		Log_Warning("VTerm", "Oh F**k, I can't open the video device '%s'", gsVT_OutputDevice);
		return ;
	}
	VT_SetResolution( giVT_RealWidth, giVT_RealHeight );
	VT_SetTerminal( 0 );
	VT_SetMode( VIDEO_BUFFMT_TEXT );
}

/**
 * \fn void VT_InitInput()
 * \brief Initialises the input
 */
void VT_InitInput()
{
	giVT_InputDevHandle = VFS_Open(gsVT_InputDevice, VFS_OPENFLAG_READ);
	if(giVT_InputDevHandle == -1) {
		Log_Warning("VTerm", "Can't open the input device '%s'", gsVT_InputDevice);
		return ;
	}
	VFS_IOCtl(giVT_InputDevHandle, KB_IOCTL_SETCALLBACK, VT_KBCallBack);
}

/**
 * \brief Set the video resolution
 * \param Width	New screen width
 * \param Height	New screen height
 */
void VT_SetResolution(int Width, int Height)
{
	tVideo_IOCtl_Mode	mode = {0};
	 int	tmp;
	 int	i;
	
	// Create the video mode
	mode.width = Width;
	mode.height = Height;
	mode.bpp = 32;
	mode.flags = 0;
	
	// Set video mode
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_FINDMODE, &mode );
	tmp = mode.id;
	if( Width != mode.width || Height != mode.height )
	{
		Log_Warning("VTerm",
			"Selected resolution (%ix%i is not supported) by the device, using (%ix%i)",
			giVT_RealWidth, giVT_RealHeight,
			mode.width, mode.height
			);
		giVT_RealWidth = mode.width;
		giVT_RealHeight = mode.height;
	}
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_GETSETMODE, &tmp );
	
	// Resize text terminals if needed
	if( gVT_Terminals[0].Text && (giVT_RealWidth != mode.width || giVT_RealHeight != mode.height) )
	{
		 int	newBufSize = (giVT_RealWidth/giVT_CharWidth)
					*(giVT_RealHeight/giVT_CharHeight)
					*(giVT_Scrollback+1);
		//tVT_Char	*tmp;
		// Resize the text terminals
		Log_Debug("VTerm", "Resizing terminals to %ix%i",
			giVT_RealWidth/giVT_CharWidth, giVT_RealHeight/giVT_CharHeight);
		for( i = 0; i < NUM_VTS; i ++ )
		{
			if( gVT_Terminals[i].Mode != TERM_MODE_TEXT )	continue;
			
			gVT_Terminals[i].TextWidth = giVT_RealWidth/giVT_CharWidth;
			gVT_Terminals[i].TextHeight = giVT_RealHeight/giVT_CharHeight;
			gVT_Terminals[i].ScrollHeight = gVT_Terminals[i].TextHeight;
			
			gVT_Terminals[i].Text = realloc(
				gVT_Terminals[i].Text,
				newBufSize*sizeof(tVT_Char)
				);
		}
	}
}

/**
 * \brief Set video output buffer mode
 */
void VT_SetMode(int Mode)
{
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &Mode );
}

/**
 * \fn char *VT_ReadDir(tVFS_Node *Node, int Pos)
 * \brief Read from the VTerm Directory
 */
char *VT_ReadDir(tVFS_Node *Node, int Pos)
{
	if(Pos < 0)	return NULL;
	if(Pos >= NUM_VTS)	return NULL;
	return strdup( gVT_Terminals[Pos].Name );
}

/**
 * \fn tVFS_Node *VT_FindDir(tVFS_Node *Node, const char *Name)
 * \brief Find an item in the VTerm directory
 * \param Node	Root node
 * \param Name	Name (number) of the terminal
 */
tVFS_Node *VT_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	num;
	
	ENTER("pNode sName", Node, Name);
	
	// Open the input and output files if needed
	if(giVT_OutputDevHandle == -2)	VT_InitOutput();
	if(giVT_InputDevHandle == -2)	VT_InitInput();
	
	// Sanity check name
	if(Name[0] < '0' || Name[0] > '9' || Name[1] != '\0') {
		LEAVE('n');
		return NULL;
	}
	// Get index
	num = Name[0] - '0';
	if(num >= NUM_VTS) {
		LEAVE('n');
		return NULL;
	}
	// Return node
	LEAVE('p', &gVT_Terminals[num].Node);
	return &gVT_Terminals[num].Node;
}

/**
 * \fn int VT_Root_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Control the VTerm Driver
 */
int VT_Root_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	 int	len;
	switch(Id)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_MISC;
	case DRV_IOCTL_IDENT:	memcpy(Data, "VT\0\0", 4);	return 0;
	case DRV_IOCTL_VERSION:	return VERSION;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case 4:	// Get Video Driver
		if(Data)	strcpy(Data, gsVT_OutputDevice);
		return strlen(gsVT_OutputDevice);
	
	case 5:	// Set Video Driver
		if(!Data)	return -EINVAL;
		if(Threads_GetUID() != 0)	return -EACCES;
		
		len = strlen(Data);
		
		// TODO: Check if the string used is a heap string
		
		free(gsVT_OutputDevice);
		
		gsVT_OutputDevice = malloc(len+1);
		strcpy(gsVT_OutputDevice, Data);
		
		VFS_Close(giVT_OutputDevHandle);
		giVT_OutputDevHandle = -1;
		
		VT_InitOutput();
		return 1;
	}
	return 0;
}

/**
 * \brief Read from a virtual terminal
 */
Uint64 VT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	pos = 0;
	 int	avail;
	tVTerm	*term = &gVT_Terminals[ Node->Inode ];
	Uint32	*codepoint_buf = Buffer;
	Uint32	*codepoint_in;
	
	Mutex_Acquire( &term->ReadingLock );
	
	// Check current mode
	switch(term->Mode)
	{
	// Text Mode (UTF-8)
	case TERM_MODE_TEXT:
		VT_int_UpdateCursor(term, 1);
	
		VFS_SelectNode(Node, VFS_SELECT_READ, NULL, "VT_Read (UTF-8)");
		
		avail = term->InputWrite - term->InputRead;
		if(avail < 0)
			avail += MAX_INPUT_CHARS8;
		if(avail > Length - pos)
			avail = Length - pos;
		
		while( avail -- )
		{
			((char*)Buffer)[pos] = term->InputBuffer[term->InputRead];
			pos ++;
			term->InputRead ++;
			while(term->InputRead > MAX_INPUT_CHARS8)
				term->InputRead -= MAX_INPUT_CHARS8;
		}
		break;
	
	//case TERM_MODE_FB:
	// Other - UCS-4
	default:
		VFS_SelectNode(Node, VFS_SELECT_READ, NULL, "VT_Read (UCS-4)");
		
		avail = term->InputWrite - term->InputRead;
		if(avail < 0)
			avail += MAX_INPUT_CHARS32;
		if(avail > Length - pos)
			avail = Length/4 - pos;
		
		codepoint_in = (void*)term->InputBuffer;
		codepoint_buf = Buffer;
		
		while( avail -- )
		{
			codepoint_buf[pos] = codepoint_in[term->InputRead];
			pos ++;
			term->InputRead ++;
			while(term->InputRead > MAX_INPUT_CHARS32)
				term->InputRead -= MAX_INPUT_CHARS32;
		}
		pos *= 4;
		break;
	}
	
	// Mark none avaliable if buffer empty
	if( term->InputRead == term->InputWrite )
		VFS_MarkAvaliable(&term->Node, 0);
	
	term->ReadingThread = -1;

//	VT_int_UpdateCursor(term, term->Mode == TERM_MODE_TEXT);

	Mutex_Release( &term->ReadingLock );
	
	return pos;
}

/**
 * \fn Uint64 VT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Write to a virtual terminal
 */
Uint64 VT_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tVTerm	*term = &gVT_Terminals[ Node->Inode ];
	 int	size;
	
	// Write
	switch( term->Mode )
	{
	// Print Text
	case TERM_MODE_TEXT:
		VT_int_PutString(term, Buffer, Length);
		break;
	
	// Framebuffer :)
	case TERM_MODE_FB:
		// - Sanity Checking
		size = term->Width*term->Height*4;
		if( Offset > size ) {
			Log_Notice("VTerm", "VT_Write: Offset (0x%llx) > FBSize (0x%x)",
				Offset, size);
			return 0;
		}
		if( Offset + Length > size ) {
			Log_Notice("VTerm", "VT_Write: Offset+Length (0x%llx) > FBSize (0x%x)",
				Offset+Length, size);
			Length = size - Offset;
		}
		
		// Update screen if needed
		if( Node->Inode == giVT_CurrentTerminal )
		{
			if( giVT_RealHeight > term->Height )
				Offset += (giVT_RealHeight - term->Height) / 2 * term->Width * 4;
			// Handle undersized virtual terminals
			if( giVT_RealWidth > term->Width )
			{
				// No? :( Well, just center it
				 int	x, y, w, h;
				Uint	dst_ofs;
				// TODO: Fix to handle the final line correctly?
				x = Offset/4;	y = x / term->Width;	x %= term->Width;
				w = Length/4+x;	h = w / term->Width;	w %= term->Width;
				
				// Center
				x += (giVT_RealWidth - term->Width) / 2;
				dst_ofs = (x + y * giVT_RealWidth) * 4;
				while(h--)
				{
					VFS_WriteAt( giVT_OutputDevHandle,
						dst_ofs,
						term->Width * 4,
						Buffer
						);
					Buffer = (void*)( (Uint)Buffer + term->Width*4 );
					dst_ofs += giVT_RealWidth * 4;
				}
				return 0;
			}
			else
			{
				return VFS_WriteAt( giVT_OutputDevHandle, Offset, Length, Buffer );
			}
		}
		else
		{
			if( !term->Buffer )
				term->Buffer = malloc( term->Width * term->Height * 4 );
			// Copy to the local cache
			memcpy( (char*)term->Buffer + (Uint)Offset, Buffer, Length );
		}
		break;
	// Just pass on (for now)
	// TODO: Handle locally too to ensure no information is lost on
	//       VT Switch (and to isolate terminals from each other)
	case TERM_MODE_2DACCEL:
	//case TERM_MODE_3DACCEL:
		if( Node->Inode == giVT_CurrentTerminal )
		{
			VFS_Write( giVT_OutputDevHandle, Length, Buffer );
		}
		break;
	}
	
	return 0;
}

/**
 * \fn int VT_Terminal_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Call an IO Control on a virtual terminal
 */
int VT_Terminal_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	 int	*iData = Data;
	 int	ret;
	tVTerm	*term = Node->ImplPtr;
	ENTER("pNode iId pData", Node, Id, Data);
	
	if(Id >= DRV_IOCTL_LOOKUP) {
		// Only root can fiddle with graphics modes
		// TODO: Remove this and replace with user ownership
		if( Threads_GetUID() != 0 )	return -1;
	}
	
	switch(Id)
	{
	// --- Core Defined
	case DRV_IOCTL_TYPE:
		LEAVE('i', DRV_TYPE_TERMINAL);
		return DRV_TYPE_TERMINAL;
	case DRV_IOCTL_IDENT:
		memcpy(Data, "VT\0\0", 4);
		LEAVE('i', 0);
		return 0;
	case DRV_IOCTL_VERSION:
		LEAVE('x', VERSION);
		return VERSION;
	case DRV_IOCTL_LOOKUP:
		LEAVE('i', 0);
		return 0;
	
	// Get/Set the mode (and apply any changes)
	case TERM_IOCTL_MODETYPE:
		if(Data != NULL)
		{
			if( CheckMem(Data, sizeof(int)) == 0 ) {
				LEAVE('i', -1);
				return -1;
			}
			Log_Log("VTerm", "VTerm %i mode set to %i", (int)Node->Inode, *iData);
			
			// Update mode if needed
			if( term->Mode != *iData || term->NewWidth || term->NewHeight)
			{
				// Adjust for text mode
				if( *iData == TERM_MODE_TEXT ) {
					term->NewHeight *= giVT_CharHeight;
					term->NewWidth *= giVT_CharWidth;
				}
				// Fill unchanged dimensions
				if(term->NewHeight == 0)	term->NewHeight = term->Height;
				if(term->NewWidth == 0)	term->NewWidth = term->Width;
				// Set new mode
				VT_int_ChangeMode(term, *iData, term->NewWidth, term->NewHeight);
				// Clear unapplied dimensions
				term->NewWidth = 0;
				term->NewHeight = 0;
			}
			
			// Update the screen dimensions
			if(Node->Inode == giVT_CurrentTerminal)
				VT_SetTerminal( giVT_CurrentTerminal );
		}
		LEAVE('i', term->Mode);
		return term->Mode;
	
	// Get/set the terminal width
	case TERM_IOCTL_WIDTH:
		if(Data != NULL) {
			if( CheckMem(Data, sizeof(int)) == 0 ) {
				LEAVE('i', -1);
				return -1;
			}
			term->NewWidth = *iData;
		}
		if( term->NewWidth )
			ret = term->NewWidth;
		else if( term->Mode == TERM_MODE_TEXT )
			ret = term->TextWidth;
		else
			ret = term->Width;
		LEAVE('i', ret);
		return ret;
	
	// Get/set the terminal height
	case TERM_IOCTL_HEIGHT:
		if(Data != NULL) {
			if( CheckMem(Data, sizeof(int)) == 0 ) {
				LEAVE('i', -1);
				return -1;
			}
			term->NewHeight = *iData;
		}
		if( term->NewHeight )
			ret = term->NewHeight;
		else if( term->Mode == TERM_MODE_TEXT )
			ret = term->TextHeight;
		else
			ret = term->Height;
		LEAVE('i', ret);
		return ret;
	
	case TERM_IOCTL_FORCESHOW:
		Log_Log("VTerm", "Thread %i forced VTerm %i to be shown",
			Threads_GetTID(), (int)Node->Inode);
		VT_SetTerminal( Node->Inode );
		LEAVE('i', 1);
		return 1;
	
	case TERM_IOCTL_GETSETCURSOR:
		if(Data != NULL)
		{
			tVideo_IOCtl_Pos	*pos = Data;
			if( !CheckMem(Data, sizeof(*pos)) ) {
				errno = -EINVAL;
				LEAVE('i', -1);
				return -1;
			}
		
			if( term->Mode == TERM_MODE_TEXT )
			{
				if(term->Flags & VT_FLAG_ALTBUF)
					term->AltWritePos = pos->x + pos->y * term->TextWidth;
				else
					term->WritePos = pos->x + pos->y * term->TextWidth + term->ViewPos;
				VT_int_UpdateCursor(term, 0);
			}
			else
			{
				term->VideoCursorX = pos->x;
				term->VideoCursorY = pos->y;
				VT_int_UpdateCursor(term, 1);
			}
		}
		ret = (term->Flags & VT_FLAG_ALTBUF) ? term->AltWritePos : term->WritePos-term->ViewPos;
		LEAVE('i', ret);
		return ret;

	case TERM_IOCTL_SETCURSORBITMAP: {
		tVideo_IOCtl_Bitmap	*bmp = Data;
		if( Data == NULL )
		{
			free( term->VideoCursor );
			term->VideoCursor = NULL;
			LEAVE('i', 0);
			return 0;
		}

		// Sanity check bitmap
		if( !CheckMem(bmp, sizeof(tVideo_IOCtl_Bitmap)) ) {
			Log_Notice("VTerm", "%p in TERM_IOCTL_SETCURSORBITMAP invalid", bmp);
			errno = -EINVAL;
			LEAVE_RET('i', -1);
		}
		if( !CheckMem(bmp->Data, bmp->W*bmp->H*sizeof(Uint32)) ) {
			Log_Notice("VTerm", "%p in TERM_IOCTL_SETCURSORBITMAP invalid", bmp);
			errno = -EINVAL;
			LEAVE_RET('i', -1);
		}

		// Reallocate if needed
		if(term->VideoCursor)
		{
			if(bmp->W * bmp->H != term->VideoCursor->W * term->VideoCursor->H) {
				free(term->VideoCursor);
				term->VideoCursor = NULL;
			}
		}
		if(!term->VideoCursor) {
			term->VideoCursor = malloc(sizeof(tVideo_IOCtl_Pos) + bmp->W*bmp->H*sizeof(Uint32));
			if(!term->VideoCursor) {
				Log_Error("VTerm", "Unable to allocate memory for cursor");
				errno = -ENOMEM;
				LEAVE_RET('i', -1);
			}
		}
		
		memcpy(term->VideoCursor, bmp, sizeof(tVideo_IOCtl_Pos) + bmp->W*bmp->H*sizeof(Uint32));
	
		Log_Debug("VTerm", "Set VT%i's cursor to %p %ix%i",
			(int)term->Node.Inode, bmp, bmp->W, bmp->H);

		if(gpVT_CurTerm == term)
			VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSORBITMAP, term->VideoCursor);
	
		LEAVE('i', 0);
		return 0; }
	}
	LEAVE('i', -1);
	return -1;
}

/**
 * \fn void VT_SetTerminal(int ID)
 * \brief Set the current terminal
 */
void VT_SetTerminal(int ID)
{
	// Copy the screen state
	if( ID != giVT_CurrentTerminal && gpVT_CurTerm->Mode != TERM_MODE_TEXT )
	{
		if( !gpVT_CurTerm->Buffer )
			gpVT_CurTerm->Buffer = malloc( gpVT_CurTerm->Width*gpVT_CurTerm->Height*4 );
		if( gpVT_CurTerm->Width < giVT_RealWidth )
		{
			 int	line;
			Uint	ofs = 0;
			Uint32	*dest = gpVT_CurTerm->Buffer;
			// Slower scanline copy
			for( line = 0; line < gpVT_CurTerm->Height; line ++ )
			{
				VFS_ReadAt(giVT_OutputDevHandle, ofs, gpVT_CurTerm->Width*4, dest);
				ofs += giVT_RealWidth * 4;
				dest += gpVT_CurTerm->Width;
			}
		}
		else
		{
			VFS_ReadAt(giVT_OutputDevHandle,
				0, gpVT_CurTerm->Height*giVT_RealWidth*4,
				gpVT_CurTerm->Buffer
				);
		}
	}

	// Update current terminal ID
	Log_Log("VTerm", "Changed terminal from %i to %i", giVT_CurrentTerminal, ID);
	giVT_CurrentTerminal = ID;
	gpVT_CurTerm = &gVT_Terminals[ID];
	
	if( gpVT_CurTerm->Mode == TERM_MODE_TEXT )
	{
		VT_SetMode( VIDEO_BUFFMT_TEXT );
	}
	else
	{
		// Update the cursor image
		if(gpVT_CurTerm->VideoCursor)
			VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSORBITMAP, gpVT_CurTerm->VideoCursor);
		VT_SetMode( VIDEO_BUFFMT_FRAMEBUFFER );
	}
	
	if(gpVT_CurTerm->Buffer)
	{
		// TODO: Handle non equal sized
		VFS_WriteAt(
			giVT_OutputDevHandle,
			0,
			gpVT_CurTerm->Width*gpVT_CurTerm->Height*sizeof(Uint32),
			gpVT_CurTerm->Buffer
			);
	}
	
	VT_int_UpdateCursor(gpVT_CurTerm, 1);
	// Update the screen
//	VT_int_UpdateScreen(gpVT_CurTerm, 1);
}

/**
 * \fn void VT_KBCallBack(Uint32 Codepoint)
 * \brief Called on keyboard interrupt
 * \param Codepoint	Pseudo-UTF32 character
 * 
 * Handles a key press and sends the key code to the user's buffer.
 * If the code creates a kernel-magic sequence, it is not passed to the
 * user and is handled in-kernel.
 */
void VT_KBCallBack(Uint32 Codepoint)
{
	tVTerm	*term = gpVT_CurTerm;

	// Key Up
	switch( Codepoint & KEY_ACTION_MASK )
	{
	case KEY_ACTION_RELEASE:
		switch(Codepoint & KEY_CODEPOINT_MASK)
		{
		case KEY_LALT:	gbVT_AltDown &= ~1;	break;
		case KEY_RALT:	gbVT_AltDown &= ~2;	break;
		case KEY_LCTRL:	gbVT_CtrlDown &= ~1;	break;
		case KEY_RCTRL:	gbVT_CtrlDown &= ~2;	break;
		}
		break;
	
	case KEY_ACTION_PRESS:
		switch(Codepoint & KEY_CODEPOINT_MASK)
		{
		case KEY_LALT:	gbVT_AltDown |= 1;	break;
		case KEY_RALT:	gbVT_AltDown |= 2;	break;
		case KEY_LCTRL:	gbVT_CtrlDown |= 1;	break;
		case KEY_RCTRL:	gbVT_CtrlDown |= 2;	break;
		}
		
		if(!gbVT_AltDown || !gbVT_CtrlDown)
			break;
		switch(Codepoint & KEY_CODEPOINT_MASK)
		{
		case KEY_F1:	VT_SetTerminal(0);	return;
		case KEY_F2:	VT_SetTerminal(1);	return;
		case KEY_F3:	VT_SetTerminal(2);	return;
		case KEY_F4:	VT_SetTerminal(3);	return;
		case KEY_F5:	VT_SetTerminal(4);	return;
		case KEY_F6:	VT_SetTerminal(5);	return;
		case KEY_F7:	VT_SetTerminal(6);	return;
		case KEY_F8:	VT_SetTerminal(7);	return;
		case KEY_F9:	VT_SetTerminal(8);	return;
		case KEY_F10:	VT_SetTerminal(9);	return;
		case KEY_F11:	VT_SetTerminal(10);	return;
		case KEY_F12:	VT_SetTerminal(11);	return;
		// Scrolling
		case KEY_PGUP:
			if( gpVT_CurTerm->Flags & VT_FLAG_ALTBUF )
				return ;
			if( gpVT_CurTerm->ViewPos > gpVT_CurTerm->Width )
				gpVT_CurTerm->ViewPos -= gpVT_CurTerm->Width;
			else
				gpVT_CurTerm->ViewPos = 0;
			return;
		case KEY_PGDOWN:
			if( gpVT_CurTerm->Flags & VT_FLAG_ALTBUF )
				return ;
			if( gpVT_CurTerm->ViewPos < gpVT_CurTerm->Width*gpVT_CurTerm->Height*(giVT_Scrollback) )
				gpVT_CurTerm->ViewPos += gpVT_CurTerm->Width;
			else
				gpVT_CurTerm->ViewPos = gpVT_CurTerm->Width*gpVT_CurTerm->Height*(giVT_Scrollback);
			return;
		}
		break;
	}
	
	// Encode key
	if(term->Mode == TERM_MODE_TEXT)
	{
		Uint8	buf[6] = {0};
		 int	len = 0;
	
		// Ignore anything that isn't a press or refire
		if( (Codepoint & KEY_ACTION_MASK) != KEY_ACTION_PRESS
		 && (Codepoint & KEY_ACTION_MASK) != KEY_ACTION_REFIRE
		    )
		{
			return ;
		}
	
		Codepoint &= KEY_CODEPOINT_MASK;

		// Ignore Modifer Keys
		if(Codepoint > KEY_MODIFIERS)	return;
		
		// Get UTF-8/ANSI Encoding
		switch(Codepoint)
		{
		// 0: No translation, don't send to user
		case 0:	break;
		case KEY_LEFT:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'D';
			len = 3;
			break;
		case KEY_RIGHT:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'C';
			len = 3;
			break;
		case KEY_UP:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'A';
			len = 3;
			break;
		case KEY_DOWN:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'B';
			len = 3;
			break;
		
		case KEY_PGUP:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = '5'; buf[3] = '~';
			len = 4;
			break;
		case KEY_PGDOWN:
			buf[0] = '\x1B'; buf[1] = '['; buf[2] = '6'; buf[3] = '~';
			len = 4;
			break;
		
		// Attempt to encode in UTF-8
		default:
			len = WriteUTF8( buf, Codepoint );
			if(len == 0) {
				Warning("Codepoint (%x) is unrepresentable in UTF-8", Codepoint);
			}
			break;
		}
		
		if(len == 0) {
			// Unprintable / Don't Pass
			return;
		}

#if 0
		// Handle meta characters
		if( !(term->Flags & VT_FLAG_RAWIN) )
		{
			switch(buf[0])
			{
			case '\3':	// ^C
				
				break;
			}
		}
#endif
		
		// Write
		if( MAX_INPUT_CHARS8 - term->InputWrite >= len )
			memcpy( &term->InputBuffer[term->InputWrite], buf, len );
		else {
			memcpy( &term->InputBuffer[term->InputWrite], buf, MAX_INPUT_CHARS8 - term->InputWrite );
			memcpy( &term->InputBuffer[0], buf, len - (MAX_INPUT_CHARS8 - term->InputWrite) );
		}
		// Roll the buffer over
		term->InputWrite += len;
		term->InputWrite %= MAX_INPUT_CHARS8;
		if( (term->InputWrite - term->InputRead + MAX_INPUT_CHARS8)%MAX_INPUT_CHARS8 < len ) {
			term->InputRead = term->InputWrite + 1;
			term->InputRead %= MAX_INPUT_CHARS8;
		}
	}
	else
	{
		// Encode the raw key event
		Uint32	*raw_in = (void*)term->InputBuffer;
		raw_in[ term->InputWrite ] = Codepoint;
		term->InputWrite ++;
		term->InputWrite %= MAX_INPUT_CHARS32;
		if(term->InputRead == term->InputWrite) {
			term->InputRead ++;
			term->InputRead %= MAX_INPUT_CHARS32;
		}
	}
	
	VFS_MarkAvaliable(&term->Node, 1);
}

/**
 * \brief Clears a line in a virtual terminal
 * \param Term	Terminal to modify
 * \param Num	Line number to clear
 */
void VT_int_ClearLine(tVTerm *Term, int Num)
{
	 int	i;
	tVT_Char	*cell;
	
	if( Num < 0 || Num >= Term->TextHeight * (giVT_Scrollback + 1) )	return ;
	
	cell = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltBuf : Term->Text;
	cell = &cell[ Num*Term->TextWidth ];
	
	for( i = Term->TextWidth; i--; )
	{
		cell[ i ].Ch = 0;
		cell[ i ].Colour = Term->CurColour;
	}
}

/**
 * \brief Handle a standard large escape code
 * 
 * Handles any escape code of the form \x1B[n,...A where n is an integer
 * and A is any letter.
 */
void VT_int_ParseEscape_StandardLarge(tVTerm *Term, char CmdChar, int argc, int *args)
{
	 int	tmp = 1;
	switch(CmdChar)
	{
	// Left
	case 'D':
		tmp = -1;
	// Right
	case 'C':
		if(argc == 1)	tmp *= args[0];
		if( Term->Flags & VT_FLAG_ALTBUF )
		{
			if( (Term->AltWritePos + tmp) % Term->TextWidth == 0 ) {
				Term->AltWritePos -= Term->AltWritePos % Term->TextWidth;
				Term->AltWritePos += Term->TextWidth - 1;
			}
			else
				Term->AltWritePos += tmp;
		}
		else
		{
			if( (Term->WritePos + tmp) % Term->TextWidth == 0 ) {
				Term->WritePos -= Term->WritePos % Term->TextWidth;
				Term->WritePos += Term->TextWidth - 1;
			}
			else
				Term->WritePos += tmp;
		}
		break;
	
	// Erase
	case 'J':
		switch(args[0])
		{
		case 0:	// Erase below
			break;
		case 1:	// Erase above
			break;
		case 2:	// Erase all
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i = Term->TextHeight;
				while( i-- )	VT_int_ClearLine(Term, i);
				Term->AltWritePos = 0;
				VT_int_UpdateScreen(Term, 1);
			}
			else
			{
				 int	i = Term->TextHeight * (giVT_Scrollback + 1);
				while( i-- )	VT_int_ClearLine(Term, i);
				Term->WritePos = 0;
				Term->ViewPos = 0;
				VT_int_UpdateScreen(Term, 1);
			}
			break;
		}
		break;
	
	// Erase in line
	case 'K':
		switch(args[0])
		{
		case 0:	// Erase to right
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i, max;
				max = Term->Width - Term->AltWritePos % Term->Width;
				for( i = 0; i < max; i ++ )
					Term->AltBuf[Term->AltWritePos+i].Ch = 0;
			}
			else
			{
				 int	i, max;
				max = Term->Width - Term->WritePos % Term->Width;
				for( i = 0; i < max; i ++ )
					Term->Text[Term->WritePos+i].Ch = 0;
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		case 1:	// Erase to left
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				 int	i = Term->AltWritePos % Term->Width;
				while( i -- )
					Term->AltBuf[Term->AltWritePos++].Ch = 0;
			}
			else
			{
				 int	i = Term->WritePos % Term->Width;
				while( i -- )
					Term->Text[Term->WritePos++].Ch = 0;
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		case 2:	// Erase all
			if( Term->Flags & VT_FLAG_ALTBUF )
			{
				VT_int_ClearLine(Term, Term->AltWritePos / Term->Width);
			}
			else
			{
				VT_int_ClearLine(Term, Term->WritePos / Term->Width);
			}
			VT_int_UpdateScreen(Term, 0);
			break;
		}
		break;
	
	// Set cursor position
	case 'H':
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = args[0] + args[1]*Term->TextWidth;
		else
			Term->WritePos = args[0] + args[1]*Term->TextWidth;
		//Log_Debug("VTerm", "args = {%i, %i}", args[0], args[1]);
		break;
	
	// Scroll up `n` lines
	case 'S':
		tmp = -1;
	// Scroll down `n` lines
	case 'T':
		if(argc == 1)	tmp *= args[0];
		if( Term->Flags & VT_FLAG_ALTBUF )
			VT_int_ScrollText(Term, tmp);
		else
		{
			if(Term->ViewPos/Term->TextWidth + tmp < 0)
				break;
			if(Term->ViewPos/Term->TextWidth + tmp  > Term->TextHeight * (giVT_Scrollback + 1))
				break;
			
			Term->ViewPos += Term->TextWidth*tmp;
		}
		break;
	
	// Set Font flags
	case 'm':
		for( ; argc--; )
		{
			 int	colour_idx;
			// Flags
			if( 0 <= args[argc] && args[argc] <= 8)
			{
				switch(args[argc])
				{
				case 0:	Term->CurColour = DEFAULT_COLOUR;	break;	// Reset
				case 1:	Term->CurColour |= 0x80000000;	break;	// Bright
				case 2:	Term->CurColour &= ~0x80000000;	break;	// Dim
				}
			}
			// Foreground Colour
			else if(30 <= args[argc] && args[argc] <= 37) {
				// Get colour index, accounting for bright bit
				colour_idx = args[argc]-30 + ((Term->CurColour>>28) & 8);
				Term->CurColour &= 0x8000FFFF;
				Term->CurColour |= (Uint32)caVT100Colours[ colour_idx ] << 16;
			}
			// Background Colour
			else if(40 <= args[argc] && args[argc] <= 47) {
				// Get colour index, accounting for bright bit
				colour_idx = args[argc]-40 + ((Term->CurColour>>12) & 8);
				Term->CurColour &= 0xFFFF8000;
				Term->CurColour |= caVT100Colours[ colour_idx ];
			}
			else {
				Log_Warning("VTerm", "Unknown font flag %i", args[argc]);
			}
		}
		break;
	
	// Set scrolling region
	case 'r':
		if( argc != 2 )	break;
		Term->ScrollTop = args[0];
		Term->ScrollHeight = args[1] - args[0];
		break;
	
	default:
		Log_Warning("VTerm", "Unknown control sequence '\\x1B[%c'", CmdChar);
		break;
	}
}

/**
 * \fn int VT_int_ParseEscape(tVTerm *Term, char *Buffer)
 * \brief Parses a VT100 Escape code
 */
int VT_int_ParseEscape(tVTerm *Term, char *Buffer)
{
	char	c;
	 int	argc = 0, j = 1;
	 int	args[6] = {0,0,0,0};
	 int	bQuestionMark = 0;
	
	switch(Buffer[0])
	{
	//Large Code
	case '[':
		// Get Arguments
		c = Buffer[j++];
		if(c == '?') {
			bQuestionMark = 1;
			c = Buffer[j++];
		}
		if( '0' <= c && c <= '9' )
		{
			do {
				if(c == ';')	c = Buffer[j++];
				while('0' <= c && c <= '9') {
					args[argc] *= 10;
					args[argc] += c-'0';
					c = Buffer[j++];
				}
				argc ++;
			} while(c == ';');
		}
		
		// Get Command
		if( ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
		{
			if( bQuestionMark )
			{
				switch(c)
				{
				// DEC Private Mode Set
				case 'h':
					if(argc != 1)	break;
					switch(args[0])
					{
					case 1047:
						VT_int_ToggleAltBuffer(Term, 1);
						break;
					}
					break;
				case 'l':
					if(argc != 1)	break;
					switch(args[0])
					{
					case 1047:
						VT_int_ToggleAltBuffer(Term, 0);
						break;
					}
					break;
				default:
					Log_Warning("VTerm", "Unknown control sequence '\\x1B[?%c'", c);
					break;
				}
			}
			else
			{
				VT_int_ParseEscape_StandardLarge(Term, c, argc, args);
			}
		}
		break;
		
	default:
		Log_Notice("VTerm", "TODO: Handle short escape codes");
		break;
	}
	
	//Log_Debug("VTerm", "j = %i, Buffer = '%s'", j, Buffer);
	return j;
}

/**
 * \fn void VT_int_PutString(tVTerm *Term, Uint8 *Buffer, Uint Count)
 * \brief Print a string to the Virtual Terminal
 */
void VT_int_PutString(tVTerm *Term, Uint8 *Buffer, Uint Count)
{
	Uint32	val;
	 int	i;
	
	// Iterate
	for( i = 0; i < Count; i++ )
	{
		// Handle escape sequences
		if( Buffer[i] == 0x1B )
		{
			i ++;
			i += VT_int_ParseEscape(Term, (char*)&Buffer[i]) - 1;
			continue;
		}
		
		// Fast check for non UTF-8
		if( Buffer[i] < 128 )	// Plain ASCII
			VT_int_PutChar(Term, Buffer[i]);
		else {	// UTF-8
			i += ReadUTF8(&Buffer[i], &val) - 1;
			VT_int_PutChar(Term, val);
		}
	}
	// Update Screen
	VT_int_UpdateScreen( Term, 0 );
}

/**
 * \fn void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
 * \brief Write a single character to a VTerm
 */
void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
{
	 int	i;
	tVT_Char	*buffer;
	 int	write_pos;
	
	if(Term->Flags & VT_FLAG_ALTBUF) {
		buffer = Term->AltBuf;
		write_pos = Term->AltWritePos;
	}
	else {
		buffer = Term->Text;
		write_pos = Term->WritePos;
	}
	
	switch(Ch)
	{
	case '\0':	return;	// Ignore NULL byte
	case '\n':
		VT_int_UpdateScreen( Term, 0 );	// Update the line before newlining
		write_pos += Term->TextWidth;
	case '\r':
		write_pos -= write_pos % Term->TextWidth;
		break;
	
	case '\t': { int tmp = write_pos / Term->TextWidth;
		write_pos %= Term->TextWidth;
		do {
			buffer[ write_pos ].Ch = '\0';
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos ++;
		} while(write_pos & 7);
		write_pos += tmp * Term->TextWidth;
		break; }
	
	case '\b':
		// Backspace is invalid at Offset 0
		if(write_pos == 0)	break;
		
		write_pos --;
		// Singe Character
		if(buffer[ write_pos ].Ch != '\0') {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			break;
		}
		// Tab
		i = 7;	// Limit it to 8
		do {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos --;
		} while(write_pos && i-- && buffer[ write_pos ].Ch == '\0');
		if(buffer[ write_pos ].Ch != '\0')
			write_pos ++;
		break;
	
	default:
		buffer[ write_pos ].Ch = Ch;
		buffer[ write_pos ].Colour = Term->CurColour;
		// Update the line before wrapping
		if( (write_pos + 1) % Term->TextWidth == 0 )
			VT_int_UpdateScreen( Term, 0 );
		write_pos ++;
		break;
	}
	
	if(Term->Flags & VT_FLAG_ALTBUF)
	{
		Term->AltBuf = buffer;
		Term->AltWritePos = write_pos;
		
		if(Term->AltWritePos >= Term->TextWidth*Term->TextHeight)
		{
			Term->AltWritePos -= Term->TextWidth;
			VT_int_ScrollText(Term, 1);
		}
		
	}
	else
	{
		Term->Text = buffer;
		Term->WritePos = write_pos;
		// Move Screen
		// - Check if we need to scroll the entire scrollback buffer
		if(Term->WritePos >= Term->TextWidth*Term->TextHeight*(giVT_Scrollback+1))
		{
			 int	base;
			
			// Update previous line
			Term->WritePos -= Term->TextWidth;
			VT_int_UpdateScreen( Term, 0 );
			
			// Update view position
			base = Term->TextWidth*Term->TextHeight*(giVT_Scrollback);
			if(Term->ViewPos < base)
				Term->ViewPos += Term->Width;
			if(Term->ViewPos > base)
				Term->ViewPos = base;
			
			VT_int_ScrollText(Term, 1);
		}
		// Ok, so we only need to scroll the screen
		else if(Term->WritePos >= Term->ViewPos + Term->TextWidth*Term->TextHeight)
		{
			// Update the last line
			Term->WritePos -= Term->TextWidth;
			VT_int_UpdateScreen( Term, 0 );
			Term->WritePos += Term->TextWidth;
			
			VT_int_ScrollText(Term, 1);
			
			Term->ViewPos += Term->TextWidth;
		}
	}
	
	//LEAVE('-');
}

void VT_int_ScrollText(tVTerm *Term, int Count)
{
	tVT_Char	*buf;
	 int	height, init_write_pos;
	 int	len, i;
	 int	scroll_top, scroll_height;

	// Get buffer pointer and attributes	
	if( Term->Flags & VT_FLAG_ALTBUF )
	{
		buf = Term->AltBuf;
		height = Term->TextHeight;
		init_write_pos = Term->AltWritePos;
		scroll_top = Term->ScrollTop;
		scroll_height = Term->ScrollHeight;
	}
	else
	{
		buf = Term->Text;
		height = Term->TextHeight*(giVT_Scrollback+1);
		init_write_pos = Term->WritePos;
		scroll_top = 0;
		scroll_height = height;
	}

	// Scroll text downwards	
	if( Count > 0 )
	{
		 int	base;
	
		// Set up
		if(Count > scroll_height)	Count = scroll_height;
		base = Term->TextWidth*(scroll_top + scroll_height - Count);
		len = Term->TextWidth*(scroll_height - Count);
		
		// Scroll terminal cache
		memmove(
			&buf[Term->TextWidth*scroll_top],
			&buf[Term->TextWidth*(scroll_top+Count)],
			len*sizeof(tVT_Char)
			);
		// Clear last rows
		for( i = 0; i < Term->TextWidth*Count; i ++ )
		{
			buf[ base + i ].Ch = 0;
			buf[ base + i ].Colour = Term->CurColour;
		}
		
		// Update Screen
		VT_int_ScrollFramebuffer( Term, Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = base;
		else
			Term->WritePos = Term->ViewPos + Term->TextWidth*(Term->TextHeight - Count);
		for( i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
			if( Term->Flags & VT_FLAG_ALTBUF )
				Term->AltWritePos += Term->TextWidth;
			else
				Term->WritePos += Term->TextWidth;
		}
	}
	else
	{
		Count = -Count;
		if(Count > scroll_height)	Count = scroll_height;
		
		len = Term->TextWidth*(scroll_height - Count);
		
		// Scroll terminal cache
		memmove(
			&buf[Term->TextWidth*(scroll_top+Count)],
			&buf[Term->TextWidth*scroll_top],
			len*sizeof(tVT_Char)
			);
		// Clear preceding rows
		for( i = 0; i < Term->TextWidth*Count; i ++ )
		{
			buf[ i ].Ch = 0;
			buf[ i ].Colour = Term->CurColour;
		}
		
		VT_int_ScrollFramebuffer( Term, -Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = Term->TextWidth*scroll_top;
		else
			Term->WritePos = Term->ViewPos;
		for( i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
			if( Term->Flags & VT_FLAG_ALTBUF )
				Term->AltWritePos += Term->TextWidth;
			else
				Term->WritePos += Term->TextWidth;
		}
	}
	
	if( Term->Flags & VT_FLAG_ALTBUF )
		Term->AltWritePos = init_write_pos;
	else
		Term->WritePos = init_write_pos;
}

/**
 * \fn void VT_int_ScrollFramebuffer( tVTerm *Term, int Count )
 * \note Scrolls the framebuffer down by \a Count text lines
 */
void VT_int_ScrollFramebuffer( tVTerm *Term, int Count )
{
	 int	tmp;
	struct {
		Uint8	Op;
		Uint16	DstX, DstY;
		Uint16	SrcX, SrcY;
		Uint16	W, H;
	} PACKED	buf;
	
	// Only update if this is the current terminal
	if( Term != gpVT_CurTerm )	return;
	
	if( Count > Term->ScrollHeight )	Count = Term->ScrollHeight;
	if( Count < -Term->ScrollHeight )	Count = -Term->ScrollHeight;
	
	// Switch to 2D Command Stream
	tmp = VIDEO_BUFFMT_2DSTREAM;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
	
	// BLIT to 0,0 from 0,giVT_CharHeight
	buf.Op = VIDEO_2DOP_BLIT;
	buf.SrcX = 0;	buf.DstX = 0;
	// TODO: Don't assume character dimensions
	buf.W = Term->TextWidth * giVT_CharWidth;
	if( Count > 0 )
	{
		buf.SrcY = (Term->ScrollTop+Count) * giVT_CharHeight;
		buf.DstY = Term->ScrollTop * giVT_CharHeight;
	}
	else	// Scroll up, move text down
	{
		Count = -Count;
		buf.SrcY = Term->ScrollTop * giVT_CharHeight;
		buf.DstY = (Term->ScrollTop+Count) * giVT_CharHeight;
	}
	buf.H = (Term->ScrollHeight-Count) * giVT_CharHeight;
	VFS_WriteAt(giVT_OutputDevHandle, 0, sizeof(buf), &buf);
	
	// Restore old mode (this function is only called during text mode)
	tmp = VIDEO_BUFFMT_TEXT;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
}

void VT_int_UpdateCursor( tVTerm *Term, int bShow )
{
	tVideo_IOCtl_Pos	csr_pos;

	if( Term != gpVT_CurTerm )	return ;

	if( !bShow )
	{
		csr_pos.x = -1;	
		csr_pos.y = -1;	
	}
	else if( Term->Mode == TERM_MODE_TEXT )
	{
		 int	offset;
		
//		if( !(Term->Flags & VT_FLAG_SHOWCSR)
//		 && ( (Term->Flags & VT_FLAG_HIDECSR) || !Term->Node.ReadThreads)
//		  )
		if( !Term->Text || Term->Flags & VT_FLAG_HIDECSR )
		{
			csr_pos.x = -1;
			csr_pos.y = -1;
		}
		else
		{
			if(Term->Flags & VT_FLAG_ALTBUF)
				offset = Term->AltWritePos;
			else
				offset = Term->WritePos - Term->ViewPos;
					
			csr_pos.x = offset % Term->TextWidth;
			csr_pos.y = offset / Term->TextWidth;
			if( 0 > csr_pos.y || csr_pos.y >= Term->TextHeight )
				csr_pos.y = -1, csr_pos.x = -1;
		}
	}
	else
	{
		csr_pos.x = Term->VideoCursorX;
		csr_pos.y = Term->VideoCursorY;
	}
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSOR, &csr_pos);
}	

/**
 * \fn void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
 * \brief Updates the video framebuffer
 */
void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
{
	tVT_Char	*buffer;
	 int	view_pos, write_pos;
	// Only update if this is the current terminal
	if( Term != gpVT_CurTerm )	return;
	
	switch( Term->Mode )
	{
	case TERM_MODE_TEXT:
		view_pos = (Term->Flags & VT_FLAG_ALTBUF) ? 0 : Term->ViewPos;
		write_pos = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltWritePos : Term->WritePos;
		buffer = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltBuf : Term->Text;
		// Re copy the entire screen?
		if(UpdateAll) {
			VFS_WriteAt(
				giVT_OutputDevHandle,
				0,
				Term->TextWidth*Term->TextHeight*sizeof(tVT_Char),
				&buffer[view_pos]
				);
		}
		// Only copy the current line
		else {
			 int	ofs = write_pos - write_pos % Term->TextWidth;
			VFS_WriteAt(
				giVT_OutputDevHandle,
				(ofs - view_pos)*sizeof(tVT_Char),
				Term->TextWidth*sizeof(tVT_Char),
				&buffer[ofs]
				);
		}
		break;
	case TERM_MODE_FB:
		break;
	}
	
	VT_int_UpdateCursor(Term, 1);
}

/**
 * \brief Update the screen mode
 * \param Term	Terminal to update
 * \param NewMode	New mode to set
 * \param NewWidth	New framebuffer width
 * \param NewHeight	New framebuffer height
 */
void VT_int_ChangeMode(tVTerm *Term, int NewMode, int NewWidth, int NewHeight)
{
	 int	oldW = Term->Width;
	 int	oldTW = Term->TextWidth;
	 int	oldH = Term->Height;
	 int	oldTH = Term->TextHeight;
	tVT_Char	*oldTBuf = Term->Text;
	Uint32	*oldFB = Term->Buffer;
	 int	w, h, i;
	
	// TODO: Increase RealWidth/RealHeight when this happens
	if(NewWidth > giVT_RealWidth)	NewWidth = giVT_RealWidth;
	if(NewHeight > giVT_RealHeight)	NewHeight = giVT_RealHeight;
	
	Term->Mode = NewMode;

	// Fast exit if no resolution change
	if(NewWidth == Term->Width && NewHeight == Term->Height)
		return ;
	
	// Calculate new dimensions
	Term->Width = NewWidth;
	Term->Height = NewHeight;
	Term->TextWidth = NewWidth / giVT_CharWidth;
	Term->TextHeight = NewHeight / giVT_CharHeight;
	Term->ScrollHeight = Term->TextHeight - (oldTH - Term->ScrollHeight) - Term->ScrollTop;
	
	// Allocate new buffers
	// - Text
	Term->Text = calloc(
		Term->TextWidth * Term->TextHeight * (giVT_Scrollback+1),
		sizeof(tVT_Char)
		);
	if(oldTBuf) {
		// Copy old buffer
		w = (oldTW > Term->TextWidth) ? Term->TextWidth : oldTW;
		h = (oldTH > Term->TextHeight) ? Term->TextHeight : oldTH;
		h *= giVT_Scrollback + 1;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Text[i*Term->TextWidth],
				&oldTBuf[i*oldTW],
				w*sizeof(tVT_Char)
				);	
		}
		free(oldTBuf);
	}
	
	// - Alternate Text
	Term->AltBuf = realloc(
		Term->AltBuf,
		Term->TextWidth * Term->TextHeight * sizeof(tVT_Char)
		);
	
	// - Framebuffer
	if(oldFB) {
		Term->Buffer = calloc( Term->Width * Term->Height, sizeof(Uint32) );
		// Copy old buffer
		w = (oldW > Term->Width) ? Term->Width : oldW;
		h = (oldH > Term->Height) ? Term->Height : oldH;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Buffer[i*Term->Width],
				&oldFB[i*oldW],
				w*sizeof(Uint32)
				);
		}
		free(oldFB);
	}
	
	// Debug
	switch(NewMode)
	{
	case TERM_MODE_TEXT:
		Log_Log("VTerm", "Set VT %p to text mode (%ix%i)",
			Term, Term->TextWidth, Term->TextHeight);
		break;
	case TERM_MODE_FB:
		Log_Log("VTerm", "Set VT %p to framebuffer mode (%ix%i)",
			Term, Term->Width, Term->Height);
		break;
	//case TERM_MODE_2DACCEL:
	//case TERM_MODE_3DACCEL:
	//	return;
	}
}


void VT_int_ToggleAltBuffer(tVTerm *Term, int Enabled)
{	
	if(Enabled)
		Term->Flags |= VT_FLAG_ALTBUF;
	else
		Term->Flags &= ~VT_FLAG_ALTBUF;
	VT_int_UpdateScreen(Term, 1);
}

// ---
// Font Render
// ---
#define MONOSPACE_FONT	10816

#if MONOSPACE_FONT == 10808	// 8x8
# include "vterm_font_8x8.h"
#elif MONOSPACE_FONT == 10816	// 8x16
# include "vterm_font_8x16.h"
#endif

// === PROTOTYPES ===
Uint8	*VT_Font_GetChar(Uint32 Codepoint);

// === GLOBALS ===
int	giVT_CharWidth = FONT_WIDTH;
int	giVT_CharHeight = FONT_HEIGHT;

// === CODE ===
/**
 * \brief Render a font character
 */
void VT_Font_Render(Uint32 Codepoint, void *Buffer, int Depth, int Pitch, Uint32 BGC, Uint32 FGC)
{
	Uint8	*font;
	 int	x, y;
	
	// 8-bpp and below
	if( Depth <= 8 )
	{
		Uint8	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 16-bpp and below
	else if( Depth <= 16 )
	{
		Uint16	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 24-bpp colour
	// - Special handling to not overwrite the next pixel
	//TODO: Endian issues here
	else if( Depth == 24 )
	{
		Uint8	*buf = Buffer;
		Uint8	bg_r = (BGC >> 16) & 0xFF;
		Uint8	bg_g = (BGC >>  8) & 0xFF;
		Uint8	bg_b = (BGC >>  0) & 0xFF;
		Uint8	fg_r = (FGC >> 16) & 0xFF;
		Uint8	fg_g = (FGC >>  8) & 0xFF;
		Uint8	fg_b = (FGC >>  0) & 0xFF;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				Uint8	r, g, b;
				
				if(*font & (1 << (FONT_WIDTH-x-1))) {
					r = fg_r;	g = fg_g;	b = fg_b;
				}
				else {
					r = bg_r;	g = bg_g;	b = bg_b;
				}
				buf[x*3+0] = b;
				buf[x*3+1] = g;
				buf[x*3+2] = r;
			}
			buf = (void*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
	// 32-bpp colour (nice and easy)
	else if( Depth == 32 )
	{
		Uint32	*buf = Buffer;
		
		font = VT_Font_GetChar(Codepoint);
		
		for(y = 0; y < FONT_HEIGHT; y ++)
		{
			for(x = 0; x < FONT_WIDTH; x ++)
			{
				if(*font & (1 << (FONT_WIDTH-x-1)))
					buf[x] = FGC;
				else
					buf[x] = BGC;
			}
			buf = (Uint32*)( (tVAddr)buf + Pitch );
			font ++;
		}
	}
}

/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a 12-bit colour into 24 bits
 */
Uint32 VT_Colour12to24(Uint16 Col12)
{
	Uint32	ret;
	 int	tmp;
	tmp = Col12 & 0xF;
	ret  = (tmp << 0) | (tmp << 4);
	tmp = (Col12 & 0xF0) >> 4;
	ret |= (tmp << 8) | (tmp << 12);
	tmp = (Col12 & 0xF00) >> 8;
	ret |= (tmp << 16) | (tmp << 20);
	return ret;
}
/**
 * \brief Converts a 12-bit colour into 15 bits
 */
Uint16 VT_Colour12to15(Uint16 Col12)
{
	Uint32	ret;
	 int	tmp;
	tmp = Col12 & 0xF;
	ret  = (tmp << 1) | (tmp & 1);
	tmp = (Col12 & 0xF0) >> 4;
	ret |= ( (tmp << 1) | (tmp & 1) ) << 5;
	tmp = (Col12 & 0xF00) >> 8;
	ret |= ( (tmp << 1) | (tmp & 1) ) << 10;
	return ret;
}

/**
 * \brief Converts a 12-bit colour into any other depth
 * \param Col12	12-bit source colour
 * \param Depth	Desired bit deptj
 * \note Green then blue get the extra avaliable bits (16:5-6-5, 14:4-5-5)
 */
Uint32 VT_Colour12toN(Uint16 Col12, int Depth)
{
	Uint32	ret;
	Uint32	r, g, b;
	 int	rSize, gSize, bSize;
	
	// Fast returns
	if( Depth == 24 )	return VT_Colour12to24(Col12);
	if( Depth == 15 )	return VT_Colour12to15(Col12);
	// - 32 is a special case, it's usually 24-bit colour with an unused byte
	if( Depth == 32 )	return VT_Colour12to24(Col12);
	
	// Bounds checks
	if( Depth < 8 )	return 0;
	if( Depth > 32 )	return 0;
	
	r = Col12 & 0xF;
	g = (Col12 & 0xF0) >> 4;
	b = (Col12 & 0xF00) >> 8;
	
	rSize = gSize = bSize = Depth / 3;
	if( rSize + gSize + bSize < Depth )	// Depth % 3 == 1
		gSize ++;
	if( rSize + gSize + bSize < Depth )	// Depth % 3 == 2
		bSize ++;
	
	// Expand
	r <<= rSize - 4;	g <<= gSize - 4;	b <<= bSize - 4;
	// Fill with the lowest bit
	if( Col12 & 0x001 )	r |= (1 << (rSize - 4)) - 1;
	if( Col12 & 0x010 )	r |= (1 << (gSize - 4)) - 1;
	if( Col12 & 0x100 )	r |= (1 << (bSize - 4)) - 1;
	
	// Create output
	ret  = r;
	ret |= g << rSize;
	ret |= b << (rSize + gSize);
	
	return ret;
}

/**
 * \fn Uint8 *VT_Font_GetChar(Uint32 Codepoint)
 * \brief Gets an index into the font array given a Unicode Codepoint
 * \note See http://en.wikipedia.org/wiki/CP437
 */
Uint8 *VT_Font_GetChar(Uint32 Codepoint)
{
	 int	index = 0;
	if(Codepoint < 128)
		return &VTermFont[Codepoint*FONT_HEIGHT];
	switch(Codepoint)
	{
	case 0xC7:	index = 128;	break;	// Ç
	case 0xFC:	index = 129;	break;	// ü
	case 0xE9:	index = 130;	break;	// é
	case 0xE2:	index = 131;	break;	// â
	case 0xE4:	index = 132;	break;	// ä
	case 0xE0:	index = 133;	break;	// à
	case 0xE5:	index = 134;	break;	// å
	case 0xE7:	index = 135;	break;	// ç
	case 0xEA:	index = 136;	break;	// ê
	case 0xEB:	index = 137;	break;	// ë
	case 0xE8:	index = 138;	break;	// è
	case 0xEF:	index = 139;	break;	// ï
	case 0xEE:	index = 140;	break;	// î
	case 0xEC:	index = 141;	break;	// ì
	case 0xC4:	index = 142;	break;	// Ä
	case 0xC5:	index = 143;	break;	// Å
	}
	
	return &VTermFont[index*FONT_HEIGHT];
}

EXPORTAS(&giVT_CharWidth, giVT_CharWidth);
EXPORTAS(&giVT_CharHeight, giVT_CharHeight);
EXPORT(VT_Font_Render);
EXPORT(VT_Colour12to24);

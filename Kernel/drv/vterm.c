/*
 * Acess2 Virtual Terminal Driver
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include <modules.h>
#include <tpl_drv_video.h>
#include <tpl_drv_keyboard.h>
#include <tpl_drv_terminal.h>
#include <errno.h>

#define	USE_CTRL_ALT	0

// === CONSTANTS ===
#define VERSION	((0<<8)|(50))

#define	NUM_VTS	8
#define MAX_INPUT_CHARS32	64
#define MAX_INPUT_CHARS8	(MAX_INPUT_CHARS32*4)
//#define DEFAULT_OUTPUT	"BochsGA"
#define DEFAULT_OUTPUT	"Vesa"
#define DEFAULT_INPUT	"PS2Keyboard"
#define	DEFAULT_WIDTH	640
#define	DEFAULT_HEIGHT	480
#define DEFAULT_SCROLLBACK	2	// 2 Screens of text + current screen
#define	DEFAULT_COLOUR	(VT_COL_BLACK|(0xAAA<<16))

#define	VT_FLAG_HIDECSR	0x01
#define	VT_FLAG_HASFB	0x10	//!< Set if the VTerm has requested the Framebuffer

enum eVT_InModes {
	VT_INMODE_TEXT8,	// UTF-8 Text Mode (VT100 Emulation)
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
	
	 int	ViewPos;	//!< View Buffer Offset (Text Only)
	 int	WritePos;	//!< Write Buffer Offset (Text Only)
	Uint32	CurColour;	//!< Current Text Colour
	
	tMutex	ReadingLock;	//!< Lock the VTerm when a process is reading from it
	tTID	ReadingThread;	//!< Owner of the lock
	 int	InputRead;	//!< Input buffer read position
	 int	InputWrite;	//!< Input buffer write position
	char	InputBuffer[MAX_INPUT_CHARS8];
	
	tVT_Char	*Text;
	Uint32		*Buffer;
	
	char	Name[2];	//!< Name of the terminal
	tVFS_Node	Node;
} tVTerm;

// === IMPORTS ===
extern void	Debug_SetKTerminal(char *File);

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
 int	VT_int_ParseEscape(tVTerm *Term, char *Buffer);
void	VT_int_PutChar(tVTerm *Term, Uint32 Ch);
void	VT_int_ScrollFramebuffer( tVTerm *Term );
void	VT_int_UpdateScreen( tVTerm *Term, int UpdateAll );
void	VT_int_ChangeMode(tVTerm *Term, int NewMode, int NewWidth, int NewHeight);

// === CONSTANTS ===
const Uint16	caVT100Colours[] = {
		// Black, Red, Green, Yellow, Blue, Purple, Cyan, Gray
		// Same again, but bright
		VT_COL_BLACK, 0x700, 0x070, 0x770, 0x007, 0x707, 0x077, 0xAAA,
		VT_COL_GREY, 0xF00, 0x0F0, 0xFF0, 0x00F, 0xF0F, 0x0FF, VT_COL_WHITE
	};

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VTerm, VT_Install, NULL, DEFAULT_OUTPUT, DEFAULT_INPUT, NULL);
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
		char	**args = Arguments;
		char	*arg, *opt, *val;
		for( ; (arg = *args); args++ )
		{
			Log_Debug("VTerm", "Argument '%s'", arg);
			opt = arg;
			val = arg + strpos(arg, '=');	*val++ = '\0';
			
			if( strcmp(opt, "Video") == 0 ) {
				gsVT_OutputDevice = val;
			}
			else if( strcmp(opt, "Input") == 0 ) {
				gsVT_InputDevice = val;
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
	
	if(gsVT_OutputDevice)	Modules_InitialiseBuiltin( gsVT_OutputDevice );
	if(gsVT_InputDevice)	Modules_InitialiseBuiltin( gsVT_InputDevice );
	
	// Apply Defaults
	if(!gsVT_OutputDevice)	gsVT_OutputDevice = DEFAULT_OUTPUT;
	if(!gsVT_InputDevice)	gsVT_InputDevice = DEFAULT_INPUT;
	
	// Create paths
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
	
	// Create Nodes
	for( i = 0; i < NUM_VTS; i++ )
	{
		gVT_Terminals[i].Mode = TERM_MODE_TEXT;
		gVT_Terminals[i].Flags = 0;
		gVT_Terminals[i].CurColour = DEFAULT_COLOUR;
		gVT_Terminals[i].WritePos = 0;
		gVT_Terminals[i].ViewPos = 0;
		
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
	}
	
	// Add to DevFS
	DevFS_AddDevice( &gVT_DrvInfo );
	
	VT_InitOutput();
	VT_InitInput();
	
	// Set kernel output to VT0
	Debug_SetKTerminal("/Devices/VTerm/0");
	
	Log_Log("VTerm", "Returning %i", MODULE_ERR_OK);
	return MODULE_ERR_OK;
}

/**
 * \fn void VT_InitOutput()
 * \brief Initialise Video Output
 */
void VT_InitOutput()
{
	giVT_OutputDevHandle = VFS_Open(gsVT_OutputDevice, VFS_OPENFLAG_WRITE);
	if(giVT_InputDevHandle == -1) {
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
	if(giVT_InputDevHandle == -1)	return ;
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
	}
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_GETSETMODE, &tmp );
	
	// Resize text terminals if needed
	if( giVT_RealWidth != mode.width || giVT_RealHeight != mode.height )
	{
		 int	newBufSize = (giVT_RealWidth/giVT_CharWidth)
					*(giVT_RealHeight/giVT_CharHeight)
					*(giVT_Scrollback+1);
		//tVT_Char	*tmp;
		// Resize the text terminals
		giVT_RealWidth = mode.width;
		giVT_RealHeight = mode.height;
		for( i = 0; i < NUM_VTS; i ++ )
		{
			if( gVT_Terminals[i].Mode != TERM_MODE_TEXT )	continue;
			
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
 * \fn Uint64 VT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 * \brief Read from a virtual terminal
 */
Uint64 VT_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	 int	pos = 0;
	tVTerm	*term = &gVT_Terminals[ Node->Inode ];
	
	Mutex_Acquire( &term->ReadingLock );
	term->ReadingThread = Threads_GetTID();
	
	// Check current mode
	switch(term->Mode)
	{
	// Text Mode (UTF-8)
	case TERM_MODE_TEXT:
		while(pos < Length)
		{
			//TODO: Sleep instead
			while(term->InputRead == term->InputWrite)	Threads_Sleep();
			
			((char*)Buffer)[pos] = term->InputBuffer[term->InputRead];
			pos ++;
			term->InputRead ++;
			term->InputRead %= MAX_INPUT_CHARS8;
		}
		break;
	
	//case TERM_MODE_FB:
	// Other - UCS-4
	default:
		while(pos < Length)
		{
			while(term->InputRead == term->InputWrite)	Threads_Sleep();
			((Uint32*)Buffer)[pos] = ((Uint32*)term->InputBuffer)[term->InputRead];
			pos ++;
			term->InputRead ++;
			term->InputRead %= MAX_INPUT_CHARS32;
		}
		break;
	}
	
	term->ReadingThread = -1;
	Mutex_Release( &term->ReadingLock );
	
	return 0;
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
		
		// Copy to the local cache
		memcpy( (void*)((Uint)term->Buffer + (Uint)Offset), Buffer, Length );
		
		// Update screen if needed
		if( Node->Inode == giVT_CurrentTerminal )
		{
			// Fill entire screen?
			if( giVT_RealWidth > term->Width || giVT_RealHeight > term->Height )
			{
				// No? :( Well, just center it
				 int	x, y, w, h;
				x = Offset/4;	y = x / term->Width;	x %= term->Width;
				w = Length/4+x;	h = w / term->Width;	w %= term->Width;
				// Center
				x += (giVT_RealWidth - term->Width) / 2;
				y += (giVT_RealHeight - term->Height) / 2;
				while(h--)
				{
					VFS_WriteAt( giVT_OutputDevHandle,
						(x + y * giVT_RealWidth)*4,
						term->Width * 4,
						Buffer
						);
					Buffer = (void*)( (Uint)Buffer + term->Width*4 );
					y ++;
				}
				return 0;
			}
			else {
				return VFS_WriteAt( giVT_OutputDevHandle, Offset, Length, Buffer );
			}
		}
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
			if( term->Mode != *iData
			 || term->NewWidth
			 || term->NewHeight)
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
			ret = term->TextHeight = *iData;
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
	// Update current terminal ID
	Log_Log("VTerm", "Changed terminal from %i to %i", giVT_CurrentTerminal, ID);
	giVT_CurrentTerminal = ID;
	gpVT_CurTerm = &gVT_Terminals[ID];
	
	// Update cursor
	if( gpVT_CurTerm->Mode == TERM_MODE_TEXT && !(gpVT_CurTerm->Flags & VT_FLAG_HIDECSR) )
	{
		tVideo_IOCtl_Pos	pos;
		pos.x = (gpVT_CurTerm->WritePos - gpVT_CurTerm->ViewPos) % gpVT_CurTerm->TextWidth;
		pos.y = (gpVT_CurTerm->WritePos - gpVT_CurTerm->ViewPos) / gpVT_CurTerm->TextWidth;
		VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSOR, &pos);
	}
	
	if( gpVT_CurTerm->Mode == TERM_MODE_TEXT )
		VT_SetMode( VIDEO_BUFFMT_TEXT );
	else
		VT_SetMode( VIDEO_BUFFMT_FRAMEBUFFER );
	
	// Update the screen
	VT_int_UpdateScreen( &gVT_Terminals[ ID ], 1 );
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
	
	// How the hell did we get a codepoint of zero?
	if(Codepoint == 0)	return;
	
	// Key Up
	if( Codepoint & 0x80000000 )
	{
		Codepoint &= 0x7FFFFFFF;
		switch(Codepoint)
		{
		#if !USE_CTRL_ALT
		case KEY_RSHIFT:	gbVT_CtrlDown = 0;	break;
		case KEY_LSHIFT:	gbVT_AltDown = 0;	break;
		#else
		case KEY_LALT:	gbVT_AltDown &= ~1;	break;
		case KEY_RALT:	gbVT_AltDown &= ~2;	break;
		case KEY_LCTRL:	gbVT_CtrlDown &= ~1	break;
		case KEY_RCTRL:	gbVT_CtrlDown &= ~2;	break;
		#endif
		}
		return;
	}
	
	switch(Codepoint)
	{
	#if !USE_CTRL_ALT	// HACK: Use both shifts instead of Ctrl-Alt
	case KEY_RSHIFT:	gbVT_CtrlDown = 1;	break;
	case KEY_LSHIFT:	gbVT_AltDown = 1;	break;
	#else
	case KEY_LALT:	gbVT_AltDown |= 1;	break;
	case KEY_RALT:	gbVT_AltDown |= 2;	break;
	case KEY_LCTRL:	gbVT_CtrlDown |= 1;	break;
	case KEY_RCTRL:	gbVT_CtrlDown |= 2;	break;
	#endif
	
	default:
		if(!gbVT_AltDown || !gbVT_CtrlDown)
			break;
		switch(Codepoint)
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
			if( gpVT_CurTerm->ViewPos > gpVT_CurTerm->Width )
				gpVT_CurTerm->ViewPos -= gpVT_CurTerm->Width;
			else
				gpVT_CurTerm->ViewPos = 0;
			return;
		case KEY_PGDOWN:
			if( gpVT_CurTerm->ViewPos < gpVT_CurTerm->Width*gpVT_CurTerm->Height*(giVT_Scrollback-1) )
				gpVT_CurTerm->ViewPos += gpVT_CurTerm->Width;
			else
				gpVT_CurTerm->ViewPos = gpVT_CurTerm->Width*gpVT_CurTerm->Height*(giVT_Scrollback-1);
			return;
		}
	}
	
	// Encode key
	if(term->Mode == TERM_MODE_TEXT)
	{
		Uint8	buf[6] = {0};
		 int	len = 0;
		
		// Ignore Modifer Keys
		if(Codepoint > KEY_MODIFIERS)	return;
		
		// Get UTF-8/ANSI Encoding
		switch(Codepoint)
		{
		case KEY_LEFT:
			buf[0] = '\x1B';	buf[1] = '[';	buf[2] = 'D';
			len = 3;
			break;
		case KEY_RIGHT:
			buf[0] = '\x1B';	buf[1] = '[';	buf[2] = 'C';
			len = 3;
			break;
		case KEY_UP:
			buf[0] = '\x1B';	buf[1] = '[';	buf[2] = 'A';
			len = 3;
			break;
		case KEY_DOWN:
			buf[0] = '\x1B';	buf[1] = '[';	buf[2] = 'B';
			len = 3;
			break;
		
		case KEY_PGUP:
			buf[0] = '\x1B';	buf[1] = '[';	buf[2] = '5';	// Some overline also
			//len = 4;	// Commented out until I'm sure
			break;
		case KEY_PGDOWN:
			len = 0;
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
		// Encode the raw UTF-32 Key
		((Uint32*)term->InputBuffer)[ term->InputWrite ] = Codepoint;
		term->InputWrite ++;
		term->InputWrite %= MAX_INPUT_CHARS32;
		if(term->InputRead == term->InputWrite) {
			term->InputRead ++;
			term->InputRead %= MAX_INPUT_CHARS32;
		}
	}
	
	// Wake up the thread waiting on us
	if( term->ReadingThread >= 0 ) {
		Threads_WakeTID(term->ReadingThread);
	}
}

/**
 * \fn void VT_int_ClearLine(tVTerm *Term, int Num)
 * \brief Clears a line in a virtual terminal
 */
void VT_int_ClearLine(tVTerm *Term, int Num)
{
	 int	i;
	tVT_Char	*cell = &Term->Text[ Num*Term->TextWidth ];
	if( Num < 0 || Num >= Term->TextHeight )	return ;
	//ENTER("pTerm iNum", Term, Num);
	for( i = Term->TextWidth; i--; )
	{
		cell[ i ].Ch = 0;
		cell[ i ].Colour = Term->CurColour;
	}
	//LEAVE('-');
}

/**
 * \fn int VT_int_ParseEscape(tVTerm *Term, char *Buffer)
 * \brief Parses a VT100 Escape code
 */
int VT_int_ParseEscape(tVTerm *Term, char *Buffer)
{
	char	c;
	 int	argc = 0, j = 1;
	 int	tmp;
	 int	args[6] = {0,0,0,0};
	
	switch(Buffer[0])
	{
	//Large Code
	case '[':
		// Get Arguments
		c = Buffer[j++];
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
		if(	('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
		{
			switch(c)
			{
			// Left
			case 'D':
				if(argc == 1)	tmp = args[0];
				else	tmp = 1;
				
				if( Term->WritePos-(tmp-1) % Term->TextWidth == 0 )
					Term->WritePos -= Term->WritePos % Term->TextWidth;
				else
					Term->WritePos -= tmp;
				break;
			
			// Right
			case 'C':
				if(argc == 1)	tmp = args[0];
				else	tmp = 1;
				if( (Term->WritePos + tmp) % Term->TextWidth == 0 ) {
					Term->WritePos -= Term->WritePos % Term->TextWidth;
					Term->WritePos += Term->TextWidth - 1;
				} else
					Term->WritePos += tmp;
				break;
			
			// Clear By Line
			case 'J':
				// Clear Screen
				switch(args[0])
				{
				case 2:
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
			// Set cursor position
			case 'h':
				Term->WritePos = args[0] + args[1]*Term->TextWidth;
				Log_Debug("VTerm", "args = {%i, %i}", args[0], args[1]);
				break;
			// Set Font flags
			case 'm':
				for( ; argc--; )
				{
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
						Term->CurColour &= 0xF000FFFF;
						Term->CurColour |= (Uint32)caVT100Colours[ args[argc]-30+(Term->CurColour>>28) ] << 16;
					}
					// Background Colour
					else if(40 <= args[argc] && args[argc] <= 47) {
						Term->CurColour &= 0xFFFF8000;
						Term->CurColour |= caVT100Colours[ args[argc]-40+((Term->CurColour>>12)&15) ];
					}
				}
				break;
			default:
				Log_Warning("VTerm", "Unknown control sequence");
				break;
			}
		}
		break;
		
	default:	break;
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
	
	// Update cursor
	if( Term == gpVT_CurTerm && !(Term->Flags & VT_FLAG_HIDECSR) )
	{
		tVideo_IOCtl_Pos	pos;
		pos.x = (Term->WritePos - Term->ViewPos) % Term->TextWidth;
		pos.y = (Term->WritePos - Term->ViewPos) / Term->TextWidth;
		VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSOR, &pos);
	}
}

/**
 * \fn void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
 * \brief Write a single character to a VTerm
 */
void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
{
	 int	i;
		
	switch(Ch)
	{
	case '\0':	return;	// Ignore NULL byte
	case '\n':
		VT_int_UpdateScreen( Term, 0 );	// Update the line before newlining
		Term->WritePos += Term->TextWidth;
	case '\r':
		Term->WritePos -= Term->WritePos % Term->TextWidth;
		break;
	
	case '\t':
		do {
			Term->Text[ Term->WritePos ].Ch = '\0';
			Term->Text[ Term->WritePos ].Colour = Term->CurColour;
			Term->WritePos ++;
		} while(Term->WritePos & 7);
		break;
	
	case '\b':
		// Backspace is invalid at Offset 0
		if(Term->WritePos == 0)	break;
		
		Term->WritePos --;
		// Singe Character
		if(Term->Text[ Term->WritePos ].Ch != '\0') {
			Term->Text[ Term->WritePos ].Ch = 0;
			Term->Text[ Term->WritePos ].Colour = Term->CurColour;
			break;
		}
		// Tab
		i = 7;	// Limit it to 8
		do {
			Term->Text[ Term->WritePos ].Ch = 0;
			Term->Text[ Term->WritePos ].Colour = Term->CurColour;
			Term->WritePos --;
		} while(Term->WritePos && i-- && Term->Text[ Term->WritePos ].Ch == '\0');
		if(Term->Text[ Term->WritePos ].Ch != '\0')
			Term->WritePos ++;
		break;
	
	default:
		Term->Text[ Term->WritePos ].Ch = Ch;
		Term->Text[ Term->WritePos ].Colour = Term->CurColour;
		Term->WritePos ++;
		break;
	}
	
	// Move Screen
	// - Check if we need to scroll the entire scrollback buffer
	if(Term->WritePos >= Term->TextWidth*Term->TextHeight*(giVT_Scrollback+1))
	{
		 int	base, i;
		
		//Debug("Scrolling entire buffer");
		
		// Move back by one
		Term->WritePos -= Term->TextWidth;
		// Update the scren
		VT_int_UpdateScreen( Term, 0 );
		
		// Update view position
		base = Term->TextWidth*Term->TextHeight*(giVT_Scrollback);
		if(Term->ViewPos < base)
			Term->ViewPos += Term->Width;
		if(Term->ViewPos > base)
			Term->ViewPos = base;
		
		// Scroll terminal cache
		base = Term->TextWidth*(Term->TextHeight*(giVT_Scrollback+1)-1);
		memcpy(
			Term->Text,
			&Term->Text[Term->TextWidth],
			base*sizeof(tVT_Char)
			);
		
		// Clear last row
		for( i = 0; i < Term->TextWidth; i ++ )
		{
			Term->Text[ base + i ].Ch = 0;
			Term->Text[ base + i ].Colour = Term->CurColour;
		}
		
		VT_int_ScrollFramebuffer( Term );
		VT_int_UpdateScreen( Term, 0 );
	}
	// Ok, so we only need to scroll the screen
	else if(Term->WritePos >= Term->ViewPos + Term->TextWidth*Term->TextHeight)
	{
		//Debug("Term->WritePos (%i) >= %i",
		//	Term->WritePos,
		//	Term->ViewPos + Term->Width*Term->Height
		//	);
		//Debug("Scrolling screen only");
		
		// Update the last line
		Term->WritePos -= Term->TextWidth;
		VT_int_UpdateScreen( Term, 0 );
		Term->WritePos += Term->TextWidth;
		
		// Scroll
		Term->ViewPos += Term->TextWidth;
		//Debug("Term->ViewPos = %i", Term->ViewPos);
		VT_int_ScrollFramebuffer( Term );
		VT_int_UpdateScreen( Term, 0 );
	}
	
	//LEAVE('-');
}

/**
 * \fn void VT_int_ScrollFramebuffer( tVTerm *Term )
 * \note Scrolls the framebuffer by 1 text line
 */
void VT_int_ScrollFramebuffer( tVTerm *Term )
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
	
	// Switch to 2D Command Stream
	tmp = VIDEO_BUFFMT_2DSTREAM;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
	
	// BLIT from 0,0 to 0,giVT_CharHeight
	buf.Op = VIDEO_2DOP_BLIT;
	buf.DstX = 0;	buf.DstY = 0;
	buf.SrcX = 0;	buf.SrcY = giVT_CharHeight;
	buf.W = Term->TextWidth * giVT_CharWidth;
	buf.H = (Term->TextHeight-1) * giVT_CharHeight;
	VFS_WriteAt(giVT_OutputDevHandle, 0, sizeof(buf), &buf);
	
	// Restore old mode (this function is only called during text mode)
	tmp = VIDEO_BUFFMT_TEXT;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
}

/**
 * \fn void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
 * \brief Updates the video framebuffer
 */
void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
{
	// Only update if this is the current terminal
	if( Term != gpVT_CurTerm )	return;
	
	switch( Term->Mode )
	{
	case TERM_MODE_TEXT:
		// Re copy the entire screen?
		if(UpdateAll) {
			VFS_WriteAt(
				giVT_OutputDevHandle,
				0,
				Term->TextWidth*Term->TextHeight*sizeof(tVT_Char),
				&Term->Text[Term->ViewPos]
				);
		}
		// Only copy the current line
		else {
			 int	pos = Term->WritePos - Term->WritePos % Term->TextWidth;
			VFS_WriteAt(
				giVT_OutputDevHandle,
				(pos - Term->ViewPos)*sizeof(tVT_Char),
				Term->TextWidth*sizeof(tVT_Char),
				&Term->Text[pos]
				);
		}
		break;
	case TERM_MODE_FB:
		VFS_WriteAt(
			giVT_OutputDevHandle,
			0,
			Term->Width*Term->Height*sizeof(Uint32),
			Term->Buffer
			);
		break;
	}
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
	 int	oldTW = oldW / giVT_CharWidth;
	 int	oldH = Term->Height;
	 int	oldTH = oldH / giVT_CharWidth;
	tVT_Char	*oldTBuf = Term->Text;
	Uint32	*oldFB = Term->Buffer;
	 int	w, h, i;
	
	// TODO: Increase RealWidth/RealHeight when this happens
	if(NewWidth > giVT_RealWidth)	NewWidth = giVT_RealWidth;
	if(NewHeight > giVT_RealHeight)	NewHeight = giVT_RealHeight;
	
	// Calculate new dimensions
	Term->TextWidth = NewWidth / giVT_CharWidth;
	Term->TextHeight = NewHeight / giVT_CharHeight;
	Term->Width = NewWidth;
	Term->Height = NewHeight;
	Term->Mode = NewMode;
	
	// Allocate new buffers
	// - Text
	Term->Text = calloc(
		Term->TextWidth * Term->TextHeight * (giVT_Scrollback+1),
		sizeof(tVT_Char)
		);
	if(oldTBuf) {
		// Copy old buffer
		w = oldTW;
		if( w > Term->TextWidth )	w = Term->TextWidth;
		h = oldTH;
		if( h > Term->TextHeight )	h = Term->TextHeight;
		h *= giVT_Scrollback + 1;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Text[i*Term->TextWidth],
				&oldTBuf[i*oldTW],
				w*sizeof(tVT_Char)
				);
				
		}
	}
	
	// - Framebuffer
	Term->Buffer = calloc( Term->Width * Term->Height, sizeof(Uint32) );
	if(oldFB) {
		// Copy old buffer
		w = oldW;
		if( w > Term->Width )	w = Term->Width;
		h = oldH;
		if( h > Term->Height )	h = Term->Height;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Buffer[i*Term->Width],
				&oldFB[i*oldW],
				w*sizeof(Uint32)
				);
		}
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
 * \fn void VT_Font_Render(Uint32 Codepoint, void *Buffer, int Pitch, Uint32 BGC, Uint32 FGC)
 * \brief Render a font character
 */
void VT_Font_Render(Uint32 Codepoint, void *Buffer, int Pitch, Uint32 BGC, Uint32 FGC)
{
	Uint8	*font;
	Uint32	*buf = Buffer;
	 int	x, y;
	
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
		buf += Pitch;
		font ++;
	}
}

/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a 
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

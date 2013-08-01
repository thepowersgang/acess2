/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm.c
 * - Virtual Terminal - Initialisation and VFS Interface
 */
#define DEBUG	1
#include "vterm.h"
#include <fs_devfs.h>
#include <modules.h>
#include <api_drv_keyboard.h>
#include <api_drv_video.h>
#include <errno.h>
#include <semaphore.h>

// === CONSTANTS ===
#define VERSION	((0<<8)|(50))

#define	NUM_VTS	8
//#define DEFAULT_OUTPUT	"BochsGA"
#define DEFAULT_OUTPUT	"Vesa"
#define FALLBACK_OUTPUT	"x86_VGAText"
#define DEFAULT_INPUT	"Keyboard"
#define	DEFAULT_WIDTH	640
#define	DEFAULT_HEIGHT	480
#define DEFAULT_SCROLLBACK	4	// 2 Screens of text + current screen
//#define DEFAULT_SCROLLBACK	0

// === TYPES ===

// === IMPORTS ===
extern void	Debug_SetKTerminal(const char *File);

// === PROTOTYPES ===
 int	VT_Install(char **Arguments);
 int	VT_Root_IOCtl(tVFS_Node *Node, int Id, void *Data);
void	VT_int_PutFBData(tVTerm *Term, size_t Offset, size_t Length, const void *Data);
void	VT_PTYOutput(void *Handle, size_t Length, const void *Data);
 int	VT_PTYResize(void *Handle, const struct ptydims *Dims); 
 int	VT_PTYModeset(void *Handle, const struct ptymode *Mode);

// === CONSTANTS ===

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VTerm, VT_Install, NULL, "PTY", NULL);
tVFS_NodeType	gVT_RootNodeType = {
	.TypeName = "VTerm Root",
	.IOCtl = VT_Root_IOCtl
	};
tDevFS_Driver	gVT_DrvInfo = {
	NULL, "VTerm",
	{
	.Flags = 0,
	.Size = NUM_VTS,
	.Inode = -1,
	.NumACLs = 0,
	.Type = &gVT_RootNodeType
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
					gsVT_OutputDevice = val;
			}
			else if( strcmp(opt, "Input") == 0 ) {
				if( !gsVT_InputDevice )
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
			else {
				Log_Notice("VTerm", "Unknown option '%s'", opt);
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
	if( Module_EnsureLoaded( gsVT_InputDevice ) ) {
		Log_Error("VTerm", "Fallback input '%s' is not avaliable, input will not be avaliable", DEFAULT_INPUT);
	}
	
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
	Log_Debug("VTerm", "Initialising nodes (and creating buffers)");
	for( i = 0; i < NUM_VTS; i++ )
	{
		gVT_Terminals[i].Mode = TERM_MODE_TEXT;
		gVT_Terminals[i].Flags = 0;
//		gVT_Terminals[i].Flags = VT_FLAG_HIDECSR;	//HACK - Stop all those memcpy calls
		gVT_Terminals[i].CurColour = DEFAULT_COLOUR;
		gVT_Terminals[i].WritePos = 0;
		gVT_Terminals[i].AltWritePos = 0;
		gVT_Terminals[i].ViewPos = 0;
		gVT_Terminals[i].ScrollHeight = 0;
		
		// Initialise
		VT_int_Resize( &gVT_Terminals[i], giVT_RealWidth, giVT_RealHeight );
		gVT_Terminals[i].Mode = PTYBUFFMT_TEXT;
		char	name[] = {'v','t','0'+i,'\0'};
		gVT_Terminals[i].PTY = PTY_Create(name, &gVT_Terminals[i],
			VT_PTYOutput, VT_PTYResize, VT_PTYModeset);
		struct ptymode mode = {
			.OutputMode = PTYBUFFMT_TEXT,
			.InputMode = PTYIMODE_CANON|PTYIMODE_ECHO
		};
		struct ptydims dims = {
			.W = giVT_RealWidth / giVT_CharWidth,
			.H = giVT_RealHeight / giVT_CharHeight,
			.PW = giVT_RealWidth,
			.PH = giVT_RealHeight
		};
		PTY_SetAttrib(gVT_Terminals[i].PTY, &dims, &mode, 0);
	}
	
	// Add to DevFS
	DevFS_AddDevice( &gVT_DrvInfo );
	
	// Set kernel output to VT0
	Log_Debug("VTerm", "Setting kernel output to VT#0");
	Debug_SetKTerminal("/Devices/pts/vt0");
	
	return MODULE_ERR_OK;
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
			"Selected resolution (%ix%i) is not supported by the device, using (%ix%i)",
			giVT_RealWidth, giVT_RealHeight,
			mode.width, mode.height
			);
		giVT_RealWidth = mode.width;
		giVT_RealHeight = mode.height;
	}
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_GETSETMODE, &tmp );
	
	// Resize text terminals if needed
	// - VT0 check is for the first resolution set
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

void VT_int_PutFBData(tVTerm *Term, size_t Offset, size_t Length, const void *Buffer)
{
	size_t	maxlen = Term->Width * Term->Height * 4;

	ENTER("pTerm xOffset xLength pBuffer", Term, Offset, Length, Buffer);

	if( Offset >= maxlen ) {
		LEAVE('-');
		return ;
	}

	LOG("maxlen = 0x%x", maxlen);
	Length = MIN(Length, maxlen - Offset);
	
	// If the terminal is currently shown, write directly to the screen
	if( Term == gpVT_CurTerm )
	{
		// Center the terminal vertically
		if( giVT_RealHeight > Term->Height ) {
			Offset += (giVT_RealHeight - Term->Height) / 2 * Term->Width * 4;
			LOG("Altered offset 0x%x", Offset);
		}
		
		// If the terminal is not native width, center it horizontally
		if( giVT_RealWidth > Term->Width )
		{
			// No? :( Well, just center it
			 int	x, y, w, h;
			Uint	dst_ofs;
			// TODO: Fix to handle the final line correctly?
			x = Offset/4;	y = x / Term->Width;	x %= Term->Width;
			w = Length/4+x;	h = w / Term->Width;	w %= Term->Width;

			LOG("(%i,%i) %ix%i", x, y, w, h);		
	
			// Center
			x += (giVT_RealWidth - Term->Width) / 2;
			dst_ofs = (x + y * giVT_RealWidth) * 4;
			while(h--)
			{
				VFS_WriteAt( giVT_OutputDevHandle,
					dst_ofs,
					Term->Width * 4,
					Buffer
					);
				Buffer = (const Uint32*)Buffer + Term->Width;
				dst_ofs += giVT_RealWidth * 4;
			}
		}
		// otherwise, just go directly to the screen
		else
		{
			VFS_WriteAt( giVT_OutputDevHandle, Offset, Length, Buffer );
		}
	}
	// If not active, write to the backbuffer (allocating if needed)
	else
	{
		if( !Term->Buffer )
			Term->Buffer = malloc( Term->Width * Term->Height * 4 );
		LOG("Direct to cache");
		// Copy to the local cache
		memcpy( (char*)Term->Buffer + Offset, Buffer, Length );
	}
	LEAVE('-');
}

void VT_PTYOutput(void *Handle, size_t Length, const void *Data)
{
	tVTerm	*term = Handle;
	switch( term->Mode )
	{
	case PTYBUFFMT_TEXT:
		VT_int_PutString(term, Data, Length);
		break;
	case PTYBUFFMT_FB:
		// TODO: How do offset?
		VT_int_PutFBData(term, 0, Length, Data);
		break;
	case PTYBUFFMT_2DCMD:
		// TODO: Impliment 2D commands
		VT_int_Handle2DCmd(term, Length, Data);
		break;
	case PTYBUFFMT_3DCMD:
		// TODO: Impliment 3D commands
		break;
	}
}

int VT_PTYResize(void *Handle, const struct ptydims *Dims)
{
	tVTerm	*term = Handle;
	 int	newW = Dims->W * (term->Mode == PTYBUFFMT_TEXT ? giVT_CharWidth : 1);
	 int	newH = Dims->H * (term->Mode == PTYBUFFMT_TEXT ? giVT_CharHeight : 1);
	if( newW > giVT_RealWidth || newH > giVT_RealHeight )
		return 1;
	VT_int_Resize(term, newW, newH);
	return 0;
}

int VT_PTYModeset(void *Handle, const struct ptymode *Mode)
{
	tVTerm	*term = Handle;
	term->Mode = (Mode->OutputMode & PTYOMODE_BUFFMT);

	memset(&term->Cmd2D, 0, sizeof(term->Cmd2D));

	if( term == gpVT_CurTerm ) {
		switch(term->Mode)
		{
		case PTYBUFFMT_TEXT:
			VT_SetMode(VIDEO_BUFFMT_TEXT);
			break;
		default:
			VT_SetMode(VIDEO_BUFFMT_FRAMEBUFFER);
			break;
		}
	}	

	return 0;
}

#if 0
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

void VT_Terminal_Reference(tVFS_Node *Node)
{
	// Append PID to list
}

void VT_Terminal_Close(tVFS_Node *Node)
{
	// Remove PID from list
}
#endif

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
			Uint	ofs = 0;
			Uint32	*dest = gpVT_CurTerm->Buffer;
			// Slower scanline copy
			for( int line = 0; line < gpVT_CurTerm->Height; line ++ )
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
		LOG("Cached screen contents");
	}

	// Update current terminal ID
	Log_Log("VTerm", "Changed terminal from %i to %i", giVT_CurrentTerminal, ID);
	giVT_CurrentTerminal = ID;
	gpVT_CurTerm = &gVT_Terminals[ID];
	
	LOG("Attempting VT_SetMode");
	
	if( gpVT_CurTerm->Mode == PTYBUFFMT_TEXT )
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

	LOG("Mode set");	

	if(gpVT_CurTerm->Buffer)
	{
		// TODO: Handle non equal sized
		VFS_WriteAt(
			giVT_OutputDevHandle,
			0,
			gpVT_CurTerm->Width*gpVT_CurTerm->Height*sizeof(Uint32),
			gpVT_CurTerm->Buffer
			);
		LOG("Updated screen contents");
	}
	
	VT_int_UpdateCursor(gpVT_CurTerm, 1);
	// Update the screen
	VT_int_UpdateScreen(gpVT_CurTerm, 1);
	LOG("done");
}

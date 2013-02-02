/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm.h
 * - Virtual Terminal - Common
 */
#ifndef _VTERM_H_
#define _VTERM_H_

#include <acess.h>
#include <api_drv_video.h>	// tVT_Char
#include <api_drv_terminal.h>
#include <vfs.h>

// === CONSTANTS ===
#define MAX_INPUT_CHARS32	64
#define MAX_INPUT_CHARS8	(MAX_INPUT_CHARS32*4)
#define	DEFAULT_COLOUR	(VT_COL_BLACK|(0xAAA<<16))

/**
 * \{
 */
#define	VT_FLAG_HIDECSR	0x01	//!< Hide the cursor
#define	VT_FLAG_ALTBUF	0x02	//!< Alternate screen buffer
#define VT_FLAG_RAWIN	0x04	//!< Don't handle ^Z/^C/^V
#define	VT_FLAG_HASFB	0x10	//!< Set if the VTerm has requested the Framebuffer
#define VT_FLAG_SHOWCSR	0x20	//!< Always show the text cursor
/**
 * \}
 */

enum eVT_InModes {
	VT_INMODE_TEXT8,	// UTF-8 Text Mode (VT100/xterm Emulation)
	VT_INMODE_TEXT32,	// UTF-32 Text Mode (Acess Native)
	NUM_VT_INMODES
};


// === TYPES ==
typedef struct sVTerm	tVTerm;

// === STRUCTURES ===
struct sVTerm
{
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

	char	EscapeCodeCache[16];
	size_t	EscapeCodeLen;

	 int	VideoCursorX;
	 int	VideoCursorY;
	
	tMutex	ReadingLock;	//!< Lock the VTerm when a process is reading from it
	tTID	ReadingThread;	//!< Owner of the lock
	 int	InputRead;	//!< Input buffer read position
	 int	InputWrite;	//!< Input buffer write position
	char	InputBuffer[MAX_INPUT_CHARS8];
	Uint32	RawScancode;	//!< last raw scancode recieved
//	tSemaphore	InputSemaphore;
	
	tPGID	OwningProcessGroup;	//!< The process group that owns the terminal

	Uint32		*Buffer;

	// TODO: Do I need to keep this about?
	// When should it be deallocated? on move to text mode, or some other time
	// Call set again, it's freed, and if NULL it doesn't get reallocated.
	tVideo_IOCtl_Bitmap	*VideoCursor;
	
	char	Name[2];	//!< Name of the terminal
	tVFS_Node	Node;
};

// === GOBALS ===
extern tVTerm	*gpVT_CurTerm;
extern int	giVT_Scrollback;
extern short	giVT_RealWidth;	//!< Screen Width
extern short	giVT_RealHeight;	//!< Screen Width
extern char	*gsVT_OutputDevice;
extern char	*gsVT_InputDevice;
extern int	giVT_OutputDevHandle;
extern int	giVT_InputDevHandle;

// === FUNCTIONS ===
extern void	VT_SetResolution(int Width, int Height);
extern void	VT_SetTerminal(int ID);
// --- Output ---
extern void	VT_InitOutput(void);
extern void	VT_SetMode(int Mode);
extern void	VT_int_ScrollFramebuffer( tVTerm *Term, int Count );
extern void	VT_int_UpdateCursor( tVTerm *Term, int bShow );
extern void	VT_int_UpdateScreen( tVTerm *Term, int UpdateAll );
// --- Input ---
extern void	VT_InitInput(void);
extern void	VT_KBCallBack(Uint32 Codepoint);
// --- VT100 Emulation ---
extern void	VT_int_ParseEscape_StandardLarge(tVTerm *Term, char CmdChar, int argc, int *args);
extern int	VT_int_ParseEscape(tVTerm *Term, const char *Buffer, size_t Bytes);
// --- Terminal Buffer ---
extern void	VT_int_PutString(tVTerm *Term, const Uint8 *Buffer, Uint Count);
extern void	VT_int_PutChar(tVTerm *Term, Uint32 Ch);
extern void	VT_int_ScrollText(tVTerm *Term, int Count);
extern void	VT_int_ClearLine(tVTerm *Term, int Num);
extern void	VT_int_ChangeMode(tVTerm *Term, int NewMode, int NewWidth, int NewHeight);
extern void	VT_int_ToggleAltBuffer(tVTerm *Term, int Enabled);

#endif


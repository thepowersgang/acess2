/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_input.c
 * - Virtual Terminal - Input code
 */
#include "vterm.h"
#include <api_drv_keyboard.h>
#define DEBUG	1

// === GLOBALS ===
// --- Key States --- (Used for VT Switching/Magic Combos)
 int	gbVT_CtrlDown = 0;
 int	gbVT_AltDown = 0;
 int	gbVT_SysrqDown = 0;

// === CODE ===
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
	LOG("VTerm input initialised");
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

	// Catch VT binds
	switch( Codepoint & KEY_ACTION_MASK )
	{
	case KEY_ACTION_RAWSYM:
		term->RawScancode = Codepoint & KEY_CODEPOINT_MASK;
		break;
	case KEY_ACTION_RELEASE:
		switch(term->RawScancode)
		{
		case KEYSYM_LEFTALT:	gbVT_AltDown &= ~1;	break;
		case KEYSYM_RIGHTALT:	gbVT_AltDown &= ~2;	break;
		case KEYSYM_LEFTCTRL:	gbVT_CtrlDown &= ~1;	break;
		case KEYSYM_RIGHTCTRL:	gbVT_CtrlDown &= ~2;	break;
		}
		break;
	
	case KEY_ACTION_PRESS:
		switch(term->RawScancode)
		{
		case KEYSYM_LEFTALT:	gbVT_AltDown |= 1;	break;
		case KEYSYM_RIGHTALT:	gbVT_AltDown |= 2;	break;
		case KEYSYM_LEFTCTRL:	gbVT_CtrlDown |= 1;	break;
		case KEYSYM_RIGHTCTRL:	gbVT_CtrlDown |= 2;	break;
		}
		
		// Check if the magic keys are held
		if(!gbVT_AltDown || !gbVT_CtrlDown)
			break;
		
		switch(term->RawScancode)
		{
		case KEYSYM_F1 :	VT_SetTerminal(0);	return;
		case KEYSYM_F2 :	VT_SetTerminal(1);	return;
		case KEYSYM_F3 :	VT_SetTerminal(2);	return;
		case KEYSYM_F4 :	VT_SetTerminal(3);	return;
		case KEYSYM_F5 :	VT_SetTerminal(4);	return;
		case KEYSYM_F6 :	VT_SetTerminal(5);	return;
		case KEYSYM_F7 :	VT_SetTerminal(6);	return;
		case KEYSYM_F8 :	VT_SetTerminal(7);	return;
		case KEYSYM_F9 :	VT_SetTerminal(8);	return;
		case KEYSYM_F10:	VT_SetTerminal(9);	return;
		case KEYSYM_F11:	VT_SetTerminal(10);	return;
		case KEYSYM_F12:	VT_SetTerminal(11);	return;
		}
		
		// Scrolling is only valid in text mode
		if(term->Mode != TERM_MODE_TEXT)
			break;
		
//		Log_Debug("VTerm", "Magic Ctrl-Alt-0x%x", term->RawScancode);	

		switch(term->RawScancode)
		{
		// Scrolling
		case KEYSYM_PGUP:
			if( term->Flags & VT_FLAG_ALTBUF )
				return ;
			term->ViewPos = MAX( 0, term->ViewPos - term->Width );
			VT_int_UpdateScreen(term, 1);
			return;
		case KEYSYM_PGDN:
			if( term->Flags & VT_FLAG_ALTBUF )
				return ;
			term->ViewPos = MIN(
				term->ViewPos + term->Width,
				term->Width * term->Height * giVT_Scrollback
				);
			VT_int_UpdateScreen(term, 1);
			return;
		}
		break;
	}
	
	// Encode key
	if(term->Mode == PTYBUFFMT_TEXT)
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

		// Get UTF-8/ANSI Encoding
		if( Codepoint == 0 )
		{
			// Non-printable keys
			switch(term->RawScancode)
			{
			case KEYSYM_LEFTARROW:
			case KEYSYM_KP4:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'D';
				len = 3;
				break;
			case KEYSYM_RIGHTARROW:
			case KEYSYM_KP6:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'C';
				len = 3;
				break;
			case KEYSYM_UPARROW:
			case KEYSYM_KP8:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'A';
				len = 3;
				break;
			case KEYSYM_DOWNARROW:
			case KEYSYM_KP2:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = 'B';
				len = 3;
				break;
			
			case KEYSYM_PGUP:
			case KEYSYM_KP9:	// If Codepoint=0, It's page up
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '5'; buf[3] = '~';
				len = 4;
				break;
			case KEYSYM_PGDN:
			case KEYSYM_KP3:	// If Codepoint=0, It's page down
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '6'; buf[3] = '~';
				len = 4;
				break;
			
			case KEYSYM_HOME:
			case KEYSYM_KP7:	// Codepoint==0, then it's home (not translated)
				buf[0] = '\x1B'; buf[1] = 'O'; buf[2] = 'H';
				len = 3;
				break;
			case KEYSYM_END:
			case KEYSYM_KP1:	// You get the drill
				buf[0] = '\x1B'; buf[1] = 'O'; buf[2] = 'F';
				len = 3;
				break;
			
			case KEYSYM_INSERT:
			case KEYSYM_KP0:	// See above
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '2'; buf[3] = '~';
				len = 4;
				break;
			case KEYSYM_DELETE:
			case KEYSYM_KPPERIOD:	// Are you that dumb? Look up
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '3'; buf[3] = '~';
				len = 4;
				break;
			}
		}
		else if( gbVT_CtrlDown )
		{
			len = 1;
			switch( term->RawScancode )
			{
			case KEYSYM_2:
				buf[0] = '\0';
				break;
			case KEYSYM_3 ... KEYSYM_7:
				buf[0] = 0x1b + (term->RawScancode - KEYSYM_3);
				break;
			case KEYSYM_8:
				buf[0] = 0x7f;
				break;
			// - Ctrl-A = \1, Ctrl-Z = \x1a
			case KEYSYM_a ... KEYSYM_z:
				buf[0] = 0x01 + (term->RawScancode - KEYSYM_a);
				break;
			default:
				goto utf_encode;
			}
		}
		else
		{
		utf_encode:
			// Attempt to encode in UTF-8
			len = WriteUTF8( buf, Codepoint );
			if(len == 0) {
				Warning("Codepoint (%x) is unrepresentable in UTF-8", Codepoint);
			}
		}
		
		if(len == 0) {
			// Unprintable / Don't Pass
			return;
		}

		PTY_SendInput(term->PTY, (void*)buf, len);
	}
	else
	{
		PTY_SendInput(term->PTY, (void*)&Codepoint, sizeof(Codepoint));
	}
}


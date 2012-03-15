/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_input.c
 * - Virtual Terminal - Input code
 */
#include "vterm.h"
#include <api_drv_keyboard.h>

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
			case KEYSYM_KP7:
				buf[0] = '\x1B'; buf[1] = 'O'; buf[2] = 'H';
				len = 3;
				break;
			case KEYSYM_END:
			case KEYSYM_KP1:
				buf[0] = '\x1B'; buf[1] = 'O'; buf[2] = 'F';
				len = 3;
				break;
			
			case KEYSYM_INSERT:
			case KEYSYM_KP0:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '2'; buf[3] = '~';
				len = 4;
				break;
			case KEYSYM_DELETE:
			case KEYSYM_KPPERIOD:
				buf[0] = '\x1B'; buf[1] = '['; buf[2] = '3'; buf[3] = '~';
				len = 4;
				break;
			}
		}
		else
		{
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

		// TODO: Implement Ctrl-C etc
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
	
		#if 0
		// Drop new keys
		if( term->InputWrite == term->InputRead )
			return ;		
		#endif

		raw_in[ term->InputWrite ] = Codepoint;
		term->InputWrite ++;
		if(term->InputWrite >= MAX_INPUT_CHARS32)
			term->InputWrite -= MAX_INPUT_CHARS32;
		
		#if 1
		// TODO: Should old or new be dropped?
		if(term->InputRead == term->InputWrite) {
			term->InputRead ++;
			if( term->InputRead >= MAX_INPUT_CHARS32 )
				term->InputRead -= MAX_INPUT_CHARS32;
		}
		#endif
	}
	
	VFS_MarkAvaliable(&term->Node, 1);
}


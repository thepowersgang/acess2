/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Core
 */
#include <axwin3/axwin.h>
#include <axwin3/menu.h>
#include <axwin3/richtext.h>
#include <axwin3/keysyms.h>
#include <stdio.h>
#include <acess/sys.h>
#include "include/display.h"
#include "include/vt100.h"
#include <string.h>
#include <unicode.h>

// === PROTOTYPES ===
 int	main(int argc, char *argv[], const char **envp);
 int	Term_KeyHandler(tHWND Window, int bPress, uint32_t KeySym, uint32_t Translated);
 int	Term_MouseHandler(tHWND Window, int bPress, int Button, int Row, int Col);
void	Term_HandleOutput(int Len, const char *Buf);

// === GLOBALS ===
tHWND	gMainWindow;
tHWND	gMenuWindow;
 int	giChildStdin;
 int	giChildStdout;

// === CODE ===
int main(int argc, char *argv[], const char **envp)
{
	AxWin3_Connect(NULL);
	
	// --- Build up window
	gMainWindow = AxWin3_RichText_CreateWindow(NULL, 0);
	AxWin3_SetWindowTitle(gMainWindow, "Terminal");	// TODO: Update title with other info

	gMenuWindow = AxWin3_Menu_Create(gMainWindow);
	AxWin3_Menu_AddItem(gMenuWindow, "Copy\tWin+C", NULL, NULL, 0, NULL);
	AxWin3_Menu_AddItem(gMenuWindow, "Paste\tWin+V", NULL, NULL, 0, NULL);
	// TODO: Populate menu	


	// TODO: Tabs?
	
	AxWin3_RichText_SetKeyHandler	(gMainWindow, Term_KeyHandler);
	AxWin3_RichText_SetMouseHandler	(gMainWindow, Term_MouseHandler);
	AxWin3_RichText_SetDefaultColour(gMainWindow, 0xFFFFFF);
	AxWin3_RichText_SetBackground   (gMainWindow, 0x000000);
	AxWin3_RichText_SetFont		(gMainWindow, "#monospace", 10);
	AxWin3_RichText_SetCursorPos	(gMainWindow, 0, 0);
	AxWin3_RichText_SetCursorType	(gMainWindow, AXWIN3_RICHTEXT_CURSOR_INV);
	AxWin3_RichText_SetCursorBlink	(gMainWindow, 1);

	// <testing>
	AxWin3_RichText_SetLineCount(gMainWindow, 3);
	AxWin3_RichText_SendLine(gMainWindow, 0, "First line!");
	AxWin3_RichText_SendLine(gMainWindow, 2, "Third line! \1ff0000A red");
	// </testing>

	Display_Init(80, 25, 100);
	AxWin3_ResizeWindow(gMainWindow, 80*8, 25*16);
	AxWin3_MoveWindow(gMainWindow, 20, 50);
	AxWin3_ShowWindow(gMainWindow, 1);
	AxWin3_FocusWindow(gMainWindow);

	// Spawn shell
	giChildStdin = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	giChildStdout = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	if( giChildStdout == -1 || giChildStdin == -1 ) {
		perror("Oh, fsck");
		_SysDebug("out,in = %i,%i", giChildStdout, giChildStdin);
		return -1;
	}

	{
		 int	fds[] = {giChildStdin, giChildStdout, giChildStdout};
		const char	*argv[] = {"CLIShell", NULL};
		int pid = _SysSpawn("/Acess/Bin/CLIShell", argv, envp, 3, fds, NULL);
		if( pid < 0 )
			_SysDebug("ERROR: Shell spawn failed");
	}

	// Main loop
	for( ;; )
	{
		fd_set	fds;
		
		FD_ZERO(&fds);
		FD_SET(giChildStdout, &fds);
		AxWin3_MessageSelect(giChildStdout + 1, &fds);
		
		if( FD_ISSET(giChildStdout, &fds) )
		{
			_SysDebug("Activity on child stdout");
			// Read and update screen
			char	buf[32];
			int len = _SysRead(giChildStdout, buf, sizeof(buf));
			if( len <= 0 )	break;
			
			Term_HandleOutput(len, buf);
		}
	}

	return 0;
}

int Term_KeyHandler(tHWND Window, int bPress, uint32_t KeySym, uint32_t Translated)
{
	static int	ctrl_state = 0;

	// Handle modifiers
	#define _bitset(var,bit,set) do{if(set)var|=1<<(bit);else var&=1<<(bit);}while(0)
	switch(KeySym)
	{
	case KEYSYM_LEFTCTRL:
		_bitset(ctrl_state, 0, bPress);
		return 0;
	case KEYSYM_RIGHTCTRL:
		_bitset(ctrl_state, 0, bPress);
		return 0;
	}
	#undef _bitset

	// Handle shortcuts
	// - Ctrl-A -- Ctrl-Z
	if( ctrl_state && KeySym >= KEYSYM_a && KeySym <= KEYSYM_z )
	{
		Translated = KeySym - KEYSYM_a + 1;
	}

	if( Translated )
	{
		char	buf[6];
		 int	len;
		
		// Encode and send
		len = WriteUTF8(buf, Translated);
		
		_SysWrite(giChildStdin, buf, len);
		
		return 0;
	}
	
	// No translation, look for escape sequences to send
	const char *str = NULL;
	switch(KeySym)
	{
	case KEYSYM_LEFTARROW:
		str = "\x1b[D";
		break;
	}
	if( str )
	{
		_SysWrite(giChildStdin, str, strlen(str));
	}
	return 0;
}

int Term_MouseHandler(tHWND Window, int bPress, int Button, int Row, int Col)
{
	return 0;
}

void Term_HandleOutput(int Len, const char *Buf)
{
	// TODO: Handle graphical / accelerated modes

	 int	ofs = 0;
	 int	esc_len = 0;

	while( ofs < Len )
	{
		esc_len = Term_HandleVT100(Len - ofs, Buf + ofs);
		if( esc_len < 0 ) {
			Display_AddText(-esc_len, Buf + ofs);
			esc_len = -esc_len;
		}
		ofs += esc_len;
		_SysDebug("Len = %i, ofs = %i", Len, ofs);
	}
	
	Display_Flush();
}


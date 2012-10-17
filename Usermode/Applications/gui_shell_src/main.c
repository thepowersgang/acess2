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
#include <stdio.h>
#include "include/display.h"

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	Term_KeyHandler(tHWND Window, int bPress, uint32_t KeySym, uint32_t Translated);
 int	Term_MouseHandler(tHWND Window, int bPress, uint32_t KeySym, uint32_t Translated);

// === GLOBALS ===
tHWND	gMainWindow;
tHWND	gMenuWindow;
 int	giChildStdin;
 int	giChildStdout;

// === CODE ===
int main(int argc, char *argv[])
{
	AxWin3_Connect(NULL);
	
	// --- Build up window
	gMainWindow = AxWin3_RichText_CreateWindow(NULL, 0);
	AxWin3_SetWindowTitle(gMainWindow, "Terminal");	// TODO: Update title with other info

	gMenuWindow = AxWin3_Menu_Create(gMainWindow);
	AxWin3_Menu_AddItem(gMenuWindow, "Copy\tWin+C", NULL, NULL, NULL, 0);
	AxWin3_Menu_AddItem(gMenuWindow, "Paste\tWin+V", NULL, NULL, NULL, 0);
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
	AxWin3_RichText_SendLine(gMainWindow, 2, "Third line! \x01""ff0000A red");
	// </testing>

	AxWin3_ResizeWindow(gMainWindow, 600, 400);
	AxWin3_MoveWindow(gMainWindow, 50, 50);
	AxWin3_ShowWindow(gMainWindow, 1);
	AxWin3_FocusWindow(gMainWindow);

	// Spawn shell
	giChildStdout = open("/Devices/FIFO/anon", O_RDWR);
	giChildStdin = open("/Devices/FIFO/anon", O_RDWR);

	{
		 int	fds[] = {giChildStdin, giChildStdout, giChildStdout};
		const char	*argv[] = {"CLIShell", NULL};
		_SysSpawn("/Acess/Bin/CLIShell", argv, envp, 3, fds, NULL);
	}

	// Main loop
	for( ;; )
	{
		fd_set	fds;
		
		FD_ZERO(&fds);
		FD_SET(&fds, giChildStdout);
		AxWin3_MessageSelect(giChildStdout + 1, &fds);
		
		if( FD_ISSET(&fds, giChildStdout) )
		{
			// Read and update screen
			char	buf[32];
			len = read(giChildStdout, sizeof(buf), buf);
			if( len <= 0 )	break;
			
			//Term_HandleOutput(len, buf);
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
	case KEY_LCTRL:
		_bitset(ctrl_state, 0, bPress);
		return 0;
	case KEY_RCTRL:
		_bitset(ctrl_state, 0, bPress);
		return 0;
	}
	#undef _bitset

	// Handle shortcuts
	// - Ctrl-A -- Ctrl-Z
	if( ctrl_state && KeySym >= KEY_a && KeySym <= KEY_z )
	{
		Translated = KeySym - KEY_a + 1;
	}

	if( Translated )
	{
		// Encode and send
		
		return 0;
	}
	
	// No translation, look for escape sequences to send
	switch(KeySym)
	{
	case KEY_LEFTARROW:
		str = "\x1b[D";
		break;
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
		Len -= esc_len;
		ofs += esc_len;
	}
}


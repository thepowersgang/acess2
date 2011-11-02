/*
 * Acess2 GUI v3 User Interface
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Interface core
 */
#include <stdlib.h>
#include <axwin3/axwin.h>

// === GLOBALS ===
tHWND	gSidebar;

// === CODE ===
int main(int argc, char *argv[])
{
	// Connect to AxWin3 Server
	AxWin3_Connect(NULL);
	
	// Create sidebar
	// TODO: Use the widget library instead
	gSidebar = AxWin3_CreateWindow(NULL, "widget", 0, 0, NULL);
	
	// Idle loop
	AxWin3_MainLoop();
	
	return 0;
}


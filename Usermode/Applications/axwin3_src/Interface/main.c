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
int sidebar_callback(tHWND Window, int Length, void *Data)
{
	return 0;
}

int main(int argc, char *argv[])
{
	// Connect to AxWin3 Server
	AxWin3_Connect(NULL);
	
	// Create sidebar
	// TODO: Use the widget library instead
	gSidebar = AxWin3_CreateWindow(NULL, "Widget", 0, 0, NULL, sidebar_callback);

	// TODO: Get screen dimensions somehow

	// Size the window
	AxWin3_SetWindowPos(gSidebar, 0, 0, 32, 600);
	
	// Show!
	AxWin3_ShowWindow(gSidebar, 1);	

	// Idle loop
	AxWin3_MainLoop();
	
	return 0;
}


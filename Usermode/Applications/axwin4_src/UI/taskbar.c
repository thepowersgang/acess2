/*
 * AxWin4 GUI - UI Core
 * - By John Hodge (thePowersGang)
 *
 * taskbar.c
 * - Main toolbar (aka Taskbar) 
 */
#include <axwin4/axwin.h>
#include "include/common.h"
#include "include/taskbar.h"
#include <time.h>

// === CONSTANTS ===
#define TASKBAR_HEIGHT	30
#define TASKBAR_BORDER	3	// Border between window edge and controls
#define TASKBAR_SYSBTN_SIZE	(TASKBAR_HEIGHT-TASKBAR_BORDER*2)
#define TASKBAR_WIN_MAXSIZE	100
#define TASKBAR_WIN_MINSIZE	24
#define TASKBAR_CLOCKSIZE	60

// === TYPES ===
typedef struct sTaskbar_Win	tTaskbar_Win;
struct sTaskbar_Win
{
	tTaskbar_Win	*Next;
	const char	*Title;
};

// === PROTOTYPES ===

// === GLOBALS ===
tAxWin4_Window	*gpTaskbar_Window;
unsigned int giTaskbar_NumWins = 0;
tTaskbar_Win	*gpTaskbar_FirstWin;

// === CODE ===
void Taskbar_Create(void)
{
	gpTaskbar_Window = AxWin4_CreateWindow("taskbar");
	tAxWin4_Window * const win = gpTaskbar_Window;

	AxWin4_MoveWindow(win, 0, 0);
	AxWin4_ResizeWindow(win, giScreenWidth, TASKBAR_HEIGHT);
	
	AxWin4_SetWindowFlags(win, AXWIN4_WNDFLAG_NODECORATE);
	Taskbar_Redraw();
	AxWin4_ShowWindow(win, true);
}

void Taskbar_Redraw(void)
{
	const int	w = giScreenWidth;
	const int	h = TASKBAR_HEIGHT;
	
	const int	active_height = h - TASKBAR_BORDER*2;
	const int	winlist_start_x = TASKBAR_BORDER+TASKBAR_SYSBTN_SIZE+TASKBAR_BORDER;
	const int	clock_start_x = w - (TASKBAR_BORDER+TASKBAR_CLOCKSIZE);
	
	// Window background: Toolbar skin
	AxWin4_DrawControl(gpTaskbar_Window, 0, 0, w, h, AXWIN4_CTL_TOOLBAR, 0);
	
	// System button
	// TODO: Use an image instead
	AxWin4_DrawControl(gpTaskbar_Window, TASKBAR_BORDER, TASKBAR_BORDER, TASKBAR_SYSBTN_SIZE, active_height, AXWIN4_CTL_BUTTON, 0);
	
	// Windows
	// TODO: Maintain/request a list of windows
	if( giTaskbar_NumWins )
	{
		int winbutton_size = (clock_start_x - winlist_start_x) / giTaskbar_NumWins;
		if(winbutton_size > TASKBAR_WIN_MAXSIZE)	winbutton_size = TASKBAR_WIN_MAXSIZE;
		int x = winlist_start_x;
		for(tTaskbar_Win *win = gpTaskbar_FirstWin; win; win = win->Next )
		{
			AxWin4_DrawControl(gpTaskbar_Window, x, TASKBAR_BORDER, winbutton_size, active_height, AXWIN4_CTL_BUTTON, 0);
		}
	}
	
	// Clock
	{
		char timestr[5];
		time_t	rawtime;
		time(&rawtime);
		strftime(timestr, 5, "%H%M", localtime(&rawtime));
		//AxWin4_DrawControl(gpTaskbar_Window, clock_start_x, TASKBAR_BORDER, TASKBAR_CLOCKSIZE, active_height, AXWIN4_CTL_BOX);
		
		AxWin4_DrawText(gpTaskbar_Window, clock_start_x, TASKBAR_BORDER, TASKBAR_CLOCKSIZE, active_height, 0, timestr);
	}
	
	AxWin4_DamageRect(gpTaskbar_Window, 0, 0, w, h);
}


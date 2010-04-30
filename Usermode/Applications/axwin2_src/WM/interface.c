/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"

// === GLOBALS ==
 int	giInterface_Width = 0;
tElement	*gpInterface_Sidebar;
tElement	*gpInterface_MainArea;
tElement	*gpInterface_HeaderBar;
tElement	*gpInterface_TabBar;
tElement	*gpInterface_TabContent;

// === CODE ===
void Interface_Init(void)
{
	tElement	*area;
	tElement	*btn, *text;
	
	giInterface_Width = giScreenWidth/16;
	
	WM_SetFlags(NULL, 0);
	
	// Create Sidebar
	gpInterface_Sidebar = WM_CreateElement(NULL, ELETYPE_TOOLBAR, ELEFLAG_VERTICAL);
	WM_SetSize( gpInterface_Sidebar, giInterface_Width );
	// Create Main Area and regions within
	gpInterface_MainArea = WM_CreateElement(NULL, ELETYPE_BOX, ELEFLAG_VERTICAL);
	gpInterface_HeaderBar = WM_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0);
	gpInterface_TabBar = WM_CreateElement(gpInterface_MainArea, ELETYPE_TABBAR, 0);
	gpInterface_TabContent = WM_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0);
	
	// Main segment of the "taskbar"
	area = WM_CreateElement(gpInterface_Sidebar, ELETYPE_BOX, ELEFLAG_VERTICAL);
	// Menu Button
	btn = WM_CreateElement(area, ELETYPE_BUTTON, ELEFLAG_NOEXPAND);
	WM_SetSize(btn, giInterface_Width);
	//text = WM_CreateElement(btn, ELETYPE_IMAGE, ELEFLAG_SCALE);
	//WM_SetText(text, "asset://LogoSmall.sif");
	text = WM_CreateElement(btn, ELETYPE_TEXT, 0);
	WM_SetText(text, "Acess");
	
	// Plain <hr/> style spacer
	WM_CreateElement(area, ELETYPE_SPACER, 0);
	
	// Windows Go Here
	
	// Bottom Segment
	area = WM_CreateElement(gpInterface_Sidebar, ELETYPE_BOX, ELEFLAG_VERTICAL|ELEFLAG_ALIGN_END);
	
	// Plain <hr/> style spacer
	WM_CreateElement(area, ELETYPE_SPACER, 0);
	
	// Version String
	text = WM_CreateElement(area, ELETYPE_TEXT, ELEFLAG_WRAP);
	WM_SetText(text, "AxWin 1.0");
}

void Interface_Update(void)
{
	giInterface_Width = giScreenWidth/16;
	WM_SetSize( gpInterface_Sidebar, giInterface_Width );
}

void Interface_Render(void)
{
	
	Video_FillRect(
		0, 0,
		giInterface_Width, giScreenHeight,
		0xDDDDDD);
	
	Video_Update();
}

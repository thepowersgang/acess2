/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 * 
 * interface.c
 * > Main Overarching UI
 */
#include "common.h"
#include "resources/LogoSmall.sif.res.h"

// === GLOBALS ==
 int	giInterface_Width = 0;
 int	giInterface_HeaderBarSize = 20;
 int	giInterface_TabBarSize = 20;
tElement	*gpInterface_Sidebar;
tElement	*gpInterface_ProgramList;
tElement	*gpInterface_MainArea;
tElement	*gpInterface_HeaderBar;
tElement	*gpInterface_TabBar;
tElement	*gpInterface_TabContent;
const char	csLogoSmall[] = "base64:///"RESOURCE_LogoSmall_sif;

// === CODE ===
/**
 * \brief Initialise the UI
 */
void Interface_Init(void)
{
	tElement	*btn, *text;
	tElement	*ele;

	// Calculate sizes
	giInterface_Width = giScreenWidth/16;
	
	// Set root window to no-border
	WM_SetFlags(NULL, 0);
	
	// -- Create Sidebar --
	gpInterface_Sidebar = WM_CreateElement(NULL, ELETYPE_TOOLBAR, ELEFLAG_VERTICAL, "Sidebar");
	WM_SetSize( gpInterface_Sidebar, giInterface_Width );
	
	// > System Menu Button
	btn = WM_CreateElement(gpInterface_Sidebar, ELETYPE_BUTTON, ELEFLAG_NOSTRETCH, "SystemMenu");
	WM_SetSize(btn, giInterface_Width-4);
	// TODO: Once image loading is implemented, switch to a logo
	#if 1
	//text = WM_CreateElement(btn, ELETYPE_IMAGE, ELEFLAG_SCALE);
	text = WM_CreateElement(btn, ELETYPE_IMAGE, 0, "MenuLogo");
	//WM_SetText(text, "file:///LogoSmall.sif");
	WM_SetText(text, csLogoSmall);
	#else
	text = WM_CreateElement(btn, ELETYPE_TEXT, 0, NULL);
	WM_SetText(text, "Acess");
	#endif
	// > Plain <hr/> style spacer
	ele = WM_CreateElement(gpInterface_Sidebar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Top");
	WM_SetSize(ele, 4);
	// > Application List
	gpInterface_ProgramList = WM_CreateElement(gpInterface_Sidebar, ELETYPE_BOX, ELEFLAG_VERTICAL, "ProgramList");
	// > Plain <hr/> style spacer
	ele = WM_CreateElement(gpInterface_Sidebar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Bottom");
	WM_SetSize(ele, 4);
	// > Version/Time
	text = WM_CreateElement(gpInterface_Sidebar, ELETYPE_TEXT, ELEFLAG_NOSTRETCH, "Version String");
	WM_SetSize(text, 20);
	WM_SetText(text, "2.0");
	
	// --
	// -- Create Main Area and regions within --
	// --
	// > Righthand Area
	gpInterface_MainArea = WM_CreateElement(NULL, ELETYPE_BOX, ELEFLAG_VERTICAL, "MainArea");
	//  > Header Bar (Title)
	gpInterface_HeaderBar = WM_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0, "HeaderBar");
	WM_SetSize(gpInterface_HeaderBar, giInterface_HeaderBarSize);
	text = WM_CreateElement(gpInterface_HeaderBar, ELETYPE_TEXT, 0, NULL);
	WM_SetText(text, "Acess2 GUI - By thePowersGang (John Hodge)");
	//  > Tab Bar (Current windows)
	gpInterface_TabBar = WM_CreateElement(gpInterface_MainArea, ELETYPE_TABBAR, 0, "TabBar");
	WM_SetSize(gpInterface_TabBar, giInterface_TabBarSize);
	//  > Application Space
	gpInterface_TabContent = WM_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0, "TabContent");
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

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
tApplication	*gpInterface_CurrentApp;

typedef struct sApplicationLink	tApplicationLink;

struct sApplicationLink {
	tApplication	*App;
	tElement	*Button;
	char	Name[];
};

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
	AxWin_SetFlags(NULL, 0);
	
	// -- Create Sidebar (Menu and Window List) --
	gpInterface_Sidebar = AxWin_CreateElement(NULL, ELETYPE_TOOLBAR, ELEFLAG_VERTICAL, "Sidebar");
	AxWin_SetSize( gpInterface_Sidebar, giInterface_Width );
	
	// > System Menu Button
	btn = AxWin_CreateElement(gpInterface_Sidebar, ELETYPE_BUTTON, ELEFLAG_NOSTRETCH, "SystemMenu");
	AxWin_SetSize(btn, giInterface_Width-4);
	//text = AxWin_CreateElement(btn, ELETYPE_IMAGE, ELEFLAG_SCALE, "MenuLogo");
	text = AxWin_CreateElement(btn, ELETYPE_IMAGE, 0, "MenuLogo");
	//AxWin_SetText(text, "file:///LogoSmall.sif");
	AxWin_SetText(text, csLogoSmall);
	
	// > Plain <hr/> style spacer
	ele = AxWin_CreateElement(gpInterface_Sidebar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Top");
	AxWin_SetSize(ele, 4);
	
	// > Application List (Window list on most OSs)
	gpInterface_ProgramList = AxWin_CreateElement(gpInterface_Sidebar, ELETYPE_BOX, ELEFLAG_VERTICAL, "ProgramList");
	
	// > Plain <hr/> style spacer
	ele = AxWin_CreateElement(gpInterface_Sidebar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "SideBar Spacer Bottom");
	AxWin_SetSize(ele, 4);
	
	// > Version/Time
	text = AxWin_CreateElement(gpInterface_Sidebar, ELETYPE_TEXT, ELEFLAG_NOSTRETCH, "Version String");
	AxWin_SetSize(text, 20);
	AxWin_SetText(text, "2.0");
	
	// --
	// -- Create Main Area and regions within --
	// --
	// > Righthand Area
	gpInterface_MainArea = AxWin_CreateElement(NULL, ELETYPE_BOX, ELEFLAG_VERTICAL, "MainArea");
	//  > Header Bar (Title)
	gpInterface_HeaderBar = AxWin_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0, "HeaderBar");
	AxWin_SetSize(gpInterface_HeaderBar, giInterface_HeaderBarSize);
	text = AxWin_CreateElement(gpInterface_HeaderBar, ELETYPE_TEXT, 0, NULL);
	AxWin_SetText(text, "Acess2 GUI - By thePowersGang (John Hodge)");
	//  > Tab Bar (Current windows)
	gpInterface_TabBar = AxWin_CreateElement(gpInterface_MainArea, ELETYPE_TABBAR, 0, "TabBar");
	AxWin_SetSize(gpInterface_TabBar, giInterface_TabBarSize);
	//  > Application Space
	gpInterface_TabContent = AxWin_CreateElement(gpInterface_MainArea, ELETYPE_BOX, 0, "TabContent");
}

void Interface_Update(void)
{
//	tApplication	*app;
//	tApplicationLink	*lnk;
	giInterface_Width = giScreenWidth/16;
	AxWin_SetSize( gpInterface_Sidebar, giInterface_Width );

	// Scan application list for changes
	// - HACK for now, just directly access it
//	for( app = gWM_Applications; app; app = app->Next )
//	{
//		AxWin_CreateElement();
//	}

	// Update current tab list
}

void Interface_Render(void)
{
	Video_FillRect(
		0, 0,
		giInterface_Width, giScreenHeight,
		0xDDDDDD);
	
	Video_Update();
}

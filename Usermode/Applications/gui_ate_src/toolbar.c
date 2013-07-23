/*
 * Acess Text Editor (ATE)
 * - By John Hodge (thePowersGang)
 *
 * toolbar.c
 * - Main toolbar code
 */
#include <axwin3/axwin.h>
#include <axwin3/widget.h>
#include "include/common.h"
#include "strings.h"

// === PROTOTYPES ===
void	add_toolbar_button(tAxWin3_Widget *Toolbar, const char *Ident, tAxWin3_Widget_FireCb Callback);
 int	Toolbar_Cb_New(tAxWin3_Widget *Widget);
 int	Toolbar_Cb_Open(tAxWin3_Widget *Widget);
 int	Toolbar_Cb_Save(tAxWin3_Widget *Widget);
 int	Toolbar_Cb_Close(tAxWin3_Widget *Widget);

// === CODE ===
void Toolbar_Init(tAxWin3_Widget *Toolbar)
{
	add_toolbar_button(Toolbar, "BtnNew", Toolbar_Cb_New);
	add_toolbar_button(Toolbar, "BtnOpen", Toolbar_Cb_Open);
	add_toolbar_button(Toolbar, "BtnSave", Toolbar_Cb_Save);
	add_toolbar_button(Toolbar, "BtnClose", Toolbar_Cb_Close);
	AxWin3_Widget_AddWidget(Toolbar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "");
	add_toolbar_button(Toolbar, "BtnUndo", NULL);
	add_toolbar_button(Toolbar, "BtnRedo", NULL);
	AxWin3_Widget_AddWidget(Toolbar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "");
	add_toolbar_button(Toolbar, "BtnCut", NULL);
	add_toolbar_button(Toolbar, "BtnCopy", NULL);
	add_toolbar_button(Toolbar, "BtnPaste", NULL);
	AxWin3_Widget_AddWidget(Toolbar, ELETYPE_SPACER, ELEFLAG_NOSTRETCH, "");
	add_toolbar_button(Toolbar, "BtnSearch", NULL);
	add_toolbar_button(Toolbar, "BtnReplace", NULL);
}

void add_toolbar_button(tAxWin3_Widget *Toolbar, const char *Ident, tAxWin3_Widget_FireCb Callback)
{
	tAxWin3_Widget *btn = AxWin3_Widget_AddWidget(Toolbar, ELETYPE_BUTTON, ELEFLAG_NOSTRETCH, Ident);
	const char *img = getimg(Ident);
	if( img )
	{
		tAxWin3_Widget *txt = AxWin3_Widget_AddWidget(btn, ELETYPE_IMAGE, 0, Ident);
		AxWin3_Widget_SetText(txt, img);
		// TODO: tooltip?
	}
	else
	{
		tAxWin3_Widget *txt = AxWin3_Widget_AddWidget(btn, ELETYPE_TEXT, 0, Ident);
		AxWin3_Widget_SetText(txt, getstr(Ident));
	}
	AxWin3_Widget_SetFireHandler(btn, Callback);
}

int Toolbar_Cb_New(tAxWin3_Widget *Widget)
{
	return 0;
}
int Toolbar_Cb_Open(tAxWin3_Widget *Widget)
{
	return 0;
}
int Toolbar_Cb_Save(tAxWin3_Widget *Widget)
{
	return 0;
}
int Toolbar_Cb_Close(tAxWin3_Widget *Widget)
{
	AxWin3_StopMainLoop(1);
	return 0;
}


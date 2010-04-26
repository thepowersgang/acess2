/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 * 
 * Window Manager and Widget Control
 */
#include "common.h"
#include <stdlib.h>
#include <strings.h>
#include "wm.h"

tElement	gWM_RootElement;

// === PROTOTYPES ===
tElement	*WM_CreateElement(tElement *Parent, int Type, int Flags);
void	WM_SetFlags(tElement *Element, int Flags);
void	WM_SetSize(tElement *Element, int Size);
void	WM_SetText(tElement *Element, char *Text);

// === CODE ===
tElement *WM_CreateElement(tElement *Parent, int Type, int Flags)
{
	tElement	*ret;
	
	if(Type < 0 || Type >= NUM_ELETYPES)	return NULL;
	
	ret = calloc(sizeof(tElement), 1);
	if(!ret)	return NULL;
	
	// Prepare
	ret->Type = Type;
	ret->Parent = Parent;
	
	// Append to parent's list
	ret->NextSibling = Parent->LastChild;
	Parent->LastChild = ret;
	if(!Parent->FirstChild)	Parent->FirstChild = ret;
	
	return ret;
}

void WM_SetFlags(tElement *Element, int Flags)
{
	// Permissions are handled in the message handler
	if(!Element) {
		gWM_RootElement.Flags = Flags;
		return ;
	}
	
	Element->Flags = Flags;
	return ;
}

void WM_SetSize(tElement *Element, int Size)
{
	if(!Element)	return ;
	Element->Size = Size;
	return ;
}

void WM_SetText(tElement *Element, char *Text)
{
	if(!Element)	return ;
	if(Element->Text)	free(Element->Text);
	Element->Text = strdup(Text);
	return ;
}

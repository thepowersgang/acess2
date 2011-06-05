/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 * 
 * Window Manager and Widget Control
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include "wm.h"
#include <acess/sys.h>	// _SysDebug

// === IMPORTS ===
extern void	Video_GetTextDims(tFont *Font, const char *Text, int *W, int *H);

// === PROTOTYPES ===
tApplication	*AxWin_RegisterClient(tIPC_Type *IPCType, void *Ident, const char *Name);
void	AxWin_DeregisterClient(tApplication *App);
tApplication	*AxWin_GetClient(tIPC_Type *Method, void *Ident);
tElement	*AxWin_CreateElement(tElement *Parent, int Type, int Flags, const char *DebugName);
void	AxWin_DeleteElement(tElement *Element);
void	AxWin_SetFlags(tElement *Element, int Flags);
void	AxWin_SetSize(tElement *Element, int Size);
void	AxWin_SetText(tElement *Element, const char *Text);

// === GLOBALS ===
// - TODO: Handle windows by having multiple root elements
tElement	gWM_RootElement = {
	.DebugName = "ROOT"
};
tApplication	*gWM_Applications;

// --- Element type flags
struct {
	void	(*Init)(tElement *This);
	void	(*Delete)(tElement *This);
	void	(*UpdateFlags)(tElement *This);
	void	(*UpdateSize)(tElement *This);
	void	(*UpdateText)(tElement *This);
}	gaWM_WidgetTypes[MAX_ELETYPES] = {
	{NULL, NULL, NULL, NULL, NULL},	// NULL
	{NULL, NULL, NULL, NULL, NULL},	// Window
	{NULL, NULL, NULL, NULL, NULL}	// Box
};
const int	ciWM_NumWidgetTypes = sizeof(gaWM_WidgetTypes)/sizeof(gaWM_WidgetTypes[0]);

// === CODE ===
tApplication *AxWin_RegisterClient(tIPC_Type *Method, void *Ident, const char *Name)
{
	 int	identlen = Method->GetIdentSize(Ident);
	// Structure, empty string, Name, Ident
	tApplication	*ret = calloc( 1, sizeof(tApplication) + 1 + strlen(Name) + 1 + identlen );
	
	// DebugName is empty
	
	// Name/Title
	ret->Name = &ret->MetaElement.DebugName[1];
	strcpy(ret->Name, Name);
	// Ident
	ret->Ident = ret->Name + strlen(Name) + 1;
	memcpy(ret->Ident, Ident, identlen);
	// IPC Type
	ret->IPCType = Method;

	// Element index
	ret->MaxElementIndex = DEFAULT_ELEMENTS_PER_APP;
	ret->EleIndex = calloc( 1, ret->MaxElementIndex * sizeof(*ret->EleIndex) );

	// Add to global list
	ret->Next = gWM_Applications;
	gWM_Applications = ret;

	// TODO: Inform listeners of the new application

	return ret;
}

void AxWin_DeregisterClient(tApplication *App)
{
	// TODO: Complete implementing DeregisterClient
	tElement	*win, *next;

	for( win = App->MetaElement.FirstChild; win; win = next )
	{
		next = win->NextSibling;
		AxWin_DeleteElement(win);
	}

	// TODO: Inform listeners of deleted application
	
	// Remove from list
	{
		tApplication	*app, *prev = NULL;
		for( app = gWM_Applications; app; app = app->Next )
		{
			if( app == App )	break;
			prev = app;
		}
		
		if( app )
		{
			if(prev)
				prev->Next = App->Next;
			else
				gWM_Applications = App->Next;
		}
	}
	
	free(App);
}

/**
 * \brief Get an application handle from a client identifier
 */
tApplication *AxWin_GetClient(tIPC_Type *Method, void *Ident)
{
	// TODO: Faster and smarter technique
	tApplication	*app;
	for( app = gWM_Applications; app; app = app->Next )
	{
		if( app->IPCType != Method )	continue;
		if( Method->CompareIdent( app->Ident, Ident ) != 0 ) continue;
		return app;
	}
	return NULL;
}

tElement *AxWin_CreateWindow(tApplication *App, const char *Name)
{
	tElement	*ret;

	// TODO: Implement _CreateTab
	
	ret = AxWin_CreateElement(&App->MetaElement, ELETYPE_WINDOW, 0, NULL);
	ret->Text = strdup(Name);
	return ret;
}

// --- Widget Creation and Control ---
tAxWin_Element *AxWin_CreateElement(tElement *Parent, int Type, int Flags, const char *DebugName)
{
	tElement	*ret;
	const char	*dbgName = DebugName ? DebugName : "";
	
	ret = calloc(sizeof(tElement)+strlen(dbgName)+1, 1);
	if(!ret)	return NULL;
	
	// Prepare
	ret->Type = Type;
	strcpy(ret->DebugName, dbgName);
	if(Parent == NULL)	Parent = &gWM_RootElement;
	ret->Parent = Parent;
	ret->Flags = Flags;
	
	// Append to parent's list
	if(Parent->LastChild)
		Parent->LastChild->NextSibling = ret;
	Parent->LastChild = ret;
	if(!Parent->FirstChild)	Parent->FirstChild = ret;
	
	ret->PaddingL = 2;
	ret->PaddingR = 2;
	ret->PaddingT = 2;
	ret->PaddingB = 2;
	
	if( Type < ciWM_NumWidgetTypes && gaWM_WidgetTypes[Type].Init )
		gaWM_WidgetTypes[Type].Init(ret);
	
	WM_UpdateMinDims(ret->Parent);
	
	return ret;
}

/**
 * \brief Delete an element
 */
void AxWin_DeleteElement(tElement *Element)
{
	tElement	*child, *next;
	
	for(child = Element->FirstChild; child; child = next)
	{
		next = child->NextSibling;
		AxWin_DeleteElement(child);
	}

	// TODO: Implement AxWin_DeleteElement
	// TODO: Clean up related data.
	if( Element->Type < ciWM_NumWidgetTypes && gaWM_WidgetTypes[Element->Type].Delete )
		gaWM_WidgetTypes[Element->Type].Delete(Element);

	if(Element->Owner)
		Element->Owner->EleIndex[ Element->ApplicationID ] = NULL;

	Element->Type = 0;
	Element->Owner = 0;
	Element->Flags = 0;

	free(Element);
}

/**
 * \brief Alter an element's flags 
 */
void AxWin_SetFlags(tElement *Element, int Flags)
{
	// Permissions are handled in the message handler
	if(!Element) {
		gWM_RootElement.Flags = Flags;
		return ;
	}
	
	Element->Flags = Flags;
	return ;
}

/**
 * \brief Set the fixed lenghthways size of an element
 */
void AxWin_SetSize(tElement *Element, int Size)
{
	if(!Element)	return ;
	Element->FixedWith = Size;
	return ;
}

/**
 * \brief Set the text field of an element
 * \note Used for the image path on ELETYPE_IMAGE
 */
void AxWin_SetText(tElement *Element, const char *Text)
{
	if(!Element)	return ;
	if(Element->Text)	free(Element->Text);
	Element->Text = strdup(Text);
	
	switch(Element->Type)
	{
	case ELETYPE_IMAGE:
		if(Element->Data)	free(Element->Data);
		Element->Data = Image_Load( Element->Text );
		if(!Element->Data) {
			Element->Flags &= ~ELEFLAG_FIXEDSIZE;
			return ;
		}
		
		//Element->Flags |= ELEFLAG_FIXEDSIZE;
		Element->CachedW = ((tImage*)Element->Data)->Width;
		Element->CachedH = ((tImage*)Element->Data)->Height;
		
		if(Element->Parent && Element->Parent->Flags & ELEFLAG_VERTICAL) {
			Element->MinCross = ((tImage*)Element->Data)->Width;
			Element->MinWith = ((tImage*)Element->Data)->Height;
		}
		else {
			Element->MinWith = ((tImage*)Element->Data)->Width;
			Element->MinCross = ((tImage*)Element->Data)->Height;
		}
		break;
	
	case ELETYPE_TEXT:
		{
		 int	w=0, h=0;
		Video_GetTextDims(NULL, Element->Text, &w, &h);
		if(Element->Parent && Element->Parent->Flags & ELEFLAG_VERTICAL) {
			Element->MinCross = w;
			Element->MinWith = h;
		}
		else {
			Element->MinWith = w;
			Element->MinCross = h;
		}
		}
		break;
	default:	// Any other, no special case
		break ;	
	}
	
	return ;
}

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

// === IMPORTS ===
extern void	Decorator_RenderWidget(tElement *Element);

// === PROTOTYPES ===
tElement	*WM_CreateElement(tElement *Parent, int Type, int Flags);
void	WM_UpdateMinDims(tElement *Element);
void	WM_SetFlags(tElement *Element, int Flags);
void	WM_SetSize(tElement *Element, int Size);
void	WM_SetText(tElement *Element, char *Text);
void	WM_UpdateDimensions(tElement *Element, int Pass);
void	WM_UpdatePosition(tElement *Element);
void	WM_RenderWidget(tElement *Element);
void	WM_Update(void);

// === GLOBALS ===
tElement	gWM_RootElement;
struct {
	void	(*Init)(tElement *This);
	void	(*UpdateFlags)(tElement *This);
	void	(*UpdateSize)(tElement *This);
	void	(*UpdateText)(tElement *This);
}	gaWM_WidgetTypes[MAX_ELETYPES] = {
	{NULL, NULL, NULL, NULL},	// NULL
	{NULL, NULL, NULL, NULL}	// Box
};

// === CODE ===
// --- Widget Creation and Control ---
tElement *WM_CreateElement(tElement *Parent, int Type, int Flags)
{
	tElement	*ret;
	
	ret = calloc(sizeof(tElement), 1);
	if(!ret)	return NULL;
	
	// Prepare
	ret->Type = Type;
	if(Parent == NULL)	Parent = &gWM_RootElement;
	ret->Parent = Parent;
	ret->Flags = Flags;
	
	// Append to parent's list
	ret->NextSibling = Parent->LastChild;
	Parent->LastChild = ret;
	if(!Parent->FirstChild)	Parent->FirstChild = ret;
	
	ret->PaddingL = 2;
	ret->PaddingR = 2;
	ret->PaddingT = 2;
	ret->PaddingB = 2;
	
	if( gaWM_WidgetTypes[Type].Init )
		gaWM_WidgetTypes[Type].Init(ret);
	
	WM_UpdateMinDims(ret->Parent);
	
	return ret;
}

/**
 * \brief Alter an element's flags 
 */
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
	Element->FixedWith = Size;
	return ;
}

/**
 * \brief Set the text field of an element
 * \note Used for the image path on ELETYPE_IMAGE
 */
void WM_SetText(tElement *Element, char *Text)
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
	}
	
	return ;
}

// --- Pre-Rendering ---
/**
 * \name Pre-Rendering
 * \brief Updates the element positions and sizes
 * \{
 */
/**
 * \brief Updates the dimensions of an element
 * 
 * The dimensions of an element are calculated from the parent's
 * cross dimension (the side at right angles to the alignment) sans some
 * padding.
 */
void WM_UpdateDimensions(tElement *Element, int Pass)
{
	tElement	*child;
	 int	nChildren = 0;
	 int	nFixed = 0;
	 int	maxCross = 0;
	 int	fixedSize = 0;
	 int	fullCross, dynWith;
	
	_SysDebug("%p -> Flags = 0x%x", Element, Element->Flags);
	_SysDebug("%p ->CachedH = %i, ->PaddingT = %i, ->PaddingB = %i",
		Element, Element->CachedH, Element->PaddingT, Element->PaddingB
		);
	_SysDebug("%p ->CachedW = %i, ->PaddingL = %i, ->PaddingR = %i",
		Element, Element->CachedW, Element->PaddingL, Element->PaddingR
		);
	
	// Pass 1
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )
			continue ;
		
		_SysDebug("%p,%p ->FixedWith = %i", Element, child, child->FixedWith);
		if( child->FixedWith )
		{
			nFixed ++;
			fixedSize += child->FixedWith;
		}
		
		if( child->FixedCross && maxCross < child->FixedCross )
			maxCross = child->FixedCross;
		if( child->MinCross && maxCross < child->MinCross )
			maxCross = child->MinCross;
		nChildren ++;
	}
	
	_SysDebug("%p - nChildren = %i, nFixed = %i", Element, nChildren, nFixed);
	if( nChildren > nFixed ) {
		if( Element->Flags & ELEFLAG_VERTICAL )
			dynWith = Element->CachedH - Element->PaddingT
				- Element->PaddingB;
		else
			dynWith = Element->CachedW - Element->PaddingL
				- Element->PaddingR;
		dynWith -= fixedSize;
		if( dynWith < 0 )	return ;
		dynWith /= nChildren - nFixed;
		_SysDebug("%p - dynWith = %i", Element, dynWith);
	}
	
	if( Element->Flags & ELEFLAG_VERTICAL )
		fullCross = Element->CachedW - Element->PaddingL - Element->PaddingR;
	else
		fullCross = Element->CachedH - Element->PaddingT - Element->PaddingB;
	
	_SysDebug("%p - fullCross = %i", Element, fullCross);
	
	// Pass 2 - Set sizes and recurse
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	cross, with;
		
		_SysDebug("%p,%p ->MinCross = %i", Element, child, child->MinCross);

		
		// --- Cross Size ---
		if( child->FixedCross )
			cross = child->FixedCross;
		// Expand to fill?
		// TODO: Extra flag so options are (Expand, Equal, Wrap)
		else if( child->Flags & ELEFLAG_NOEXPAND )
			cross = child->MinCross;
		else
			cross = fullCross;
		_SysDebug("%p,%p - cross = %i", Element, child, cross);
		if( Element->Flags & ELEFLAG_VERTICAL )
			child->CachedW = cross;
		else
			child->CachedH = cross;
		
		// --- With Size ---
		if( child->FixedWith)
			with = child->FixedWith;
		else if( child->Flags & ELEFLAG_NOSTRETCH )
			with = child->MinWith;
		else
			with = dynWith;
		_SysDebug("%p,%p - with = %i", Element, child, with);
		if( Element->Flags & ELEFLAG_VERTICAL )
			child->CachedH = with;
		else
			child->CachedW = with;
		
		WM_UpdateDimensions(child, 0);
	}
}

/**
 * \brief Updates the position of an element
 * 
 * The parent element sets the positions of its children
 */
void WM_UpdatePosition(tElement *Element)
{
	tElement	*child;
	 int	x, y;
	
	if( Element->Flags & ELEFLAG_NORENDER )	return ;
	
	_SysDebug("Element=%p{PaddingL:%i, PaddingT:%i}",
		Element, Element->PaddingL, Element->PaddingT);
	
	// Initialise
	x = Element->CachedX + Element->PaddingL;
	y = Element->CachedY + Element->PaddingT;
	
	// Update each child
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		child->CachedX = x;
		child->CachedY = y;
		
		// Set Alignment
		if( Element->Flags & ELEFLAG_ALIGN_CENTER ) {
			if(Element->Flags & ELEFLAG_VERTICAL )
				child->CachedX += Element->CachedW/2 - child->CachedW/2;
			else
				child->CachedY += Element->CachedH/2 - child->CachedH/2;
		}
		else if( Element->Flags & ELEFLAG_ALIGN_END ) {
			if(Element->Flags & ELEFLAG_VERTICAL )
				child->CachedX += Element->CachedW - child->CachedW;
			else
				child->CachedY += Element->CachedH - child->CachedH;
		}
		
		// Update child's children positions
		WM_UpdatePosition(child);
		
		// Increment
		if(Element->Flags & ELEFLAG_VERTICAL ) {
			y += child->CachedH + Element->GapSize;
		}
		else {
			x += child->CachedW + Element->GapSize;
		}
	}
	
	_SysDebug("Element %p (%i,%i)",
		 Element, Element->CachedX, Element->CachedY
		);
}

/**
 * \brief Update the minimum dimensions of the element
 * \note Called after a child's minimum dimensions have changed
 */
void WM_UpdateMinDims(tElement *Element)
{
	tElement	*child;
	
	if(!Element)	return;
	
	Element->MinCross = 0;
	Element->MinWith = 0;
	
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		if( Element->Parent &&
			(Element->Flags & ELEFLAG_VERTICAL) == (Element->Parent->Flags & ELEFLAG_VERTICAL)
			)
		{
			if(child->FixedCross)
				Element->MinCross += child->FixedCross;
			else
				Element->MinCross += child->MinCross;
			if(child->FixedWith)
				Element->MinWith += child->FixedWith;
			else
				Element->MinWith += child->MinWith;
		}
		else
		{
			if(child->FixedCross)
				Element->MinWith += child->FixedCross;
			else
				Element->MinWith += child->MinCross;
			if(child->FixedWith)
				Element->MinCross += child->FixedWith;
			else
				Element->MinCross += child->MinWith;
		}
	}
	
	// Recurse upwards
	WM_UpdateMinDims(Element->Parent);
}
/**
 * \}
 */

// --- Render ---
void WM_RenderWidget(tElement *Element)
{
	tElement	*child;
	
	if( Element->Flags & ELEFLAG_NORENDER )	return ;
	if( Element->Flags & ELEFLAG_INVISIBLE )	return ;
	
	Decorator_RenderWidget(Element);
	
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		WM_RenderWidget(child);
	}
}

void WM_Update(void)
{
	gWM_RootElement.CachedX = 0;	gWM_RootElement.CachedY = 0;
	gWM_RootElement.CachedW = giScreenWidth;
	gWM_RootElement.CachedH = giScreenHeight;
	gWM_RootElement.Flags |= ELEFLAG_NOEXPAND|ELEFLAG_ABSOLUTEPOS|ELEFLAG_FIXEDSIZE;
	
	WM_UpdateDimensions( &gWM_RootElement, 0 );
	WM_UpdatePosition( &gWM_RootElement );
	WM_RenderWidget( &gWM_RootElement );
	
	Video_Update();
}

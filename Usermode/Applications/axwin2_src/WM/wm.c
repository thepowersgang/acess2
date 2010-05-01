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

// === IMPORTS ===
extern void	Decorator_RenderWidget(tElement *Element);

// === PROTOTYPES ===
tElement	*WM_CreateElement(tElement *Parent, int Type, int Flags);
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
	
	// Append to parent's list
	ret->NextSibling = Parent->LastChild;
	Parent->LastChild = ret;
	if(!Parent->FirstChild)	Parent->FirstChild = ret;
	
	ret->PaddingL = 2;
	ret->PaddingR = 2;
	ret->PaddingT = 2;
	ret->PaddingB = 2;
	
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
	
	switch(Element->Type)
	{
	case ELETYPE_IMAGE:
		if(Element->Data)	free(Element->Data);
		Element->Data = Image_Load( Element->Text );
		if(!Element->Data) {
			Element->Flags &= ~ELEFLAG_FIXEDSIZE;
			return ;
		}
		
		Element->Flags |= ELEFLAG_FIXEDSIZE;
		Element->CachedW = ((tImage*)Element->Data)->Width;
		Element->CachedH = ((tImage*)Element->Data)->Height;
		
		if(Element->Parent && Element->Parent->Flags & ELEFLAG_VERTICAL)
			Element->Size = Element->CachedH;
		else
			Element->Size = Element->CachedW;
		break;
	}
	
	return ;
}

// --- Pre-Rendering ---
#if 1
void WM_UpdateDimensions(tElement *Element, int Pass)
{
	 int 	fixedSize = 0, maxCross = 0;
	 int	nFixed = 0, nChildren = 0;
	 int	minSize = 0;
	tElement	*child;
	
	// Pass zero intialises
	if( Pass == 0 )
	{
		// If not a fixed size element, initialise the sizes
		if( !(Element->Flags & ELEFLAG_FIXEDSIZE) )
		{
			Element->CachedH = 0;
			Element->CachedW = 0;
			if( Element->Size )
			{
				if( Element->Parent->Flags & ELEFLAG_VERTICAL )
					Element->CachedH = Element->Size;
				else
					Element->CachedW = Element->Size;
			}
			
			if( !(Element->Flags & ELEFLAG_NOEXPAND) )
			{
				if( Element->Parent->Flags & ELEFLAG_VERTICAL )
					Element->CachedW = Element->Parent->CachedW;
				else
					Element->CachedH = Element->Parent->CachedH;
			}
		}
	}

	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		WM_UpdateDimensions( child, 0 );
		
		if( Element->Flags & ELEFLAG_VERTICAL )
		{
			if( child->CachedH ) {
				fixedSize += child->CachedH;
				minSize += child->CachedH;
				nFixed ++;
			}
			else
				minSize += child->MinHeight;
			
			if( maxCross < child->CachedW )
				maxCross = child->CachedW;;
		}
		else
		{
			if( child->CachedW ) {
				fixedSize += child->CachedW;
				minSize += child->CachedW;
				nFixed ++;
			}
			else
				minSize += child->MinWidth;
			
			if( maxCross < child->CachedH )
				maxCross = child->CachedH;
		}
		nChildren ++;
	}
	_SysDebug("WM_UpdateDimensions: nFixed=%i, fixedSize=%i, minSize=%i, maxCross=%i",
		nFixed, fixedSize, minSize, maxCross
		);


	// If we don't have our dimensions, get the child dimensions
	if( Element->CachedW == 0 || Element->CachedH == 0 )
	{	
		if( Element->Flags & ELEFLAG_VERTICAL )
		{
			if( Element->CachedW == 0 && maxCross )
				Element->CachedW = Element->PaddingL
					+ Element->PaddingR + maxCross;
			
			if( Element->CachedH == 0 && nFixed == nChildren )
				Element->CachedH = Element->PaddingT
					+ Element->PaddingB + fixedSize
					+ nChildren * Element->GapSize;
		}
		else
		{
			if( maxCross )
				Element->CachedH = Element->PaddingT
					+ Element->PaddingB + maxCross;
			
			if( Element->CachedW == 0 && nFixed == nChildren )
				Element->CachedW = Element->PaddingL
					+ Element->PaddingR + fixedSize
					+ nChildren * Element->GapSize;
		}
	}
	
	// Now, if we have the "length" of the widget, we can size the children
	if( (Element->Flags & ELEFLAG_VERTICAL && Element->CachedH > 0)
	 || (!(Element->Flags & ELEFLAG_VERTICAL) && Element->CachedW > 0) )
	{
		 int	dynSize;
		
		// Calculate the size of dynamically sized elements
		if( Element->Flags & ELEFLAG_VERTICAL )
			dynSize = Element->CachedH - Element->PaddingT
				 - Element->PaddingB - fixedSize;
		else
			dynSize = Element->CachedW - Element->PaddingL
				 - Element->PaddingR - fixedSize;
		dynSize /= nChildren - nFixed;
		
		// Itterate children again
		for( child = Element->FirstChild; child; child = child->NextSibling )
		{
			 int	tmp;
			
			// Get the size of them
			if(child->Size)
				tmp = child->Size;
			else
				tmp = dynSize;
			
			if( Element->Flags & ELEFLAG_VERTICAL ) {
				if( tmp < child->MinHeight )
					tmp = child->MinHeight;
				child->CachedH = tmp;
			}
			else {
				if( tmp < child->MinWidth )
					tmp = child->MinWidth;
				child->CachedW = tmp;
			}
			
			WM_UpdateDimensions(child, 1);
		}
	}
}

#else

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
	 int	fixedChildSize = 0;
	 int	dynamicSize;
	 int	nChildren = 0;
	 int	nFixed = 0;
	
	_SysDebug("WM_UpdateDimensions: (Element=%p{Flags:0x%x}, Pass=%i)",
		Element, Element->Flags,
		Pass);
	
	if( Pass == 0 )
	{
		if( Element->Flags & ELEFLAG_NORENDER )	return ;
		
		if( !(Element->Flags & ELEFLAG_ABSOLUTEPOS) ) {
			Element->CachedX = 0;
			Element->CachedY = 0;
		}
		if( !(Element->Flags & ELEFLAG_FIXEDSIZE) ) {
			Element->CachedW = 0;
			Element->CachedH = 0;
		}
	}
	
	if( !(Element->Flags & ELEFLAG_FIXEDSIZE) ) {
		// If the element is sized, fix its dimension(s)
		if(Element->Size)
		{
			if(Element->Flags & ELEFLAG_NOEXPAND)
			{
				Element->CachedW = Element->Size;
				Element->CachedH = Element->Size;
			}
			else {
				if( Element->Parent->Flags & ELEFLAG_VERTICAL ) {
					Element->CachedH = Element->Size;
					Element->CachedW = Element->Parent->CachedW;
					if(Element->CachedW)
						Element->CachedW -= (Element->Parent->PaddingL + Element->Parent->PaddingR);
				}
				else {
					Element->CachedW = Element->Size;
					Element->CachedH = Element->Parent->CachedH;
					if(Element->CachedH)
						Element->CachedH -= (Element->Parent->PaddingT + Element->Parent->PaddingB);
				}
			}
		}
		else {
			// Ok, so now we need to calculate the size of all child elements
			// However, if ELEFLAG_NOEXPAND is not set, we can still set one
			// dimension
			if( !(Element->Flags & ELEFLAG_NOEXPAND) ) {
				if( Element->Parent->Flags & ELEFLAG_VERTICAL ) {
					Element->CachedW = Element->Parent->CachedW;
					if(Element->CachedW)
						Element->CachedW -= (Element->Parent->PaddingL + Element->Parent->PaddingR);
				}
				else {
					Element->CachedH = Element->Parent->CachedH;
					if(Element->CachedH)
						Element->CachedH -= (Element->Parent->PaddingT + Element->Parent->PaddingB);
				}
			}
		}
	}
	
	// Process Children (first pass)
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		if( child->Flags & ELEFLAG_NORENDER )	continue;
		WM_UpdateDimensions(child, 0);
		
		// Children that don't inherit positions are ignored
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )	continue;
		
		fixedChildSize += child->Size;
		if(child->Size > 0)
			nFixed ++;
		nChildren ++;
		
		// If we are wrapping the children, get the largest cross size
		if( !(Element->Flags & ELEFLAG_FIXEDSIZE)
		 && Element->Flags & ELEFLAG_NOEXPAND
		 && Element->Size == 0 )
		{
			if( Element->Flags & ELEFLAG_VERTICAL ) {
				if( Element->CachedW < child->CachedW )
					Element->CachedW = child->CachedW;
			}
			else {
				if( Element->CachedH < child->CachedH )
					Element->CachedH = child->CachedH;
			}
		}
	}
	
	// Let's avoid a #DIV0 shall we?
	if( nChildren > 0 )
	{
		// Calculate the size of dynamically sized children
		if( Element->Flags & ELEFLAG_VERTICAL ) {
			if( Element->CachedH == 0 ) {
				if( nFixed == nChildren )
					Element->CachedH = fixedChildSize;
				else
					return ;
			}
			dynamicSize = (Element->CachedH - (Element->PaddingT + Element->PaddingB)  - fixedChildSize) / nChildren;
		}
		else {
			if( Element->CachedW == 0 ) {
				if( nFixed == nChildren )
					Element->CachedW = fixedChildSize;
				else
					return ;
			}
			dynamicSize = (Element->CachedW - (Element->PaddingL + Element->PaddingR) - fixedChildSize) / nChildren;
		}
		
		// Process Children (second pass)
		for( child = Element->FirstChild; child; child = child->NextSibling )
		{
			if( child->Flags & ELEFLAG_NORENDER )	continue;
			// Children that don't inherit positions are ignored
			if( child->Flags & ELEFLAG_ABSOLUTEPOS )	continue;
			
			if(!child->Size) {
				if(child->Flags & ELEFLAG_VERTICAL)
					child->CachedH = dynamicSize;
				else
					child->CachedW = dynamicSize;
			}
			
			WM_UpdateDimensions(child, 1);
			
			// If we are wrapping the children, get the largest cross size
			if( Element->Flags & ELEFLAG_NOEXPAND ) {
				if( Element->Flags & ELEFLAG_VERTICAL ) {
					if( Element->CachedW < child->CachedW )
						Element->CachedW = child->CachedW;
				}
				else {
					if( Element->CachedH < child->CachedH )
						Element->CachedH = child->CachedH;
				}
			}
		}
	}
	
	// Add the padding
	//Element->CachedW += Element->PaddingL + Element->PaddingR;
	//Element->CachedH += Element->PaddingT + Element->PaddingB;
	
	_SysDebug("Pass %i, Element %p %ix%i",
		Pass, Element, Element->CachedW, Element->CachedH
		);
	
	// We should be done
	// Next function will do the coordinates
}
#endif


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

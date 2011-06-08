/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 * 
 * Rendering code
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include "wm.h"
#include <acess/sys.h>	// _SysDebug

// === IMPORTS ===
extern void	Decorator_RenderWidget(tElement *Element);
extern tElement	gWM_RootElement;
extern tApplication	*gWM_Applications;
extern int	giWM_MaxAreaX;
extern int	giWM_MaxAreaY;
extern int	giWM_MaxAreaW;
extern int	giWM_MaxAreaH;

// === PROTOTYPES ===
void	WM_UpdateMinDims(tElement *Element);
void	WM_UpdateDimensions(tElement *Element, int Pass);
void	WM_UpdatePosition(tElement *Element);
void	WM_RenderWidget(tElement *Element);
void	WM_Update(void);

// === CODE ===
/**
 * \brief Updates the dimensions of an element
 * \todo What is the \a Pass parameter for
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
	
	_SysDebug("WM_UpdateDimensions %p'%s'", Element, Element->DebugName);
	_SysDebug(" -> Flags = 0x%x", Element->Flags);
	_SysDebug(" ->CachedH = %i, ->PaddingT = %i, ->PaddingB = %i",
		Element->CachedH, Element->PaddingT, Element->PaddingB
		);
	_SysDebug(" ->CachedW = %i, ->PaddingL = %i, ->PaddingR = %i",
		Element->CachedW, Element->PaddingL, Element->PaddingR
		);
	
	// Pass 1
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )
			continue ;
		
		_SysDebug(" > %p'%s' ->FixedWith = %i", child, child->DebugName, child->FixedWith);
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
	
	_SysDebug(" - nChildren = %i, nFixed = %i", Element, nChildren, nFixed);
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
		_SysDebug(" - dynWith = %i", dynWith);
	}
	
	if( Element->Flags & ELEFLAG_VERTICAL )
		fullCross = Element->CachedW - Element->PaddingL - Element->PaddingR;
	else
		fullCross = Element->CachedH - Element->PaddingT - Element->PaddingB;
	
	_SysDebug(" - fullCross = %i", Element, fullCross);
	
	// Pass 2 - Set sizes and recurse
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	cross, with;
		
		_SysDebug(" > %p'%s' ->MinCross = %i", child, child->DebugName, child->MinCross);

		
		// --- Cross Size ---
		if( child->FixedCross )
			cross = child->FixedCross;
		// Expand to fill?
		// TODO: Extra flag so options are (Expand, Equal, Wrap)
		else if( child->Flags & ELEFLAG_NOEXPAND )
			cross = child->MinCross;
		else
			cross = fullCross;
		_SysDebug(" > %p'%s' - cross = %i", child, child->DebugName, cross);
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
		_SysDebug(" > %p'%s' - with = %i", child, child->DebugName, with);
		if( Element->Flags & ELEFLAG_VERTICAL )
			child->CachedH = with;
		else
			child->CachedW = with;
		
		WM_UpdateDimensions(child, Pass);
	}
	
	_SysDebug("%p'%s' Done", Element, Element->DebugName);
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
	static int	depth = 0;
	char	indent[depth+1];
	
	if( Element->Flags & ELEFLAG_NORENDER )	return ;
	
	memset(indent, ' ', depth);
	indent[depth] = '\0';
	depth ++;
	
	_SysDebug("%sWM_UpdatePosition %p'%s'{PaddingL:%i, PaddingT:%i}",
		indent, Element, Element->DebugName, Element->PaddingL, Element->PaddingT);
	
	// Initialise
	x = Element->CachedX + Element->PaddingL;
	y = Element->CachedY + Element->PaddingT;
	
	_SysDebug("%s- Alignment = %s", indent,
		(Element->Flags & ELEFLAG_VERTICAL) ? "vertical" : "horizontal");

	// Update each child
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		_SysDebug("%s- x = %i, y = %i", indent, x, y);
		child->CachedX = x;
		child->CachedY = y;
		
		// Set Alignment
		if( Element->Flags & ELEFLAG_ALIGN_CENTER ) {
			_SysDebug("%sChild being aligned to center", indent);
			if(Element->Flags & ELEFLAG_VERTICAL)
				child->CachedX += Element->CachedW/2 - child->CachedW/2;
			else
				child->CachedY += Element->CachedH/2 - child->CachedH/2;
		}
		else if( Element->Flags & ELEFLAG_ALIGN_END) {
			_SysDebug("%sChild being aligned to end", indent);
			if(Element->Flags & ELEFLAG_VERTICAL )
				child->CachedX += Element->CachedW
					- Element->PaddingL - Element->PaddingR
					- child->CachedW;
			else
				child->CachedY += Element->CachedH
					- Element->PaddingT
					- Element->PaddingB
					- child->CachedH;
		}
		
		_SysDebug("%s> %p'%s' at (%i,%i)", indent, child, child->DebugName,
			child->CachedX, child->CachedY);
	
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
	
	_SysDebug("%sElement %p'%s' (%i,%i)",
		indent, Element, Element->DebugName, Element->CachedX, Element->CachedY
		);
	depth --;
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

void WM_UpdateWindow(tElement *Ele)
{	
	WM_UpdateDimensions( Ele, 0 );
	WM_UpdatePosition( Ele );
	WM_RenderWidget( Ele );
}

void WM_Update(void)
{
	tApplication	*app;
	tElement	*ele;
	
	for( app = gWM_Applications; app; app = app->Next )
	{
		for( ele = app->MetaElement.FirstChild; ele; ele = ele->NextSibling ) {
			if( ele->Flags & ELEFLAG_WINDOW_MAXIMISED ) {
				ele->CachedX = giWM_MaxAreaX;
				ele->CachedY = giWM_MaxAreaY;
				ele->CachedW = giWM_MaxAreaW;
				ele->CachedH = giWM_MaxAreaH;
			}
			ele->Flags |= ELEFLAG_NOEXPAND|ELEFLAG_ABSOLUTEPOS|ELEFLAG_FIXEDSIZE;
			WM_UpdateWindow(ele);
		}
	}
	
	gWM_RootElement.CachedX = 0;
	gWM_RootElement.CachedY = 0;
	gWM_RootElement.CachedW = giScreenWidth;
	gWM_RootElement.CachedH = giScreenHeight;
	gWM_RootElement.Flags |= ELEFLAG_NOEXPAND|ELEFLAG_ABSOLUTEPOS|ELEFLAG_FIXEDSIZE;
	
	WM_UpdateWindow( &gWM_RootElement );
	
	Video_Update();
}

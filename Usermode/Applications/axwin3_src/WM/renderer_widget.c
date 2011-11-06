/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_widget.c
 * - AxWin2 Widget port
 */
#include <common.h>
#include <wm_renderer.h>
#include <renderer_widget.h>
#include <string.h>
#include <wm_messages.h>
#include <stdlib.h>
#include "include/image.h"

#define DEFAULT_ELETABLE_SIZE	64

// === PROTOTYPES ===
 int	Renderer_Widget_Init(void);
tWindow	*Renderer_Widget_Create(int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);

void	Widget_RenderWidget(tWindow *Window, tElement *Element);
void	Widget_UpdateDimensions(tElement *Element);
void	Widget_UpdatePosition(tElement *Element);
// --- Messages
tElement	*Widget_GetElementById(tWidgetWin *Info, uint32_t ID);
void	Widget_NewWidget(tWidgetWin *Info, size_t Len, tWidgetMsg_Create *Msg);
void	Widget_SetFlags(tWidgetWin *Info, int Len, tWidgetMsg_SetFlags *Msg);
void	Widget_SetSize(tWidgetWin *Info, int Len, tWidgetMsg_SetSize *Msg);
void	Widget_SetText(tWidgetWin *Info, int Len, tWidgetMsg_SetText *Msg);
 int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);
// --- Type helpers
void	Widget_TextBox_UpdateText(tElement *Element, const char *Text);
void	Widget_Image_UpdateText(tElement *Element, const char *Text);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Widget",
	.CreateWindow = Renderer_Widget_Create,
	.Redraw = Renderer_Widget_Redraw,
	.HandleMessage = Renderer_Widget_HandleMessage
};
	
// --- Element type flags
struct {
	void	(*Init)(tElement *Ele);
	void	(*Delete)(tElement *Ele);
	void	(*UpdateFlags)(tElement *Ele);
	void	(*UpdateSize)(tElement *Ele);
	void	(*UpdateText)(tElement *Ele, const char *Text);	// This should update Ele->Text
}	gaWM_WidgetTypes[NUM_ELETYPES] = {
	{NULL, NULL, NULL, NULL, NULL},	// NULL
	{NULL, NULL, NULL, NULL, NULL},	// Box
	{NULL, NULL, NULL, NULL, Widget_TextBox_UpdateText},	// Text
	{NULL, NULL, NULL, NULL, Widget_Image_UpdateText},	// Image
	{NULL, NULL, NULL, NULL, NULL}	// Button
};
const int	ciWM_NumWidgetTypes = sizeof(gaWM_WidgetTypes)/sizeof(gaWM_WidgetTypes[0]);

// === CODE ===
int Renderer_Widget_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Widget);	

	return 0;
}

tWindow	*Renderer_Widget_Create(int Flags)
{
	tWindow	*ret;
	tWidgetWin	*info;
	 int	eletable_size = DEFAULT_ELETABLE_SIZE;

	_SysDebug("Renderer_Widget_Create: (Flags = 0x%x)", Flags);

	// TODO: Use `Flags` as default element count?
	// - Actaully, it's taken by the root ele flags
	// - Use the upper bits?

	ret = WM_CreateWindowStruct( sizeof(tWidgetWin) + sizeof(tElement*)*eletable_size );
	info = ret->RendererInfo;
	
	info->TableSize = eletable_size;
	info->RootElement.ID = -1;
	info->RootElement.BackgroundColour = 0xCCCCCC;
	info->RootElement.Flags = Flags;
	
	return ret;
}

void Renderer_Widget_Redraw(tWindow *Window)
{
	tWidgetWin	*info = Window->RendererInfo;
	WM_Render_FillRect(Window, 0, 0, 0xFFF, 0xFFF, info->RootElement.BackgroundColour);

	Widget_UpdateDimensions(&info->RootElement);
	Widget_UpdatePosition(&info->RootElement);

	Widget_RenderWidget(Window, &info->RootElement);
}

// --- Render / Resize ---
void Widget_RenderWidget(tWindow *Window, tElement *Element)
{
	tElement	*child;
	
	if( Element->Flags & ELEFLAG_NORENDER )	return ;
	if( Element->Flags & ELEFLAG_INVISIBLE )	return ;
	
	Widget_Decorator_RenderWidget(Window, Element);
	
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		Widget_RenderWidget(Window, child);
	}
}

void Widget_UpdateDimensions(tElement *Element)
{
	tElement	*child;
	 int	nChildren = 0;
	 int	nFixed = 0;
	 int	maxCross = 0;
	 int	fixedSize = 0;
	 int	fullCross, dynWith;
	
	// Pass 1
	// - Get the fixed and minimum sizes of the element
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		// Ignore elements that will not be rendered
		if( child->Flags & ELEFLAG_NORENDER )	continue ;
		
		// Absolutely positioned elements don't affect dimensions
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )	continue ;
	
		// Fixed width elements	
		if( child->FixedWith )
		{
			nFixed ++;
			fixedSize += child->FixedWith;
		}
		else if( child->Flags & ELEFLAG_NOSTRETCH )
		{
			nFixed ++;
			fixedSize += child->MinWith;
		}
		
		if( child->FixedCross && maxCross < child->FixedCross )
			maxCross = child->FixedCross;
		if( child->MinCross && maxCross < child->MinCross )
			maxCross = child->MinCross;
		nChildren ++;
	}

	// Get the dynamic with size from the unused space in the element
	if( nChildren > nFixed ) {
		if( Element->Flags & ELEFLAG_VERTICAL )
			dynWith = Element->CachedH - Element->PaddingT - Element->PaddingB;
		else
			dynWith = Element->CachedW - Element->PaddingL - Element->PaddingR;
		dynWith -= fixedSize;
		if( dynWith < 0 )	return ;
		dynWith /= nChildren - nFixed;
	}
	
	_SysDebug("%i - nChildren = %i, nFixed = %i, dynWith = %i, fixedSize = %i",
		Element->ID, nChildren, nFixed, dynWith, fixedSize);

	// Get the cross size
	if( Element->Flags & ELEFLAG_VERTICAL )
		fullCross = Element->CachedW - Element->PaddingL - Element->PaddingR;
	else
		fullCross = Element->CachedH - Element->PaddingT - Element->PaddingB;
	
	// Pass 2 - Set sizes and recurse
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	cross, with;

		// Ignore elements that will not be rendered
		if( child->Flags & ELEFLAG_NORENDER )	continue ;
		
		// --- Cross Size ---
		// TODO: Expand to fill?
		// TODO: Extra flag so options are (Expand, Equal, Wrap)
		if( child->FixedCross )
			cross = child->FixedCross;
		else if( child->Flags & ELEFLAG_NOEXPAND )
			cross = child->MinCross;
		else
			cross = fullCross;
		
		// --- With Size ---
		if( child->FixedWith )
			with = child->FixedWith;
		else if( child->Flags & ELEFLAG_NOSTRETCH )
			with = child->MinWith;
		else
			with = dynWith;
	

		if(with < child->MinWith)	with = child->MinWith;
		if(cross < child->MinCross)	cross = child->MinCross;
		
		// Update the dimensions if they have changed
		if( Element->Flags & ELEFLAG_VERTICAL ) {
			// If no change, don't recurse
			if( child->CachedW == cross && child->CachedH == with )
				continue ;
			child->CachedW = cross;
			child->CachedH = with;
		}
		else {
			// If no change, don't recurse
			if( child->CachedW == with && child->CachedH == cross )
				continue ;
			child->CachedW = with;
			child->CachedH = cross;
		}
		
		// Force the positions of child elements to be recalculated
		child->CachedX = -1;
	
		// Recurse down so the child elements can be updated	
		Widget_UpdateDimensions(child);
	}
	
}

/**
 * \brief Update the position of child elements
 */
void Widget_UpdatePosition(tElement *Element)
{
	tElement	*child;
	 int	x, y;
	
	if( Element->Flags & ELEFLAG_NORENDER )	return ;

	_SysDebug("Widget_UpdatePosition: (Element=%p(%i Type=%i Flags=0x%x))",
		Element, Element->ID, Element->Type, Element->Flags);
	
	// Initialise
	x = Element->CachedX + Element->PaddingL;
	y = Element->CachedY + Element->PaddingT;
	
	// Update each child
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		 int	newX, newY;
		// Ignore elements that will not be rendered
		if( child->Flags & ELEFLAG_NORENDER )	continue ;

		newX = x; newY = y;
		
		// Handle alignment
		if( Element->Flags & ELEFLAG_ALIGN_CENTER ) {
			if(Element->Flags & ELEFLAG_VERTICAL)
				newX += Element->CachedW/2 - child->CachedW/2;
			else
				newY += Element->CachedH/2 - child->CachedH/2;
		}
		else if( Element->Flags & ELEFLAG_ALIGN_END ) {
			if(Element->Flags & ELEFLAG_VERTICAL )
				newX += Element->CachedW - child->CachedW
					- Element->PaddingL - Element->PaddingR;
			else
				newY += Element->CachedH - child->CachedH
					- Element->PaddingT - Element->PaddingB;
		}

		_SysDebug(" Widget_UpdatePosition[%i]: newX = %i, newY = %i", Element->ID, newX, newY);

		// Check for changes, and don't update if there was no change
		if( newX != child->CachedX || newY != child->CachedY )
		{
			child->CachedX = newX;
			child->CachedY = newY;
			// Update child's children positions
			Widget_UpdatePosition(child);
		}
		
		// Increment
		if(Element->Flags & ELEFLAG_VERTICAL ) {
			y += child->CachedH + Element->GapSize;
		}
		else {
			x += child->CachedW + Element->GapSize;
		}
	}
}

/**
 * \brief Update the minimum dimensions of the element
 * \note Called after a child's minimum dimensions have changed
 */
void Widget_UpdateMinDims(tElement *Element)
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
	Widget_UpdateMinDims(Element->Parent);
}


// --- Helpers ---
tElement *Widget_GetElementById(tWidgetWin *Info, uint32_t ID)
{
	tElement	*ele;

	if( ID == -1 )	return &Info->RootElement;
	
	if( ID < Info->TableSize )	return Info->ElementTable[ID];

	ele = Info->ElementTable[ID % Info->TableSize];
	while(ele && ele->ID != ID)	ele = ele->ListNext;
	return ele;
}

// --- Message Handlers ---
void Widget_NewWidget(tWidgetWin *Info, size_t Len, tWidgetMsg_Create *Msg)
{
	const int	max_debugname_len = Len - sizeof(tWidgetMsg_Create);
	tElement	*parent, *new;

	// Sanity check
	if( Len < sizeof(tWidgetMsg_Create) )
		return ;
	if( strnlen(Msg->DebugName, max_debugname_len) == max_debugname_len )
		return ;
	
	_SysDebug("Widget_NewWidget (%i %i Type %i Flags 0x%x)",
		Msg->Parent, Msg->NewID, Msg->Type, Msg->Flags);
	
	// Create
	parent = Widget_GetElementById(Info, Msg->Parent);
	if(!parent)
	{
		_SysDebug("Widget_NewWidget - Bad parent ID %i", Msg->Parent);
		return ;
	}

	// Check if the ID is already in use
	if( Widget_GetElementById(Info, Msg->NewID) )
		return ;

	// Create new element
	new = calloc(sizeof(tElement), 1);
	new->ID = Msg->NewID;
	new->Type = Msg->Type;
	new->Parent = parent;
	new->Flags = Msg->Flags;
	new->PaddingT = 2;
	new->PaddingB = 2;
	new->PaddingL = 2;
	new->PaddingR = 2;
	new->CachedX = -1;
	
	if( new->Type < ciWM_NumWidgetTypes && gaWM_WidgetTypes[new->Type].Init )
		gaWM_WidgetTypes[new->Type].Init(new);
	
	// Add to parent's list
	if(parent->LastChild)
		parent->LastChild->NextSibling = new;
	else
		parent->FirstChild = new;
	parent->LastChild = new;

	// Add to info
	{
		tElement	*ele, *prev = NULL;
		for(ele = Info->ElementTable[new->ID % Info->TableSize]; ele; prev = ele, ele = ele->ListNext);
		if(prev)
			prev->ListNext = new;
		else
			Info->ElementTable[new->ID % Info->TableSize] = new;
	}

	Widget_UpdateMinDims(parent);
}

void Widget_SetFlags(tWidgetWin *Info, int Len, tWidgetMsg_SetFlags *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(tWidgetMsg_SetFlags) )
		return ;

	_SysDebug("Widget_SetFlags: (%i 0x%x 0x%x)", Msg->WidgetID, Msg->Value, Msg->Mask);
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return;

	Msg->Value &= Msg->Mask;
	
	ele->Flags &= ~Msg->Mask;
	ele->Flags |= Msg->Value;
}

void Widget_SetSize(tWidgetWin *Info, int Len, tWidgetMsg_SetSize *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(tWidgetMsg_SetSize) )
		return ;
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return ;
	
	ele->FixedWith = Msg->Value;
}

void Widget_SetText(tWidgetWin *Info, int Len, tWidgetMsg_SetText *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(tWidgetMsg_SetText) + 1 )
		return ;
	if( Msg->Text[Len - sizeof(tWidgetMsg_SetText) - 1] != '\0' )
		return ;

	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return ;


	if( ele->Type < ciWM_NumWidgetTypes && gaWM_WidgetTypes[ele->Type].UpdateText )
	{
		gaWM_WidgetTypes[ele->Type].UpdateText( ele, Msg->Text );
	}
//	else
//	{
//		if(ele->Text)	free(ele->Text);
//		ele->Text = strdup(Msg->Text);
//	}
}

int Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data)
{
	tWidgetWin	*info = Target->RendererInfo;
	switch(Msg)
	{
	case WNDMSG_RESIZE: {
		struct sWndMsg_Resize	*msg = Data;
		if(Len < sizeof(*msg))	return -1;		

		info->RootElement.CachedW = msg->W;		
		info->RootElement.CachedH = msg->H;
		return 0; }

	// New Widget
	case MSG_WIDGET_CREATE:
		Widget_NewWidget(info, Len, Data);
		return 0;

	case MSG_WIDGET_DELETE:
		_SysDebug("TODO: Implement MSG_WIDGET_DELETE");
		return 0;

	// Set Flags
	case MSG_WIDGET_SETFLAGS:
		Widget_SetFlags(info, Len, Data);
		return 0;
	
	// Set length
	case MSG_WIDGET_SETSIZE:
		Widget_SetSize(info, Len, Data);
		return 0;
	
	// Set text
	case MSG_WIDGET_SETTEXT:
		Widget_SetText(info, Len, Data);
		return 0;
	
	// 
	default:
		return 1;	// Unhandled, pass to user
	}
}

// --- Type Helpers
void Widget_TextBox_UpdateText(tElement *Element, const char *Text)
{
	 int	w=0, h=0;

	if(Element->Text)	free(Element->Text);
	Element->Text = strdup(Text);

	WM_Render_GetTextDims(NULL, Element->Text, &w, &h);
	if(Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL)) {
		Element->MinCross = w;
		Element->MinWith = h;
	}
	else {
		Element->MinWith = w;
		Element->MinCross = h;
	}

	Widget_UpdateMinDims(Element->Parent);
}

void Widget_Image_UpdateText(tElement *Element, const char *Text)
{
	if(Element->Data)	free(Element->Data);
	Element->Data = Image_Load( Text );
	if(!Element->Data) {
//		Element->Flags &= ~ELEFLAG_FIXEDSIZE;
		return ;
	}
	
	Element->CachedW = ((tImage*)Element->Data)->Width;
	Element->CachedH = ((tImage*)Element->Data)->Height;
	
	if(Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL) ) {
		Element->MinCross = ((tImage*)Element->Data)->Width;
		Element->MinWith = ((tImage*)Element->Data)->Height;
	}
	else {
		Element->MinWith = ((tImage*)Element->Data)->Width;
		Element->MinCross = ((tImage*)Element->Data)->Height;
	}

	Widget_UpdateMinDims(Element->Parent);
	
	// NOTE: Doesn't update Element->Text because it's useless
}


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

#define DEFAULT_ELETABLE_SIZE	64

// === PROTOTYPES ===
 int	Renderer_Widget_Init(void);
tWindow	*Renderer_Widget_Create(int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);
void	Widget_RenderWidget(tWindow *Window, tElement *Element);
 int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);
void	Widget_UpdateDimensions(tElement *Element);
void	Widget_UpdatePosition(tElement *Element);
tElement	*Widget_GetElementById(tWidgetWin *Info, uint32_t ID);
void	Widget_NewWidget(tWidgetWin *Info, size_t Len, tWidgetMsg_Create *Msg);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Widget",
	.CreateWindow = Renderer_Widget_Create,
	.Redraw = Renderer_Widget_Redraw,
	.HandleMessage = Renderer_Widget_HandleMessage
};

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

	// TODO: Use `Flags` as default element count?

	ret = WM_CreateWindowStruct( sizeof(tWidgetWin) + sizeof(tElement*)*eletable_size );
	info = ret->RendererInfo;
	
	info->TableSize = eletable_size;
	info->RootElement.BackgroundColour = 0xCCCCCC;
	
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
		if( child->FixedWith)
			with = child->FixedWith;
		else if( child->Flags & ELEFLAG_NOSTRETCH )
			with = child->MinWith;
		else
			with = dynWith;
		
		
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


// --- Helpers ---
tElement *Widget_GetElementById(tWidgetWin *Info, uint32_t ID)
{
	tElement	*ele;

	if(ID == -1)
		return &Info->RootElement;
	
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
	
	// Add to parent's list
	if(parent->LastChild)
		parent->LastChild->NextSibling = new;
	else
		parent->FirstChild = new;
	new->NextSibling = parent->LastChild;
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


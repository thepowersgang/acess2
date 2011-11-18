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
#include "widget/common.h"

#define DEFAULT_ELETABLE_SIZE	64
#define BORDER_EVERYTHING	1

// === PROTOTYPES ===
 int	Renderer_Widget_Init(void);
tWindow	*Renderer_Widget_Create(int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);

void	Widget_RenderWidget(tWindow *Window, tElement *Element);
void	Widget_UpdateDimensions(tElement *Element);
void	Widget_UpdatePosition(tElement *Element);
// --- Messages
tElement	*Widget_GetElementById(tWidgetWin *Info, uint32_t ID);
void	Widget_NewWidget(tWidgetWin *Info, size_t Len, const tWidgetMsg_Create *Msg);
void	Widget_SetFlags(tWidgetWin *Info, int Len, const tWidgetMsg_SetFlags *Msg);
void	Widget_SetSize(tWidgetWin *Info, int Len, const tWidgetMsg_SetSize *Msg);
void	Widget_SetText(tWidgetWin *Info, int Len, const tWidgetMsg_SetText *Msg);
 int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Widget",
	.CreateWindow = Renderer_Widget_Create,
	.Redraw = Renderer_Widget_Redraw,
	.HandleMessage = Renderer_Widget_HandleMessage
};
	
// --- Element callbacks
tWidgetDef	*gaWM_WidgetTypes[NUM_ELETYPES];
const int	ciWM_NumWidgetTypes = sizeof(gaWM_WidgetTypes)/sizeof(gaWM_WidgetTypes[0]);
tWidgetDef	gWidget_NullWidgetDef;

// === CODE ===
int Renderer_Widget_Init(void)
{
	 int	i;
	WM_RegisterRenderer(&gRenderer_Widget);	

	for(i = 0; i < ciWM_NumWidgetTypes; i ++)
	{
		if(gaWM_WidgetTypes[i] != NULL)	continue;
		
		gaWM_WidgetTypes[i] = &gWidget_NullWidgetDef;
	}

	return 0;
}

void Widget_int_SetTypeDef(int Type, tWidgetDef *Ptr)
{
	if( Type < 0 || Type >= ciWM_NumWidgetTypes ) {
		_SysDebug("ERROR - Widget ID %i out of range (from %p)",
			Type, __builtin_return_address(0)
			);
		return ;
	}
	
	if( gaWM_WidgetTypes[Type] && gaWM_WidgetTypes[Type] != &gWidget_NullWidgetDef )
	{
		_SysDebug("ERROR - Widget ID %i redefined by %p",
			Type, __builtin_return_address(0)
			);
		return ;
	}
	
	gaWM_WidgetTypes[Type] = Ptr;
	_SysDebug("Registered type %i to %p", Type, Ptr);
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
	info->RootElement.Window = ret;
	info->RootElement.ID = -1;
	info->RootElement.BackgroundColour = 0xCCCCCC;
	info->RootElement.Flags = Flags;
	info->RootElement.PaddingT = 2;
	info->RootElement.PaddingB = 2;
	info->RootElement.PaddingL = 2;
	info->RootElement.PaddingR = 2;
	
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

	#if BORDER_EVERYTHING
	WM_Render_DrawRect(
		Window,
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		0
		);
	#endif	

	if(gaWM_WidgetTypes[Element->Type]->Render)
	{
		gaWM_WidgetTypes[Element->Type]->Render(Window, Element);
	}
	else
	{
		Widget_Decorator_RenderWidget(Window, Element);
	}
	
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
	 int	fullCross, dynWith = 0;

	// Check if this element can have children
	if( (gaWM_WidgetTypes[Element->Type]->Flags & WIDGETTYPE_FLAG_NOCHILDREN) )
		return ;
	
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
		dynWith -= Element->GapSize * (nChildren-1);
		if( dynWith < 0 )	return ;
		dynWith /= nChildren - nFixed;
	}
	else {
		dynWith = 0;
	}
	
	// Get the cross size
	if( Element->Flags & ELEFLAG_VERTICAL )
		fullCross = Element->CachedW - Element->PaddingL - Element->PaddingR;
	else
		fullCross = Element->CachedH - Element->PaddingT - Element->PaddingB;

	_SysDebug("%i (p=%i) - WxH=%ix%i",
		Element->ID, (Element->Parent ? Element->Parent->ID : -1),
		Element->CachedW, Element->CachedH
		);
	_SysDebug("  %s dynWith = %i, fullCross = %i",
		(Element->Flags & ELEFLAG_VERTICAL ? "Vert" : "Horiz"),
		dynWith, fullCross
		);
	
	// Pass 2 - Set sizes and recurse
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	cross, with;

		// Ignore elements that will not be rendered
		if( child->Flags & ELEFLAG_NORENDER )	continue ;
		// Don't resize floating elements
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )	continue ;
		
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
	
		_SysDebug("with = %i", with);
	
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

	// Check if this element can have children
	if( (gaWM_WidgetTypes[Element->Type]->Flags & WIDGETTYPE_FLAG_NOCHILDREN) )
		return ;

//	_SysDebug("Widget_UpdatePosition: (Element=%p(%i Type=%i Flags=0x%x))",
//		Element, Element->ID, Element->Type, Element->Flags);
	
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
		
		// Handle alignment (across parent)
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

//		_SysDebug(" Widget_UpdatePosition[%i]: newX = %i, newY = %i", Element->ID, newX, newY);

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
	 int	minW, minH;
	
	if(!Element)	return;
	
	minW = 0;
	minH = 0;
	
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		if( (Element->Flags & ELEFLAG_VERTICAL) )
		{
			if(child->FixedCross)
				minW += child->FixedCross;
			else
				minW += child->MinCross;
			if(child->FixedWith)
				minH += child->FixedWith;
			else
				minH += child->MinWith;
		}
		else
		{
			if(child->FixedCross)
				minH += child->FixedCross;
			else
				minH += child->MinCross;
			if(child->FixedWith)
				minW += child->FixedWith;
			else
				minW += child->MinWith;
		}
	}
	
	if( Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL) )
	{
		Element->MinCross = Element->PaddingL + minW + Element->PaddingR;
		Element->MinWith  = Element->PaddingT + minH + Element->PaddingB;
	}
	else
	{
		Element->MinWith  = Element->PaddingL + minW + Element->PaddingR;
		Element->MinCross = Element->PaddingL + minH + Element->PaddingR;
	}

	// Recurse upwards
	Widget_UpdateMinDims(Element->Parent);
}

tElement *Widget_GetElementByPos(tWidgetWin *Info, int X, int Y)
{
	tElement	*ret, *next, *ele;
	
	next = &Info->RootElement;
	while(next)
	{
		ret = next;
		next = NULL;
		for(ele = ret->FirstChild; ele; ele = ele->NextSibling)
		{
			if(ele->Flags & ELEFLAG_NORENDER)	continue;
			if(X < ele->CachedX)	continue;
			if(Y < ele->CachedY)	continue;
			if(X >= ele->CachedX + ele->CachedW)	continue;
			if(Y >= ele->CachedY + ele->CachedH)	continue;
			next = ele;
		}
	}
	return ret;
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
void Widget_NewWidget(tWidgetWin *Info, size_t Len, const tWidgetMsg_Create *Msg)
{
	const int	max_debugname_len = Len - sizeof(tWidgetMsg_Create);
	tElement	*parent, *new;

	// Sanity check
	if( Len < sizeof(*Msg) )
		return ;
	if( strnlen(Msg->DebugName, max_debugname_len) == max_debugname_len )
		return ;
	
	_SysDebug("Widget_NewWidget (%i %i Type %i Flags 0x%x)",
		Msg->Parent, Msg->NewID, Msg->Type, Msg->Flags);
	
	if(Msg->Type >= ciWM_NumWidgetTypes)
	{
		_SysDebug("Widget_NewWidget - Bad widget type %i", Msg->Type);
		return ;
	}

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
	new->Window = parent->Window;
	new->ID = Msg->NewID;
	new->Type = Msg->Type;
	new->Parent = parent;
	new->Flags = Msg->Flags;
	new->PaddingT = 2;
	new->PaddingB = 2;
	new->PaddingL = 2;
	new->PaddingR = 2;
	new->CachedX = -1;
	
	if( gaWM_WidgetTypes[new->Type]->Init )
		gaWM_WidgetTypes[new->Type]->Init(new);
	
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

void Widget_SetFlags(tWidgetWin *Info, int Len, const tWidgetMsg_SetFlags *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(*Msg) )
		return ;

	_SysDebug("Widget_SetFlags: (%i 0x%x 0x%x)", Msg->WidgetID, Msg->Value, Msg->Mask);
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return;

	ele->Flags &= ~Msg->Mask;
	ele->Flags |= Msg->Value & Msg->Mask;
}

void Widget_SetSize(tWidgetWin *Info, int Len, const tWidgetMsg_SetSize *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(*Msg) )
		return ;
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return ;
	
	ele->FixedWith = Msg->Value;
}

void Widget_SetText(tWidgetWin *Info, int Len, const tWidgetMsg_SetText *Msg)
{
	tElement	*ele;
	
	if( Len < sizeof(*Msg) + 1 )
		return ;
	if( Msg->Text[Len - sizeof(*Msg) - 1] != '\0' )
		return ;

	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return ;


	if( gaWM_WidgetTypes[ele->Type]->UpdateText )
	{
		gaWM_WidgetTypes[ele->Type]->UpdateText( ele, Msg->Text );
	}
//	else
//	{
//		if(ele->Text)	free(ele->Text);
//		ele->Text = strdup(Msg->Text);
//	}
}

int Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	tWidgetWin	*info = Target->RendererInfo;
	switch(Msg)
	{
	case WNDMSG_RESIZE: {
		const struct sWndMsg_Resize	*msg = Data;
		if(Len < sizeof(*msg))	return -1;		

		info->RootElement.CachedW = msg->W;		
		info->RootElement.CachedH = msg->H;
		
		// TODO: Update dimensions of all child elements?
		
		return 0; }

	case WNDMSG_MOUSEMOVE: {
//		_SysDebug("TODO: Support widget mouse move events");
		return 0; }

	case WNDMSG_MOUSEBTN: {
		const struct sWndMsg_MouseButton	*msg = Data;
		tWidgetMsg_MouseBtn	client_msg;
		tElement	*ele;
		 int	x, y;
		 int	rv;
		
		if(Len < sizeof(*msg))	return -1;

		x = msg->X; y = msg->Y;
		client_msg.Button = msg->Button;
		client_msg.bPressed = msg->bPressed;

		ele = Widget_GetElementByPos(info, x, y);
		// Send event to all elements from `ele` upwards
		for( ; ele; ele = ele->Parent )
		{
			if(gaWM_WidgetTypes[ele->Type]->MouseButton)
			{
				rv = gaWM_WidgetTypes[ele->Type]->MouseButton(
					ele,
					x - ele->CachedX, y - ele->CachedY,
					msg->Button, msg->bPressed
					);
				// Allow a type to trap the input from going any higher
				if(rv == 0)	break;
			}
			else
			{
				// Pass to user
				client_msg.X = x - ele->CachedX;
				client_msg.Y = y - ele->CachedY;
				client_msg.WidgetID = ele->ID;
				WM_SendMessage(Target, Target, MSG_WIDGET_MOUSEBTN, sizeof(client_msg), &client_msg);
			}
		}
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

void Widget_Fire(tElement *Element)
{
	tWidgetMsg_Fire	msg;
	msg.WidgetID = Element->ID;
	WM_SendMessage(Element->Window, Element->Window, MSG_WIDGET_FIRE, sizeof(msg), &msg);
}


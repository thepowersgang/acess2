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
#define BORDER_EVERYTHING	0

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
}

tWindow	*Renderer_Widget_Create(int Flags)
{
	tWindow	*ret;
	tWidgetWin	*info;
	 int	eletable_size = DEFAULT_ELETABLE_SIZE;

	//_SysDebug("Renderer_Widget_Create: (Flags = 0x%x)", Flags);

	// TODO: Use `Flags` as default element count?
	// - Actaully, it's taken by the root ele flags
	// - Use the upper bits?

	ret = WM_CreateWindowStruct( sizeof(tWidgetWin) + sizeof(tElement*)*eletable_size );
	info = ret->RendererInfo;
	
	info->TableSize = eletable_size;
	info->FocusedElement = &info->RootElement;
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
	WM_Render_FillRect(Window, 0, 0, Window->W, Window->H, info->RootElement.BackgroundColour);

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
	 int	bVertical = Element->Flags & ELEFLAG_VERTICAL;

	// Check if this element can have children
	if( (gaWM_WidgetTypes[Element->Type]->Flags & WIDGETTYPE_FLAG_NOCHILDREN) )
		return ;
	
	// Pass 1
	// - Get the fixed and minimum sizes of the element
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	minWith  = bVertical ? child->MinH : child->MinW;
		 int	minCross = bVertical ? child->MinW : child->MinH;

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
			fixedSize += minWith;
		}
		
		if( maxCross < child->FixedCross )	maxCross = child->FixedCross;
		if( maxCross < minCross )	maxCross = minCross;
		nChildren ++;
	}

	// Get the dynamic with size from the unused space in the element
	if( nChildren > nFixed ) {
		if( bVertical )
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
	if( bVertical )
		fullCross = Element->CachedW - Element->PaddingL - Element->PaddingR;
	else
		fullCross = Element->CachedH - Element->PaddingT - Element->PaddingB;

	//_SysDebug("%i (p=%i) - WxH=%ix%i",
	//	Element->ID, (Element->Parent ? Element->Parent->ID : -1),
	//	Element->CachedW, Element->CachedH
	//	);
	//_SysDebug("  %s dynWith = %i, fullCross = %i",
	//	(Element->Flags & ELEFLAG_VERTICAL ? "Vert" : "Horiz"),
	//	dynWith, fullCross
	//	);
	
	// Pass 2 - Set sizes and recurse
	for( child = Element->FirstChild; child; child = child->NextSibling )
	{
		 int	w, h;

		// Ignore elements that will not be rendered
		if( child->Flags & ELEFLAG_NORENDER )	continue ;
		// Don't resize floating elements
		if( child->Flags & ELEFLAG_ABSOLUTEPOS )	continue ;
		
		// --- Width ---
		if( child->Flags & ELEFLAG_NOEXPAND )
			w = child->MinW;
		else if( bVertical )
			w = child->FixedCross ? child->FixedCross : fullCross;
		else
			w = child->FixedWith ? child->FixedWith : dynWith;
	
		// --- Height ---
		if( child->Flags & ELEFLAG_NOSTRETCH )
			h = child->MinH;
		else if( bVertical )
			h = child->FixedWith ? child->FixedWith : dynWith;
		else
			h = child->FixedCross ? child->FixedCross : fullCross;

		if(w < child->MinW)	w = child->MinW;
		if(h < child->MinH)	h = child->MinH;
	
//		_SysDebug("Child %ix%i (min %ix%i)", w, h, child->MinW, child->MinH);
	
		// Update the dimensions if they have changed
		if( child->CachedW == w && child->CachedH == h )
			continue ;
		child->CachedW = w;
		child->CachedH = h;
		
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
	 int	nChildren;
	
	if(!Element)	return;
	
	minW = 0;
	minH = 0;
	nChildren = 0;
	
	for(child = Element->FirstChild; child; child = child->NextSibling)
	{
		 int	cross;
		
		if(Element->Flags & ELEFLAG_NORENDER)	continue ;
		
		if( (Element->Flags & ELEFLAG_VERTICAL) )
		{
			cross = child->FixedCross ? child->FixedCross : child->MinW;
			if(minW < cross)	minW = cross;
			minH += child->FixedWith  ? child->FixedWith  : child->MinH;
		}
		else
		{
			cross = child->FixedCross ? child->FixedCross : child->MinH;
			minW += child->FixedWith  ? child->FixedWith  : child->MinW;
			if(minH < cross)	minH = cross;
		}
//		_SysDebug("%i/%i cross = %i", Element->ID, child->ID, cross);
	
		nChildren ++;
	}
	
	if( Element->Flags & ELEFLAG_VERTICAL )
		minH += (nChildren - 1) * Element->GapSize;
	else
		minW += (nChildren - 1) * Element->GapSize;

	Element->MinW = Element->PaddingL + minW + Element->PaddingR;
	Element->MinH = Element->PaddingT + minH + Element->PaddingB;

	// Recurse upwards
	Widget_UpdateMinDims(Element->Parent);
}

tElement *Widget_GetElementByPos(tWidgetWin *Info, int X, int Y)
{
	tElement	*ret, *next, *ele;
	
	next = &Info->RootElement;
	do
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
	} while(next);
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

tElement *Widget_int_Create(tWidgetWin *Info, tElement *Parent, int ID, int Type, int Flags)
{
	if( Widget_GetElementById(Info, ID) )
		return NULL;

	// Create new element
	tElement *new = calloc(sizeof(tElement), 1);
	new->Window = Parent->Window;
	new->ID = ID;
	new->Type = Type;
	new->Parent = Parent;
	new->Flags = Flags;
	new->PaddingT = 2;
	new->PaddingB = 2;
	new->PaddingL = 2;
	new->PaddingR = 2;
	new->CachedX = -1;
	
	if( gaWM_WidgetTypes[Type]->Init )
		gaWM_WidgetTypes[Type]->Init(new);
	
	// Add to parent's list
	if(Parent->LastChild)
		Parent->LastChild->NextSibling = new;
	else
		Parent->FirstChild = new;
	Parent->LastChild = new;

	// Add to info
	{
		tElement	*ele, *prev = NULL;
		for(ele = Info->ElementTable[new->ID % Info->TableSize]; ele; prev = ele, ele = ele->ListNext);
		if(prev)
			prev->ListNext = new;
		else
			Info->ElementTable[new->ID % Info->TableSize] = new;
	}
	
	return new;
}

// --- Message Handlers ---
void Widget_NewWidget(tWidgetWin *Info, size_t Len, const tWidgetMsg_Create *Msg)
{
	const int	max_debugname_len = Len - sizeof(tWidgetMsg_Create);
	tElement	*parent;

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

	Widget_int_Create(Info, parent, Msg->NewID, Msg->Type, Msg->Flags);

	Widget_UpdateMinDims(parent);
}

void Widget_NewWidgetSubwin(tWidgetWin *Info, size_t Len, const tWidgetMsg_CreateSubWin *Msg)
{
	const int	max_debugname_len = Len - sizeof(tWidgetMsg_CreateSubWin);
	tElement	*parent, *new;

	// Sanity check
	if( Len < sizeof(*Msg) )
		return ;
	if( strnlen(Msg->DebugName, max_debugname_len) == max_debugname_len )
		return ;
	
	parent = Widget_GetElementById(Info, Msg->Parent);
	if(!parent)	return;
	if( Widget_GetElementById(Info, Msg->NewID) )	return ;
	
	new = Widget_int_Create(Info, parent, Msg->NewID, Msg->Type, Msg->Flags);
	new->Data = WM_GetWindowByID(parent->Window, Msg->WindowHandle);
	Widget_UpdateMinDims(parent);
}

void Widget_SetFocus(tWidgetWin *Info, tElement *Ele)
{
	// TODO: Callbacks

	Info->FocusedElement = Ele;
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

int Widget_GetText(tWidgetWin *Info, int Len, const tWidgetMsg_SetText *Msg)
{
	if( Len < sizeof(*Msg) )
		return 0;
	if( Len > sizeof(*Msg) )
		return 1;	// Pass to user
	
	const char	*text = NULL;
	tElement *ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(ele)
		text = ele->Text;
	
	char	buf[sizeof(tWidgetMsg_SetText) + strlen(text?text:"") + 1];
	tWidgetMsg_SetText	*omsg = (void*)buf;
	
	if( text ) {
		omsg->WidgetID = Msg->WidgetID;
		strcpy(omsg->Text, text);
	}
	else {
		omsg->WidgetID = -1;
		omsg->Text[0] = 0;
	}
	
	WM_SendMessage(Info->RootElement.Window, Info->RootElement.Window, MSG_WIDGET_GETTEXT, sizeof(buf), buf);
	return 0;
}

int Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	tWidgetWin	*info = Target->RendererInfo;
	tElement	*ele;
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
		 int	x, y;
		 int	rv;
		
		if(Len < sizeof(*msg))	return -1;

		x = msg->X; y = msg->Y;
		client_msg.Button = msg->Button;
		client_msg.bPressed = msg->bPressed;

		ele = Widget_GetElementByPos(info, x, y);
		Widget_SetFocus(info, ele);
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

	case WNDMSG_KEYDOWN: {
		const struct sWndMsg_KeyAction	*msg = Data;
		if(Len < sizeof(*msg))	return -1;
		
		if(!info->FocusedElement)	return 0;
		ele = info->FocusedElement;

		if(gaWM_WidgetTypes[ele->Type]->KeyDown)
			gaWM_WidgetTypes[ele->Type]->KeyDown(ele, msg->KeySym, msg->UCS32);
		else
		{
			// TODO: Pass to user
		}	

		return 0; }
	
	case WNDMSG_KEYFIRE: {
		const struct sWndMsg_KeyAction	*msg = Data;
		if(Len < sizeof(*msg))	return -1;
		
		if(!info->FocusedElement)	return 0;
		ele = info->FocusedElement;

		if(gaWM_WidgetTypes[ele->Type]->KeyFire)
			gaWM_WidgetTypes[ele->Type]->KeyFire(ele, msg->KeySym, msg->UCS32);
		else
		{
			// TODO: Pass the buck
		}
		return 0; }
	
	case WNDMSG_KEYUP: {
		const struct sWndMsg_KeyAction	*msg = Data;
		if(Len < sizeof(*msg))	return -1;
		
		if(!info->FocusedElement)	return 0;
		ele = info->FocusedElement;

		if(gaWM_WidgetTypes[ele->Type]->KeyUp)
			gaWM_WidgetTypes[ele->Type]->KeyUp(ele, msg->KeySym);
		else
		{
			// TODO: Pass the buck
		}
		return 0; }

	// New Widget
	case MSG_WIDGET_CREATE:
		Widget_NewWidget(info, Len, Data);
		return 0;
	case MSG_WIDGET_CREATESUBWIN:
		Widget_NewWidgetSubwin(info, Len, Data);
		return 0;

	// Delete a widget
	case MSG_WIDGET_DELETE:
		_SysDebug("TODO: Implement MSG_WIDGET_DELETE");
		return 0;

	// Set focused widget
	case MSG_WIDGET_SETFOCUS: {
		tElement	*ele;
		const tWidgetMsg_SetFocus	*msg = Data;
		if(Len < sizeof(*msg))	return -1;
		
		ele = Widget_GetElementById(info, msg->WidgetID);
		Widget_SetFocus(info, ele);
		return 0; }

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
	case MSG_WIDGET_GETTEXT:
		return Widget_GetText(info, Len, Data);
	
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


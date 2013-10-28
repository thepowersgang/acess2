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

// === IMPORTS ===
extern tWidgetDef	_widget_typedef_ELETYPE_IMAGE;
extern tWidgetDef	_widget_typedef_ELETYPE_BUTTON;
extern tWidgetDef	_widget_typedef_ELETYPE_TEXT;
extern tWidgetDef	_widget_typedef_ELETYPE_TEXTINPUT;
extern tWidgetDef	_widget_typedef_ELETYPE_SPACER;
extern tWidgetDef	_widget_typedef_ELETYPE_SUBWIN;

// === PROTOTYPES ===
 int	Renderer_Widget_Init(void);
tWindow	*Renderer_Widget_Create(int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);

void	Widget_RenderWidget(tWindow *Window, tElement *Element);
void	Widget_UpdateDimensions(tElement *Element);
void	Widget_UpdatePosition(tElement *Element);
// --- Messages
tElement	*Widget_GetElementById(tWidgetWin *Info, uint32_t ID);
 int	Widget_IPC_Create(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_NewWidgetSubwin(tWindow *Win, size_t Len, const void *Data);
// int	Widget_IPC_Delete(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_SetFocus(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_SetFlags(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_SetSize(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_SetText(tWindow *Win, size_t Len, const void *Data);
 int	Widget_IPC_GetText(tWindow *Win, size_t Len, const void *Data);
// int	Widget_IPC_SetColour(tWindow *Win, size_t Len, const void *Data);
 int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Widget",
	.CreateWindow = Renderer_Widget_Create,
	.Redraw = Renderer_Widget_Redraw,
	.HandleMessage = Renderer_Widget_HandleMessage,
	.nIPCHandlers = N_IPC_WIDGET,
	.IPCHandlers = {
		[IPC_WIDGET_CREATE] = Widget_IPC_Create,
		[IPC_WIDGET_CREATESUBWIN] = Widget_IPC_NewWidgetSubwin,
		[IPC_WIDGET_SETFOCUS] = Widget_IPC_SetFocus,
		[IPC_WIDGET_SETFLAGS] = Widget_IPC_SetFlags,
		[IPC_WIDGET_SETSIZE] = Widget_IPC_SetSize,
		[IPC_WIDGET_SETTEXT] = Widget_IPC_SetText,
		[IPC_WIDGET_GETTEXT] = Widget_IPC_GetText,
	}
};
	
// --- Element callbacks
tWidgetDef	*gaWM_WidgetTypes[NUM_ELETYPES] = {
	[ELETYPE_IMAGE]     = &_widget_typedef_ELETYPE_IMAGE,
	[ELETYPE_BUTTON]    = &_widget_typedef_ELETYPE_BUTTON,
	[ELETYPE_TEXT]      = &_widget_typedef_ELETYPE_TEXT,
	[ELETYPE_TEXTINPUT] = &_widget_typedef_ELETYPE_TEXTINPUT,
	[ELETYPE_SPACER]    = &_widget_typedef_ELETYPE_SPACER,
	[ELETYPE_SUBWIN]    = &_widget_typedef_ELETYPE_SUBWIN,
	};
const int	ciWM_NumWidgetTypes = sizeof(gaWM_WidgetTypes)/sizeof(gaWM_WidgetTypes[0]);
tWidgetDef	gWidget_NullWidgetDef;

// === CODE ===
int Renderer_Widget_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Widget);	

	for(int i = 0; i < ciWM_NumWidgetTypes; i ++)
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
	_SysDebug("Registered widget type %i '%s'", Type, Ptr->Name);
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
		if( child->Flags & (bVertical ? ELEFLAG_NOEXPAND : ELEFLAG_NOSTRETCH) )
			w = child->MinW;
		else if( bVertical )
			w = child->FixedCross ? child->FixedCross : fullCross;
		else
			w = child->FixedWith ? child->FixedWith : dynWith;
	
		// --- Height ---
		if( child->Flags & (bVertical ? ELEFLAG_NOSTRETCH : ELEFLAG_NOEXPAND) )
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
	for(tElement *child = Element->FirstChild; child; child = child->NextSibling)
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
	 int	minW, minH;
	 int	nChildren;
	
	if(!Element)	return;
	
	minW = 0;
	minH = 0;
	nChildren = 0;
	
	for( tElement *child = Element->FirstChild; child; child = child->NextSibling )
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
	tElement *ret;
	tElement *next = &Info->RootElement;
	// Scan down tree
	do {
		ret = next;
		next = NULL;
		for(tElement *ele = ret->FirstChild; ele; ele = ele->NextSibling)
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
	if( Type >= NUM_ELETYPES ) {
		return NULL;
	}
	
	_SysDebug("Widget Create #%i '%s' 0x%x",
		ID, gaWM_WidgetTypes[Type]->Name, Flags);

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

void Widget_SetFocus(tWidgetWin *Info, tElement *Ele)
{
	// TODO: Callbacks

	Info->FocusedElement = Ele;
}


// --- Message Handlers ---
int Widget_IPC_Create(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*Info = Win->RendererInfo;
	const tWidgetIPC_Create	*Msg = Data;
	const int	max_debugname_len = Len - sizeof(*Msg);
	tElement	*parent;

	// Sanity check
	if( Len < sizeof(*Msg) )
		return -1;
	if( strnlen(Msg->DebugName, max_debugname_len) == max_debugname_len )
		return -1;
	
	_SysDebug("Widget_NewWidget (%i %i Type %i Flags 0x%x)",
		Msg->Parent, Msg->NewID, Msg->Type, Msg->Flags);
	
	if(Msg->Type >= ciWM_NumWidgetTypes)
	{
		_SysDebug("Widget_NewWidget - Bad widget type %i", Msg->Type);
		return 1;
	}

	// Create
	parent = Widget_GetElementById(Info, Msg->Parent);
	if(!parent)
	{
		_SysDebug("Widget_NewWidget - Bad parent ID %i", Msg->Parent);
		return 1;
	}

	Widget_int_Create(Info, parent, Msg->NewID, Msg->Type, Msg->Flags);

	Widget_UpdateMinDims(parent);
	return 0;
}

int Widget_IPC_NewWidgetSubwin(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*Info = Win->RendererInfo;
	const tWidgetIPC_CreateSubWin	*Msg = Data;
	const int	max_debugname_len = Len - sizeof(*Msg);
	tElement	*parent, *new;

	// Sanity check
	if( Len < sizeof(*Msg) )
		return -1;
	if( strnlen(Msg->DebugName, max_debugname_len) == max_debugname_len )
		return -1;

	_SysDebug("Widget_NewWidgetSubwin(%i %i  Type %i Flags 0x%x Subwin %i)",
		Msg->Parent, Msg->NewID, Msg->Type, Msg->Flags, Msg->WindowHandle);
	
	parent = Widget_GetElementById(Info, Msg->Parent);
	if(!parent)	return 1;
	if( Widget_GetElementById(Info, Msg->NewID) )	return 1;
	
	new = Widget_int_Create(Info, parent, Msg->NewID, Msg->Type, Msg->Flags);
	new->Data = WM_GetWindowByID(parent->Window, Msg->WindowHandle);
	Widget_UpdateMinDims(parent);
	return 0;
}

// TODO: Widget_IPC_Delete

int Widget_IPC_SetFocus(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*info = Win->RendererInfo;
	tElement	*ele;
	const tWidgetIPC_SetFocus	*msg = Data;
	if(Len < sizeof(*msg))	return -1;

	_SysDebug("Widget_SetFocus(%i)", msg->WidgetID);
	
	ele = Widget_GetElementById(info, msg->WidgetID);
	Widget_SetFocus(info, ele);
	return 0;
}

int Widget_IPC_SetFlags(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin *Info = Win->RendererInfo;
	const tWidgetIPC_SetFlags	*Msg = Data;
	tElement	*ele;
	
	if( Len < sizeof(*Msg) )
		return -1;

	_SysDebug("Widget_SetFlags: (%i 0x%x 0x%x)", Msg->WidgetID, Msg->Value, Msg->Mask);
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return 1;

	ele->Flags &= ~Msg->Mask;
	ele->Flags |= Msg->Value & Msg->Mask;
	
	return 0;
}

int Widget_IPC_SetSize(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*Info = Win->RendererInfo;
	const tWidgetIPC_SetSize	*Msg = Data;
	tElement	*ele;
	
	if( Len < sizeof(*Msg) )
		return -1;

	_SysDebug("Widget_SetSize(%i, %i)", Msg->WidgetID, Msg->Value);
	
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return 1;
	
	ele->FixedWith = Msg->Value;
	return 0;
}

int Widget_IPC_SetText(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*Info = Win->RendererInfo;
	const tWidgetIPC_SetText	*Msg = Data;
	tElement	*ele;
	
	if( Len < sizeof(*Msg) + 1 )
		return -1;
	if( Msg->Text[Len - sizeof(*Msg) - 1] != '\0' )
		return -1;

	_SysDebug("Widget_SetText(%i, '%.30s')", Msg->WidgetID, Msg->Text);
	ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(!ele)	return 1;

	if( gaWM_WidgetTypes[ele->Type]->UpdateText )
	{
		_SysDebug(" - calling handler");
		gaWM_WidgetTypes[ele->Type]->UpdateText( ele, Msg->Text );
	}
//	else
//	{
//		if(ele->Text)	free(ele->Text);
//		ele->Text = strdup(Msg->Text);
//	}
	return 0;
}

int Widget_IPC_GetText(tWindow *Win, size_t Len, const void *Data)
{
	tWidgetWin	*Info = Win->RendererInfo;
	const tWidgetIPC_SetText	*Msg = Data;
	if( Len < sizeof(*Msg) )
		return -1;
	
	const char	*text = NULL;
	tElement *ele = Widget_GetElementById(Info, Msg->WidgetID);
	if(ele)
		text = ele->Text;
	
	char	buf[sizeof(tWidgetIPC_SetText) + strlen(text?text:"") + 1];
	tWidgetIPC_SetText	*omsg = (void*)buf;
	
	if( text ) {
		omsg->WidgetID = Msg->WidgetID;
		strcpy(omsg->Text, text);
	}
	else {
		omsg->WidgetID = -1;
		omsg->Text[0] = 0;
	}
	
	WM_SendIPCReply(Win, IPC_WIDGET_GETTEXT, sizeof(buf), buf);
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

	// 
	default:
		return 1;	// Unhandled, pass to user
	}
}

void Widget_Fire(tElement *Element)
{
	tWidgetMsg_Fire	msg;
	msg.WidgetID = Element->ID;
	_SysDebug("Widget_Fire: Fire on %p %i", Element->Window, Element->ID);
	WM_SendMessage(Element->Window, Element->Window, MSG_WIDGET_FIRE, sizeof(msg), &msg);
}


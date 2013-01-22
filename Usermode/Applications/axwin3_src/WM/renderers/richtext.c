/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render/richtext.c
 * - Formatted Line Editor
 */
#include <common.h>
#include <wm_renderer.h>
#include <wm_messages.h>
#include <richtext_messages.h>
#include <stdio.h>	// sscanf
#include <string.h>	// memcpy

#define LINES_PER_BLOCK	30

// === TYPES ===
typedef struct sRichText_Line
{
	struct sRichText_Line	*Next;
	struct sRichText_Line	*Prev;
	 int	Num;
	// TODO: Pre-rendered cache?
	short	ByteLength;
	short	Space;
	char	Data[];
} tRichText_Line;
typedef struct sRichText_Window
{
	 int	DispLines, DispCols;
	 int	FirstVisRow, FirstVisCol;
	 int	nLines, nCols;
	 int	CursorRow, CursorCol;
	tRichText_Line	*FirstLine;
	tRichText_Line	*FirstVisLine;
	tColour	DefaultFG;
	tColour	DefaultBG;
	tFont	*Font;
	
	short	LineHeight;
} tRichText_Window;

// === PROTOTYPES ===
 int	Renderer_RichText_Init(void);
tWindow	*Renderer_RichText_Create(int Flags);
void	Renderer_RichText_Redraw(tWindow *Window);
 int	Renderer_RichText_HandleIPC_SetAttr(tWindow *Window, size_t Len, const void *Data);
 int	Renderer_RichText_HandleIPC_WriteLine(tWindow *Window, size_t Len, const void *Data);
 int	Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_RichText = {
	.Name = "RichText",
	.CreateWindow	= Renderer_RichText_Create,
	.Redraw 	= Renderer_RichText_Redraw,
	.HandleMessage	= Renderer_RichText_HandleMessage,
	.nIPCHandlers = N_IPC_RICHTEXT,
	.IPCHandlers = {
		[IPC_RICHTEXT_SETATTR] = Renderer_RichText_HandleIPC_SetAttr,
		[IPC_RICHTEXT_WRITELINE] = Renderer_RichText_HandleIPC_WriteLine
	}
};

// === CODE ===
int Renderer_RichText_Init(void)
{
	WM_RegisterRenderer(&gRenderer_RichText);	
	return 0;
}

tWindow *Renderer_RichText_Create(int Flags)
{
	tRichText_Window	*info;
	tWindow *ret = WM_CreateWindowStruct( sizeof(*info) );
	if(!ret)	return NULL;
	info = ret->RendererInfo;
	
	// Initialise font (get an idea of dimensions)
	int h;
	WM_Render_GetTextDims(NULL, "yY!", 3, NULL, &h);
	info->LineHeight = h;
	
	return ret;
}

static inline int Renderer_RichText_RenderText_Act(tWindow *Window, tRichText_Window *info, int X, int Row, const char *Text, int Bytes, tColour FG, tColour BG, int Flags)
{
	 int	rwidth;
	// TODO: Fill only what is needed? What about the rest of the line?
	WM_Render_DrawRect(Window, X, Row*info->LineHeight,
		Window->W - X, info->LineHeight,
		BG
		);
	// TODO: Bold, Italic, Underline
	rwidth = WM_Render_DrawText(Window,
		X, Row*info->LineHeight,
		Window->W - X, info->LineHeight,
		info->Font, FG,
		Text, Bytes
		);
	return rwidth;
}

void Renderer_RichText_RenderText(tWindow *Window, int Line, const char *Text)
{
	tRichText_Window	*info = Window->RendererInfo;
	tColour	fg = info->DefaultFG;
	tColour	bg = info->DefaultBG;
	 int	flagset = 0;
	 int	bRender = 0;
	 int	curx = 0;
	const char	*oldtext = Text;
	
	for( int i = 0; curx < Window->W; i ++ )
	{
		char	ch, flags;
		 int	len;

		if( i == info->FirstVisCol )
			bRender = 1;

		ch = *Text++;
		if( ch == 0 )	break;

		// Not an escape - move along
		if( ch > 4 )
			continue ;		

		if( bRender ) {
			// Render previous characters
			curx += Renderer_RichText_RenderText_Act(Window, info, curx, Line,
				oldtext, Text - oldtext - 1, fg, bg, flagset);
			if( curx >= Window->W )
				break;
		}
		oldtext = Text;
		switch(ch)
		{
		case 1:	// FG Select (\1 RRGGBB)
			if( sscanf(Text, "%6x%n", &fg, &len) != 1 || len != 6 ) {
				// Bad client
				_SysDebug("foreground scanf failed - len=%i", len);
				len = 0;
			}
			Text += len;
			oldtext = Text;
			_SysDebug("FG update to %x", fg);
			break ;
		case 2:	// BG Select (\2 RRGGBB)
			if( sscanf(Text, "%6x%n", &bg, &len) != 1 || len != 6 ) {
				// Bad client
				_SysDebug("background scanf failed - len=%i", len);
				len = 0;
			}
			Text += len;
			oldtext = Text;
			_SysDebug("BG update to %x", bg);
			break ;
		case 3:	// Flagset (0,it,uline,bold)
			if( sscanf(Text, "%1hhx%n", &flags, &len) != 1 || len != 1 ) {
				// Bad client
				_SysDebug("Flagset scanf failed - len=%i", len);
			}
			flagset = flags & 7;
			Text += len;
			oldtext = Text;
			break ;
		case 4:	// Escape (do nothing)
			Text ++;
			// NOTE: No update to oldtext
			break;
		default: // Error.
			break;
		}
	}
	curx += Renderer_RichText_RenderText_Act(Window, info, curx,
		Line, oldtext, Text - oldtext + 1, fg, bg, flagset);
	WM_Render_DrawRect(Window, curx, Line * info->LineHeight,
		Window->W - curx, info->LineHeight, info->DefaultBG);
}

void Renderer_RichText_Redraw(tWindow *Window)
{
	tRichText_Window	*info = Window->RendererInfo;
	 int	i;
	tRichText_Line	*line = info->FirstVisLine;
	
	if( !line ) {
		line = info->FirstLine;
		while(line && line->Num < info->FirstVisRow )
			line = line->Next;
		info->FirstVisLine = line;
	}
	while( line && line->Prev && line->Prev->Num > info->FirstVisRow )
		line = line->Prev;

	for( i = 0; i < info->DispLines && line; i ++ )
	{
		if( i >= info->nLines - info->FirstVisRow )
			break;
		// TODO: Dirty rectangles?
		WM_Render_FillRect(Window,
			0, i*info->LineHeight,
			Window->W, info->LineHeight,
			info->DefaultBG
			);
		if( line->Num > info->FirstVisRow + i )
			continue ;
		// TODO: Horizontal scrolling?
		// TODO: Formatting
		
		// Formatted text out
		Renderer_RichText_RenderText(Window, i, line->Data);
		_SysDebug("RichText: %p - Render %i '%.*s'", Window,
			line->Num, line->ByteLength, line->Data);

		line = line->Next;
	}
	// Clear out i -- info->DispLines
	_SysDebug("RichText: %p - Clear %i px lines with %06x starting at %i",
		Window, (info->DispLines-i)*info->LineHeight, info->DefaultBG, i*info->LineHeight);
	WM_Render_FillRect(Window,
		0, i*info->LineHeight,
		Window->W, (info->DispLines-i)*info->LineHeight,
		info->DefaultBG
		);

	// HACK!
	info->DispCols = Window->W / 8;	

	// TODO: Text cursor
	_SysDebug("Cursor at %i,%i", info->CursorCol, info->CursorRow);
	_SysDebug(" Range [%i+%i],[%i+%i]", info->FirstVisRow, info->DispLines, info->FirstVisCol, info->DispCols);
	if( info->CursorRow >= info->FirstVisRow && info->CursorRow < info->FirstVisRow + info->DispLines )
	{
		if( info->CursorCol >= info->FirstVisCol && info->CursorCol < info->FirstVisCol + info->DispCols )
		{
			// TODO: Kill hardcoded 8 with cached text distance
			WM_Render_FillRect(Window,
				(info->CursorCol - info->FirstVisCol) * 8,
				(info->CursorRow - info->FirstVisRow) * info->LineHeight,
				1,
				info->LineHeight,
				info->DefaultFG
				);
		}
	}
}

int Renderer_RichText_HandleIPC_SetAttr(tWindow *Window, size_t Len, const void *Data)
{
	tRichText_Window	*info = Window->RendererInfo;
	const struct sRichTextIPC_SetAttr *msg = Data;
	if(Len < sizeof(*msg))	return -1;

	_SysDebug("RichText Attr %i set to %x", msg->Attr, msg->Value);
	switch(msg->Attr)
	{
	case _ATTR_DEFBG:
		info->DefaultBG = msg->Value;
		break;
	case _ATTR_DEFFG:
		info->DefaultFG = msg->Value;
		break;
	case _ATTR_CURSORPOS:
		info->CursorRow = msg->Value >> 12;
		info->CursorCol = msg->Value & 0xFFF;
		break;
	case _ATTR_SCROLL:
		// TODO: Set scroll flag
		break;
	case _ATTR_LINECOUNT:
		info->nLines = msg->Value;
		break;
	}
	
	return 0;
}

int Renderer_RichText_HandleIPC_WriteLine(tWindow *Window, size_t Len, const void *Data)
{
	tRichText_Window	*info = Window->RendererInfo;
	const struct sRichTextIPC_WriteLine	*msg = Data;
	if( Len < sizeof(*msg) )	return -1;
	if( msg->Line >= info->nLines )	return 1;	// Bad count

	tRichText_Line	*line = info->FirstLine;
	tRichText_Line	*prev = NULL;
	while(line && line->Num < msg->Line)
		prev = line, line = line->Next;
	if( !line || line->Num > msg->Line )
	{
		// New line!
		// Round up to 32
		 int	space = ((Len - sizeof(*msg)) + 32-1) & ~(32-1);
		tRichText_Line	*new = malloc(sizeof(*line) + space);
		// TODO: Bookkeeping on how much memory each window uses
		new->Next = line;
		new->Prev = prev;
		new->Num = msg->Line;
		new->Space = space;
		if(new->Prev)	new->Prev->Next = new;
		else	info->FirstLine = new;
		if(new->Next)	new->Next->Prev = new;
		line = new;
	}
	else if( line->Space < Len - sizeof(*msg) )
	{
		// Need to allocate more space
		 int	space = ((Len - sizeof(*msg)) + 32-1) & ~(32-1);
		tRichText_Line *new = realloc(line, space);
		// TODO: Bookkeeping on how much memory each window uses
		new->Space = space;

		if(new->Prev)	new->Prev->Next = new;
		else	info->FirstLine = new;
		if(new->Next)	new->Next->Prev = new;
		line = new;
	}
	else
	{
		// It fits :)
	}
	line->ByteLength = Len - sizeof(*msg);
	memcpy(line->Data, msg->LineData, Len - sizeof(*msg));

	WM_Invalidate( Window );

	return  0;
}

int Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	tRichText_Window	*info = Target->RendererInfo;
	switch(Msg)
	{
	case WNDMSG_RESIZE: {
		const struct sWndMsg_Resize *msg = Data;
		if(Len < sizeof(*msg))	return -1;
		info->DispLines = msg->H / info->LineHeight;
		return 1; }
	case WNDMSG_KEYDOWN:
	case WNDMSG_KEYUP:
	case WNDMSG_KEYFIRE:
		return 1;
	}
	return 0;
}

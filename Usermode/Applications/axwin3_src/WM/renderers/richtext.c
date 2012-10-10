/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render/richtext.c
 * - Formatted Line Editor
 */
#include <common.h>
#include <wm_renderer.h>
#include <richtext_messages.h>
#include <stdio.h>	// sscanf

#define LINES_PER_BLOCK	30

// === TYPES ===
typedef struct sRichText_LineBlock
{
	struct sRichText_LineBlock	*Next;
	 int	FirstLine;
	char	*Lines[LINES_PER_BLOCK];
} tRichText_LineBlock;
typedef struct sRichText_Window
{
	 int	DispLines, DispCols;
	 int	FirstVisRow, FirstVisCol;
	 int	nLines, nCols;
	 int	CursorRow, CursorCol;
	tRichText_LineBlock	FirstBlock;
	tColour	DefaultFG;
	tColour	DefaultBG;
	tFont	*Font;
	
	short	LineHeight;
} tRichText_Window;

// === PROTOTYPES ===
 int	Renderer_RichText_Init(void);
tWindow	*Renderer_RichText_Create(int Flags);
void	Renderer_RichText_Redraw(tWindow *Window);
 int	Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_RichText = {
	.Name = "RichText",
	.CreateWindow	= Renderer_RichText_Create,
	.Redraw 	= Renderer_RichText_Redraw,
	.HandleMessage	= Renderer_RichText_HandleMessage
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
	// Everything starts at zero?
	return ret;
}

static inline int Renderer_RichText_RenderText_Act(tWindow *Window, tRichText_Window *info, int X, int Row, const char *Text, int Bytes, tColour FG, tColour BG)
{
	 int	rwidth;
	// TODO: Fill only what is needed
	WM_Render_DrawRect(Window, X, Row*info->LineHeight,
		Window->W - X, info->LineHeight,
		BG
		);
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
	 int	bBold = 0;
	 int	bULine = 0;
	 int	bItalic = 0;
	 int	bRender = 0;
	 int	curx = 0;
	const char	*oldtext = Text;
	
	for( int i = 0; i < info->FirstVisCol + info->DispCols; i ++ )
	{
		char	ch, flags;
		 int	len;

		if( i == info->FirstVisCol )
			bRender = 1;

		ch = *Text++;
		if( ch == 0 )	break;
		if( ch <=3 && bRender ) {
			// Render previous characters
			curx += Renderer_RichText_RenderText_Act(Window, info, curx, Line, oldtext, Text - oldtext, fg, bg);
			oldtext = Text;
		}
		switch(ch)
		{
		case 1:	// FG Select (\1 RRGGBB)
			len = sscanf(Text, "%6x", &fg);
			if( len != 6 ) {
				// Bad client
			}
			Text += len;
			break ;
		case 2:	// BG Select (\2 RRGGBB)
			len = sscanf(Text, "%6x", &bg);
			if( len != 6 ) {
				// Bad client
			}
			Text += len;
			break ;
		case 3:	// Flagset (0,it,uline,bold)
			len = sscanf(Text, "%1x", &flags);
			if( len != 1 ) {
				// Bad client
			}
			Text += len;
			bItalic = !!(flags & (1 << 2));
			bULine = !!(flags & (1 << 1));
			bBold = !!(flags & (1 << 0));
			break ;
		default: // Any char, nop
			break;
		}
	}
	curx += Renderer_RichText_RenderText_Act(Window, info, curx, Line, oldtext, Text - oldtext, fg, bg);
	WM_Render_DrawRect(Window, curx, Line * info->LineHeight, Window->W - curx, info->LineHeight, info->DefaultBG);
}

void Renderer_RichText_Redraw(tWindow *Window)
{
	tRichText_Window	*info = Window->RendererInfo;
	 int	i;
	tRichText_LineBlock	*lines = &info->FirstBlock;
	
	// Locate the first line block
	for( i = info->FirstVisRow; i > LINES_PER_BLOCK && lines; i -= LINES_PER_BLOCK )
		lines = lines->Next;

	for( i = 0; i < info->DispLines && lines; i ++ )
	{
		if( i >= info->nLines - info->FirstVisRow )
			break;
		// TODO: Dirty rectangles?
		WM_Render_DrawRect(Window,
			0, i*info->LineHeight,
			Window->W, info->LineHeight,
			info->DefaultBG
			);
		// TODO: Horizontal scrolling?
		// TODO: Formatting
		//Renderer_RichText_RenderText(Window, i, lines->Lines[i % LINES_PER_BLOCK]);
		WM_Render_DrawText(Window,
			0, i*info->LineHeight,
			Window->W, info->LineHeight,
			info->Font, info->DefaultFG,
			lines->Lines[i % LINES_PER_BLOCK],
			-1);
	
		if( (i + 1) % LINES_PER_BLOCK == 0 )
			lines = lines->Next;
	}
	// Clear out i -- info->DispLines
	WM_Render_DrawRect(Window,
		0, i*info->LineHeight,
		Window->W, (info->DispLines-i)*info->LineHeight,
		info->DefaultBG
		);
}

int Renderer_RichText_HandleMessage(tWindow *Target, int Msg, int Len, const void *Data)
{
	switch(Msg)
	{
	case MSG_RICHTEXT_SETATTR:
		break;
	}
	return 0;
}

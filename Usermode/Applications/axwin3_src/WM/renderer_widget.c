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

// === TYPES ===
typedef struct sWidgetWin	tWidgetWin;
typedef struct sAxWin_Element	tElement;

// === STRUCTURES ===
struct sAxWin_Element
{
	enum eElementTypes	Type;
	
	// Element Tree
	tElement	*Parent;
	tElement	*FirstChild;
	tElement	*LastChild;
	tElement	*NextSibling;
	
	// Application
	tApplication	*Owner;	//!< Owning application
	uint16_t	ApplicationID;	//!< Index into sApplication::EleIndex

	// User modifiable attributes	
	short	PaddingL, PaddingR;
	short	PaddingT, PaddingB;
	short	GapSize;
	
	uint32_t	Flags;
	
	short	FixedWith;	//!< Fixed lengthways Size attribute (height)
	short	FixedCross;	//!< Fixed Cross Size attribute (width)
	
	char	*Text;
	
	// -- Attributes maitained by the element code
	// Not touched by the user
	short	MinWith;	//!< Minimum long size
	short	MinCross;	//!< Minimum cross size
	void	*Data;	//!< Per-type data
	
	// -- Render Cache
	short	CachedX, CachedY;
	short	CachedW, CachedH;
	
	char	DebugName[];
};
struct sWidgetWin
{
	tElement	RootElement;
};

// === PROTOTYPES ===
tWindow	*Renderer_Widget_Create(int Width, int Height, int Flags);
void	Renderer_Widget_Redraw(tWindow *Window);
int	Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data);

// === GLOBALS ===
tWMRenderer	gRenderer_Widget = {
	.Name = "Classful",
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

tWindow	*Renderer_Widget_Create(int Width, int Height, int Flags)
{
	// TODO: Add info
	return WM_CreateWindowStruct( sizeof(tWidgetWin) );
}

void Renderer_Widget_Redraw(tWindow *Window)
{
}

int Renderer_Widget_HandleMessage(tWindow *Target, int Msg, int Len, void *Data)
{
	tWidgetWin	*info = Target->RendererInfo;
	switch(Msg)
	{
	default:
		return 1;	// Unhandled
	}
}


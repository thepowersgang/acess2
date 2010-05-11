
#ifndef _WM_H_
#define _WM_H_

typedef struct sElement
{
	 int	Type;
	
	struct sElement	*Parent;
	struct sElement	*FirstChild;
	struct sElement	*LastChild;
	struct sElement	*NextSibling;
	
	short	PaddingL, PaddingR;
	short	PaddingT, PaddingB;
	short	GapSize;
	
	short	FixedWith;	// Fixed Long Size attribute (height)
	short	FixedCross;	// Fixed Cross Size attribute (width)
	
	char	*Text;
	
	// -- Attributes maitained by the element code
	// Not touched by the user
	short	MinWith;	// Minimum long size
	short	MinCross;	// Minimum cross size
	void	*Data;
	
	uint32_t	Flags;
	
	// -- Render Cache
	short	CachedX, CachedY;
	short	CachedW, CachedH;
}	tElement;

typedef struct sTab
{
	 int	Type;	// Should be zero, allows a tab to be the parent of an element
	
	struct sElement	*Parent;
	struct sElement	*FirstChild;
	struct sElement	*LastChild;
	
	char	*Name;
	
	tElement	*RootElement;
}	tTab;

typedef struct sApplication
{
	pid_t	PID;
	
	 int	nTabs;
	tTab	*Tabs;
	
	char	Name[];
}	tApplication;

// === CONSTANTS ===
enum eElementFlags
{
	/**
	 * \brief Rendered
	 * 
	 * If set, the element will be ignored in calculating sizes and
	 * rendering.
	 */
	ELEFLAG_NORENDER    = 0x001,
	/**
	 * \brief Element visibility
	 * 
	 * If set, the element is not drawn.
	 */
	ELEFLAG_INVISIBLE   = 0x002,
	
	/**
	 * \brief Position an element absulutely
	 */
	ELEFLAG_ABSOLUTEPOS = 0x004,
	
	/**
	 * \brief Fixed size element
	 */
	ELEFLAG_FIXEDSIZE   = 0x008,
	
	/**
	 * \brief Element "orientation"
	 */
	ELEFLAG_VERTICAL    = 0x010,//	ELEFLAG_HORIZONTAL  = 0x000,
	/**
	 * \brief Action for text that overflows
	 */
	ELEFLAG_WRAP        = 0x020,//	ELEFLAG_NOWRAP      = 0x000,
	/**
	 * \brief Cross size action
	 * 
	 * If this flag is set, the element will only be as large (across
	 * its parent) as is needed to encase the contents of the element.
	 * Otherwise, the element will expand to fill all avaliable space.
	 */
	ELEFLAG_NOEXPAND    = 0x040,
	
	ELEFLAG_NOSTRETCH   = 0x080,
	
	/**
	 * \brief Center alignment
	 */
	ELEFLAG_ALIGN_CENTER= 0x100,
	/**
	 * \brief Right/Bottom alignment
	 */
	ELEFLAG_ALIGN_END = 0x200
};

/**
 */
enum eElementTypes
{
	ELETYPE_NONE,
	
	ELETYPE_BOX,	//!< Content box
	ELETYPE_TABBAR,	//!< Tab Bar
	ELETYPE_TOOLBAR,	//!< Tool Bar
	
	ELETYPE_BUTTON,	//!< Push Button
	ELETYPE_TEXT,	//!< Text
	ELETYPE_IMAGE,	//!< Image
	
	ELETYPE_SPACER,	//!< Visual Spacer
	
	MAX_ELETYPES	= 0x100
};

// === FUNCTIONS ===
/**
 * \brief Create a new element as a child of \a Parent
 */
extern tElement	*WM_CreateElement(tElement *Parent, int Type, int Flags);
extern void	WM_SetFlags(tElement *Element, int Flags);
extern void	WM_SetSize(tElement *Element, int Size);
extern void	WM_SetText(tElement *Element, char *Text);

#endif

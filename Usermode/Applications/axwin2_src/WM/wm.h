
#ifndef _WM_H_
#define _WM_H_

typedef struct sElement
{
	 int	Type;
	
	struct sElement	*Parent;
	struct sElement	*FirstChild;
	struct sElement	*LastChild;
	struct sElement	*NextSibling;
	
	
	short	CachedX;
	short	CachedY;
	short	CachedW;
	short	CachedH;
	
	short	Size;	// Size attribute
	
	char	*Text;
	void	*Data;
	
	uint32_t	Flags;
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
	ELEFLAG_VISIBLE     = 0x001,	ELEFLAG_INVISIBLE   = 0x000,
	ELEFLAG_VERTICAL    = 0x002,	ELEFLAG_HORIZONTAL  = 0x000,
	ELEFLAG_WRAP        = 0x004,	ELEFLAG_NOWRAP      = 0x000,
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
	
	ELETYPE_SPACER,	//!< Visual Spacer
	ELETYPE_GAP,	//!< Alignment Gap
	
	NUM_ELETYPES
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

/*
 * Acess2 Window Manager (AxWin2)
 */
#ifndef _WM_H_
#define _WM_H_

#include <axwin2/axwin.h>

typedef struct sAxWin_Element	tElement;

struct sAxWin_Element
{
	 int	Type;
	
	tElement	*Parent;
	tElement	*FirstChild;
	tElement	*LastChild;
	tElement	*NextSibling;
	
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
	
	char	DebugName[];
};

typedef struct sTab
{
	 int	Type;	// Should be zero, allows a tab to be the parent of an element
	
	tElement	*Parent;
	tElement	*FirstChild;
	tElement	*LastChild;
	struct sTab	*NextTab;
	
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

#endif

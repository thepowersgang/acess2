/*
 * Acess2 Window Manager (AxWin2)
 */
#ifndef _WM_H_
#define _WM_H_

#include <axwin2/axwin.h>
#include "common.h"

typedef struct sAxWin_Element	tElement;
typedef struct sTab	tTab;
typedef struct sApplication	tApplication;

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

struct sTab
{
	 int	Type;	// Should be zero, allows a tab to be the parent of an element
	
	tElement	*Parent;
	tElement	*FirstChild;
	tElement	*LastChild;
	tTab	*NextTab;
	
	char	*Name;
	
	tElement	*RootElement;
};

struct sApplication
{
	tApplication	*Next;	

	void	*Ident;
	tMessages_Handle_Callback	*SendMessage;
	
	 int	nTabs;
	tTab	*Tabs;
	tTab	*CurrentTab;
	
	char	Name[];
};

#endif

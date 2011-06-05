/*
 * Acess2 Window Manager (AxWin2)
 */
#ifndef _WM_H_
#define _WM_H_

#include <axwin2/axwin.h>
#include "common.h"

/**
 * \brief Number of elements that can be owned by each application)
 */
// TODO: Fine tune these values
#define MAX_ELEMENTS_PER_APP	1024
#define DEFAULT_ELEMENTS_PER_APP	128

typedef struct sAxWin_Element	tElement;
typedef struct sApplication	tApplication;

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
	
	short	FixedWith;	//!< Fixed Long Size attribute (height)
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

struct sApplication
{
	tApplication	*Next;

	void	*Ident;	//!< Client Identifier
	tMessages_Handle_Callback	*SendMessage;
	
	char	*Name;	//!< Application name
	
	 int	MaxElementIndex;	//!< Number of entries in \a EleIndex
	tElement	**EleIndex;	//!< Array of pointers to elements owned by this application
	
	tElement	MetaElement;	//!< Windows child off this
};

// === FUNCTIONS ===

// --- Render
extern void	WM_UpdateMinDims(tElement *Element);
extern void	WM_UpdateDimensions(tElement *Element, int Pass);
extern void	WM_UpdatePosition(tElement *Element);
extern void	WM_RenderWidget(tElement *Element);
extern void	WM_Update(void);

#endif

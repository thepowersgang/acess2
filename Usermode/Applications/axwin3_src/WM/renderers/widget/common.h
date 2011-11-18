/**
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * renderer/widget/common.h
 * - Widget common definitions
 */
#ifndef _RENDERER_WIDGET_COMMON_H
#define _RENDERER_WIDGET_COMMON_H

#include <renderer_widget.h>	// Widget types

typedef struct sWidgetDef	tWidgetDef;

#define WIDGETTYPE_FLAG_NOCHILDREN	0x001

struct sWidgetDef
{
	 int	Flags;
	void	(*Init)(tElement *Ele);
	void	(*Delete)(tElement *Ele);

	void	(*Render)(tWindow *Window, tElement *Ele);
	
	void	(*UpdateFlags)(tElement *Ele);
	void	(*UpdateSize)(tElement *Ele);
	void	(*UpdateText)(tElement *Ele, const char *Text);	// This should update Ele->Text

	/**
	 * \name Input handlers
	 * \note Returns boolean unhandled
	 * \{
	 */	
	 int	(*MouseButton)(tElement *Ele, int X, int Y, int Button, int bPressed);
	 int	(*MouseMove)(tElement *Ele, int X, int Y);
	 int	(*KeyDown)(tElement *Ele, int KeySym, int Character);
	 int	(*KeyUp)(tElement *Ele, int KeySym);
	 int	(*KeyFire)(tElement *Ele, int KeySym, int Character);
	/**
	 * \}
	 */
};

extern void	Widget_int_SetTypeDef(int Type, tWidgetDef *Def);
extern void	Widget_UpdateMinDims(tElement *Element);
extern void	Widget_Fire(tElement *Element);

#define DEFWIDGETTYPE(_type, _flags, _attribs...) \
tWidgetDef	_widget_typedef_##_type = {.Flags=(_flags),_attribs};\
void _widget_set_##_type(void) __attribute__((constructor));\
void _widget_set_##_type(void) { Widget_int_SetTypeDef(_type, &_widget_typedef_##_type);}

#endif


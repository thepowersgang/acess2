/*
 * Acess2
 * AcessNative Kernel
 * 
 * ui.h - User Interface common header
 */
#ifndef _UI_H_
#define _UI_H_

extern const int	giUI_Width;
extern const int	giUI_Height;
extern const int	giUI_Pitch;
extern const Uint32	* const gUI_Framebuffer;

extern void	UI_SetWindowDims(int Width, int Height);
extern void	UI_BlitBitmap(int DstX, int DstY, int SrcW, int SrcH, const Uint32 *Bitmap);
extern void	UI_BlitFramebuffer(int DstX, int DstY, int SrcX, int SrcY, int W, int H);
extern void	UI_FillBitmap(int DstX, int DstY, int SrcW, int SrcH, Uint32 Value);
extern void	UI_Redraw(void);

typedef void (*tUI_KeybardCallback)(Uint32 Key);
extern tUI_KeybardCallback	gUI_KeyboardCallback;
extern void	Mouse_HandleEvent(Uint32 ButtonState, int *AxisDeltas, int *AxisValues);

#endif

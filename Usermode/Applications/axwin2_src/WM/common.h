/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <acess/sys.h>	// _SysDebug

typedef void tMessages_Handle_Callback(void*, size_t, void*);
typedef struct sFont	tFont;

#include "wm.h"
#include "image.h"
//#include "font.h"

// === MACROS ===
static inline uint32_t Video_AlphaBlend(uint32_t _orig, uint32_t _new, uint8_t _alpha)
{
	uint16_t	ao,ro,go,bo;
	uint16_t	an,rn,gn,bn;
	if( _alpha == 0 )	return _orig;
	if( _alpha == 255 )	return _new;
	
	ao = (_orig >> 24) & 0xFF;
	ro = (_orig >> 16) & 0xFF;
	go = (_orig >>  8) & 0xFF;
	bo = (_orig >>  0) & 0xFF;
	
	an = (_new >> 24) & 0xFF;
	rn = (_new >> 16) & 0xFF;
	gn = (_new >>  8) & 0xFF;
	bn = (_new >>  0) & 0xFF;

	if( _alpha == 0x80 ) {
		ao = (ao + an) / 2;
		ro = (ro + rn) / 2;
		go = (go + gn) / 2;
		bo = (bo + bn) / 2;
	}
	else {
		ao = ao*(255-_alpha) + an*_alpha;
		ro = ro*(255-_alpha) + rn*_alpha;
		go = go*(255-_alpha) + gn*_alpha;
		bo = bo*(255-_alpha) + bn*_alpha;
		ao /= 255*2;
		ro /= 255*2;
		go /= 255*2;
		bo /= 255*2;
	}

	return (ao << 24) | (ro << 16) | (go << 8) | bo;
}

// === GLOBALS ===
extern const char	*gsTerminalDevice;
extern const char	*gsMouseDevice;

extern int	giScreenWidth;
extern int	giScreenHeight;
extern uint32_t	*gpScreenBuffer;

extern int	giTerminalFD;
extern int	giMouseFD;

// === Functions ===
extern void	memset32(void *ptr, uint32_t val, size_t count);
// --- Initialisation ---
extern void	ParseCommandline(int argc, char *argv[]);
extern void	IPC_Init(void);
extern void	IPC_FillSelect(int *nfds, fd_set *set);
extern void	IPC_HandleSelect(fd_set *set);
extern void	Input_FillSelect(int *nfds, fd_set *set);
extern void	Input_HandleSelect(fd_set *set);
// --- Local WM ---
extern tApplication	*AxWin_RegisterClient(int IdentLen, void *Ident, tMessages_Handle_Callback *Cb, const char *Name);
extern void	AxWin_DeregisterClient(tApplication *App);
// --- Video ---
extern void	Video_Setup(void);
extern void	Video_Update(void);
extern void	Video_FillRect(short X, short Y, short W, short H, uint32_t Color);
extern void	Video_DrawRect(short X, short Y, short W, short H, uint32_t Color);
extern int	Video_DrawText(short X, short Y, short W, short H, tFont *Font, uint32_t Color, char *Text);
extern void	Video_DrawImage(short X, short Y, short W, short H, tImage *Image);
// --- Interface ---
extern void	Interface_Init(void);
extern void	Interface_Update(void);
extern void	Interface_Render(void);
// --- Decorator ---
extern void	Decorator_RenderWidget(tElement *Element);

#endif

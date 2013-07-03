/*
 * Acess2 GUI (AxWin3) WM
 * - By John Hodge (thePowersGang)
 *
 * video_SDL.c
 * - SDL build video output
 */
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

// === GLOBALS ===
SDL_Surface	*gScreen;
 int	giScreenWidth = 1280;
 int	giScreenHeight = 720;

// === CODE ===
void Video_Setup(void)
{
	gScreen = SDL_SetVideoMode(giScreenWidth, giScreenHeight, 32, SDL_SWSURFACE);
	if( !gScreen ) {
		fprintf(stderr, "Unable to set %ix%i video mode: %s\n",
			giScreenWidth, giScreenHeight,
			SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("Acess2 GUI v3", "AxWin3");
}

void Video_Update(void)
{
	SDL_Flip(gScreen);
}

void Video_FillRect(int X, int Y, int W, int H, uint32_t Colour)
{
	SDL_Rect	r = {.x = X, .y = Y, .w = W, .h = H};
	SDL_FillRect(gScreen, &r, Colour);
}

void Video_Blit(uint32_t *Source, short DstX, short DstY, short W, short H)
{
	printf("Blit (%p, (%i,%i) %ix%i)\n",
		Source, DstX, DstY, W, H);
	SDL_Rect	r = {.x = DstX, .y = DstY, .w = W, .h = H};
	SDL_Surface	*tmps = SDL_CreateRGBSurfaceFrom(
		Source, W, H, 32, W*4,
		0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
	SDL_BlitSurface(tmps, NULL, gScreen, &r);
}

uint32_t Video_AlphaBlend(uint32_t _orig, uint32_t _new, uint8_t _alpha)
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


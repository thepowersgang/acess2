/*
 * Acess2 Native Kernel
 * 
 * SDL User Interface
 */
#include <SDL/SDL.h>
#define const
#include "ui.h"
#undef const
#include <tpl_drv_keyboard.h>

// === IMPORTS ===
extern void	AcessNative_Exit(void);

// === PROTOTYPES ===
 int	UI_Initialise(int MaxWidth, int MaxHeight);
 int	UI_MainThread(void *Unused);
void	UI_BlitBitmap(int DstX, int DstY, int SrcW, int SrcH, Uint32 *Bitmap);
void	UI_BlitFramebuffer(int DstX, int DstY, int SrcX, int SrcY, int W, int H);
void	UI_FillBitmap(int X, int Y, int W, int H, Uint32 Value);
void	UI_Redraw(void);

// === GLOBALS ===
SDL_Surface	*gScreen;
SDL_Thread	*gInputThread;
 int	giUI_Width = 0;
 int	giUI_Height = 0;
 int	giUI_Pitch = 0;
tUI_KeybardCallback	gUI_KeyboardCallback;
Uint32	gUI_Keymap[2][SDLK_LAST];	// Upper/Lower case

// === FUNCTIONS ===
int UI_Initialise(int MaxWidth, int MaxHeight)
{	
	// Changed when the video mode is set
	giUI_Width = MaxWidth;
	giUI_Height = MaxHeight;
	
	// Start main thread
	gInputThread = SDL_CreateThread(UI_MainThread, NULL);
	
	return 0;
}

Uint32 UI_GetAcessKeyFromSDL(SDLKey Sym, Uint16 Unicode)
{
	Uint8	*keystate = SDL_GetKeyState(NULL);
	 int	shiftState = 0;
	Uint32	ret = 0;
	
	if( keystate[SDLK_RSHIFT] || keystate[SDLK_LSHIFT] )
		shiftState = 1;
	
	// Fast return
	if( gUI_Keymap[shiftState][Sym] )
		return gUI_Keymap[shiftState][Sym];
	
	// How nice of you, a unicode value
	if( Unicode )
	{
		ret = Unicode;
	}
	// Ok, we need to do work :(
	else
	{
		switch(Sym)
		{
		case SDLK_UP:	ret = KEY_UP;	break;
		case SDLK_DOWN:	ret = KEY_DOWN;	break;
		case SDLK_LEFT:	ret = KEY_LEFT;	break;
		case SDLK_RIGHT:ret = KEY_RIGHT;break;
		case SDLK_CAPSLOCK:	ret = KEY_CAPSLOCK;	break;
		case SDLK_F1:	ret = KEY_F1;	break;
		case SDLK_F2:	ret = KEY_F2;	break;
		case SDLK_F3:	ret = KEY_F3;	break;
		case SDLK_F4:	ret = KEY_F4;	break;
		case SDLK_F5:	ret = KEY_F5;	break;
		case SDLK_F6:	ret = KEY_F6;	break;
		case SDLK_F7:	ret = KEY_F7;	break;
		case SDLK_F8:	ret = KEY_F8;	break;
		case SDLK_F9:	ret = KEY_F9;	break;
		case SDLK_F10:	ret = KEY_F10;	break;
		case SDLK_F11:	ret = KEY_F11;	break;
		case SDLK_F12:	ret = KEY_F12;	break;
		default:
			printf("Unhandled key code %i\n", Sym);
			break;
		}
	}
	
	gUI_Keymap[shiftState][Sym] = ret;
	return ret;
}

int UI_MainThread(void *Unused)
{
	SDL_Event	event;
	Uint32	acess_sym;
	
	SDL_Init(SDL_INIT_VIDEO);
	
	// Set up video
	gScreen = SDL_SetVideoMode(giUI_Width, giUI_Height, 32, 0);
 	if( !gScreen ) {
		fprintf(stderr, "Couldn't set %ix%i video mode: %s\n", giUI_Width, giUI_Height, SDL_GetError());
		SDL_Quit();
 	 	exit(2);
	}
	SDL_WM_SetCaption("Acess2", "Acess2");
	
	giUI_Width = gScreen->w;
	giUI_Height = gScreen->h;
	giUI_Pitch = gScreen->pitch;
	
	SDL_EnableUNICODE(1);
	
	for( ;; )
	{
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_QUIT:
				AcessNative_Exit();
				return 0;
				
			case SDL_KEYDOWN:
				acess_sym = UI_GetAcessKeyFromSDL(event.key.keysym.sym,
					event.key.keysym.unicode);
				
				if( gUI_KeyboardCallback )
					gUI_KeyboardCallback(acess_sym);
				break;
			
			case SDL_KEYUP:
				acess_sym = UI_GetAcessKeyFromSDL(event.key.keysym.sym,
					event.key.keysym.unicode);
				
				if( gUI_KeyboardCallback )
					gUI_KeyboardCallback(0x80000000|acess_sym);
				break;
			
			default:
				break;
			}
		}
	}
}

void UI_BlitBitmap(int DstX, int DstY, int SrcW, int SrcH, Uint32 *Bitmap)
{
	SDL_Surface	*tmp;
	SDL_Rect	dstRect;
	
	tmp = SDL_CreateRGBSurfaceFrom(Bitmap, SrcW, SrcH, 32, SrcW*4,
		0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
	SDL_SetAlpha(tmp, 0, SDL_ALPHA_OPAQUE);
	
	dstRect.x = DstX;	dstRect.y = DstY;
	
	SDL_BlitSurface(tmp, NULL, gScreen, &dstRect);
	
	SDL_FreeSurface(tmp);
}

void UI_BlitFramebuffer(int DstX, int DstY, int SrcX, int SrcY, int W, int H)
{
	SDL_Rect	srcRect;
	SDL_Rect	dstRect;
	
	srcRect.x = SrcX;	srcRect.y = SrcY;
	srcRect.w = W;	srcRect.h = H;
	dstRect.x = DstX;	dstRect.y = DstY;
	
	SDL_BlitSurface(gScreen, &srcRect, gScreen, &dstRect); 
}

void UI_FillBitmap(int X, int Y, int W, int H, Uint32 Value)
{
	SDL_Rect	dstRect;
	
	dstRect.x = X;	dstRect.y = Y;
	dstRect.w = W;	dstRect.h = H;
	
	SDL_FillRect(gScreen, &dstRect, Value);
}

void UI_Redraw(void)
{
	// No-Op, we're not using double buffering
}

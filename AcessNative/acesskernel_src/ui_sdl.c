/*
 * Acess2 Native Kernel
 * 
 * SDL User Interface
 */
#include <SDL/SDL.h>
#define const
#include "ui.h"
#undef const
#include <api_drv_keyboard.h>

// === IMPORTS ===
extern void	AcessNative_Exit(void);

// === PROTOTYPES ===
 int	UI_Initialise(int MaxWidth, int MaxHeight);
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
	
	// Set up video
	SDL_Init(SDL_INIT_VIDEO);
	printf("UI attempting %ix%i %ibpp\n", giUI_Width, giUI_Height, 32);
	gScreen = SDL_SetVideoMode(giUI_Width, giUI_Height, 32, SDL_DOUBLEBUF);
 	if( !gScreen ) {
		fprintf(stderr, "Couldn't set %ix%i video mode: %s\n", giUI_Width, giUI_Height, SDL_GetError());
		SDL_Quit();
 	 	exit(2);
	}
	SDL_WM_SetCaption("Acess2", "Acess2");
	
	giUI_Width = gScreen->w;
	giUI_Height = gScreen->h;
	giUI_Pitch = gScreen->pitch;

	printf("UI window %ix%i %i bytes per line\n", giUI_Width, giUI_Height, giUI_Pitch);
	
	SDL_EnableUNICODE(1);

	return 0;
}

Uint32 UI_GetAcessKeyFromSDL(SDLKey Sym)
{
	Uint8	*keystate = SDL_GetKeyState(NULL);
	 int	shiftState = 0;
	Uint32	ret = 0;
	
	if( keystate[SDLK_RSHIFT] || keystate[SDLK_LSHIFT] )
		shiftState = 1;
	
	// Fast return
	if( gUI_Keymap[shiftState][Sym] )
		return gUI_Keymap[shiftState][Sym];

	switch(Sym)
	{
	case SDLK_a ... SDLK_z:
		ret = Sym - SDLK_a + KEYSYM_a;
		break;
	case SDLK_0 ... SDLK_9:
		ret = Sym - SDLK_0 + KEYSYM_0;
		break;
	case SDLK_CAPSLOCK:	ret = KEYSYM_CAPS;	break;
	case SDLK_TAB:	ret = KEYSYM_TAB;	break;
	case SDLK_UP:	ret = KEYSYM_UPARROW;	break;
	case SDLK_DOWN:	ret = KEYSYM_DOWNARROW;	break;
	case SDLK_LEFT:	ret = KEYSYM_LEFTARROW;	break;
	case SDLK_RIGHT:ret = KEYSYM_RIGHTARROW;break;
	case SDLK_F1:	ret = KEYSYM_F1;	break;
	case SDLK_F2:	ret = KEYSYM_F2;	break;
	case SDLK_F3:	ret = KEYSYM_F3;	break;
	case SDLK_F4:	ret = KEYSYM_F4;	break;
	case SDLK_F5:	ret = KEYSYM_F5;	break;
	case SDLK_F6:	ret = KEYSYM_F6;	break;
	case SDLK_F7:	ret = KEYSYM_F7;	break;
	case SDLK_F8:	ret = KEYSYM_F8;	break;
	case SDLK_F9:	ret = KEYSYM_F9;	break;
	case SDLK_F10:	ret = KEYSYM_F10;	break;
	case SDLK_F11:	ret = KEYSYM_F11;	break;
	case SDLK_F12:	ret = KEYSYM_F12;	break;
	case SDLK_RETURN:	ret = KEYSYM_RETURN;	break;
	case SDLK_LALT: 	ret = KEYSYM_LEFTALT;	break;
	case SDLK_LCTRL: 	ret = KEYSYM_LEFTCTRL;	break;
	case SDLK_LSHIFT:	ret = KEYSYM_LEFTSHIFT;	break;
	case SDLK_LSUPER:	ret = KEYSYM_LEFTGUI;	break;
	case SDLK_RALT: 	ret = KEYSYM_RIGHTALT;	break;
	case SDLK_RCTRL: 	ret = KEYSYM_RIGHTCTRL;	break;
	case SDLK_RSHIFT: 	ret = KEYSYM_RIGHTSHIFT;	break;
	case SDLK_RSUPER:	ret = KEYSYM_RIGHTGUI;	break;
	default:
		printf("Unhandled key code %i\n", Sym);
		break;
	}
	
	gUI_Keymap[shiftState][Sym] = ret;
	return ret;
}

Uint32 UI_GetButtonBits(Uint8 sdlstate)
{
	Uint32	rv = 0;
	rv |= sdlstate & SDL_BUTTON(SDL_BUTTON_LEFT)	? (1 << 0) : 0;
	rv |= sdlstate & SDL_BUTTON(SDL_BUTTON_RIGHT)	? (1 << 1) : 0;
	rv |= sdlstate & SDL_BUTTON(SDL_BUTTON_MIDDLE)	? (1 << 2) : 0;
	rv |= sdlstate & SDL_BUTTON(SDL_BUTTON_X1)	? (1 << 3) : 0;
	rv |= sdlstate & SDL_BUTTON(SDL_BUTTON_X2)	? (1 << 4) : 0;
	return rv;
}

void UI_MainLoop(void)
{
	SDL_Event	event;
	Uint32	acess_sym;

	while( SDL_WaitEvent(&event) )
	{
		switch(event.type)
		{
		case SDL_QUIT:
			AcessNative_Exit();
			return ;
			
		case SDL_KEYDOWN:
			acess_sym = UI_GetAcessKeyFromSDL(event.key.keysym.sym);
			// Enter key on acess returns \n, but SDL returns \r
			if(event.key.keysym.sym == SDLK_RETURN)
				event.key.keysym.unicode = '\n';				

			if( gUI_KeyboardCallback ) {
				gUI_KeyboardCallback(KEY_ACTION_RAWSYM|acess_sym);
				gUI_KeyboardCallback(KEY_ACTION_PRESS|event.key.keysym.unicode);
			}
			break;
		
		case SDL_KEYUP:
			acess_sym = UI_GetAcessKeyFromSDL(event.key.keysym.sym);
			
			if( gUI_KeyboardCallback ) {
				gUI_KeyboardCallback(KEY_ACTION_RAWSYM|acess_sym);
				gUI_KeyboardCallback(KEY_ACTION_RELEASE|0);
			}
			break;

		case SDL_USEREVENT:
			SDL_UpdateRect(gScreen, 0, 0, giUI_Width, giUI_Height);
			SDL_Flip(gScreen);
			break;
		
		case SDL_MOUSEMOTION: {
			int abs[] = {event.motion.x, event.motion.y};
			int delta[] = {event.motion.xrel, event.motion.yrel};
			Mouse_HandleEvent(UI_GetButtonBits(SDL_GetMouseState(NULL, NULL)), delta, abs);
			break; }
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN: {
			int abs[] = {event.button.x, event.button.y};
			Mouse_HandleEvent(UI_GetButtonBits(SDL_GetMouseState(NULL, NULL)), NULL, abs);
			break; }

		default:
			break;
		}
	}
}

void UI_BlitBitmap(int DstX, int DstY, int SrcW, int SrcH, Uint32 *Bitmap)
{
	SDL_Surface	*tmp;
	SDL_Rect	dstRect;
	
//	printf("UI_BlitBitmap: Blit to (%i,%i) from %p (%ix%i 32bpp bitmap)\n",
//		DstX, DstY, Bitmap, SrcW, SrcH);
	
	tmp = SDL_CreateRGBSurfaceFrom(Bitmap, SrcW, SrcH, 32, SrcW*4,
		0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
	SDL_SetAlpha(tmp, 0, SDL_ALPHA_OPAQUE);
	
	dstRect.x = DstX;	dstRect.y = DstY;
	dstRect.w = -1;	dstRect.h = -1;
	
	SDL_BlitSurface(tmp, NULL, gScreen, &dstRect);
	//SDL_BlitSurface(tmp, NULL, gScreen, NULL);
	
	SDL_FreeSurface(tmp);
//	SDL_Flip(gScreen);
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
	
//	printf("UI_FillBitmap: gScreen = %p\n", gScreen);
	SDL_FillRect(gScreen, &dstRect, Value);
}

void UI_Redraw(void)
{
	// TODO: Keep track of changed rectangle
//	SDL_UpdateRect(gScreen, 0, 0, giUI_Width, giUI_Height);
	SDL_Event	e;

	e.type = SDL_USEREVENT;
	e.user.code = 0;
	e.user.data1 = 0;
	e.user.data2 = 0;	

	SDL_PushEvent( &e );
}

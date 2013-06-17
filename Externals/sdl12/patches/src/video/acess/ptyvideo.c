/*
 * Acess2 SDL Video Driver
 * - By John Hodge (thePowersGang)
 * 
 * video/acess/ptyvideo.c
 * - PTY-based video driver
 */
#include "SDL_config.h"
#include <acess/sys.h>
#include <acess/devices/pty.h>
#include "SDL_video.h"
#include "../SDL_sysvideo.h"

#define _THIS SDL_VideoDevice *this
#define HIDDEN	(this->hidden)

// === TYPES ===
struct SDL_PrivateVideoData
{
	 int	OutFD;
	SDL_Rect	MaxRect;
	SDL_Rect	*FullscreenModes[2];
	Uint32	*Palette;	// 256 colours
};

// === PROTOTYPES ===
static int AcessPTY_Avaliable(void);
static SDL_VideoDevice *AcessPTY_Create(int devidx);
static void AcessPTY_Destroy(SDL_VideoDevice *device);
// - Core functions
static int	AcessPTY_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect	**AcessPTY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *AcessPTY_SetVideoMode(_THIS, SDL_Surface *Current, int W, int H, int BPP, Uint32 Flags);
static int	AcessPTY_SetColors(_THIS, int FirstColour, int NColours, SDL_Color *Colours);
static void	AcessPTY_VideoQuit(_THIS);
static void	AcessPTY_UpdateRects(_THIS, int NumRects, SDL_Rect *Rects);
// - Hardware Surfaces (TODO)
//static int	AcessPTY_AllocHWSurface(_THIS, SDL_Surface *Surface);
//static int	AcessPTY_FillHWRect(_THIS, SDL_Surface *Dst, SDL_Rect *Rect, Uint32 Color);

// === GLOBALS ===
VideoBootStrap	AcessPTY_bootstrap = {
	"acesspty", "Acess2 PTY Video",
	AcessPTY_Avaliable, AcessPTY_Create
};
SDL_VideoDevice	AcessPTY_TemplateDevice = {
	.VideoInit	= AcessPTY_VideoInit,
	.ListModes	= AcessPTY_ListModes,
	.SetVideoMode	= AcessPTY_SetVideoMode,
	.SetColors	= AcessPTY_SetColors,
	.VideoQuit	= AcessPTY_VideoQuit,
	.UpdateRects	= AcessPTY_UpdateRects,
	//.FillHWRect = AcessPTY_FillHWRect,
	.free	= AcessPTY_Destroy
};

int AcessPTY_Avaliable(void)
{
	// - test on stdin so stdout can be redirected without breaking stuff
	if( _SysIOCtl(0, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL )
		return 0;
	// TODO: Check if the terminal supports graphics modes
	return 1;
}

SDL_VideoDevice	*AcessPTY_Create(int devidx)
{
	SDL_VideoDevice	*ret;

	// Allocate and pre-populate
	ret = malloc( sizeof(SDL_VideoDevice) );
	if( !ret ) {
		SDL_OutOfMemory();
		return NULL;
	}
	memcpy(ret, &AcessPTY_TemplateDevice, sizeof(SDL_VideoDevice));
	ret->hidden = malloc( sizeof(struct SDL_PrivateVideoData) );
	if( !ret->hidden ) {
		SDL_OutOfMemory();
		free(ret);
		return NULL;
	}
	memset(ret->hidden, 0, sizeof(*ret->hidden));

	ret->hidden->OutFD = _SysCopyFD(0, -1);
	if( ret->hidden->OutFD == -1 ) {
		free(ret->hidden);
		free(ret);
		return NULL;
	}

	// Redirect stdout/stderr if they point to the terminal
	if( 1 ) { //_SysIsSameNode(0, 1) ) {
		_SysReopen(1, "stdout.txt", OPENFLAG_WRITE);
	}
	if( 1 ) { //_SysIsSameNode(0, 2) ) {
		_SysReopen(2, "stderr.txt", OPENFLAG_WRITE);
	}

	return ret;
}

void AcessPTY_Destroy(SDL_VideoDevice *device)
{
	_SysClose(device->hidden->OutFD);
	free(device->hidden);
	free(device);
}

// ---
// General functions
// ---
int AcessPTY_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	// Set PTY to graphics mode
	struct ptymode mode = {.OutputMode = PTYBUFFMT_FB, .InputMode = PTYIMODE_RAW};
	_SysIOCtl(HIDDEN->OutFD, PTY_IOCTL_SETMODE, &mode);

	// Get current dimensions (and use these as the max)
	// - Not strictly true when using a GUI Terminal, but does apply for VTerms
	struct ptydims	dims;
	_SysIOCtl(HIDDEN->OutFD, PTY_IOCTL_GETDIMS, &dims);
	
	HIDDEN->MaxRect.x = 0;
	HIDDEN->MaxRect.y = 0;
	HIDDEN->MaxRect.w = dims.PW;
	HIDDEN->MaxRect.h = dims.PH;
	
	HIDDEN->FullscreenModes[0] = &HIDDEN->MaxRect;
	HIDDEN->FullscreenModes[1] = NULL;
	
	return 0;
}

SDL_Rect **AcessPTY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	if( flags & SDL_FULLSCREEN)
		return HIDDEN->FullscreenModes;	// Should be only one entry
//	else if( SDLToDFBPixelFormat (format) != DSPF_UNKNOWN )
		return (SDL_Rect**) -1;
//	else
//		return NULL;
}

SDL_Surface *AcessPTY_SetVideoMode(_THIS, SDL_Surface *Current, int W, int H, int BPP, Uint32 Flags)
{
	// Sanity first
	if( W <= 0 || W > HIDDEN->MaxRect.w )
		return NULL;
	if( H <= 0 || H > HIDDEN->MaxRect.h )
		return NULL;

	struct ptydims dims = {.PW = W, .PH = H};
	_SysIOCtl(HIDDEN->OutFD, PTY_IOCTL_SETDIMS, &dims);

	Current->pixels = SDL_malloc(W * H * 4);
	if( !Current->pixels ) {
		
	}
	
	return Current;
}

int AcessPTY_SetColors(_THIS, int FirstColour, int NColours, SDL_Color *Colours)
{
	return 1;
}

void AcessPTY_VideoQuit(_THIS)
{
}

void AcessPTY_UpdateRects(_THIS, int NumRects, SDL_Rect *Rects)
{
	 int	w = this->screen->w;
	 int	h = this->screen->h;
	_SysWrite(HIDDEN->OutFD, w*h*4, this->screen->pixels);
}


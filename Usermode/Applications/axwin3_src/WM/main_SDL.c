/*
 * Acess2 GUI (AxWin3) WM
 * - By John Hodge (thePowersGang)
 *
 * main_SDL.c
 * - SDL build main function
 */
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

// === IMPORTS ===
extern void	Video_Setup(void);
extern void	WM_Initialise(void);
extern void	WM_Update(void);

// === CODE ===
int main(int argc, char *argv[])
{
	// Set up basic SDL environment
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Unable to initialise SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	
	Video_Setup();
	WM_Initialise();

	// Main loop
	printf("Entering idle loop\n");
	 int	bRunning = 1;
	while(bRunning)
	{
		SDL_Event	ev;
		WM_Update();
		SDL_WaitEvent(&ev);
		switch( ev.type )
		{
		case SDL_QUIT:
			bRunning = 0;
			break;
		case SDL_KEYDOWN:
			break;
		case SDL_USEREVENT:
			break;
		}
	}
	
	// TODO: Clean up

	return 0;
}


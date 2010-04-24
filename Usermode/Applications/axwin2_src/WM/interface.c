/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"

// === GLOBALS ==
 int	giInterface_Width = 0;

// === CODE ===
void Interface_Render(void)
{
	giInterface_Width = giScreenWidth/16;
	
	Video_FillRect(
		0, 0,
		giInterface_Width, giScreenHeight,
		0xDDDDDD);
	
	Video_Update();
}

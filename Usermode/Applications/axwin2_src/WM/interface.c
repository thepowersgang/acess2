/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"

// === CODE ===
void Interface_Render(void)
{
	Video_FillRect(
		0, 0,
		giScreenWidth/16, giScreenHeight,
		0xDDDDDD);
	
	Video_Update();
}

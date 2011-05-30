/*
 * AxWin Window Manager Interface Library
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess Licence. See the
 * file COPYING for details.
 * 
 * common.h - Internal Variable and Constant definitions
 */
#ifndef _COMMON_H_
#define _COMMON_H_

// === Includes ===
#include <acess/sys.h>
#include <axwin2/axwin.h>
#include <stdlib.h>

// === Constants ===
enum eAxWin_Modes
{
	AXWIN_MODE_IPC
};

// === Variables ===
extern int	giAxWin_Mode;
extern int	giAxWin_PID;

#endif

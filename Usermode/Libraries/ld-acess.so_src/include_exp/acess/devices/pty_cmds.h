/*
 * Acess2 User Core
 * - By John Hodge (thePowersGang)
 *
 * acess/devices/pty_cmds.h
 * - Defintions for 2D/3D command stream PTY output modes
 */
#ifndef _ACESS_DEVICES_PTY_CMDS_H_
#define _ACESS_DEVICES_PTY_CMDS_H_

#include <stdint.h>

enum
{
	PTY2D_CMD_NOP,
	PTY2D_CMD_SETCURSORPOS,
	PTY2D_CMD_SETCURSORBMP,
	PTY2D_CMD_NEWSRF,
	PTY2D_CMD_DELSRF,
	PTY2D_CMD_BLIT,
	PTY2D_CMD_SEND,
	PTY2D_CMD_RECV,
};

struct ptycmd_setcursorpos
{
	uint16_t	cmd;
	uint16_t	x;
	uint16_t	y;
};

#endif


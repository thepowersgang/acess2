/*
 * Acess2 System Calls
 * - By John Hodge (thePowersGang)
 *
 * acess/devices/joystick.h
 * - Joystick IOCtls and structures
 */
#ifndef _SYS_DEVICES_JOYSTICK_H
#define _SYS_DEVICES_JOYSTICK_H

#if __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct mouse_attribute
{
	uint32_t	Num;
	uint32_t	Value;
};

struct mouse_header
{
	uint16_t	NAxies;
	uint16_t	NButtons;
};

struct mouse_axis
{
	 int16_t	MinValue;
	 int16_t	MaxValue;
	 int16_t	CurValue;
	uint16_t	CursorPos;
};

enum {
	JOY_IOCTL_GETSETAXISLIMIT = 6,
	JOY_IOCTL_GETSETAXISPOSITION,
};

#if __cplusplus
}
#endif

#endif


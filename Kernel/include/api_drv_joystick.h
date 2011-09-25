/**
 * \file api_drv_joystick
 * \brief Joystick Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section dirs VFS Layout
 * Joystick drivers define a single VFS node, that acts as a fixed size file.
 * Reads from this file return the current state, writes are ignored.
 *
 * \section File Structure
 * The device file must begin with a valid sJoystick_FileHeader structure.
 * The file header is followed by \a NAxies instances of sJoystick_Axis, one
 * for each axis.
 * This is followed by \a NButtons boolean values (represented using \a Uint8),
 * each representing the state of a single button (where 0 is unpressed,
 * 0xFF is fully depressed - intermediate values are valid in the case of
 * variable-pressure buttons)
 */
#ifndef _API_DRV_JOYSTICK_H
#define _API_DRV_JOYSTICK_H

#include <api_drv_common.h>

/**
 * \enum eTplJoystick_IOCtl
 * \brief Common Joystick IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplJoystick_IOCtl {
	/**
	 * ioctl(..., tJoystick_Callback *Callback)
	 * \brief Sets the callback
	 * \note Can be called from kernel mode only
	 *
	 * Sets the callback that is called when a event occurs (button or axis
	 * change). This function pointer must be in kernel mode (although,
	 * kernel->user or kernel->ring3driver abstraction functions can be used)
	 * 
	 * Axis events depend on the axis limit, if non-zero, the callback fires
	 * if the cursor position changes. Otherwise it fires when the axis value
	 * (cursor accelleration) changes.
	 */
	JOY_IOCTL_SETCALLBACK = 4,

	/**
	 * ioctl(..., int *Argument)
	 * \brief Set the argument passed as the first parameter to the callback
	 * \note Kernel mode only
	 */
	JOY_IOCTL_SETCALLBACKARG,

	/**
	 * ioctl(..., tJoystickNumValue *)
	 * \brief Set maximum value for sJoystick_Axis.CurState
	 * \note If \a Value is equal to -1 (all bits set), the value is not changed
	 */
	JOY_IOCTL_GETSETAXISLIMIT,

	/**
	 * ioctl(..., tJoystickNumValue *)
	 * \brief Set the value of sJoystick_Axis.CurState
	 * \note If \a Value is equal to -1 (all bits set), the value is not changed
	 */
	JOY_IOCTL_GETSETAXISPOSITION,
	
	/**
	 * ioctl(..., tJoystickNumValue *)
	 * \brief Set axis flags
	 * \note If \a Value is equal to -1 (all bits set), the value is not changed
	 * \todo Define flag values
	 */
	JOY_IOCTL_GETSETAXISFLAGS,

	/**
	 * ioctl(..., tJoystickNumValue *)
	 * \brief Set Button Flags
	 * \note If \a Value is equal to -1 (all bits set), the value is not changed
	 * \todo Define flag values
	 */
	JOY_IOCTL_GETSETBUTTONFLAGS,
};

#define	DRV_JOY_IOCTLNAMES	"set_callback", "set_callback_arg", "getset_axis_limit", "getset_axis_position", \
	"getset_axis_flags", "getset_button_flags"

// === TYPES ===
typedef struct sJoystick_NumValue	tJoystick_NumValue;
typedef struct sJoystick_FileHeader	tJoystick_FileHeader;
typedef struct sJoystick_Axis	tJoystick_Axis;

/**
 * \brief Number/Value pair for joystick IOCtls
 */
struct sJoystick_NumValue
{
	 int	Num;	//!< Axis/Button number
	 int	Value;	//!< Value (see IOCtl defs for meaning)
};

/**
 * \brief Callback type for JOY_IOCTL_SETCALLBACK
 * \param Ident	Ident value passed to JOY_IOCTL_SETCALLBACK
 * 
 */
typedef void (*tJoystick_Callback)(int Ident, int bIsAxis, int Num, int Delta);

/**
 * \struct sJoystick_FileHeader
 */
struct sJoystick_FileHeader
{
	Uint16	NAxies;	//!< Number of Axies
	Uint16	NButtons;	//!< Number of buttons
};

/**
 * \brief Axis Definition in file data
 *
 * Describes the current state of an axis on the joystick.
 * \a CursorPos is between zero and the current limit set by the
 * JOY_IOCTL_GETSETAXISLIMIT IOCtl, while \a CurValue indicates the
 * current position of the joystick axis. This is defined to be between
 * \a MinValue and \a MaxValue.
 */
struct sJoystick_Axis
{
	Sint16	MinValue;	//!< Minumum value for \a CurValue
	Sint16	MaxValue;	//!< Maximum value for \a CurValue
	Sint16	CurValue;	//!< Current value (joystick position)
	Uint16	CursorPos;	//!< Current state (cursor position)
};

#define JOY_INFOSTRUCT(_naxies, _nbuttons) struct { \
	Uint16	NAxies, NButtons;\
	tJoystick_Axis	Axies[_naxies];\
	Uint16	Buttons[_nbuttons];\
	}

#endif

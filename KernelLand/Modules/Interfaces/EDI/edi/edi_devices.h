#ifndef EDI_DEVICES_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

/* Edited by thePowersGang (John Hodge) June 2009
 * - Add #ifdef EDI_MAIN_FILE
 */

#define EDI_DEVICES_H

/*! \file edi_devices.h
 * \brief Declaration and description of simple classes for implementation by EDI drivers to represent hardware devices.
 *
 * Data structures and algorithms this header represents:
 *
 *	DATA STRUCTURE AND ALGORITHM: BASIC DEVICES - There are two functions, select() for waiting on devices and ioctl() for
 * controlling them, common to many POSIX devices.  Implementations of EDI-CHARACTER-DEVICE or EDI-BLOCK-DEVICE may implement either of
 * these or both, and users of such objects much query for the methods to see if they're supported.  Obviously, runtime or driver
 * developers don't *need* to support these.
 *
 *	DATA STRUCTURE AND ALGORITHM: CHARACTER DEVICES - The class EDI-CHARACTER-DEVICE provides a very basic interface to character
 * devices, which read and write streams of characters.  As such, this class only provides read() and write().  The calls attempt a
 * likeness to POSIX.
 *
 *	DATA STRUCTURE AND ALGORITHM: BLOCK DEVICES - The class EDI-BLOCK-DEVICE provides a very basic interface to block devices, which
 * can read(), write() and seek() to blocks of a specific size in an array of blocks with a specific size.  Its declarations and
 * semantics should behave like those of most POSIX operating systems.
 *
 * Note that EDI runtimes should not implement these classes.  Their declarations are provided for drivers to implement. */

#include "edi_objects.h"

/* Methods common to all EDI device classes specified in this header. */

/*!\brief EAGAIN returned by functions for block and character devices.
 *
 * Means that the amount of data the device has ready is less than count. */
#define EAGAIN -1
/*!\brief EBADOBJ returned by functions for block and character devices.
 *
 * Means that the object passed as the method's this point was not a valid object of the needed class. */
#define EBADOBJ -2
/*!\brief EINVAL returned by functions for block and character devices.
 *
 * Means that the method got passed invalid parameters. */
#ifdef EINVAL
# undef EINVAL
#endif
#define EINVAL -3

/*!\brief select() type to wait until device is writable. */
#define EDI_SELECT_WRITABLE 0
/*!\brief select() type to wait until device is readable. */
#define EDI_SELECT_READABLE 1

/*!\brief Argument to seek().  Sets the block offset (ie: the "current block" index) to the given whence value. */
#define EDI_SEEK_SET 0
/*!\brief Argument to seek().  Sets the block offset (ie: the "current block" index) to its current value + whence. */
#define EDI_SEEK_CURRENT 1

#ifdef EDI_MAIN_FILE
/*!\brief Arguments to EDI's basic select() function. */
edi_variable_declaration_t select_arguments[2] = {{"pointer void","device",1},
						 {"unsigned int32_t","select_type",1}};
/*!\brief Declaration of EDI's basic select() function. 
 *
 * Contrary to the POSIX version, this select() puts its error codes in its return value. */
edi_function_declaration_t select_declaration = {"int32_t","edi_device_select",0,2,select_arguments,NULL};
#else
extern edi_function_declaration_t select_declaration;	// Declare for non main files
#endif

#ifdef EDI_MAIN_FILE
/*!\brief Arguments to EDI's basic ioctl() function. */
edi_variable_declaration_t ioctl_arguments[3] = {{"pointer void","device",1},{"int32_t","request",1},{"pointer void","argp",1}};
/*!\brief Declaration of EDI's basic ioctl() function. 
 *
 * Contrary to the POSIX version, this ioctl() puts its error codes in its return value. */
edi_function_declaration_t ioctl_declaration = {"int32_t","edi_device_ioctl",0,3,ioctl_arguments,NULL};
#else
extern edi_class_declaration_t ioctl_declaration;	// Declare for non main files
#endif

#ifdef EDI_MAIN_FILE
/*!\brief Declaration of the arguments EDI-CHARACTER-DEVICE's read() and write() methods. */
edi_variable_declaration_t chardev_read_write_arguments[3] = {{"pointer void","chardev",1},
							      {"pointer void","buffer",1},
							      {"unsigned int32_t","char_count",1}};
/*!\brief Declarations of the methods of EDI-CHARACTER-DEVICE, read() and write().
 *
 * The code pointers of these function declarations are all given as NULL.  Driver developers implementing EDI-CHARACTER-DEVICE should
 * fill in these entries with pointers to their own functions. */
EDI_DEFVAR edi_function_declaration_t chardev_methods[2]= {{"int32_t","edi_chardev_read",0,3,chardev_read_write_arguments,NULL},
						{"int32_t","edi_chardev_write",0,3,chardev_read_write_arguments,NULL}};
/*!\brief Declaration of the EDI-CHARACTER-DEVICE class.
 *
 * Driver developers implementing this class should fill in their own values for constructor, destructor, and possibly even parent
 * before passing the filled-in structure to the EDI runtime. */
EDI_DEFVAR edi_class_declaration_t chardev_class = {"EDI-CHARACTER-DEVICE",0,2,chardev_methods,NULL,NULL,NULL};
#else
extern edi_class_declaration_t chardev_class;	// Declare for non main files
#endif

#ifdef EDI_MAIN_FILE
/*!\brief Arguments to EDI-BLOCK-DEVICE's read() and write() methods. */
edi_variable_declaration_t blockdev_read_write_arguments[3] = {{"pointer void","blockdev",1},
							       {"pointer void","buffer",1},
							       {"unsigned int32_t","blocks",1}};
/*!\brief Arguments to EDI-BLOCK-DEVICE's seek() method. */
edi_variable_declaration_t blockdev_seek_arguments[3] = {{"pointer void","blockdev",1},
							 {"int32_t","offset",1},
							 {"int32_t","whence",1}};
/*!\brief Declaration of the methods of EDI-BLOCK-DEVICE, read(), write(), seek(), and get_block_size(). 
 *
 * The code pointers of these function declarations are all given as NULL.  Driver developers implementing EDI-BLOCK-DEVICE should fill
 * these entries in with pointers to their own functions. */
edi_function_declaration_t blockdev_methods[4] = {{"int32_t","edi_blockdev_read",0,3,blockdev_read_write_arguments,NULL},
						  {"int32_t","edi_blockdev_write",0,3,blockdev_read_write_arguments,NULL},
						  {"int32_t","edi_blockdev_seek",0,3,blockdev_seek_arguments,NULL},
						  {"unsigned int32_t","edi_blockdev_get_block_size",0,0,NULL,NULL}};
/*!\brief Declaration of the EDI-BLOCK-DEVICE class.
 *
 * Driver developers implementing this class should fill in their own values for constructor, destructor, and possibly even parent
 * before passing the filled-in structure to the EDI runtime. */
edi_class_declaration_t blockdev_class = {"EDI-BLOCK-DEVICE",0,4,blockdev_methods,NULL,NULL,NULL};
#else
extern edi_class_declaration_t blockdev_class;	// Declare for non main files
#endif

#endif

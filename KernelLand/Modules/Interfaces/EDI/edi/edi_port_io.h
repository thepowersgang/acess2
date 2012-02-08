#ifndef EDI_PORT_IO_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

/* Modified by thePowersGang (John Hodge)
 * - Surround variable definitions with an #ifdef IMPLEMENTING_EDI
 */

#define EDI_PORT_IO_H

/*! \file edi_port_io.h
 * \brief Declaration and description of EDI's port I/O class.
 *
 * Data structures and algorithms this header represents:
 *
 *	DATA STRUCTURE AND ALGORITHM: PORT I/O OBJECTS - A class named "EDI-IO-PORT" is defined as an encapsulation of the port I/O
 * used on some machine architectures.  Each object of this class represents a single I/O port which can be read from and written to
 * in various sizes.  Each port can be held by one object only at a time. */

#include "edi_objects.h"

/*! \brief Macro to create methods for reading from ports.
 *
 * This macro creates four similar methods, differing in the size of the type they read from the I/O port held by the object.  Their
 * parameter is a pointer to the output type, which is filled with the value read from the I/O port.  They return 1 for success, -1
 * for an uninitialized I/O port object, and 0 for other errors. */
#define port_read_method(type,name) int32_t (*name)(object_pointer port_object, type *out)
/*! \brief Macro to create methods for writing to ports.
 *
 * This macro creates four more similar methods, differing in the size of the type they write to the I/O port held by the object.
 * Their parameter is the value to write to the port.  They return 1 for success, -1 for an uninitialized I/O port object and 0 for
 * other errors. */
#define port_write_method(type,name) int32_t (*name)(object_pointer port_object, type in)

/*! \brief Name of EDI I/O port class. (Constant)
 *
 * A CPP constant with the value of #io_port_class */
#define	IO_PORT_CLASS	"EDI-IO-PORT"
/*! \brief Name of EDI I/O port class.
 *
 * An edi_string_t containing the class name "EDI-IO-PORT". */
#if defined(EDI_MAIN_FILE) || defined(IMPLEMENTING_EDI)
const edi_string_t io_port_class = IO_PORT_CLASS;
#else
extern const edi_string_t io_port_class;
#endif

#ifndef IMPLEMENTING_EDI
/*! \brief int32_t EDI-IO-PORT.init_io_port(unsigned int16_t port);
 *
 * This method takes an unsigned int16_t representing a particular I/O port and initializes the invoked EDI-IO-PORT object with it.
 * The method returns 1 if successful, -1 if the I/O port could not be obtained for the object, and 0 for all other errors. */
EDI_DEFVAR int32_t (*init_io_port)(object_pointer port_object, uint16_t port);
/*! \brief Get the port number from a port object. */
EDI_DEFVAR uint16_t (*get_port_number)(object_pointer port);
/*! \brief Method created by port_read_method() in order to read bytes (int8s) from I/O ports. */
EDI_DEFVAR int32_t (*read_byte_io_port)(object_pointer port_object, int8_t *out);
/*! \brief Method created by port_read_method() in order to read words (int16s) from I/O ports. */
EDI_DEFVAR int32_t (*read_word_io_port)(object_pointer port_object, int16_t *out);
/*! \brief Method created by port_read_method() in order to read longwords (int32s) from I/O ports. */
EDI_DEFVAR int32_t (*read_long_io_port)(object_pointer port_object, int32_t *out);
/*! \brief Method created by port_read_method() in order to read long longwords (int64s) from I/O ports. */
EDI_DEFVAR int32_t (*read_longlong_io_port)(object_pointer port_object,int64_t *out);
/*! \brief Method of EDI-IO-PORT to read long strings of data from I/O ports.
 *
 * Reads arbitrarily long strings of data from the given I/O port.  Returns 1 for success, -1 for an uninitialized port object, -2
 * for a bad pointer to the destination buffer, and 0 for all other errors. */
EDI_DEFVAR int32_t (*read_string_io_port)(object_pointer port_object, uint32_t data_length, uint8_t *out);
/*! \brief Method created by port_write_method() in order to write bytes (int8s) to I/O ports. */
EDI_DEFVAR int32_t (*write_byte_io_port)(object_pointer port_object, int8_t in);
/*! \brief Method created by port_write_method() in order to write words (int16s) to I/O ports. */
EDI_DEFVAR int32_t (*write_word_io_port)(object_pointer port_object, int16_t in);
/*! \brief Method created by port_write_method() in order to write longwords (int32s) to I/O ports. */
EDI_DEFVAR int32_t (*write_long_io_port)(object_pointer port_object, int32_t in);
/*! \brief Method created by port_write_method() in order to write long longwords (int64s) to I/O ports. */
EDI_DEFVAR int32_t (*write_longlong_io_port)(object_pointer port_object, int64_t in);
/*! \brief Method of EDI-IO-PORT to write long strings of data to I/O ports.
 *
 * Writes arbitrarily long strings of data to the given I/O port.  Returns 1 for success, -1 for an uninitialized port object, -2
 * for a bad pointer to the source buffer, and 0 for all other errors. */
EDI_DEFVAR int32_t (*write_string_io_port)(object_pointer port_object, uint32_t data_length, uint8_t *in);

#endif // defined(IMPLEMENTING_EDI)

#endif

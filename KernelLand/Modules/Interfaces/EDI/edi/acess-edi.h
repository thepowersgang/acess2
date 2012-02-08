/*! \file acess-edi.h
 * \brief Acess Specific EDI Objects
 * 
 * Contains documentation and information for
 * - Timers
 */

/* Copyright (c)  2006  John Hodge
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#ifndef ACESS_EDI_H
#define ACESS_EDI_H

#include "edi_objects.h"

/// \brief Name of Acess EDI Time Class
#define	ACESS_TIMER_CLASS	"ACESSEDI-TIMER"

#ifndef IMPLEMENTING_EDI
/*! \brief int32_t ACESSEDI-TIMER.init_timer(uint32_t Delay, void (*Callback)(int), int Arg);
 *
 * Takes a timer pointer and intialises the timer object to fire after \a Delay ms
 * When the timer fires, \a Callback is called with \a Arg passed to it.
 */
EDI_DEFVAR int32_t (*init_timer)(object_pointer port_object, uint32_t Delay, void (*fcn)(int), int arg);

/*! \brief void ACESSEDI-TIMER.disable_timer();
 * 
 * Disables the timer and prevents it from firing
 * After this has been called, the timer can then be initialised again.
 */
EDI_DEFVAR void (*disable_timer)(object_pointer port_object);


#endif // defined(IMPLEMENTING_EDI)

#endif

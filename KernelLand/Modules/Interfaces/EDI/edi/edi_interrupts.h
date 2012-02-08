#ifndef EDI_INTERRUPTS_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_INTERRUPTS_H

/*! \file edi_interrupts.h
 * \brief Declaration and description of EDI's interrupt handling class.
 *
 * Data structures and algorithms this header represents:
 *	DATA STRUCTURE AND ALGORITHM: INTERRUPT OBJECTS - The class EDI-INTERRUPT encapsulates the handling of machine interrupts.
 * It is initialized with an interrupt number to handle and a handler routine to call when that interrupt occurs.  Only a couple of
 * guarantees are made to the driver regarding the runtime's implementation of interrupt handling: 1) That the driver's handler is
 * called for every time the interrupt associated with a valid and initialized interrupt object occurs, in the order of the
 * occurences, 2) That the runtime handle the architecture-specific (general to the entire machine, not just this device)
 * end-of-interrupt code when the driver is called without first returning from the machine interrupt.  Note that the runtime hands
 * out interrupt numbers at its own discretion and policy. */

#include "edi_objects.h"

/*! \brief Macro constant containing the name of the interrupt class
 */
#define INTERRUPTS_CLASS	"EDI-INTERRUPT"
/*! \brief The name of EDI's interrupt-handling class.
 *
 * An edi_string_t holding the name of the runtime-implemented interrupt object class.  It's value is "EDI-INTERRUPT". */
#if defined(EDI_MAIN_FILE) || defined(IMPLEMENTING_EDI)
const edi_string_t interrupts_class = INTERRUPTS_CLASS;
#else
extern const edi_string_t interrupts_class;
#endif

/*! \brief A pointer to an interrupt handling function.
 *
 * A pointer to a function called to handle interrupts.  Its unsigned int32_t parameter is the interrupt number that is being
 * handled. */
typedef void (*interrupt_handler_t)(uint32_t interrupt_number);

#ifndef IMPLEMENTING_EDI
/*! \brief Initializes an interrupt object with an interrupt number and a pointer to a handler function.
 *
 * A pointer to the init_interrupt() method of class EDI-INTERRUPT.  This method initializes a newly-created interrupt object with an
 * interrupt number and a pointer to the driver's handler of type interrupt_handler_t.  It can only be called once per object, and
 * returns 1 on success, fails with -1 when the interrupt number is invalid or unacceptable to the runtime, fails with -2 when the
 * pointer to the driver's interrupt handler is invalid, and fails with -3 for all other errors. */
EDI_DEFVAR int32_t (*init_interrupt)(object_pointer interrupt, uint32_t interrupt_number, interrupt_handler_t handler);
/*! \brief Get this interrupt object's interrupt number. */
EDI_DEFVAR uint32_t (*interrupt_get_irq)(object_pointer interrupt);
/*! \brief Set a new handler for this interrupt object. */
EDI_DEFVAR void (*interrupt_set_handler)(object_pointer interrupt, interrupt_handler_t handler);
/*! \brief Return from this interrupt, letting the runtime run any necessary End-Of-Interrupt code.
 *
 * A pointer to the interrupt_return() method of class EDI-INTERRUPT.  This method returns from the interrupt designated by the
 * calling interrupt object.  If there is a machine-wide end-of-interrupt procedure and the driver was called during the handling of
 * the machine interrupt (as opposed to delaying the handling and letting the runtime EOI), the runtime runs it during this method.
 * This method has no return value, since once it's called control leaves the calling thread. */
EDI_DEFVAR void (*interrupt_return)(object_pointer interrupt);
#endif

#endif

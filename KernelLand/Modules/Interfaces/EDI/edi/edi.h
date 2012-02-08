#ifndef EDI_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_H
/*! \file edi.h
 * \brief The unitive EDI header to include others, start EDI, and stop EDI.
 *
 * Data structures and algorithms this header represents:
 *	DATA STRUCTURE: CLASS QUOTAS - The runtime and the driver have the right to set a quota on how many objects of a given class
 * owned by that party the other may construct.  These quotas are kept internally by the driver or runtime, are optional and are
 * exposed to the other party via the quota() function (for quotas of runtime-owned classes) and the k_quota() function pointer given
 * to the runtime by the driver.
 *
 *	ALGORITHMS: INITIALIZATION AND SHUTDOWN - On initialization of the runtime's EDI environment for this driver it calls the
 * driver's driver_init() routine (which must match driver_init_t) to initialize the driver with a list of EDI objects the runtime
 * thinks the driver should run with.  The driver then initializes.  This can include calling edi_negotiate_resources() to try and
 * obtain more or different objects.  Eventually driver_init() returns an edi_initialization_t structure containing its quota
 * function and the list of classes belonging to the driver which the runtime can construct.  Either the driver or the runtime can
 * shut down EDI by calling edi_shutdown(), which in turn calls the driver's driver_finish() routine.  On shutdown all objects, of
 * classes belonging to both the runtime and driver, are destroyed. */

#include "edi_objects.h"
#include "edi_dma_streams.h"
#include "edi_pthreads.h"
#include "edi_port_io.h"
#include "edi_memory_mapping.h"
#include "edi_devices.h"
#include "edi_interrupts.h"

/*! \brief A pointer to a function the runtime can call if it fails to construct one of the driver's classes to find out what the
 * runtime's quota is for that class.
 *
 * A pointer to a function which takes an edi_string_t as a parameter and returns in int32_t.  This function follows the same
 * semantics as the quota() function, returning the number of objects of the given class that can be constructed, -1 for infinity or
 * -2 for an erroneous class name.  It is used to tell the runtime the location of such a function in the driver so that the runtime
 * can check quotas on driver-owned classes. */
typedef int32_t (*k_quota_t)(edi_string_t resource_class);
/*!\struct edi_initialization_t
 * \brief Structure containing driver classes available to the runtime and the driver's quota function after the driver has initialized. 
 *
 * Structure containing driver classes available to runtime, the driver's quota function and the driver's name provided to the runtime
 * after the driver has initialized.  driver_bus, vendor_id, and device_id are all optional fields which coders should consider
 * supplementary information.  Kernels can require these fields if they so please, but doing so for devices which don't run on a Vendor
 * ID/Product ID supporting bus is rather unwise. */
typedef struct {
	/*!\brief The number of driver classes in the driver_classes array. */
	int32_t	num_driver_classes;
	/*!\brief An array of declarations of driver classes available to the runtime. 
	 *
	 * This array should not necessarily contain the entire list of EDI classes implemented by the driver.  Instead, it should
	 * contain a list of those classes which the driver has correctly initialized itself to provide instances of with full
	 * functionality. */
	edi_class_declaration_t *driver_classes;
	/*!\brief The driver's quota function. */
	k_quota_t k_quota;
	/*!\brief The driver's name. */
	edi_string_t driver_name;
	/*!\brief The bus of the device this driver wants to drive, if applicable.
	 *
	 * The driver does not have to supply this field, and can also supply "MULTIPLE BUSES" here to indicate that it drives devices
	 * on multiple buses. */
	edi_string_t driver_bus;
	/*!\brief The driver's vendor ID, if applicable.
	 *
	 * The driver does not need to supply this field, and should supply -1 to indicate that it does not wish to. */
	int16_t vendor_id;
	/*!\brief The driver's device ID, if applicable.
	 *
	 * The driver does not need to supply this field, but can supply it along with vendor_id.  If either vendor_id or this field are
	 * set to -1 the runtime should consider this field not supplied. */
	int16_t driver_id;
} edi_initialization_t;	
/*!\brief A pointer to a driver's initialization function.
 *
 * The protocol for the driver's initialization function.  The runtime gives the driver a set of EDI objects representing the
 * resources it thinks the driver should run with.  This function returns an edi_initialization_t structure containing declarations
 * of the EDI classes the driver can make available to the runtime after initialization.  If any member of that structure contains 0
 * or NULL, it is considered invalid and the runtime should destroy the driver without calling its driver_finish() routine. */
typedef edi_initialization_t (*driver_init_t)(int32_t num_resources,edi_object_metadata_t *resources);
/*!\brief Requests more resources from the runtime.  Can be called during driver initialization.
 *
 * Called to negotiate with the runtime for the right to create further EDI objects/obtain further resources owned by the runtime.
 * When the driver calls this routine, the runtime decides whether to grant more resources.  If yes, this call returns true, and the
 * driver can proceed to try and create the objects it desires, in addition to destroying EDI objects it doesn't want.  Otherwise,
 * it returns false.
 * The driver must deal with whatever value this routine returns. */
bool edi_negotiate_resources();

/*! \brief Returns the driver's quota of objects for a given runtime-owned class. 
 *
 * This function takes an edi_string_t with the name of a runtime-owned class in it and returns the number of objects of that class
 * which drivers can construct, -1 for infinity, or -2 for an erroneous class name. */
int32_t quota(edi_string_t resource_class);
/*! \brief Sends a string to the operating systems debug output or logging facilities. */
void edi_debug_write(uint32_t debug_string_length, char *debug_string);
/*! \brief This call destroys all objects and shuts down the entire EDI environment of the driver. 
 *
 * This function shuts down EDI as described in INITIALIZATION AND SHUTDOWN above.  All objects are destroyed, EDI functions can no
 * longer be successfully called, etc.  This function only succeeds when EDI has already been initialized, so it returns -1 when EDI
 * hasn't been, 1 on success, or 0 for all other errors. */
int32_t shutdown_edi(void);

/*!\brief A pointer to the driver's finishing/shutdown function.
 *
 * The protocol for the driver's shutting down.  This function should do anything the driver wants done before it dies. */
typedef void (*driver_finish_t)();

#endif

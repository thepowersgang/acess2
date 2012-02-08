#ifndef EDI_MEMORY_MAPPING_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_MEMORY_MAPPING_H

/*! \file edi_memory_mapping.h
 * \brief Declaration and description of EDI's class for mapping physical pages into the driver's address space.
 *
 * Data structures and algorithms this header represents:
 *	ALGORITHM: MEMORY MAPPINGS - Memory mapping objects of the class EDI-MEMORY-MAPPING are used to give virtual (driver-visible)
 * addresses to sections of physical memory.  These can either be memory mappings belonging to hardware devices or plain RAM which
 * the driver wants page-aligned.  A memory mapping object is initialized with the physical address for the memory mapping and the
 * number of pages the mapping takes up, or simply the desired length of the a physically contiguous buffer in pages.  The class's
 * two methods map the section of memory into and out of the driver's virtual address space. */

#include "edi_objects.h"

/*! \brief The name of EDI's memory mapping class.
 *
 * An edi_string_t with the name of the memory mapping class, "EDI-MEMORY-MAPPING". */
#if defined(EDI_MAIN_FILE) || defined(IMPLEMENTING_EDI)
const edi_string_t memory_mapping_class = "EDI-MEMORY-MAPPING";
#else
extern const edi_string_t memory_mapping_class;
#endif

/*! \brief Flag representing Strong Uncacheable caching method. */
#define CACHING_STRONG_UNCACHEABLE 0
/*! \brief Flag representing Uncacheable caching method. */
#define CACHING_UNCACHEABLE 1
/*! \brief Flag representing Write combining caching method. */
#define CACHING_WRITE_COMBINING 2
/*! \brief Flag representing Write Through caching method. */
#define CACHING_WRITE_THROUGH 3
/*! \brief Flag representing Write Back caching method. */
#define CACHING_WRITE_BACK 3
/*! \brief Flag representing Write Protected caching method. */
#define CACHING_WRITE_PROTECTED 3

#ifndef IMPLEMENTING_EDI
/*! \brief Initialize an EDI-MEMORY-MAPPING object with a physical address range.
 *
 * This method takes the start_physical_address of a memory mapping and the number of pages in that mapping and uses these arguments
 * to initialize an EDI-MEMORY-MAPPING object.  It can only be called once per object.  It returns 1 when successful, -1 when an
 * invalid physical address is given (one that the runtime knows is neither a physical memory mapping belonging to a device nor
 * normal RAM), -2 when the number of pages requested is bad (for the same reasons as the starting address can be bad), and 0 for
 * all other errors. 
 *
 * Note that this method can't be invoked on an object which has already initialized via init_memory_mapping_with_pages(). */
EDI_DEFVAR int32_t (*init_memory_mapping_with_address)(object_pointer mapping, data_pointer start_physical_address, uint32_t pages);
/*! \brief Initialize an EDI-MEMORY-MAPPING object by requesting a number of new physical pages.
 *
 * This method takes a desired number of physical pages for a memory mapping, and uses that number to initialize an
 * EDI-MEMORY-MAPPING object by creating a buffer of contiguous physical pages.  It can only be called once per object.  It returns
 * 1 when successful, -1 when the request for pages cannot be fulfilled, and 0 for all other errors.
 *
 * Note that this method cannot be called if init_memory_mapping_with_address() has already been used on the given object. */
EDI_DEFVAR int32_t (*init_memory_mapping_with_pages)(object_pointer mapping, uint32_t pages);
/*! \brief Map the memory-mapping into this driver's visible address space.
 *
 * This asks the runtime to map a given memory mapping into the driver's virtual address space.  Its parameter is the address of a
 * data_pointer to place the virtual address of the mapping into.  This method returns 1 on success, -1 on an invalid argument, -2
 * for an uninitialized object, and 0 for all other errors. */
EDI_DEFVAR int32_t (*map_in_mapping)(object_pointer mapping, data_pointer *address_mapped_to);
/*! \brief Unmap the memory mapping from this driver's visible address space.
 *
 * This method tries to map the given memory mapping out of the driver's virtual address space.  It returns 1 for success, -1
 * for an uninitialized memory mapping object, -2 if the mapping isn't mapped into the driver's address space already, and 0
 * for all other errors. */
EDI_DEFVAR int32_t (*map_out_mapping)(object_pointer mapping);

/*! \brief Set the caching flags for a memory mapping. */
EDI_DEFVAR void (*mapping_set_caching_method)(object_pointer mapping, uint32_t caching_method);
/*! \brief Get the current caching method for a memory mapping. */
EDI_DEFVAR uint32_t (*mapping_get_caching_method)(object_pointer mapping);
/*! \brief Flush write-combining buffers on CPU to make sure changes to memory mapping actually get written.  Only applies to a Write Combining caching method (I think.).*/
EDI_DEFVAR void (*flush_write_combining_mapping)(object_pointer mapping);
#endif

#endif

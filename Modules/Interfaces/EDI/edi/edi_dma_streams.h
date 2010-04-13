#ifndef EDI_DMA_STREAMS_H

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_DMA_STREAMS_H

/*! \file edi_dma_streams.h 
 * \brief EDI's stream subclass for handling Direct Memory Access hardware.
 *
 * Data structures and algorithms this header represents:
 *
 *	DATA STRUCTURE: DMA STREAMS - DMA streams are objects of the class EDI-STREAM-DMA used to pass data between a buffer of
 * memory and the computer's DMA hardware.  It is the responsibility of the object to allocate memory for its stream memory buffer
 * which can be used with DMA hardware and to program the DMA hardware for transmissions.  DMA streams can be bidirectional if the
 * correct DMA mode is used. */

#include "edi_objects.h"

#define DMA_STREAM_CLASS	"EDI-STREAM-DMA"

/*! \brief The name of the EDI DMA stream class.
 *
 * An edi_string_t with the class name "EDI-STREAM-DMA" in it. */
#if defined(EDI_MAIN_FILE) || defined(IMPLEMENTING_EDI)
const edi_string_t dma_stream_class = DMA_STREAM_CLASS;
#else
extern const edi_string_t dma_stream_class;
#endif

#ifndef IMPLEMENTING_EDI
/*! \brief int32_t EDI-STREAM-DMA.init_dma_stream(unsigned int32_t channel,unsigned int32_t mode,unsigned int32_t buffer_pages);
 *
 * Pointer to the init_dma_stream() method of class EDI-STREAM-DMA, which initializes a DMA stream with a DMA channel, DMA mode, and
 * the number of DMA-accessible memory pages to keep as a buffer.  It will only work once per stream object.  It's possible return
 * values are 1 for sucess, -1 for invalid DMA channel, -2 for invalid DMA mode, -3 for inability to allocate enough buffer pages and
 * 0 for all other errors. */
EDI_DEFVAR  int32_t (*init_dma_stream)(object_pointer stream, uint32_t channel, uint32_t mode, uint32_t buffer_pages);
/*! \brief int32_t EDI-STREAM-DMA.transmit(data_pointer *anchor,unsigned int32 num_bytes,bool sending);
 *
 * Pointer to the dma_stream_transmit() method of class EDI-STREAM-DMA, which transmits the given number of bytes of data through
 * the DMA stream to/from the given anchor (either source or destination), in the given direction.  It returns 1 on success, -1 on
 * an uninitialized or invalid DMA stream object, -2 when the anchor was NULL or otherwise invalid, -3 if the DMA stream can't
 * transmit in the given direction, and 0 for all other errors. */
EDI_DEFVAR int32_t (*dma_stream_transmit)(object_pointer stream, data_pointer anchor, uint32_t num_bytes, bool sending);
#endif

#endif

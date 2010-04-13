/**
 * \file apidoc/arch_x86.h
 * \brief x86(-64) Specific Functions
 * \author John Hodge (thePowersGang)
 *
 * \section toc	Table of Contents
 * - \ref portio "Port IO"
 * - \ref dma "DMA - Direct Memory Access"
 * 
 * \section portio Port IO
 * The x86 architecture has two memory spaces, the first is the system
 * memory accessable using standard loads and stores. The second is the
 * 16-bit IO Bus. This bus is accessed using the \a in and \a out opcodes
 * and is used to configure devices attached to the system.
 * A driver should not use \a in and \a out directly, but instead use
 * the provided \a in* and \a out* functions to access the IO Bus.
 * This allows the kernel to run a driver in userspace if requested without
 * the binary needing to be altered.
 * 
 * \section dma	DMA - Direct Memory Access
 */

/**
 * \name IO Bus Access
 * \{
 */
extern Uint8	inb(Uint16 Port);	//!< Read 1 byte from the IO Bus
extern Uint16	inw(Uint16 Port);	//!< Read 2 bytes from the IO Bus
extern Uint32	inl(Uint16 Port);	//!< Read 4 bytes from the IO Bus
extern Uint64	inq(Uint16 Port);	//!< Read 8 bytes from the IO Bus\

extern void	outb(Uint16 Port, Uint8 Value);	//!< Write 1 byte to the IO Bus
extern void	outw(Uint16 Port, Uint16 Value);	//!< Write 2 bytes to the IO Bus
extern void	outl(Uint16 Port, Uint32 Value);	//!< Write 4 bytes to the IO Bus
extern void	outq(Uint16 Port, Uint64 Value);	//!< Write 8 bytes to the IO Bus
/**
 * \}
 */

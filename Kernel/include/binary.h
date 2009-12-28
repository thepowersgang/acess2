/**
 * \file binary.h
 * \brief Binary Loader Definitions
 * \author John Hodge (thePowersGang)
 */
#ifndef _BINARY_H
#define _BINARY_H

// === TYPES ===
typedef struct sBinaryPage
{
	/**
	 * \brief Physical address, or file offset
	 * 
	 * Physical address of this page or, when the file is not yet
	 * loaded, this is a file offset (or -1 for uninitialised data)
	 */
	tPAddr	Physical;
	tVAddr	Virtual;	//!< Virtual load address
	Uint16	Size;	//!< Number of bytes to load from the file
	Uint16	Flags;	//!< Load Flags
} __attribute__ ((packed))	tBinaryPage;

/**
 * \brief Defines a binary file
 * 
 * This structure defines and maintains the state of a binary during and
 * after loading.
 * Before the binary is loaded into memory (when it has just been returned
 * from tBinaryType.Load) the \a Pages array will contain the file offsets
 * to the page data in the \a Physical fields (or -1 for uninitialised
 * data) and the \a Size fields define how much data is stored in-file
 * for the page (to allow partial pages to be loaded from disk)
 * Once the binary is loaded (NOTE: Drivers do not need to know about this,
 * it is here for information only) the \a Physical fields now contain the
 * physical addresses of the pages filled with the data. The \a Virtual
 * fields contain the preferred virtual address of the pages (a given
 * process may have these pages mapped to a different location).
 */
typedef struct sBinary
{
	struct sBinary	*Next;	//!< Pointer used by the kernel
	/**
	 * \brief True path of the file
	 * \note Used to uniquely identify the loaded binary to reduce in-memory
	 *       duplication.
	 */
	char	*TruePath;
	/**
	 * \brief Interpreter used to load the file
	 * \note This can be either requested by the individual file, or a per-driver option
	 */
	char	*Interpreter;
	/**
	 * \brief Entrypoint of the binary (at requested base);
	 */
	Uint	Entry;
	/**
	 * \brief File's requested load base
	 */
	Uint	Base;
	/**
	 * \brief Number of times this binary has been mapped
	 */
	 int	ReferenceCount;
	/**
	 * \brief Number of pages defined in the file
	 */
	 int	NumPages;
	/**
	 * \brief Array of pages defined by this binary
	 * \note Contains \a NumPages entries
	 */
	tBinaryPage	Pages[];
}	tBinary;

/**
 * \brief Binary type definition
 * 
 * This structure is used to define a loader for a specific binary type
 * so that the kernel's core binary loader can understand that type.
 * The tBinaryType.Relocate and tBinaryType.GetSymbol need only be non-NULL
 * if the binary type is to be used for kernel modules, otherwise it will
 * only be able to load binaries for user space.
 */
typedef struct sBinaryType
{
	/**
	 * \brief Pointer used by the kernel
	 * \note Should not be touched by the driver (initialise to NULL)
	 */
	struct sBinaryType	*Next;
	/**
	 * \brief Identifying DWord
	 * 
	 * If he first 32-bits of the file match this value (when ANDed with
	 * tBinaryType.Mask), this binary loader will be used to load the file.
	 */
	Uint32	Ident;
	Uint32	Mask;	//!< Mask value for tBinaryType.Ident
	char	*Name;	//!< Name of this executable type (for debug purpouses)
	/**
	 * \brief Read a binary from a file
	 * \param FD	VFS File handle to file to load
	 * \return Pointer to a ::tBinary that describes how to load the file
	 * 
	 * This function reads a binary file and returns a ::tBinary pointer
	 * that tells the core binary loader how to read the data from the file
	 * and where to map it to.
	 */
	tBinary	*(*Load)(int FD);
	
	/**
	 * \brief Prepares a mapped binary for execution at this address
	 * \param Base	Binary loaded in memory
	 * \return Boolean Success
	 * \note This pointer can be NULL, but then the binary cannot be used
	 *       to load a kernel module.
	 * 
	 * tBinaryType.Relocate takes a binary that was loaded according to
	 * tBinaryType.Load and prepares it to run at the address it is
	 * loaded to, attempting to satisfy any external unresolved symbols
	 * required, if a symbol cannot be located, the function will return
	 * zero.
	 */
	 int	(*Relocate)(void *Base);
	 
	 /**
	  * \brief Gets a symbol's address from a loaded binary
	  * \note The binary pointed to by \a Base may not have been through
	  *       tBinaryType.Relocate at this time, so the driver should
	  *       accomodate this.
	  */
	 int	(*GetSymbol)(void *Base, char *Name, Uint *Dest);
} tBinaryType;

/**
 * \brief Registers an interpreter path with the binary loader
 * \param Path	Path to the requested interpreter (need not be a "true" path)
 * \return Pointer to the cached string
 * 
 * Speeds up checking if the intepreter is loaded in the kernel by allowing
 * the search to use pointer comparisons instead of string comparisons.
 */
extern char	*Binary_RegInterp(char *Path);

#endif

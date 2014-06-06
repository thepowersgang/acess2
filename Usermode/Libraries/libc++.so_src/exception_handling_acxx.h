/*
 * Acess2 C++ Support Library
 * - By John Hodge (thePowersGang)
 *
 * exception_handling_acxx.h
 * - C++ Exception handling definions from HP's aC++ document
 *
 * Reference: http://mentorembedded.github.io/cxx-abi/exceptions.pdf
 */
#ifndef _EXCEPTION_HANLDING_ACXX_H_
#define _EXCEPTION_HANLDING_ACXX_H_

/**
 * \brief Language Specific Data Area
 * \note Pointer obtained via _Unwind_GetLanguageSpecificData
 */
struct sLSDA_Header
{
	const void	*Base;
	uintptr_t	LPStart;
	uint8_t 	TTEncoding;
	uintptr_t	TypePtrBase;	// base address for type pointers
	uintptr_t	TTBase;	// Base address for type offsets
	uint8_t	CallSiteEncoding;
	const void	*CallSiteTable;
	const void	*ActionTable;
};

/* Pointer encodings, from dwarf2.h.  */ 
#define DW_EH_PE_absptr         0x00 
#define DW_EH_PE_omit           0xff 

// - Data format (mask with 0x0F)
#define DW_EH_PE_fmtmask	0x0F
#define DW_EH_PE_uleb128        0x01 
#define DW_EH_PE_udata2         0x02 
#define DW_EH_PE_udata4         0x03 
#define DW_EH_PE_udata8         0x04 
#define DW_EH_PE_sleb128        0x09 
#define DW_EH_PE_sdata2         0x0A 
#define DW_EH_PE_sdata4         0x0B 
#define DW_EH_PE_sdata8         0x0C 
#define DW_EH_PE_signed         0x08 
// - Data maipulation (0x70)
#define DW_EH_PE_relmask	0x70
#define DW_EH_PE_pcrel          0x10 
#define DW_EH_PE_textrel        0x20 
#define DW_EH_PE_datarel        0x30 
#define DW_EH_PE_funcrel        0x40 
#define DW_EH_PE_aligned        0x50 
 
#define DW_EH_PE_indirect       0x80 

typedef	uint64_t	leb128u_t;
typedef	 int64_t	leb128s_t;


#endif


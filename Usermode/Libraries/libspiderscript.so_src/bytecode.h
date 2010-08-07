/*
 * SpiderScript
 * - Bytecode definitions
 */
#ifndef _BYTECODE_H_
#define _BYTECODE_H_

struct sBytecodeHeader
{
	uint32_t	Magic;	//!< Magic Value (identifier) "\x8FSS\r"
	uint32_t	NFunctions;	//!< Number of functions
	struct {
		uint32_t	NameOffset;	//!< Offset to the name 
		uint32_t	CodeOffset;	//!< Offset to the code
	}	Functions[];
};

enum eBytecodeOperations
{
	BCOP_UNDEF,
	BCOP_NOP,
	
	BCOP_DEFVAR,
	BCOP_RETURN,
	
	NUM_BCOP
}

#endif

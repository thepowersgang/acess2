/*
 * SpiderScript
 * - Bytecode definitions
 */
#ifndef _BYTECODE_H_
#define _BYTECODE_H_

#include "bytecode_ops.h"

typedef struct sBC_Op	tBC_Op;
typedef struct sBC_Function	tBC_Function;

struct sBC_Op
{
	tBC_Op	*Next;
	 int	Operation;
	 int	bUseInteger;	// Used for serialisation
	union {
		struct {
			const char *String;
			 int	Integer;
		} StringInt;
		
		uint64_t	Integer;
		double	Real;
	} Content;
};

struct sBC_Function
{
	 int	LabelCount;
	 int	LabelSpace;
	tBC_Op	**Labels;
	
	 int	MaxVariableCount;
	// NOTE: These fields are invalid after compilation
	 int	VariableCount;
	 int	VariableSpace;
	const char	**VariableNames;
	 int	CurContextDepth;	// Used to get the real var count

	 int	OperationCount;
	tBC_Op	*Operations;
	tBC_Op	*OperationsEnd;
};

#endif

/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * bytecode_makefile.c
 * - Generate a bytecode file
 */
#include <stdlib.h>
#include "bytecode_ops.h"

typedef sBC_StackEnt	tBC_StackEnt;

enum eBC_StackEntTypes
{
	ET_NULL,	// Start of the stack
	ET_FUNCTION_START,	// Start of the function
	ET_INTEGER,	// Integer / Boolean
	ET_REAL,	// Real number
	ET_OBJECT,	// Object
	ET_REFERENCE	// Reference to a tSpiderValue
};

struct sBC_StackEnt
{
	uint8_t	EntType;
	uint8_t	_padding[3];
	union {
		uint64_t	Integer;
		double  	Real;
		tSpiderValue	*Reference;
		tSpiderObject	*Object;
	};
};

struct sBC_Stack
{
	 int	EntrySpace;
	 int	EntryCount;
	tBC_StackEnt	Entries[];
};

// === CODE ===
int Bytecode_int_StackPop(tBC_Stack *Stack, tBC_StackEnt *Dest)
{
	if( Stack->EntryCount == 0 )	return 1;
	Stack->EntryCount --;
	*Dest = Stack->Entries[Stack->EntryCount];
	return 0;
}

int Bytecode_int_StackPush(tBC_Stack *Stack, tBC_StackEnt *Src)
{
	if( Stack->EntryCount == Stack->EntrySpace )	return 1;
	Stack->Entries[Stack->EntryCount] = *Src;
	Stack->EntryCount ++;
	return 0;
}

#define GET_STACKVAL(dst)	if((ret = Bytecode_int_StackPop(Stack, &dst)))    return ret;

int Bytecode_ExecuteFunction(tBC_Function *Fcn, tBC_Stack *Stack, int ArgCount);
{
	tBC_Op	*op;
	tBC_StackEnt	val1, val2;
	tBC_StackEnt	local_vars[Fcn->MaxVariableCount+Fcn->ArgumentCount];
	
	// Pop off arguments
	
	// Mark the start

	// Execute!
	op = Fcn->Operations;
	while(op)
	{
		tBC_Op	*nextop = op->Next;
		switch(op->Type)
		{
		case BC_OP_JUMP:
			nextop = Fcn->Labels[op->StringInt.Integer];
			break;
		case BC_OP_JUMPIF:
			GET_STACKVAL(val1);
			if( Bytecode_int_IsStackEntTrue(&val1) )
				nextop = Fcn->Labels[op->StringInt.Integer];
			break;
		case BC_OP_JUMPIFNOT:
			GET_STACKVAL(val1);
			if( !Bytecode_int_IsStackEntTrue(&val1) )
				nextop = Fcn->Labels[op->StringInt.Integer];
			break;
		default:
			// TODO:
			break;
		}
		op = nextop;
	}
	
	// Clean up
}

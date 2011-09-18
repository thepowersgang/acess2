/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * exec_bytecode.c
 * - Execute bytecode
 */
#include <stdlib.h>
#include <stdint.h>
#include "common.h"
#include "bytecode.h"
#include <string.h>
#include "ast.h"

typedef struct sBC_StackEnt	tBC_StackEnt;
typedef struct sBC_Stack	tBC_Stack;

enum eBC_StackEntTypes
{
	ET_NULL,	// Start of the stack
	// SS_DATATYPE_*
	ET_FUNCTION_START = NUM_SS_DATATYPES,
	ET_REFERENCE	// Reference to a tSpiderValue
};

struct sBC_StackEnt
{
	uint8_t	Type;
	union {
		uint64_t	Integer;
		double  	Real;
		tSpiderValue	*Reference;	// Used for everything else
		tSpiderObject	*Object;
	};
};

struct sBC_Stack
{
	 int	EntrySpace;
	 int	EntryCount;
	tBC_StackEnt	Entries[];
};

// === PROTOTYPES ===
tSpiderValue	*Bytecode_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Args);
 int	Bytecode_int_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, tBC_Stack *Stack, int ArgCount);

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

int Bytecode_int_IsStackEntTrue(tBC_StackEnt *Ent)
{
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
		return !!Ent->Integer;
	case SS_DATATYPE_REAL:
		return (-.5f < Ent->Real && Ent->Real < 0.5f);
	case SS_DATATYPE_OBJECT:
		return Ent->Object != NULL;
	case ET_FUNCTION_START:
		return -1;
	default:
		return SpiderScript_IsValueTrue(Ent->Reference);
	}
}

tSpiderValue *Bytecode_int_GetSpiderValue(tBC_StackEnt *Ent, tSpiderValue *tmp)
{
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
		tmp->Type = SS_DATATYPE_INTEGER;
		tmp->ReferenceCount = 2;	// Stops it being freed
		tmp->Integer = Ent->Integer;
		return tmp;
	case SS_DATATYPE_REAL:
		tmp->Type = SS_DATATYPE_REAL;
		tmp->ReferenceCount = 2;	// Stops it being freed
		tmp->Real = Ent->Real;
		return tmp;
	case SS_DATATYPE_OBJECT:
		tmp->Type = SS_DATATYPE_OBJECT;
		tmp->ReferenceCount = 2;
		tmp->Object = Ent->Object;
		return tmp;
	case ET_FUNCTION_START:
		return NULL;
	default:
		return Ent->Reference;
	}
}

void Bytecode_int_SetSpiderValue(tBC_StackEnt *Ent, tSpiderValue *Value)
{
	switch(Value->Type)
	{
	case SS_DATATYPE_INTEGER:
		Ent->Type = SS_DATATYPE_INTEGER;
		Ent->Integer = Value->Integer;
		break;
	case SS_DATATYPE_REAL:
		Ent->Type = SS_DATATYPE_REAL;
		Ent->Real = Value->Real;
		break;
	case SS_DATATYPE_OBJECT:
		Ent->Type = SS_DATATYPE_OBJECT;
		Ent->Object = Value->Object;
		break;
	default:
		SpiderScript_ReferenceValue(Value);
		Ent->Reference = Value;
		break;
	}
}

#define GET_STACKVAL(dst)	if((ret = Bytecode_int_StackPop(Stack, &dst)))    return ret;
#define OP_INDX(op_ptr)	((op_ptr)->Content.StringInt.Integer)
#define OP_STRING(op_ptr)	((op_ptr)->Content.StringInt.String)

tSpiderValue *Bytecode_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Args)
{
	const int	stack_size = 100;
	tSpiderValue	*ret, tmpsval;
	tBC_Stack	*stack;
	tBC_StackEnt	val;
	 int	i;
	
	stack = malloc(sizeof(tBC_Stack) + stack_size*sizeof(tBC_StackEnt));
	stack->EntrySpace = stack_size;
	stack->EntryCount = 0;

	// Push arguments in order (so top is last arg)
	for( i = 0; i < NArguments; i ++ )
	{
		Bytecode_int_SetSpiderValue(&val, Args[i]);
		Bytecode_int_StackPush(stack, &val);
	}

	// Call
	Bytecode_int_ExecuteFunction(Script, Fcn, stack, NArguments);

	// Get return value
	Bytecode_int_StackPop(stack, &val);
	ret = Bytecode_int_GetSpiderValue(&val, &tmpsval);
	// Ensure it's a heap value
	if(ret == &tmpsval) {
		ret = malloc(sizeof(tSpiderValue));
		memcpy(ret, &tmpsval, sizeof(tSpiderValue));
	}
	return ret;
}

/**
 * \brief Execute a bytecode function with a stack
 */
int Bytecode_int_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, tBC_Stack *Stack, int ArgCount)
{
	 int	ret, ast_op, i;
	tBC_Op	*op;
	tBC_StackEnt	val1, val2;
	tBC_StackEnt	local_vars[Fcn->BCFcn->MaxVariableCount];	// Includes arguments
	tSpiderValue	tmpVal1, tmpVal2;	// temp storage
	tSpiderValue	*pval1, *pval2, *ret_val;
	
	// Pop off arguments
	if( ArgCount > Fcn->ArgumentCount )	return -1;
	for( i = Fcn->ArgumentCount; --i != ArgCount; )
	{
		local_vars[i].Integer = 0;
		local_vars[i].Type = Fcn->Arguments[i].Type;
	}
	for( ; i --; )
	{
		GET_STACKVAL(local_vars[i]);
		// TODO: Type checks / enforcing
	}
	
	// Mark the start
	memset(&val1, 0, sizeof(val1));
	val1.Type = ET_FUNCTION_START;
	Bytecode_int_StackPush(Stack, &val1);

	// Execute!
	op = Fcn->BCFcn->Operations;
	while(op)
	{
		tBC_Op	*nextop = op->Next;
		ast_op = 0;
		switch(op->Operation)
		{
		// Jumps
		case BC_OP_JUMP:
			nextop = Fcn->BCFcn->Labels[ OP_INDX(op) ];
			break;
		case BC_OP_JUMPIF:
			GET_STACKVAL(val1);
			if( Bytecode_int_IsStackEntTrue(&val1) )
				nextop = Fcn->BCFcn->Labels[ OP_INDX(op) ];
			break;
		case BC_OP_JUMPIFNOT:
			GET_STACKVAL(val1);
			if( !Bytecode_int_IsStackEntTrue(&val1) )
				nextop = Fcn->BCFcn->Labels[ OP_INDX(op) ];
			break;
		
		// Define variables
		case BC_OP_DEFINEVAR: {
			 int	type, slot;
			type = OP_INDX(op) & 0xFFFF;
			slot = OP_INDX(op) >> 16;
			if(slot < 0 || slot >= sizeof(local_vars)/sizeof(local_vars[0]))	return -1;
			memset(&local_vars[slot], 0, sizeof(local_vars[0]));
			local_vars[slot].Type = type;
			} break;
		
		// Operations
		case BC_OP_ADD:
			ast_op = NODETYPE_ADD;
		case BC_OP_SUBTRACT:
			if(!ast_op)	ast_op = NODETYPE_SUBTRACT;
			
			GET_STACKVAL(val2);
			GET_STACKVAL(val1);
			pval1 = Bytecode_int_GetSpiderValue(&val1, &tmpVal1);
			pval2 = Bytecode_int_GetSpiderValue(&val2, &tmpVal2);
			ret_val = AST_ExecuteNode_BinOp(Script, NULL, ast_op, pval1, pval2);
			if(pval1 != &tmpVal1)	SpiderScript_DereferenceValue(pval1);
			if(pval2 != &tmpVal2)	SpiderScript_DereferenceValue(pval2);
			Bytecode_int_SetSpiderValue(&val1, ret_val);
			if(ret_val != &tmpVal1)	SpiderScript_DereferenceValue(ret_val);
			break;

		default:
			// TODO:
			break;
		}
		op = nextop;
	}
	
	// Clean up
	// - Delete local vars

	
	// - Restore stack
	

	return 0;
}


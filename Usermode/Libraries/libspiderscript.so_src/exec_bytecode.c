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
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include <inttypes.h>

#define TRACE	0

#if TRACE
# define DEBUG_F(v...)	printf(v)
#else
# define DEBUG_F(v...)
#endif

// === IMPORTS ===
extern void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === TYPES ===
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
		int64_t	Integer;
		double 	Real;
		tSpiderValue	*Reference;	// Used for everything else
		tSpiderObject	*Object;
		tSpiderNamespace	*Namespace;
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
		return !(-.5f < Ent->Real && Ent->Real < 0.5f);
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
	case SS_DATATYPE_REAL:
	case SS_DATATYPE_OBJECT:
		if(!tmp) {
			tmp = malloc(sizeof(tSpiderValue));
			tmp->ReferenceCount = 1;
		} else {
			tmp->ReferenceCount = 2;
		}
		break;
	default:
		break;
	}
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
		tmp->Type = SS_DATATYPE_INTEGER;
		tmp->Integer = Ent->Integer;
		return tmp;
	case SS_DATATYPE_REAL:
		tmp->Type = SS_DATATYPE_REAL;
		tmp->Real = Ent->Real;
		return tmp;
	case SS_DATATYPE_OBJECT:
		tmp->Type = SS_DATATYPE_OBJECT;
		tmp->Object = Ent->Object;
		return tmp;
	case ET_FUNCTION_START:
		AST_RuntimeError(NULL, "_GetSpiderValue on ET_FUNCTION_START");
		return NULL;
	default:
		SpiderScript_ReferenceValue(Ent->Reference);
		return Ent->Reference;
	}
}

void Bytecode_int_SetSpiderValue(tBC_StackEnt *Ent, tSpiderValue *Value)
{
	if(!Value) {
		Ent->Type = ET_REFERENCE;
		Ent->Reference = NULL;
		return ;
	}
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
		Ent->Object->ReferenceCount ++;
		break;
	default:
		SpiderScript_ReferenceValue(Value);
		Ent->Type = ET_REFERENCE;
		Ent->Reference = Value;
		break;
	}
}

void Bytecode_int_DerefStackValue(tBC_StackEnt *Ent)
{
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
	case SS_DATATYPE_REAL:
		break;
	case SS_DATATYPE_OBJECT:
		if(Ent->Object) {
			Ent->Object->ReferenceCount --;
			if(Ent->Object->ReferenceCount == 0) {
				Ent->Object->Type->Destructor( Ent->Object );
			}
//			printf("Object %p derefed (obj refcount = %i)\n", Ent->Object, Ent->Object->ReferenceCount);
		}
		Ent->Object = NULL;
		break;
	default:
		if(Ent->Reference)
			SpiderScript_DereferenceValue(Ent->Reference);
		Ent->Reference = NULL;
		break;
	}
}
void Bytecode_int_RefStackValue(tBC_StackEnt *Ent)
{
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
	case SS_DATATYPE_REAL:
		break;
	case SS_DATATYPE_OBJECT:
		if(Ent->Object) {
			Ent->Object->ReferenceCount ++;
//			printf("Object %p referenced (count = %i)\n", Ent->Object, Ent->Object->ReferenceCount);
		}
		break;
	default:
		if(Ent->Reference)
			SpiderScript_ReferenceValue(Ent->Reference);
		break;
	}
}

void Bytecode_int_PrintStackValue(tBC_StackEnt *Ent)
{
	switch(Ent->Type)
	{
	case SS_DATATYPE_INTEGER:
		printf("0x%"PRIx64, Ent->Integer);
		break;
	case SS_DATATYPE_REAL:
		printf("%lf", Ent->Real);
		break;
	case SS_DATATYPE_OBJECT:
		printf("Obj %p", Ent->Object);
		break;
	default:
		printf("*%p", Ent->Reference);
		break;
	}
}

#if TRACE
# define PRINT_STACKVAL(val)	Bytecode_int_PrintStackValue(&val)
#else
# define PRINT_STACKVAL(val)
#endif

#define GET_STACKVAL(dst)	if((ret = Bytecode_int_StackPop(Stack, &dst))) { \
	AST_RuntimeError(NULL, "Stack pop failed, empty stack");\
	return ret; \
}
#define PUT_STACKVAL(src)	if((ret = Bytecode_int_StackPush(Stack, &src))) { \
	AST_RuntimeError(NULL, "Stack push failed, full stack");\
	return ret; \
}
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
	if( Bytecode_int_StackPop(stack, &val) ) {
		free(stack);
		return NULL;
	}
	free(stack);
	ret = Bytecode_int_GetSpiderValue(&val, &tmpsval);
	// Ensure it's a heap value
	if(ret == &tmpsval) {
		ret = malloc(sizeof(tSpiderValue));
		memcpy(ret, &tmpsval, sizeof(tSpiderValue));
	}

	return ret;
}

tSpiderNamespace *Bytecode_int_ResolveNamespace(tSpiderNamespace *Start, const char *Name, const char **FinalName)
{
	char	*pos;
	tSpiderNamespace	*ns = Start;
	while( (pos = strchr(Name, BC_NS_SEPARATOR)) )
	{
		 int	len = pos - Name;
		for( ns = ns->FirstChild; ns; ns = ns->Next )
		{
			if(memcmp(ns->Name, Name, len) == 0 && ns->Name[len] == 0)
			break;
		}
		if(!ns) {
			return NULL;
		}
		Name += len + 1;
	}
	if(FinalName)	*FinalName = Name;
	return ns;
}

#define STATE_HDR()	DEBUG_F("%p %2i ", op, Stack->EntryCount)

/**
 * \brief Execute a bytecode function with a stack
 */
int Bytecode_int_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, tBC_Stack *Stack, int ArgCount)
{
	 int	ret, ast_op, i;
	tBC_Op	*op;
	tBC_StackEnt	val1, val2;
	 int	local_var_count = Fcn->BCFcn->MaxVariableCount;
	tBC_StackEnt	local_vars[local_var_count];	// Includes arguments
	tSpiderValue	tmpVal1, tmpVal2;	// temp storage
	tSpiderValue	*pval1, *pval2, *ret_val;
	tSpiderNamespace	*default_namespace = &Script->Variant->RootNamespace;

	// Initialise local vars
	for( i = 0; i < local_var_count; i ++ )
		local_vars[i].Type = ET_NULL;
	
	// Pop off arguments
	if( ArgCount > Fcn->ArgumentCount )	return -1;
	DEBUG_F("Fcn->ArgumentCount = %i\n", Fcn->ArgumentCount);
	for( i = Fcn->ArgumentCount; i > ArgCount; )
	{
		i --;
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
	PUT_STACKVAL(val1);

	// Execute!
	op = Fcn->BCFcn->Operations;
	while(op)
	{
		const char	*opstr = "";
		tBC_Op	*nextop = op->Next, *jmp_target;
		ast_op = 0;
		switch(op->Operation)
		{
		case BC_OP_NOP:
			STATE_HDR();
			DEBUG_F("NOP\n");
			break;
		// Jumps
		case BC_OP_JUMP:
			STATE_HDR();
			// NOTE: Evil, all jumps are off by -1, so fix that
			jmp_target = Fcn->BCFcn->Labels[ OP_INDX(op) ]->Next;
			DEBUG_F("JUMP #%i %p\n", OP_INDX(op), jmp_target);
			nextop = jmp_target;
			break;
		case BC_OP_JUMPIF:
			STATE_HDR();
			jmp_target = Fcn->BCFcn->Labels[ OP_INDX(op) ]->Next;
			DEBUG_F("JUMPIF #%i %p\n", OP_INDX(op), jmp_target);
			GET_STACKVAL(val1);
			if( Bytecode_int_IsStackEntTrue(&val1) )
				nextop = jmp_target;
			break;
		case BC_OP_JUMPIFNOT:
			STATE_HDR();
			jmp_target = Fcn->BCFcn->Labels[ OP_INDX(op) ]->Next;
			DEBUG_F("JUMPIFNOT #%i %p\n", OP_INDX(op), jmp_target);
			GET_STACKVAL(val1);
			if( !Bytecode_int_IsStackEntTrue(&val1) )
				nextop = jmp_target;
			break;
		
		// Define variables
		case BC_OP_DEFINEVAR: {
			 int	type, slot;
			type = OP_INDX(op) & 0xFFFF;
			slot = OP_INDX(op) >> 16;
			if(slot < 0 || slot >= local_var_count) {
				DEBUG_F("ERROR: slot %i out of range (max %i)\n", slot, local_var_count);
				return -1;
			}
			STATE_HDR();
			DEBUG_F("DEFVAR %i of type %i\n", slot, type);
			if( local_vars[slot].Type != ET_NULL ) {
				Bytecode_int_DerefStackValue( &local_vars[slot] );
				local_vars[slot].Type = ET_NULL;
			}
			memset(&local_vars[slot], 0, sizeof(local_vars[0]));
			local_vars[slot].Type = type;
			} break;

		// Enter/Leave context
		// - NOP now		
		case BC_OP_ENTERCONTEXT:
			STATE_HDR();
			DEBUG_F("ENTERCONTEXT\n");
			break;
		case BC_OP_LEAVECONTEXT:
			STATE_HDR();
			DEBUG_F("LEAVECONTEXT\n");
			break;

		// Variables
		case BC_OP_LOADVAR: {
			 int	slot = OP_INDX(op);
			STATE_HDR();
			DEBUG_F("LOADVAR %i ", slot);
			if( slot < 0 || slot >= local_var_count ) {
				AST_RuntimeError(NULL, "Loading from invalid slot %i", slot);
				return -1;
			}
			DEBUG_F("("); PRINT_STACKVAL(local_vars[slot]); DEBUG_F(")\n");
			PUT_STACKVAL(local_vars[slot]);
			Bytecode_int_RefStackValue( &local_vars[slot] );
			} break;
		case BC_OP_SAVEVAR: {
			 int	slot = OP_INDX(op);
			STATE_HDR();
			DEBUG_F("SAVEVAR %i = ", slot);
			if( slot < 0 || slot >= local_var_count ) {
				AST_RuntimeError(NULL, "Loading from invalid slot %i", slot);
				return -1;
			}
			DEBUG_F("[Deref "); PRINT_STACKVAL(local_vars[slot]); DEBUG_F("] ");
			Bytecode_int_DerefStackValue( &local_vars[slot] );
			GET_STACKVAL(local_vars[slot]);
			PRINT_STACKVAL(local_vars[slot]);
			DEBUG_F("\n");
			} break;

		// Constants:
		case BC_OP_LOADINT:
			STATE_HDR();
			DEBUG_F("LOADINT 0x%lx\n", op->Content.Integer);
			val1.Type = SS_DATATYPE_INTEGER;
			val1.Integer = op->Content.Integer;
			PUT_STACKVAL(val1);
			break;
		case BC_OP_LOADREAL:
			STATE_HDR();
			DEBUG_F("LOADREAL %lf\n", op->Content.Real);
			val1.Type = SS_DATATYPE_REAL;
			val1.Real = op->Content.Real;
			PUT_STACKVAL(val1);
			break;
		case BC_OP_LOADSTR:
			STATE_HDR();
			DEBUG_F("LOADSTR %i \"%s\"\n", OP_INDX(op), OP_STRING(op));
			val1.Type = SS_DATATYPE_STRING;
			val1.Reference = SpiderScript_CreateString(OP_INDX(op), OP_STRING(op));
			PUT_STACKVAL(val1);
			break;

		case BC_OP_CAST:
			STATE_HDR();
			val2.Type = OP_INDX(op);
			DEBUG_F("CAST to %i\n", val2.Type);
			GET_STACKVAL(val1);
			if(val1.Type == val2.Type) {
				PUT_STACKVAL(val1);
				break;
			}
			switch(val2.Type * 100 + val1.Type )
			{
			case SS_DATATYPE_INTEGER*100 + SS_DATATYPE_REAL:
				val2.Integer = val1.Real;
				PUT_STACKVAL(val2);
				break;
			case SS_DATATYPE_REAL*100 + SS_DATATYPE_INTEGER:
				val2.Real = val1.Integer;
				PUT_STACKVAL(val2);
				break;
			default: {
				pval1 = Bytecode_int_GetSpiderValue(&val1, &tmpVal1);
				pval2 = SpiderScript_CastValueTo(val2.Type, pval1);
				if(pval1 != &tmpVal1)	SpiderScript_DereferenceValue(pval1);
				Bytecode_int_SetSpiderValue(&val2, pval2);
				SpiderScript_DereferenceValue(pval2);
				PUT_STACKVAL(val2);
				} break;
			}
			break;

		case BC_OP_DUPSTACK:
			STATE_HDR();
			DEBUG_F("DUPSTACK ");
			GET_STACKVAL(val1);
			PRINT_STACKVAL(val1);
			DEBUG_F("\n");
			PUT_STACKVAL(val1);
			PUT_STACKVAL(val1);
			Bytecode_int_RefStackValue(&val1);
			break;

		// Discard the top item from the stack
		case BC_OP_DELSTACK:
			STATE_HDR();
			DEBUG_F("DELSTACK\n");
			GET_STACKVAL(val1);
			break;

		// Unary Operations
		case BC_OP_LOGICNOT:
			STATE_HDR();
			DEBUG_F("LOGICNOT\n");
			
			GET_STACKVAL(val1);
			val2.Type = SS_DATATYPE_INTEGER;
			val2.Integer = !Bytecode_int_IsStackEntTrue(&val1);
			Bytecode_int_StackPush(Stack, &val2);
			Bytecode_int_DerefStackValue(&val1);
			break;
		
		case BC_OP_BITNOT:
			if(!ast_op)	ast_op = NODETYPE_BWNOT,	opstr = "BITNOT";
		case BC_OP_NEG:
			if(!ast_op)	ast_op = NODETYPE_NEGATE,	opstr = "NEG";

			STATE_HDR();
			DEBUG_F("%s\n", opstr);

			GET_STACKVAL(val1);
			pval1 = Bytecode_int_GetSpiderValue(&val1, &tmpVal1);
			Bytecode_int_DerefStackValue(&val1);			

			ret_val = AST_ExecuteNode_UniOp(Script, NULL, ast_op, pval1);
			if(pval1 != &tmpVal1)	SpiderScript_DereferenceValue(pval1);
			Bytecode_int_SetSpiderValue(&val1, ret_val);
			if(ret_val != &tmpVal1)	SpiderScript_DereferenceValue(ret_val);
			Bytecode_int_StackPush(Stack, &val1);
			
			break;

		// Binary Operations
		case BC_OP_LOGICAND:
			if(!ast_op)	ast_op = NODETYPE_LOGICALAND,	opstr = "LOGICAND";
		case BC_OP_LOGICOR:
			if(!ast_op)	ast_op = NODETYPE_LOGICALOR,	opstr = "LOGICOR";
		case BC_OP_LOGICXOR:
			if(!ast_op)	ast_op = NODETYPE_LOGICALXOR,	opstr = "LOGICXOR";
	
			STATE_HDR();
			DEBUG_F("%s\n", opstr);

			GET_STACKVAL(val1);
			GET_STACKVAL(val2);
			
			switch(op->Operation)
			{
			case BC_OP_LOGICAND:
				i = Bytecode_int_IsStackEntTrue(&val1) && Bytecode_int_IsStackEntTrue(&val2);
				break;
			case BC_OP_LOGICOR:
				i = Bytecode_int_IsStackEntTrue(&val1) || Bytecode_int_IsStackEntTrue(&val2);
				break;
			case BC_OP_LOGICXOR:
				i = Bytecode_int_IsStackEntTrue(&val1) ^ Bytecode_int_IsStackEntTrue(&val2);
				break;
			}
			Bytecode_int_DerefStackValue(&val1);
			Bytecode_int_DerefStackValue(&val2);

			val1.Type = SS_DATATYPE_INTEGER;
			val1.Integer = i;
			Bytecode_int_StackPush(Stack, &val1);
			break;

		case BC_OP_BITAND:
			if(!ast_op)	ast_op = NODETYPE_BWAND,	opstr = "BITAND";
		case BC_OP_BITOR:
			if(!ast_op)	ast_op = NODETYPE_BWOR, 	opstr = "BITOR";
		case BC_OP_BITXOR:
			if(!ast_op)	ast_op = NODETYPE_BWXOR,	opstr = "BITXOR";

		case BC_OP_BITSHIFTLEFT:
			if(!ast_op)	ast_op = NODETYPE_BITSHIFTLEFT,	opstr = "BITSHIFTLEFT";
		case BC_OP_BITSHIFTRIGHT:
			if(!ast_op)	ast_op = NODETYPE_BITSHIFTRIGHT, opstr = "BITSHIFTRIGHT";
		case BC_OP_BITROTATELEFT:
			if(!ast_op)	ast_op = NODETYPE_BITROTATELEFT, opstr = "BITROTATELEFT";

		case BC_OP_ADD:
			if(!ast_op)	ast_op = NODETYPE_ADD,	opstr = "ADD";
		case BC_OP_SUBTRACT:
			if(!ast_op)	ast_op = NODETYPE_SUBTRACT,	opstr = "SUBTRACT";
		case BC_OP_MULTIPLY:
			if(!ast_op)	ast_op = NODETYPE_MULTIPLY,	opstr = "MULTIPLY";
		case BC_OP_DIVIDE:
			if(!ast_op)	ast_op = NODETYPE_DIVIDE,	opstr = "DIVIDE";
		case BC_OP_MODULO:
			if(!ast_op)	ast_op = NODETYPE_MODULO,	opstr = "MODULO";

		case BC_OP_EQUALS:
			if(!ast_op)	ast_op = NODETYPE_EQUALS,	opstr = "EQUALS";
		case BC_OP_NOTEQUALS:
			if(!ast_op)	ast_op = NODETYPE_NOTEQUALS,	opstr = "NOTEQUALS";
		case BC_OP_LESSTHAN:
			if(!ast_op)	ast_op = NODETYPE_LESSTHAN,	opstr = "LESSTHAN";
		case BC_OP_LESSTHANOREQUAL:
			if(!ast_op)	ast_op = NODETYPE_LESSTHANEQUAL, opstr = "LESSTHANOREQUAL";
		case BC_OP_GREATERTHAN:
			if(!ast_op)	ast_op = NODETYPE_GREATERTHAN,	opstr = "GREATERTHAN";
		case BC_OP_GREATERTHANOREQUAL:
			if(!ast_op)	ast_op = NODETYPE_GREATERTHANEQUAL, opstr = "GREATERTHANOREQUAL";

			STATE_HDR();
			DEBUG_F("BINOP %i %s (bc %i)\n", ast_op, opstr, op->Operation);

			GET_STACKVAL(val2);	// Right
			GET_STACKVAL(val1);	// Left

			DEBUG_F(" ("); PRINT_STACKVAL(val1); DEBUG_F(")");
			DEBUG_F(" ("); PRINT_STACKVAL(val2); DEBUG_F(")\n");

			#define PERFORM_NUM_OP(_type, _field) if(val1.Type == _type && val1.Type == val2.Type) { \
				switch(op->Operation) { \
				case BC_OP_ADD:	val1._field = val1._field + val2._field;	break; \
				case BC_OP_SUBTRACT:	val1._field = val1._field - val2._field;	break; \
				case BC_OP_MULTIPLY:	val1._field = val1._field * val2._field;	break; \
				case BC_OP_DIVIDE:	val1._field = val1._field / val2._field;	break; \
				\
				case BC_OP_EQUALS:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = (val1._field == val2._field);	break; \
				case BC_OP_NOTEQUALS:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = (val1._field != val2._field);	break; \
				case BC_OP_LESSTHAN:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = val1._field < val2._field;	break; \
				case BC_OP_LESSTHANOREQUAL:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = val1._field <= val2._field;	break; \
				case BC_OP_GREATERTHAN:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = val1._field > val2._field;	break; \
				case BC_OP_GREATERTHANOREQUAL:	val1.Type=SS_DATATYPE_INTEGER; val1.Integer = val1._field >= val2._field;	break; \
				\
				case BC_OP_BITAND:	val1._field = (int64_t)val1._field & (int64_t)val2._field;	break; \
				case BC_OP_BITOR:	val1._field = (int64_t)val1._field | (int64_t)val2._field;	break; \
				case BC_OP_BITXOR:	val1._field = (int64_t)val1._field ^ (int64_t)val2._field;	break; \
				case BC_OP_MODULO:	val1._field = (int64_t)val1._field % (int64_t)val2._field;	break; \
				default:	AST_RuntimeError(NULL, "Invalid operation on datatype %i", _type); nextop = NULL; break;\
				}\
				DEBUG_F(" - Fast local op\n");\
				PUT_STACKVAL(val1);\
				break;\
			}

			PERFORM_NUM_OP(SS_DATATYPE_INTEGER, Integer);
			PERFORM_NUM_OP(SS_DATATYPE_REAL, Real);
		
			pval1 = Bytecode_int_GetSpiderValue(&val1, &tmpVal1);
			pval2 = Bytecode_int_GetSpiderValue(&val2, &tmpVal2);
			Bytecode_int_DerefStackValue(&val1);
			Bytecode_int_DerefStackValue(&val2);

			ret_val = AST_ExecuteNode_BinOp(Script, NULL, ast_op, pval1, pval2);
			if(pval1 != &tmpVal1)	SpiderScript_DereferenceValue(pval1);
			if(pval2 != &tmpVal2)	SpiderScript_DereferenceValue(pval2);

			if(ret_val == ERRPTR) {
				AST_RuntimeError(NULL, "_BinOp returned ERRPTR");
				nextop = NULL;
				break;
			}
			Bytecode_int_SetSpiderValue(&val1, ret_val);
			if(ret_val != &tmpVal1)	SpiderScript_DereferenceValue(ret_val);
			PUT_STACKVAL(val1);
			break;

		// Functions etc
		case BC_OP_CREATEOBJ:
		case BC_OP_CALLFUNCTION:
		case BC_OP_CALLMETHOD: {
			tScript_Function	*fcn = NULL;
			const char	*name = OP_STRING(op);
			 int	arg_count = OP_INDX(op);
			
			STATE_HDR();
			DEBUG_F("CALL FUNCTION %s %i args\n", name, arg_count);

			if( op->Operation == BC_OP_CALLFUNCTION )
			{
				// Check current script functions (for fast call)
				for(fcn = Script->Functions; fcn; fcn = fcn->Next)
				{
					if(strcmp(name, fcn->Name) == 0) {
						break;
					}
				}
				if(fcn && fcn->BCFcn)
				{
					DEBUG_F(" - Fast call\n");
					Bytecode_int_ExecuteFunction(Script, fcn, Stack, arg_count);
					break;
				}
			}
			
			// Slower call
			{
				tSpiderNamespace	*ns = NULL;
				tSpiderValue	*args[arg_count];
				tSpiderValue	*rv;
				// Read arguments
				for( i = arg_count; i --; )
				{
					GET_STACKVAL(val1);
					args[i] = Bytecode_int_GetSpiderValue(&val1, NULL);
					Bytecode_int_DerefStackValue(&val1);
				}
				
				// Resolve namespace into pointer
				if( op->Operation != BC_OP_CALLMETHOD ) {
					if( name[0] == BC_NS_SEPARATOR ) {
						name ++;
						ns = Bytecode_int_ResolveNamespace(&Script->Variant->RootNamespace, name, &name);
					}
					else {
						// TODO: Support multiple default namespaces
						ns = Bytecode_int_ResolveNamespace(default_namespace, name, &name);
					}
				}
				
				// Call the function etc.
				if( op->Operation == BC_OP_CALLFUNCTION )
				{
					rv = SpiderScript_ExecuteFunction(Script, ns, name, arg_count, args);
				}
				else if( op->Operation == BC_OP_CREATEOBJ )
				{
					rv = SpiderScript_CreateObject(Script, ns, name, arg_count, args);
				}
				else if( op->Operation == BC_OP_CALLMETHOD )
				{
					tSpiderObject	*obj;
					GET_STACKVAL(val1);
					
					if(val1.Type == SS_DATATYPE_OBJECT)
						obj = val1.Object;
					else if(val1.Type == ET_REFERENCE && val1.Reference && val1.Reference->Type == SS_DATATYPE_OBJECT)
						obj = val1.Reference->Object;
					else {
						// Error
						AST_RuntimeError(NULL, "OP_CALLMETHOD on non object");
						nextop = NULL;
						break;
					}
					rv = SpiderScript_ExecuteMethod(Script, obj, name, arg_count, args);
					Bytecode_int_DerefStackValue(&val1);
				}
				else
				{
					AST_RuntimeError(NULL, "BUG - Unknown operation for CALL/CREATEOBJ (%i)", op->Operation);
					rv = ERRPTR;
				}
				if(rv == ERRPTR) {
					AST_RuntimeError(NULL, "SpiderScript_ExecuteFunction returned ERRPTR");
					nextop = NULL;
					break;
				}
				// Clean up args
				for( i = arg_count; i --; )
					SpiderScript_DereferenceValue(args[i]);
				// Get and push return
				Bytecode_int_SetSpiderValue(&val1, rv);
				PUT_STACKVAL(val1);
				// Deref return
				SpiderScript_DereferenceValue(rv);
			}
			} break;

		case BC_OP_RETURN:
			STATE_HDR();
			DEBUG_F("RETURN\n");
			nextop = NULL;
			break;

		default:
			// TODO:
			STATE_HDR();
			AST_RuntimeError(NULL, "Unknown operation %i\n", op->Operation);
			nextop = NULL;
			break;
		}
		op = nextop;
	}
	
	// Clean up
	// - Delete local vars
	for( i = 0; i < local_var_count; i ++ )
	{
		if( local_vars[i].Type != ET_NULL )
		{
			Bytecode_int_DerefStackValue(&local_vars[i]);
		}
	}
	
	// - Restore stack
//	printf("TODO: Roll back stack\n");
	if( Stack->Entries[Stack->EntryCount - 1].Type == ET_FUNCTION_START )
		Stack->EntryCount --;
	else
	{
		GET_STACKVAL(val1);
		while( Stack->EntryCount && Stack->Entries[ --Stack->EntryCount ].Type != ET_FUNCTION_START )
		{
			Bytecode_int_DerefStackValue( &Stack->Entries[Stack->EntryCount] );
		}
		PUT_STACKVAL(val1);
	}
	

	return 0;
}


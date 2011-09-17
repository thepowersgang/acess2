/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * bytecode_gen.c
 * - Generate bytecode
 */
#include <stdlib.h>
#include <stdint.h>
#include "bytecode_ops.h"
#include <stdio.h>
#include "bytecode_gen.h"
#include <string.h>

// === IMPORTS ===

// === STRUCTURES ===
typedef struct sBC_Op	tBC_Op;

struct sBC_Op
{
	tBC_Op	*Next;
	 int	Operation;
	 int	bUseInteger;
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
	const char	*Name;
	
	 int	LabelCount;
	 int	LabelSpace;
	tBC_Op	**Labels;
	
	 int	MaxVariableCount;
	// NOTE: These fields are invalid after compilation
	 int	VariableCount;
	 int	VariableSpace;
	const char	**VariableNames;	// Only type needs to be stored
	 int	CurContextDepth;	// Used to get the real var count

	 int	OperationCount;
	tBC_Op	*Operations;
	tBC_Op	*OperationsEnd;

	 int	ArgumentCount;
	struct {
		char	*Name;
		 int	Type;
	}	Arguments[];
};

// === PROTOTYPES ===
tBC_Op	*Bytecode_int_AllocateOp(int Operation);

// === GLOBALS ===

// === CODE ===
tBC_Op *Bytecode_int_AllocateOp(int Operation)
{
	tBC_Op	*ret;

	ret = malloc(sizeof(tBC_Op));
	if(!ret)	return NULL;

	ret->Next = NULL;
	ret->Operation = Operation;
	ret->bUseInteger = 0;

	return ret;
}

tBC_Function *Bytecode_CreateFunction(const char *Name, int ArgCount, char **ArgNames, int *ArgTypes)
{
	tBC_Function *ret;
	 int	i;

	ret = malloc(sizeof(tBC_Function) + ArgCount*sizeof(ret->Arguments[0]));
	if(!ret)	return NULL;
	
	ret->Name = Name;
	ret->LabelSpace = ret->LabelCount = 0;
	ret->Labels = NULL;
	
	ret->VariableCount = ret->VariableSpace = 0;
	ret->VariableNames = NULL;

	ret->OperationCount = 0;
	ret->Operations = NULL;
	ret->OperationsEnd = (void*)&ret->Operations;

	ret->ArgumentCount = ArgCount;
	for( i = 0; i < ArgCount; i ++ )
	{
		ret->Arguments[i].Name = strdup(ArgNames[i]);
		ret->Arguments[i].Type = ArgTypes[i];
	}

	return ret;
}

void Bytecode_DeleteFunction(tBC_Function *Fcn)
{
	tBC_Op	*op;
	 int	i;
	for( i = 0; i < Fcn->ArgumentCount; i ++ )
	{
		free(Fcn->Arguments[i].Name);
	}
	for( op = Fcn->Operations; op; )
	{
		tBC_Op	*nextop = op->Next;
		free(op);
		op = nextop;
	}
	free(Fcn->VariableNames);
	free(Fcn->Labels);
	free(Fcn);
}

int StringList_GetString(tStringList *List, const char *String, int Length)
{
	 int	strIdx = 0;
	tString	*ent;
	for(ent = List->Head; ent; ent = ent->Next, strIdx ++)
	{
		if(ent->Length == Length && memcmp(ent->Data, String, Length) == 0)	break;
	}
	if( ent ) {
		ent->RefCount ++;
	}
	else {
		ent = malloc(sizeof(tString) + Length + 1);
		if(!ent)	return -1;
		ent->Next = NULL;
		ent->Length = Length;
		ent->RefCount = 1;
		memcpy(ent->Data, String, Length);
		ent->Data[Length] = '\0';
		
		if(List->Head)
			List->Tail->Next = ent;
		else
			List->Head = ent;
		List->Tail = ent;
		List->Count ++;
	}
	return strIdx;
}

int Bytecode_int_Serialize(const tBC_Function *Function, void *Output, int *LabelOffsets, tStringList *Strings)
{
	tBC_Op	*op;
	 int	len = 0, idx = 0;
	 int	i;

	void _put_byte(uint8_t byte)
	{
		uint8_t	*buf = Output;
		if(Output)	buf[len] = byte;
		len ++;
	}

	void _put_dword(uint32_t value)
	{
		uint8_t	*buf = Output;
		if(Output) {
			buf[len+0] = value & 0xFF;
			buf[len+1] = value >> 8;
			buf[len+2] = value >> 16;
			buf[len+3] = value >> 24;
		}
		len += 4;
	}
	
	void _put_qword(uint64_t value)
	{
		_put_dword(value & 0xFFFFFFFF);
		_put_dword(value >> 32);
	}

	void _put_double(double value)
	{
		// TODO: Machine agnostic
		if(Output) {
			*(double*)( (char*)Output + len ) = value;
		}
		len += sizeof(double);
	}

	void _put_string(const char *str, int len)
	{
		 int	strIdx = 0;
		if( Output ) {
			strIdx = StringList_GetString(Strings, str, len);
		}
	
		// TODO: Relocations	
		_put_dword(strIdx);
	}

	for( op = Function->Operations; op; op = op->Next, idx ++ )
	{
		// If first run, convert labels into byte offsets
		if( !Output )
		{
			for( i = 0; i < Function->LabelCount; i ++ )
			{
				if(LabelOffsets[i])	continue;
				if(op != Function->Labels[i])	continue;
				
				LabelOffsets[i] = len;
			}
		}

		_put_byte(op->Operation);
		switch(op->Operation)
		{
		// Relocate jumps (the value only matters if `Output` is non-NULL)
		case BC_OP_JUMP:
		case BC_OP_JUMPIF:
		case BC_OP_JUMPIFNOT:
			// TODO: Relocations?
			_put_dword( LabelOffsets[op->Content.StringInt.Integer] );
			break;
		// Special case for inline values
		case BC_OP_LOADINT:
			_put_qword(op->Content.Integer);
			break;
		case BC_OP_LOADREAL:
			_put_double(op->Content.Real);
			break;
		case BC_OP_LOADSTR:
			_put_string(op->Content.StringInt.String, op->Content.StringInt.Integer);
			break;
		// Everthing else just gets handled nicely
		default:
			if( op->Content.StringInt.String )
				_put_string(op->Content.StringInt.String, strlen(op->Content.StringInt.String));
			if( op->bUseInteger )
				_put_dword(op->Content.StringInt.Integer);
			break;
		}
	}

	return len;
}

char *Bytecode_SerialiseFunction(const tBC_Function *Function, int *Length, tStringList *Strings)
{
	 int	len;
	 int	*label_offsets;
	char	*code;

	label_offsets = calloc( sizeof(int), Function->LabelCount );
	if(!label_offsets)	return NULL;

	len = Bytecode_int_Serialize(Function, NULL, label_offsets, Strings);

	code = malloc(len);
	
	Bytecode_int_Serialize(Function, code, label_offsets, Strings);

	free(label_offsets);

	*Length = len;

	return code;
}

int Bytecode_AllocateLabel(tBC_Function *Handle)
{
	 int	ret;
	
	if( Handle->LabelCount == Handle->LabelSpace ) {
		void *tmp;
		Handle->LabelSpace += 20;	// TODO: Don't hardcode increment
		tmp = realloc(Handle->Labels, Handle->LabelSpace * sizeof(Handle->Labels[0]));
		if( !tmp ) {
			Handle->LabelSpace -= 20;
			return -1;
		}
		Handle->Labels = tmp;
	}
	ret = Handle->LabelCount ++;
	Handle->Labels[ret] = 0;
	return ret;
}

void Bytecode_SetLabel(tBC_Function *Handle, int Label)
{
	if(Label < 0)	return ;
	
	if(Label >= Handle->LabelCount)	return ;

	Handle->Labels[Label] = Handle->OperationsEnd;
	return ;
}

void Bytecode_int_AppendOp(tBC_Function *Fcn, tBC_Op *Op)
{
	Op->Next = NULL;
	if( Fcn->Operations )
		Fcn->OperationsEnd->Next = Op;
	else
		Fcn->Operations = Op;
	Fcn->OperationsEnd = Op;
}

void Bytecode_int_AddVariable(tBC_Function *Handle, const char *Name)
{
	if(Handle->VariableCount == Handle->VariableSpace) {
		void	*tmp;
		Handle->VariableSpace += 10;
		tmp = realloc(Handle->VariableNames, Handle->VariableSpace * sizeof(Handle->VariableNames[0]));
		if(!tmp)	return ;	// TODO: Error
		Handle->VariableNames = tmp;
	}
	Handle->VariableNames[Handle->VariableCount] = Name;
	Handle->VariableCount ++;
	// Get max count (used when executing to get the frame size)
	if(Handle->VariableCount - Handle->CurContextDepth >= Handle->MaxVariableCount)
		Handle->MaxVariableCount = Handle->VariableCount - Handle->CurContextDepth;
}

int Bytecode_int_GetVarIndex(tBC_Function *Handle, const char *Name)
{
	 int	i;
	// Get the start of this context
	for( i = Handle->VariableCount; i --; )
	{
		if( Handle->VariableNames[i] == NULL )	break;
	}
	// Check for duplicate allocation
	for( ; i < Handle->VariableCount; i ++ )
	{
		if( strcmp(Name, Handle->VariableNames[i]) == 0 )
			return i;
	}
	return -1;
}

#define DEF_BC_NONE(_op) { \
	tBC_Op *op = Bytecode_int_AllocateOp(_op); \
	op->Content.Integer = 0; \
	op->bUseInteger = 0; \
	Bytecode_int_AppendOp(Handle, op);\
}

#define DEF_BC_STRINT(_op, _str, _int) { \
	tBC_Op *op = Bytecode_int_AllocateOp(_op);\
	op->Content.StringInt.Integer = _int;\
	op->Content.StringInt.String = _str;\
	op->bUseInteger = 1;\
	Bytecode_int_AppendOp(Handle, op);\
}
#define DEF_BC_STR(_op, _str) {\
	tBC_Op *op = Bytecode_int_AllocateOp(_op);\
	op->Content.StringInt.String = _str;\
	op->bUseInteger = 0;\
	Bytecode_int_AppendOp(Handle, op);\
}

// --- Flow Control
void Bytecode_AppendJump(tBC_Function *Handle, int Label)
	DEF_BC_STRINT(BC_OP_JUMP, NULL, Label)
void Bytecode_AppendCondJump(tBC_Function *Handle, int Label)
	DEF_BC_STRINT(BC_OP_JUMPIF, NULL, Label)
void Bytecode_AppendReturn(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_RETURN);

// --- Variables
void Bytecode_AppendLoadVar(tBC_Function *Handle, const char *Name)
	DEF_BC_STRINT(BC_OP_LOADVAR, NULL, Bytecode_int_GetVarIndex(Handle, Name))
//	DEF_BC_STR(BC_OP_LOADVAR, Name)
void Bytecode_AppendSaveVar(tBC_Function *Handle, const char *Name)	// (Obj->)?var = 
	DEF_BC_STRINT(BC_OP_SAVEVAR, NULL, Bytecode_int_GetVarIndex(Handle, Name))
//	DEF_BC_STR(BC_OP_SAVEVAR, Name)

// --- Constants
void Bytecode_AppendConstInt(tBC_Function *Handle, uint64_t Value)
{
	tBC_Op *op = Bytecode_int_AllocateOp(BC_OP_LOADINT);
	op->Content.Integer = Value;
	Bytecode_int_AppendOp(Handle, op);
}
void Bytecode_AppendConstReal(tBC_Function *Handle, double Value)
{
	tBC_Op *op = Bytecode_int_AllocateOp(BC_OP_LOADREAL);
	op->Content.Real = Value;
	Bytecode_int_AppendOp(Handle, op);
}
void Bytecode_AppendConstString(tBC_Function *Handle, const void *Data, size_t Length)
	DEF_BC_STRINT(BC_OP_LOADSTR, Data, Length)

// --- Indexing / Scoping
void Bytecode_AppendSubNamespace(tBC_Function *Handle, const char *Name)
	DEF_BC_STR(BC_OP_SCOPE, Name)
void Bytecode_AppendElement(tBC_Function *Handle, const char *Name)
	DEF_BC_STR(BC_OP_ELEMENT, Name)
void Bytecode_AppendIndex(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_INDEX)

void Bytecode_AppendCreateObj(tBC_Function *Handle, const char *Name, int ArgumentCount)
	DEF_BC_STRINT(BC_OP_CREATEOBJ, Name, ArgumentCount)
void Bytecode_AppendMethodCall(tBC_Function *Handle, const char *Name, int ArgumentCount)
	DEF_BC_STRINT(BC_OP_CALLMETHOD, Name, ArgumentCount)
void Bytecode_AppendFunctionCall(tBC_Function *Handle, const char *Name, int ArgumentCount)
	DEF_BC_STRINT(BC_OP_CALLFUNCTION, Name, ArgumentCount)

void Bytecode_AppendBinOp(tBC_Function *Handle, int Operation)
	DEF_BC_NONE(Operation)
void Bytecode_AppendUniOp(tBC_Function *Handle, int Operation)
	DEF_BC_NONE(Operation)
void Bytecode_AppendCast(tBC_Function *Handle, int Type)
	DEF_BC_STRINT(BC_OP_CAST, NULL, Type)

// Does some bookeeping to allocate variable slots at compile time
void Bytecode_AppendEnterContext(tBC_Function *Handle)
{
	Handle->CurContextDepth ++;
	Bytecode_int_AddVariable(Handle, NULL);	// NULL to record the extent of this	

	DEF_BC_NONE(BC_OP_ENTERCONTEXT)
}
void Bytecode_AppendLeaveContext(tBC_Function *Handle)
{
	 int	i;
	for( i = Handle->VariableCount; i --; )
	{
		if( Handle->VariableNames[i] == NULL )	break;
	}
	Handle->CurContextDepth --;

	DEF_BC_NONE(BC_OP_LEAVECONTEXT)
}
//void Bytecode_AppendImportNamespace(tBC_Function *Handle, const char *Name);
//	DEF_BC_STRINT(BC_OP_IMPORTNS, Name, 0)
void Bytecode_AppendDefineVar(tBC_Function *Handle, const char *Name, int Type)
{
	#if 1
	// Check for duplicates
	if( Bytecode_int_GetVarIndex(Handle, Name) )
		return ;	// TODO: Error
	#endif

	Bytecode_int_AddVariable(Handle, Name);
	
	DEF_BC_STRINT(BC_OP_DEFINEVAR, Name, Type)
}

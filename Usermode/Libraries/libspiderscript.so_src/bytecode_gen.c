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
#include "bytecode.h"

// === IMPORTS ===

// === STRUCTURES ===

// === PROTOTYPES ===
tBC_Op	*Bytecode_int_AllocateOp(int Operation, int ExtraBytes);
 int	Bytecode_int_AddVariable(tBC_Function *Handle, const char *Name);

// === GLOBALS ===

// === CODE ===
tBC_Op *Bytecode_int_AllocateOp(int Operation, int ExtraBytes)
{
	tBC_Op	*ret;

	ret = malloc(sizeof(tBC_Op) + ExtraBytes);
	if(!ret)	return NULL;

	ret->Next = NULL;
	ret->Operation = Operation;
	ret->bUseInteger = 0;
	ret->bUseString = (ExtraBytes > 0);

	return ret;
}

tBC_Function *Bytecode_CreateFunction(tScript_Function *Fcn)
{
	tBC_Function *ret;
	 int	i;

	ret = malloc(sizeof(tBC_Function));
	if(!ret)	return NULL;
	
	ret->LabelSpace = ret->LabelCount = 0;
	ret->Labels = NULL;

	ret->MaxVariableCount = 0;
	ret->CurContextDepth = 0;	
	ret->VariableCount = ret->VariableSpace = 0;
	ret->VariableNames = NULL;

	ret->OperationCount = 0;
	ret->Operations = NULL;
	ret->OperationsEnd = (void*)&ret->Operations;

	for( i = 0; i < Fcn->ArgumentCount; i ++ )
	{
		Bytecode_int_AddVariable(ret, Fcn->Arguments[i].Name);
	}

	return ret;
}

void Bytecode_DeleteFunction(tBC_Function *Fcn)
{
	tBC_Op	*op;
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

	void _put_index(uint32_t value)
	{
		if( !Output && !value ) {
			len += 5;
			return ;
		}
		if( value < 0x8000 ) {
			_put_byte(value >> 8);
			_put_byte(value & 0xFF);
		}
		else if( value < 0x400000 ) {
			_put_byte( (value >> 16) | 0x80 );
			_put_byte(value >> 8);
			_put_byte(value & 0xFF);
		}
		else {
			_put_byte( 0xC0 );
			_put_byte(value >> 24);
			_put_byte(value >> 16);
			_put_byte(value >> 8 );
			_put_byte(value & 0xFF);
		}
	}	

	void _put_qword(uint64_t value)
	{
		if( value < 0x80 ) {	// 7 bits into 1 byte
			_put_byte(value);
		}
		else if( !(value >> (8+6)) ) {	// 14 bits packed into 2 bytes
			_put_byte( 0x80 | ((value >> 8) & 0x3F) );
			_put_byte( value & 0xFF );
		}
		else if( !(value >> (32+5)) ) {	// 37 bits into 5 bytes
			_put_byte( 0xC0 | ((value >> 32) & 0x1F) );
			_put_dword(value & 0xFFFFFFFF);
		}
		else {
			_put_byte( 0xE0 );	// 64 (actually 68) bits into 9 bytes
			_put_dword(value & 0xFFFFFFFF);
			_put_dword(value >> 32);
		}
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
		_put_index(strIdx);
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
			_put_index( LabelOffsets[op->Content.StringInt.Integer] );
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
			if( op->bUseString )
				_put_string(op->Content.StringInt.String, strlen(op->Content.StringInt.String));
			if( op->bUseInteger )
				_put_index(op->Content.StringInt.Integer);
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

	// Update length to the correct length (may decrease due to encoding)	
	len = Bytecode_int_Serialize(Function, code, label_offsets, Strings);

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

int Bytecode_int_AddVariable(tBC_Function *Handle, const char *Name)
{
	if(Handle->VariableCount == Handle->VariableSpace) {
		void	*tmp;
		Handle->VariableSpace += 10;
		tmp = realloc(Handle->VariableNames, Handle->VariableSpace * sizeof(Handle->VariableNames[0]));
		if(!tmp)	return -1;	// TODO: Error
		Handle->VariableNames = tmp;
	}
	Handle->VariableNames[Handle->VariableCount] = Name;
	Handle->VariableCount ++;
	// Get max count (used when executing to get the frame size)
	if(Handle->VariableCount - Handle->CurContextDepth >= Handle->MaxVariableCount)
		Handle->MaxVariableCount = Handle->VariableCount - Handle->CurContextDepth;
//	printf("_AddVariable: %s given %i\n", Name, Handle->VariableCount - Handle->CurContextDepth - 1);
	return Handle->VariableCount - Handle->CurContextDepth - 1;
}

int Bytecode_int_GetVarIndex(tBC_Function *Handle, const char *Name)
{
	 int	i, context_depth = Handle->CurContextDepth;
	// Get the start of this context
	for( i = Handle->VariableCount; i --; )
	{
		if( !Handle->VariableNames[i] ) {
			context_depth --;
			continue ;
		}
		if( strcmp(Name, Handle->VariableNames[i]) == 0 )
			return i - context_depth;
	}
	return -1;
}

#define DEF_BC_NONE(_op) { \
	tBC_Op *op = Bytecode_int_AllocateOp(_op, 0); \
	op->Content.Integer = 0; \
	op->bUseInteger = 0; \
	Bytecode_int_AppendOp(Handle, op);\
}

#define DEF_BC_INT(_op, _int) {\
	tBC_Op *op = Bytecode_int_AllocateOp(_op, 0);\
	op->Content.StringInt.Integer = _int;\
	op->bUseInteger = 1;\
	op->bUseString = 0;\
	Bytecode_int_AppendOp(Handle, op);\
}

#define DEF_BC_STRINT(_op, _str, _int) { \
	tBC_Op *op = Bytecode_int_AllocateOp(_op, strlen(_str));\
	op->Content.StringInt.Integer = _int;\
	strcpy(op->Content.StringInt.String, _str);\
	op->bUseInteger = 1;\
	op->bUseString = 1;\
	Bytecode_int_AppendOp(Handle, op);\
}
#define DEF_BC_STR(_op, _str) {\
	tBC_Op *op = Bytecode_int_AllocateOp(_op, strlen(_str));\
	strcpy(op->Content.StringInt.String, _str);\
	op->bUseInteger = 0;\
	Bytecode_int_AppendOp(Handle, op);\
}

// --- Flow Control
void Bytecode_AppendJump(tBC_Function *Handle, int Label)
	DEF_BC_INT(BC_OP_JUMP, Label)
void Bytecode_AppendCondJump(tBC_Function *Handle, int Label)
	DEF_BC_INT(BC_OP_JUMPIF, Label)
void Bytecode_AppendCondJumpNot(tBC_Function *Handle, int Label)
	DEF_BC_INT(BC_OP_JUMPIFNOT, Label)
void Bytecode_AppendReturn(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_RETURN);

// --- Variables
void Bytecode_AppendLoadVar(tBC_Function *Handle, const char *Name)
	DEF_BC_INT(BC_OP_LOADVAR, Bytecode_int_GetVarIndex(Handle, Name))
//	DEF_BC_STR(BC_OP_LOADVAR, Name)
void Bytecode_AppendSaveVar(tBC_Function *Handle, const char *Name)	// (Obj->)?var = 
	DEF_BC_INT(BC_OP_SAVEVAR, Bytecode_int_GetVarIndex(Handle, Name))
//	DEF_BC_STR(BC_OP_SAVEVAR, Name)

// --- Constants
void Bytecode_AppendConstInt(tBC_Function *Handle, uint64_t Value)
{
	tBC_Op *op = Bytecode_int_AllocateOp(BC_OP_LOADINT, 0);
	op->Content.Integer = Value;
	Bytecode_int_AppendOp(Handle, op);
}
void Bytecode_AppendConstReal(tBC_Function *Handle, double Value)
{
	tBC_Op *op = Bytecode_int_AllocateOp(BC_OP_LOADREAL, 0);
	op->Content.Real = Value;
	Bytecode_int_AppendOp(Handle, op);
}
void Bytecode_AppendConstString(tBC_Function *Handle, const void *Data, size_t Length)
{
	tBC_Op *op = Bytecode_int_AllocateOp(BC_OP_LOADSTR, Length+1);
	op->Content.StringInt.Integer = Length;
	memcpy(op->Content.StringInt.String, Data, Length);
	op->Content.StringInt.String[Length] = 0;
	Bytecode_int_AppendOp(Handle, op);
}

// --- Indexing / Scoping
void Bytecode_AppendElement(tBC_Function *Handle, const char *Name)
	DEF_BC_STR(BC_OP_ELEMENT, Name)
void Bytecode_AppendSetElement(tBC_Function *Handle, const char *Name)
	DEF_BC_STR(BC_OP_SETELEMENT, Name)
void Bytecode_AppendIndex(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_INDEX)
void Bytecode_AppendSetIndex(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_SETINDEX);

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
	DEF_BC_INT(BC_OP_CAST, Type)
void Bytecode_AppendDuplicate(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_DUPSTACK);
void Bytecode_AppendDelete(tBC_Function *Handle)
	DEF_BC_NONE(BC_OP_DELSTACK);

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
	Handle->VariableCount = i;

	DEF_BC_NONE(BC_OP_LEAVECONTEXT);
}
//void Bytecode_AppendImportNamespace(tBC_Function *Handle, const char *Name);
//	DEF_BC_STRINT(BC_OP_IMPORTNS, Name, 0)
void Bytecode_AppendDefineVar(tBC_Function *Handle, const char *Name, int Type)
{
	 int	i;
	#if 1
	// Get the start of this context
	for( i = Handle->VariableCount; i --; )
	{
		if( Handle->VariableNames[i] == NULL )	break;
	}
	// Check for duplicate allocation
	for( i ++; i < Handle->VariableCount; i ++ )
	{
		if( strcmp(Name, Handle->VariableNames[i]) == 0 )
			return ;
	}
	#endif

	i = Bytecode_int_AddVariable(Handle, Name);
//	printf("Variable %s given slot %i\n", Name, i);	

	DEF_BC_STRINT(BC_OP_DEFINEVAR, Name, (Type&0xFFFF) | (i << 16))
}

/*
 * SpiderScript Library
 * - By John Hodge (thePowersGang)
 *
 * bytecode_gen.h
 * - Bytecode Generation header
 */
#ifndef _BYTECODE_GEN_H_
#define _BYTECODE_GEN_H_

#include "common.h"
#include "ast.h"
#include "bytecode.h"

typedef struct sStringList	tStringList;
typedef struct sString	tString;

struct sString
{
	tString	*Next;
	 int	Length;
	 int	RefCount;
	char	Data[];
};

struct sStringList
{
	tString	*Head;
	tString	*Tail;
	 int	Count;
};


extern int	Bytecode_ConvertScript(tSpiderScript *Script, const char *DestFile);
extern tBC_Function	*Bytecode_ConvertFunction(tScript_Function *Fcn);
extern tBC_Function	*Bytecode_NewBlankFunction(void);
extern void	Bytecode_DeleteFunction(tBC_Function *Fcn);

extern char *Bytecode_SerialiseFunction(const tBC_Function *Function, int *Length, tStringList *Strings);
extern int	StringList_GetString(tStringList *List, const char *String, int Length);
extern tBC_Function	*Bytecode_CreateFunction(tScript_Function *Fcn);

extern int	Bytecode_AllocateLabel(tBC_Function *Handle);
extern void	Bytecode_SetLabel(tBC_Function *Handle, int Label);
// Bytecode adding
// - Flow Control
extern void	Bytecode_AppendJump(tBC_Function *Handle, int Label);
extern void	Bytecode_AppendCondJump(tBC_Function *Handle, int Label);
extern void	 Bytecode_AppendCondJumpNot(tBC_Function *Handle, int Label);
extern void	Bytecode_AppendReturn(tBC_Function *Handle);
// - Operation Stack
//  > Load/Store
extern void	Bytecode_AppendLoadVar(tBC_Function *Handle, const char *Name);
extern void	Bytecode_AppendSaveVar(tBC_Function *Handle, const char *Name);	// (Obj->)?var = 
extern void	Bytecode_AppendConstInt(tBC_Function *Handle, uint64_t Value);
extern void	Bytecode_AppendConstReal(tBC_Function *Handle, double Value);
extern void	Bytecode_AppendConstString(tBC_Function *Handle, const void *Data, size_t Length);
extern void	Bytecode_AppendConstNull(tBC_Function *Handle);
//  > Scoping
extern void	Bytecode_AppendElement(tBC_Function *Handle, const char *Name);	// Obj->SubObj
extern void	Bytecode_AppendSetElement(tBC_Function *Handle, const char *Name);	// Set an object member
extern void	Bytecode_AppendIndex(tBC_Function *Handle);	// Index into an array
extern void	Bytecode_AppendSetIndex(tBC_Function *Handle);	// Write an array element
//  > Function Calls
extern void	Bytecode_AppendCreateObj(tBC_Function *Handle, const char *Name, int ArgumentCount);
extern void	Bytecode_AppendMethodCall(tBC_Function *Handle, const char *Name, int ArgumentCount);
extern void	Bytecode_AppendFunctionCall(tBC_Function *Handle, const char *Name, int ArgumentCount);
//  > Manipulation
extern void	Bytecode_AppendBinOp(tBC_Function *Handle, int Operation);
extern void	Bytecode_AppendUniOp(tBC_Function *Handle, int Operation);
extern void	Bytecode_AppendCast(tBC_Function *Handlde, int Type);
extern void	Bytecode_AppendDuplicate(tBC_Function *Handlde);
extern void	Bytecode_AppendDelete(tBC_Function *Handle);
// - Context
//   TODO: Are contexts needed? Should variables be allocated like labels?
extern void	Bytecode_AppendEnterContext(tBC_Function *Handle);
extern void	Bytecode_AppendLeaveContext(tBC_Function *Handle);
//extern void	Bytecode_AppendImportNamespace(tBC_Function *Handle, const char *Name);
extern void	Bytecode_AppendDefineVar(tBC_Function *Handle, const char *Name, int Type);

#endif


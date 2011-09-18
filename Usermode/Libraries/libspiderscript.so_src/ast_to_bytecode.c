/*
 * SpiderScript Library
 *
 * AST to Bytecode Conversion
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "ast.h"
#include "bytecode_gen.h"
#include "bytecode_ops.h"

#define TRACE_VAR_LOOKUPS	0
#define TRACE_NODE_RETURNS	0

// === IMPORTS ===
extern tSpiderFunction	*gpExports_First;

// === TYPES ===
typedef struct sAST_BlockInfo
{
	struct sAST_BlockInfo	*Parent;
	void	*Handle;
	const char	*Tag;

	 int	BreakTarget;
	 int	ContinueTarget;
} tAST_BlockInfo;

// === PROTOTYPES ===
// Node Traversal
 int	AST_ConvertNode(tAST_BlockInfo *Block, tAST_Node *Node);
// Variables
 int 	BC_Variable_Define(tAST_BlockInfo *Block, int Type, const char *Name);
 int	BC_Variable_SetValue(tAST_BlockInfo *Block, tAST_Node *VarNode);
 int	BC_Variable_GetValue(tAST_BlockInfo *Block, tAST_Node *VarNode);
// - Errors
void	AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...);
void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === GLOBALS ===
// int	giNextBlockIdent = 1;

// === CODE ===
/**
 * \brief Convert a function into bytecode
 */
tBC_Function *Bytecode_ConvertFunction(tScript_Function *Fcn)
{
	tBC_Function	*ret;
	tAST_BlockInfo bi = {0};

	// TODO: Return BCFcn instead?
	if(Fcn->BCFcn)	return NULL;
	
	ret = Bytecode_CreateFunction(Fcn);
	if(!ret)	return NULL;
	
	bi.Handle = ret;
	if( AST_ConvertNode(&bi, Fcn->ASTFcn) )
	{
		Bytecode_DeleteFunction(ret);
		return NULL;
	}

	Fcn->BCFcn = ret;

	return ret;
}

/**
 * \brief Convert a node into bytecode
 * \param Block	Execution context
 * \param Node	Node to execute
 */
int AST_ConvertNode(tAST_BlockInfo *Block, tAST_Node *Node)
{
	tAST_Node	*node;
	 int	ret = 0;
	 int	i, op = 0;
	
	switch(Node->Type)
	{
	// No Operation
	case NODETYPE_NOP:
		break;
	
	// Code block
	case NODETYPE_BLOCK:
		Bytecode_AppendEnterContext(Block->Handle);	// Create a new block
		{
			tAST_BlockInfo	blockInfo;
			blockInfo.Parent = Block;
			blockInfo.Handle = Block->Handle;
			blockInfo.ContinueTarget = 0;
			blockInfo.BreakTarget = 0;
			// Loop over all nodes, or until the return value is set
			for(node = Node->Block.FirstChild;
				node;
				node = node->NextSibling )
			{
				AST_ConvertNode(Block, node);
			}
		}
		Bytecode_AppendLeaveContext(Block->Handle);	// Leave this context
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		// TODO: Support assigning to object attributes
		if( Node->Assign.Dest->Type != NODETYPE_VARIABLE ) {
			AST_RuntimeError(Node, "LVALUE of assignment is not a variable");
			return -1;
		}
		ret = AST_ConvertNode(Block, Node->Assign.Value);
		if(ret)	return ret;
		
		// Perform assignment operation
		if( Node->Assign.Operation != NODETYPE_NOP )
		{
			
			ret = BC_Variable_GetValue(Block, Node->Assign.Dest);
			if(ret)	return ret;
			Bytecode_AppendBinOp(Block->Handle, Node->Assign.Operation);
		}
		
		// Set the variable value
		ret = BC_Variable_SetValue( Block, Node->Assign.Dest );
		break;
	
	// Post increment/decrement
	case NODETYPE_POSTINC:
	case NODETYPE_POSTDEC:
		Bytecode_AppendConstInt(Block->Handle, 1);
		
		// TODO: Support assigning to object attributes
		if( Node->UniOp.Value->Type != NODETYPE_VARIABLE ) {
			AST_RuntimeError(Node, "LVALUE of assignment is not a variable");
			return -1;
		}

		ret = BC_Variable_GetValue(Block, Node->UniOp.Value);
		if(ret)	return ret;

		if( Node->Type == NODETYPE_POSTDEC )
			Bytecode_AppendBinOp(Block->Handle, NODETYPE_SUBTRACT);
		else
			Bytecode_AppendBinOp(Block->Handle, NODETYPE_ADD);
		if(ret)	return ret;

		ret = BC_Variable_SetValue(Block, Node->UniOp.Value);
		if(ret)	return ret;
		break;
	
	// Function Call
	case NODETYPE_METHODCALL:
	case NODETYPE_FUNCTIONCALL:
	case NODETYPE_CREATEOBJECT:
		i = 0;
		for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling)
		{
			ret = AST_ConvertNode(Block, node);
			if(ret)	return ret;
			i ++;
		}
		
		// Call the function
		if( Node->Type == NODETYPE_CREATEOBJECT )
		{
			// TODO: Sanity check stack top
			Bytecode_AppendCreateObj(Block->Handle, Node->FunctionCall.Name, i);
		}
		else if( Node->Type == NODETYPE_METHODCALL )
		{
			// TODO: Sanity check stack top
			ret = AST_ConvertNode(Block, Node->FunctionCall.Object);
			if(ret)	return ret;
			Bytecode_AppendMethodCall(Block->Handle, Node->FunctionCall.Name, i);
		}
		else
		{
			Bytecode_AppendFunctionCall(Block->Handle, Node->FunctionCall.Name, i);
		}
		break;
	
	// Conditional
	case NODETYPE_IF: {
		 int	if_true, if_end;
		ret = AST_ConvertNode(Block, Node->If.Condition);
		if(ret)	return ret;
		
		if_true = Bytecode_AllocateLabel(Block->Handle);
		if_end = Bytecode_AllocateLabel(Block->Handle);
	
		Bytecode_AppendCondJump(Block->Handle, if_true);

		// False
		ret = AST_ConvertNode(Block, Node->If.False);
		if(ret)	return ret;
		Bytecode_AppendJump(Block->Handle, if_end);
		
		// True
		Bytecode_SetLabel(Block->Handle, if_true);
		ret = AST_ConvertNode(Block, Node->If.True);
		if(ret)	return ret;

		// End
		Bytecode_SetLabel(Block->Handle, if_end);
		} break;
	
	// Loop
	case NODETYPE_LOOP: {
		 int	loop_start, loop_end;
		 int	saved_break, saved_continue;
		const char	*saved_tag;

		// Initialise
		ret = AST_ConvertNode(Block, Node->For.Init);
		if(ret)	return ret;
		
		loop_start = Bytecode_AllocateLabel(Block->Handle);
		loop_end = Bytecode_AllocateLabel(Block->Handle);

		saved_break = Block->BreakTarget;
		saved_continue = Block->ContinueTarget;
		saved_tag = Block->Tag;
		Block->BreakTarget = loop_end;
		Block->ContinueTarget = loop_end;
		Block->Tag = Node->For.Tag;

		Bytecode_SetLabel(Block->Handle, loop_start);

		// Check initial condition
		if( !Node->For.bCheckAfter )
		{
			ret = AST_ConvertNode(Block, Node->For.Condition);
			if(ret)	return ret;
			Bytecode_AppendUniOp(Block->Handle, NODETYPE_LOGICALNOT);
			Bytecode_AppendCondJump(Block->Handle, loop_end);
		}
	
		// Code
		ret = AST_ConvertNode(Block, Node->For.Code);
		if(ret)	return ret;
		
		// Increment
		ret = AST_ConvertNode(Block, Node->For.Increment);
		if(ret)	return ret;

		// Tail check
		if( Node->For.bCheckAfter )
		{
			ret = AST_ConvertNode(Block, Node->For.Condition);
			if(ret)	return ret;
			Bytecode_AppendCondJump(Block->Handle, loop_start);
		}

		Bytecode_SetLabel(Block->Handle, loop_end);

		Block->BreakTarget = saved_break;
		Block->ContinueTarget = saved_continue;
		Block->Tag = saved_tag;
		} break;
	
	// Return
	case NODETYPE_RETURN:
		ret = AST_ConvertNode(Block, Node->UniOp.Value);
		if(ret)	return ret;
		Bytecode_AppendReturn(Block->Handle);
		break;
	
	case NODETYPE_BREAK:
	case NODETYPE_CONTINUE: {
		tAST_BlockInfo	*bi = Block;
		if( Node->Variable.Name[0] ) {
			while(bi && strcmp(bi->Tag, Node->Variable.Name) == 0)	bi = bi->Parent;
		}
		if( !bi )	return 1;
		// TODO: Check if BreakTarget/ContinueTarget are valid
		if( Node->Type == NODETYPE_BREAK )
			Bytecode_AppendJump(Block->Handle, bi->BreakTarget);
		else
			Bytecode_AppendJump(Block->Handle, bi->ContinueTarget);
		} break;
	
	// Define a variable
	case NODETYPE_DEFVAR:
		ret = BC_Variable_Define(Block, Node->DefVar.DataType, Node->DefVar.Name);
		if(ret)	return ret;
		
		if( Node->DefVar.InitialValue )
		{
			ret = AST_ConvertNode(Block, Node->DefVar.InitialValue);
			if(ret)	return ret;
			Bytecode_AppendSaveVar(Block->Handle, Node->DefVar.Name);
		}
		break;
	
	// Scope
	case NODETYPE_SCOPE:
		Bytecode_AppendSubNamespace(Block->Handle, Node->Scope.Name);
		ret = AST_ConvertNode(Block, Node->Scope.Element);
		break;
	
	// Variable
	case NODETYPE_VARIABLE:
		ret = BC_Variable_GetValue( Block, Node );
		break;
	
	// Element of an Object
	case NODETYPE_ELEMENT:
		ret = AST_ConvertNode( Block, Node->Scope.Element );
		if(ret)	return ret;

		Bytecode_AppendElement(Block->Handle, Node->Scope.Name);
		break;

	// Cast a value to another
	case NODETYPE_CAST:
		ret = AST_ConvertNode(Block, Node->Cast.Value);
		if(ret)	return ret;
		Bytecode_AppendCast(Block->Handle, Node->Cast.DataType);
		break;

	// Index into an array
	case NODETYPE_INDEX:
		ret = AST_ConvertNode(Block, Node->BinOp.Left);	// Array
		if(ret)	return ret;
		ret = AST_ConvertNode(Block, Node->BinOp.Right);	// Offset
		if(ret)	return ret;
		
		Bytecode_AppendIndex(Block->Handle);
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		// TODO: Scan namespace for constant name
		AST_RuntimeError(Node, "TODO - Runtime Constants");
		ret = -1;
		break;
	
	// Constant Values
	case NODETYPE_STRING:
		Bytecode_AppendConstString(Block->Handle, Node->Constant.String.Data, Node->Constant.String.Length);
		break;
	case NODETYPE_INTEGER:
		Bytecode_AppendConstInt(Block->Handle, Node->Constant.Integer);
		break;
	case NODETYPE_REAL:
		Bytecode_AppendConstInt(Block->Handle, Node->Constant.Real);
		break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALNOT:	// Logical NOT (!)
		if(!op)	op = BC_OP_LOGICNOT;
	case NODETYPE_BWNOT:	// Bitwise NOT (~)
		if(!op)	op = BC_OP_BITNOT;
	case NODETYPE_NEGATE:	// Negation (-)
		if(!op)	op = BC_OP_NEG;
		ret = AST_ConvertNode(Block, Node->UniOp.Value);
		if(ret)	return ret;
		Bytecode_AppendUniOp(Block->Handle, op);
		break;

	case NODETYPE_LOGICALAND:	// Logical AND (&&)
		if(!op)	op = BC_OP_LOGICAND;
	case NODETYPE_LOGICALOR:	// Logical OR (||)
		if(!op)	op = BC_OP_LOGICOR;
	case NODETYPE_LOGICALXOR:	// Logical XOR (^^)
		if(!op)	op = BC_OP_LOGICXOR;
	// Comparisons
	case NODETYPE_EQUALS:	if(!op)	op = BC_OP_EQUALS;
	case NODETYPE_LESSTHAN:	if(!op)	op = BC_OP_LESSTHAN;
	case NODETYPE_GREATERTHAN:	if(!op)	op = BC_OP_GREATERTHAN;
	case NODETYPE_LESSTHANEQUAL:	if(!op)	op = BC_OP_LESSTHANOREQUAL;
	case NODETYPE_GREATERTHANEQUAL:	if(!op)	op = BC_OP_GREATERTHANOREQUAL;
	// General Binary Operations
	case NODETYPE_ADD:	if(!op)	op = BC_OP_ADD;
	case NODETYPE_SUBTRACT:	if(!op)	op = BC_OP_SUBTRACT;
	case NODETYPE_MULTIPLY:	if(!op)	op = BC_OP_MULTIPLY;
	case NODETYPE_DIVIDE:	if(!op)	op = BC_OP_DIVIDE;
	case NODETYPE_MODULO:	if(!op)	op = BC_OP_MODULO;
	case NODETYPE_BWAND:	if(!op)	op = BC_OP_BITAND;
	case NODETYPE_BWOR:	if(!op)	op = BC_OP_BITOR;
	case NODETYPE_BWXOR:	if(!op)	op = BC_OP_BITXOR;
	case NODETYPE_BITSHIFTLEFT:	if(!op)	op = BC_OP_BITSHIFTLEFT;
	case NODETYPE_BITSHIFTRIGHT:	if(!op)	op = BC_OP_BITSHIFTRIGHT;
	case NODETYPE_BITROTATELEFT:	if(!op)	op = BC_OP_BITROTATELEFT;
		ret = AST_ConvertNode(Block, Node->BinOp.Left);
		if(ret)	return ret;
		ret = AST_ConvertNode(Block, Node->BinOp.Right);
		if(ret)	return ret;
		
		Bytecode_AppendBinOp(Block->Handle, op);
		break;
	
	//default:
	//	ret = NULL;
	//	AST_RuntimeError(Node, "BUG - SpiderScript AST_ConvertNode Unimplemented %i", Node->Type);
	//	break;
	}

	#if TRACE_NODE_RETURNS
	if(ret && ret != ERRPTR) {
		AST_RuntimeError(Node, "Ret type of %p %i is %i", Node, Node->Type, ret->Type);
	}
	else {
		AST_RuntimeError(Node, "Ret type of %p %i is %p", Node, Node->Type, ret);
	}
	#endif

	return ret;
}

/**
 * \brief Define a variable
 * \param Block	Current block state
 * \param Type	Type of the variable
 * \param Name	Name of the variable
 * \return Boolean Failure
 */
int BC_Variable_Define(tAST_BlockInfo *Block, int Type, const char *Name)
{
	#if 0
	tAST_Variable	*var, *prev = NULL;
	
	for( var = Block->FirstVar; var; prev = var, var = var->Next )
	{
		if( strcmp(var->Name, Name) == 0 ) {
			AST_RuntimeError(NULL, "Redefinition of variable '%s'", Name);
			return ERRPTR;
		}
	}
	
	var = malloc( sizeof(tAST_Variable) + strlen(Name) + 1 );
	var->Next = NULL;
	var->Type = Type;
	strcpy(var->Name, Name);
	
	if(prev)	prev->Next = var;
	else	Block->FirstVar = var;
	
	return var;
	#else
	Bytecode_AppendDefineVar(Block->Handle, Name, Type);
	return 0;
	#endif
}

tAST_Variable *BC_Variable_Lookup(tAST_BlockInfo *Block, tAST_Node *VarNode, int CreateType)
{
	#if 0
	tAST_Variable	*var = NULL;
	
	// Speed hack
	if( VarNode->BlockState == Block && VarNode->BlockIdent == Block->Ident ) {
		var = VarNode->ValueCache;
		#if TRACE_VAR_LOOKUPS
		AST_RuntimeMessage(VarNode, "debug", "Fast var fetch on '%s' %p (%p:%i)",
			VarNode->Variable.Name, var,
			VarNode->BlockState, VarNode->BlockIdent
			);
		#endif
	}
	else
	{
		tAST_BlockInfo	*bs;
		for( bs = Block; bs; bs = bs->Parent )
		{
			for( var = bs->FirstVar; var; var = var->Next )
			{
				if( strcmp(var->Name, VarNode->Variable.Name) == 0 )
					break;
			}
			if(var)	break;
		}
		
		if( !var )
		{
			if( Block->Script->Variant->bDyamicTyped && CreateType != SS_DATATYPE_UNDEF ) {
				// Define variable
				var = BC_Variable_Define(Block, CreateType, VarNode->Variable.Name, NULL);
			}
			else
			{
				AST_RuntimeError(VarNode, "Variable '%s' is undefined", VarNode->Variable.Name);
				return NULL;
			}
		}
		
		#if TRACE_VAR_LOOKUPS
		AST_RuntimeMessage(VarNode, "debug", "Saved variable lookup of '%s' %p (%p:%i)",
			VarNode->Variable.Name, var,
			Block, Block->Ident);
		#endif
		
		VarNode->ValueCache = var;
		VarNode->BlockState = Block;
		VarNode->BlockIdent = Block->Ident;
	}
	
	return var;
	#else
	return (void*)1;
	#endif
}

/**
 * \brief Set the value of a variable
 * \return Boolean Failure
 */
int BC_Variable_SetValue(tAST_BlockInfo *Block, tAST_Node *VarNode)
{
	tAST_Variable	*var;
	
	var = BC_Variable_Lookup(Block, VarNode, SS_DATATYPE_UNDEF);
	if(!var)	return -1;

	// TODO: Check types

	Bytecode_AppendSaveVar(Block->Handle, VarNode->Variable.Name);
	return 0;
}

/**
 * \brief Get the value of a variable
 */
int BC_Variable_GetValue(tAST_BlockInfo *Block, tAST_Node *VarNode)
{
	tAST_Variable	*var;

	var = BC_Variable_Lookup(Block, VarNode, 0);	
	if(!var)	return -1;
	
	Bytecode_AppendLoadVar(Block->Handle, VarNode->Variable.Name);
	return 0;
}

#if 0
void AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...)
{
	va_list	args;
	
	if(Node) {
		fprintf(stderr, "%s:%i: ", Node->File, Node->Line);
	}
	fprintf(stderr, "%s: ", Type);
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
void AST_RuntimeError(tAST_Node *Node, const char *Format, ...)
{
	va_list	args;
	
	if(Node) {
		fprintf(stderr, "%s:%i: ", Node->File, Node->Line);
	}
	fprintf(stderr, "error: ");
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
	fprintf(stderr, "\n");
}
#endif

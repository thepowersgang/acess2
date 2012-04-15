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
#define MAX_NAMESPACE_DEPTH	10

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

	 int	NamespaceDepth;
	const char	*CurNamespaceStack[MAX_NAMESPACE_DEPTH];
} tAST_BlockInfo;

// === PROTOTYPES ===
// Node Traversal
 int	AST_ConvertNode(tAST_BlockInfo *Block, tAST_Node *Node, int bKeepValue);
 int	BC_SaveValue(tAST_BlockInfo *Block, tAST_Node *DestNode);
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
int SpiderScript_BytecodeScript(tSpiderScript *Script)
{
	tScript_Function	*fcn;
	for(fcn = Script->Functions; fcn; fcn = fcn->Next)
	{
		if( Bytecode_ConvertFunction(fcn) == 0 )
			return -1;
	}
	return 0;
}

/**
 * \brief Convert a function into bytecode
 */
tBC_Function *Bytecode_ConvertFunction(tScript_Function *Fcn)
{
	tBC_Function	*ret;
	tAST_BlockInfo bi = {0};

	// TODO: Return BCFcn instead?
	if(Fcn->BCFcn)	return Fcn->BCFcn;
	
	ret = Bytecode_CreateFunction(Fcn);
	if(!ret)	return NULL;
	
	bi.Handle = ret;
	if( AST_ConvertNode(&bi, Fcn->ASTFcn, 0) )
	{
		Bytecode_DeleteFunction(ret);
		return NULL;
	}

	Bytecode_AppendConstInt(ret, 0);	// TODO: NULL
	Bytecode_AppendReturn(ret);
	Fcn->BCFcn = ret;

	return ret;
}

// Indepotent operation
#define CHECK_IF_NEEDED(b_warn) do { if(!bKeepValue) {\
	if(b_warn)AST_RuntimeMessage(Node, "Bytecode", "Operation without saving");\
	Bytecode_AppendDelete(Block->Handle);\
} } while(0)

/**
 * \brief Convert a node into bytecode
 * \param Block	Execution context
 * \param Node	Node to execute
 */
int AST_ConvertNode(tAST_BlockInfo *Block, tAST_Node *Node, int bKeepValue)
{
	tAST_Node	*node;
	 int	ret = 0;
	 int	i, op = 0;
	 int	bAddedValue = 1;	// Used to tell if the value needs to be deleted
	
	switch(Node->Type)
	{
	// No Operation
	case NODETYPE_NOP:
		bAddedValue = 0;
		break;
	
	// Code block
	case NODETYPE_BLOCK:
		Bytecode_AppendEnterContext(Block->Handle);	// Create a new block
		{
			tAST_BlockInfo	blockInfo = {0};
			blockInfo.Parent = Block;
			blockInfo.Handle = Block->Handle;
			// Loop over all nodes, or until the return value is set
			for(node = Node->Block.FirstChild;
				node;
				node = node->NextSibling )
			{
				AST_ConvertNode(Block, node, 0);
			}
		}
		Bytecode_AppendLeaveContext(Block->Handle);	// Leave this context
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		// Perform assignment operation
		if( Node->Assign.Operation != NODETYPE_NOP )
		{
			ret = AST_ConvertNode(Block, Node->Assign.Dest, 1);
			if(ret)	return ret;
			ret = AST_ConvertNode(Block, Node->Assign.Value, 1);
			if(ret)	return ret;
			switch(Node->Assign.Operation)
			{
			// General Binary Operations
			case NODETYPE_ADD:	op = BC_OP_ADD;	break;
			case NODETYPE_SUBTRACT:	op = BC_OP_SUBTRACT;	break;
			case NODETYPE_MULTIPLY:	op = BC_OP_MULTIPLY;	break;
			case NODETYPE_DIVIDE:	op = BC_OP_DIVIDE;	break;
			case NODETYPE_MODULO:	op = BC_OP_MODULO;	break;
			case NODETYPE_BWAND:	op = BC_OP_BITAND;	break;
			case NODETYPE_BWOR:	op = BC_OP_BITOR;	break;
			case NODETYPE_BWXOR:	op = BC_OP_BITXOR;	break;
			case NODETYPE_BITSHIFTLEFT:	op = BC_OP_BITSHIFTLEFT;	break;
			case NODETYPE_BITSHIFTRIGHT:	op = BC_OP_BITSHIFTRIGHT;	break;
			case NODETYPE_BITROTATELEFT:	op = BC_OP_BITROTATELEFT;	break;

			default:
				AST_RuntimeError(Node, "Unknown operation in ASSIGN %i", Node->Assign.Operation);
				break;
			}
//			printf("assign, op = %i\n", op);
			Bytecode_AppendBinOp(Block->Handle, op);
		}
		else
		{
			ret = AST_ConvertNode(Block, Node->Assign.Value, 1);
			if(ret)	return ret;
		}
		
		if( bKeepValue )
			Bytecode_AppendDuplicate(Block->Handle);
		
		ret = BC_SaveValue(Block, Node->Assign.Dest);
		break;
	
	// Post increment/decrement
	case NODETYPE_POSTINC:
	case NODETYPE_POSTDEC:
		// Save original value if requested
		if(bKeepValue) {
			ret = BC_Variable_GetValue(Block, Node->UniOp.Value);
			if(ret)	return ret;
		}
		
		Bytecode_AppendConstInt(Block->Handle, 1);
		
		ret = AST_ConvertNode(Block, Node->UniOp.Value, 1);
		if(ret)	return ret;

		if( Node->Type == NODETYPE_POSTDEC )
			Bytecode_AppendBinOp(Block->Handle, BC_OP_SUBTRACT);
		else
			Bytecode_AppendBinOp(Block->Handle, BC_OP_ADD);
		if(ret)	return ret;

		ret = BC_SaveValue(Block, Node->UniOp.Value);
		break;

	// Function Call
	case NODETYPE_METHODCALL: {
		 int	nargs = 0;
		
		ret = AST_ConvertNode(Block, Node->FunctionCall.Object, 1);
		if(ret)	return ret;

		// Push arguments to the stack
		for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling)
		{
			ret = AST_ConvertNode(Block, node, 1);
			if(ret)	return ret;
			nargs ++;
		}
		
		// TODO: Sanity check stack top?
		Bytecode_AppendMethodCall(Block->Handle, Node->FunctionCall.Name, nargs);
		
		CHECK_IF_NEEDED(0);	// Don't warn
		// TODO: Implement warn_unused_ret
		
		} break;
	case NODETYPE_FUNCTIONCALL:
	case NODETYPE_CREATEOBJECT: {
		 int	nargs = 0;
		
		// Get name (mangled into a single string)
		 int	newnamelen = 0;
		char	*manglename;
		for( i = 0; i < Block->NamespaceDepth; i ++ )
			newnamelen += strlen(Block->CurNamespaceStack[i]) + 1;
		newnamelen += strlen(Node->FunctionCall.Name) + 1;
		manglename = alloca(newnamelen);
		manglename[0] = 0;
		for( i = 0; i < Block->NamespaceDepth; i ++ ) {
			 int	pos;
			strcat(manglename, Block->CurNamespaceStack[i]);
			pos = strlen(manglename);
			manglename[pos] = BC_NS_SEPARATOR;
			manglename[pos+1] = '\0';
		}
		strcat(manglename, Node->FunctionCall.Name);
		Block->NamespaceDepth = 0;

		// Push arguments to the stack
		for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling)
		{
			ret = AST_ConvertNode(Block, node, 1);
			if(ret)	return ret;
			nargs ++;
		}
		
		// Call the function
		if( Node->Type == NODETYPE_CREATEOBJECT )
		{
			Bytecode_AppendCreateObj(Block->Handle, manglename, nargs);
		}
		else
		{
			Bytecode_AppendFunctionCall(Block->Handle, manglename, nargs);
		}
		
		CHECK_IF_NEEDED(0);	// Don't warn
		// TODO: Implement warn_unused_ret
		} break;
	
	// Conditional
	case NODETYPE_IF: {
		 int	if_end;
		ret = AST_ConvertNode(Block, Node->If.Condition, 1);
		if(ret)	return ret;
		
		if_end = Bytecode_AllocateLabel(Block->Handle);

		if( Node->If.False->Type != NODETYPE_NOP )
		{
			 int	if_true = Bytecode_AllocateLabel(Block->Handle);
			
			Bytecode_AppendCondJump(Block->Handle, if_true);
	
			// False
			ret = AST_ConvertNode(Block, Node->If.False, 0);
			if(ret)	return ret;
			Bytecode_AppendJump(Block->Handle, if_end);
			Bytecode_SetLabel(Block->Handle, if_true);
		}
		else
		{
			Bytecode_AppendCondJumpNot(Block->Handle, if_end);
		}
		
		// True
		ret = AST_ConvertNode(Block, Node->If.True, 0);
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
		ret = AST_ConvertNode(Block, Node->For.Init, 0);
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
			ret = AST_ConvertNode(Block, Node->For.Condition, 1);
			if(ret)	return ret;
			Bytecode_AppendUniOp(Block->Handle, BC_OP_LOGICNOT);
			Bytecode_AppendCondJump(Block->Handle, loop_end);
		}
	
		// Code
		ret = AST_ConvertNode(Block, Node->For.Code, 0);
		if(ret)	return ret;
		
		// Increment
		ret = AST_ConvertNode(Block, Node->For.Increment, 0);
		if(ret)	return ret;

		// Tail check
		if( Node->For.bCheckAfter )
		{
			ret = AST_ConvertNode(Block, Node->For.Condition, 1);
			if(ret)	return ret;
			Bytecode_AppendCondJump(Block->Handle, loop_start);
		}
		else
		{
			Bytecode_AppendJump(Block->Handle, loop_start);
		}

		Bytecode_SetLabel(Block->Handle, loop_end);

		Block->BreakTarget = saved_break;
		Block->ContinueTarget = saved_continue;
		Block->Tag = saved_tag;
		} break;
	
	// Return
	case NODETYPE_RETURN:
		ret = AST_ConvertNode(Block, Node->UniOp.Value, 1);
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
			ret = AST_ConvertNode(Block, Node->DefVar.InitialValue, 1);
			if(ret)	return ret;
			Bytecode_AppendSaveVar(Block->Handle, Node->DefVar.Name);
		}
		break;
	
	// Scope
	case NODETYPE_SCOPE:
		if( Block->NamespaceDepth == MAX_NAMESPACE_DEPTH ) {
			AST_RuntimeError(Node, "Exceeded max explicit namespace depth (%i)", MAX_NAMESPACE_DEPTH);
			return 2;
		}
		Block->CurNamespaceStack[ Block->NamespaceDepth ] = Node->Scope.Name;
		Block->NamespaceDepth ++;
		ret = AST_ConvertNode(Block, Node->Scope.Element, 2);
		if( Block->NamespaceDepth != 0 ) {
			AST_RuntimeError(Node, "Namespace scope used but no element at the end");
		}
		CHECK_IF_NEEDED(0);	// No warning?
		break;
	
	// Variable
	case NODETYPE_VARIABLE:
		ret = BC_Variable_GetValue( Block, Node );
		CHECK_IF_NEEDED(1);
		break;
	
	// Element of an Object
	case NODETYPE_ELEMENT:
		ret = AST_ConvertNode( Block, Node->Scope.Element, 1 );
		if(ret)	return ret;

		Bytecode_AppendElement(Block->Handle, Node->Scope.Name);
		CHECK_IF_NEEDED(1);
		break;

	// Cast a value to another
	case NODETYPE_CAST:
		ret = AST_ConvertNode(Block, Node->Cast.Value, 1);
		if(ret)	return ret;
		Bytecode_AppendCast(Block->Handle, Node->Cast.DataType);
		CHECK_IF_NEEDED(1);
		break;

	// Index into an array
	case NODETYPE_INDEX:
		ret = AST_ConvertNode(Block, Node->BinOp.Left, 1);	// Array
		if(ret)	return ret;
		ret = AST_ConvertNode(Block, Node->BinOp.Right, 1);	// Offset
		if(ret)	return ret;
		
		Bytecode_AppendIndex(Block->Handle);
		CHECK_IF_NEEDED(1);
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		// TODO: Scan namespace for constant name
		AST_RuntimeError(Node, "TODO - Runtime Constants");
		Block->NamespaceDepth = 0;
		ret = -1;
		break;
	
	// Constant Values
	case NODETYPE_STRING:
		Bytecode_AppendConstString(Block->Handle, Node->Constant.String.Data, Node->Constant.String.Length);
		CHECK_IF_NEEDED(1);
		break;
	case NODETYPE_INTEGER:
		Bytecode_AppendConstInt(Block->Handle, Node->Constant.Integer);
		CHECK_IF_NEEDED(1);
		break;
	case NODETYPE_REAL:
		Bytecode_AppendConstReal(Block->Handle, Node->Constant.Real);
		CHECK_IF_NEEDED(1);
		break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALNOT:	// Logical NOT (!)
		if(!op)	op = BC_OP_LOGICNOT;
	case NODETYPE_BWNOT:	// Bitwise NOT (~)
		if(!op)	op = BC_OP_BITNOT;
	case NODETYPE_NEGATE:	// Negation (-)
		if(!op)	op = BC_OP_NEG;
		ret = AST_ConvertNode(Block, Node->UniOp.Value, 1);
		if(ret)	return ret;
		Bytecode_AppendUniOp(Block->Handle, op);
		CHECK_IF_NEEDED(1);
		break;

	// Logic
	case NODETYPE_LOGICALAND:	if(!op)	op = BC_OP_LOGICAND;
	case NODETYPE_LOGICALOR:	if(!op)	op = BC_OP_LOGICOR;
	case NODETYPE_LOGICALXOR:	if(!op)	op = BC_OP_LOGICXOR;
	// Comparisons
	case NODETYPE_EQUALS:   	if(!op)	op = BC_OP_EQUALS;
	case NODETYPE_NOTEQUALS:	if(!op)	op = BC_OP_NOTEQUALS;
	case NODETYPE_LESSTHAN: 	if(!op)	op = BC_OP_LESSTHAN;
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
		ret = AST_ConvertNode(Block, Node->BinOp.Left, 1);
		if(ret)	return ret;
		ret = AST_ConvertNode(Block, Node->BinOp.Right, 1);
		if(ret)	return ret;
	
		Bytecode_AppendBinOp(Block->Handle, op);
		CHECK_IF_NEEDED(1);
		break;
	
	default:
		ret = -1;
		AST_RuntimeError(Node, "BUG - SpiderScript AST_ConvertNode Unimplemented %i", Node->Type);
		break;
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

int BC_SaveValue(tAST_BlockInfo *Block, tAST_Node *DestNode)
{
	 int	ret;
	switch(DestNode->Type)
	{
	// Variable, simple
	case NODETYPE_VARIABLE:
		ret = BC_Variable_SetValue( Block, DestNode );
		break;
	// Array index
	case NODETYPE_INDEX:
		ret = AST_ConvertNode(Block, DestNode->BinOp.Left, 1);	// Array
		if(ret)	return ret;
		ret = AST_ConvertNode(Block, DestNode->BinOp.Right, 1);	// Offset
		if(ret)	return ret;
		Bytecode_AppendSetIndex( Block->Handle );
		break;
	// Object element
	case NODETYPE_ELEMENT:
		ret = AST_ConvertNode(Block, DestNode->Scope.Element, 1);
		if(ret)	return ret;
		Bytecode_AppendSetElement( Block->Handle, DestNode->Scope.Name );
		break;
	// Anything else
	default:
		// TODO: Support assigning to object attributes
		AST_RuntimeError(DestNode, "Assignment target is not a LValue");
		return -1;
	}
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

/*
 * SpiderScript Library
 *
 * AST Execution
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "ast.h"

#define USE_AST_EXEC	1
#define TRACE_VAR_LOOKUPS	0
#define TRACE_NODE_RETURNS	0

// === IMPORTS ===

// === PROTOTYPES ===
// - Node Execution
tSpiderValue	*AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node);
tSpiderValue	*AST_ExecuteNode_BinOp(tSpiderScript *Script, tAST_Node *Node, int Operation, tSpiderValue *Left, tSpiderValue *Right);
tSpiderValue	*AST_ExecuteNode_UniOp(tSpiderScript *Script, tAST_Node *Node, int Operation, tSpiderValue *Value);
tSpiderValue	*AST_ExecuteNode_Index(tSpiderScript *Script, tAST_Node *Node, tSpiderValue *Array, int Index, tSpiderValue *SaveValue);
// - Variables
tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name, tSpiderValue *Value);
 int	Variable_SetValue(tAST_BlockState *Block, tAST_Node *VarNode, tSpiderValue *Value);
tSpiderValue	*Variable_GetValue(tAST_BlockState *Block, tAST_Node *VarNode);
void	Variable_Destroy(tAST_Variable *Variable);
// - Errors
void	AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...);
void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === GLOBALS ===
 int	giNextBlockIdent = 1;

// === CODE ===
#if USE_AST_EXEC
tSpiderValue *AST_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Arguments)
{
	tAST_BlockState	bs;
	tSpiderValue	*ret;
	 int	i = 0;
	
	// Build a block State
	bs.FirstVar = NULL;
	bs.RetVal = NULL;
	bs.Parent = NULL;
	bs.BaseNamespace = &Script->Variant->RootNamespace;
	bs.CurNamespace = NULL;
	bs.Script = Script;
	bs.Ident = giNextBlockIdent ++;
	
	// Parse arguments
	for( i = 0; i < Fcn->ArgumentCount; i ++ )
	{
		if( i >= NArguments )	break;	// TODO: Return gracefully
		// TODO: Type checks
		Variable_Define(&bs,
			Fcn->Arguments[i].Type, Fcn->Arguments[i].Name,
			Arguments[i]);
	}
			
	// Execute function
	ret = AST_ExecuteNode(&bs, Fcn->ASTFcn);
	if(ret != ERRPTR)
	{
		SpiderScript_DereferenceValue(ret);	// Dereference output of last block statement
		ret = bs.RetVal;	// Set to return value of block
	}
			
	while(bs.FirstVar)
	{
		tAST_Variable	*nextVar = bs.FirstVar->Next;
		Variable_Destroy( bs.FirstVar );
		bs.FirstVar = nextVar;
	}
	return ret;
}

/**
 * \brief Execute an AST node and return its value
 * \param Block	Execution context
 * \param Node	Node to execute
 */
tSpiderValue *AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node)
{
	tAST_Node	*node;
	tSpiderValue	*ret = NULL, *tmpobj;
	tSpiderValue	*op1, *op2;	// Binary operations
	 int	i;
	
	switch(Node->Type)
	{
	// No Operation
	case NODETYPE_NOP:
		ret = NULL;
		break;
	
	// Code block
	case NODETYPE_BLOCK:
		{
			tAST_BlockState	blockInfo;
			blockInfo.Parent = Block;
			blockInfo.Script = Block->Script;
			blockInfo.FirstVar = NULL;
			blockInfo.RetVal = NULL;
			blockInfo.BaseNamespace = Block->BaseNamespace;
			blockInfo.CurNamespace = NULL;
			blockInfo.BreakTarget = NULL;
			blockInfo.Ident = giNextBlockIdent ++;
			ret = NULL;
			// Loop over all nodes, or until the return value is set
			for(node = Node->Block.FirstChild;
				node && !blockInfo.RetVal && !blockInfo.BreakTarget;
				node = node->NextSibling )
			{
				ret = AST_ExecuteNode(&blockInfo, node);
				if(ret == ERRPTR)	break;	// Error check
				if(ret != NULL)	SpiderScript_DereferenceValue(ret);	// Free unused value
			}
			// Clean up variables
			while(blockInfo.FirstVar)
			{
				tAST_Variable	*nextVar = blockInfo.FirstVar->Next;
				Variable_Destroy( blockInfo.FirstVar );
				blockInfo.FirstVar = nextVar;
			}
			// Clear ret if not an error
			if(ret != ERRPTR)	ret = NULL;
			
			// Set parent's return value if needed
			if( blockInfo.RetVal )
				Block->RetVal = blockInfo.RetVal;
			if( blockInfo.BreakTarget ) {
				Block->BreakTarget = blockInfo.BreakTarget;
				Block->BreakType = blockInfo.BreakType;
			}
			
			// TODO: Unset break if break type deontes a block break
		}
		
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		// TODO: Support assigning to object attributes
		if( Node->Assign.Dest->Type != NODETYPE_VARIABLE ) {
			AST_RuntimeError(Node, "LVALUE of assignment is not a variable");
			return ERRPTR;
		}
		ret = AST_ExecuteNode(Block, Node->Assign.Value);
		if(ret == ERRPTR)	return ERRPTR;
		
		// Perform assignment operation
		if( Node->Assign.Operation != NODETYPE_NOP )
		{
			tSpiderValue	*varVal, *value;

			varVal = Variable_GetValue(Block, Node->Assign.Dest);
			if(varVal == ERRPTR)	return ERRPTR;
			#if 0
			#else
			if(varVal && varVal->ReferenceCount == 2) {
				SpiderScript_DereferenceValue(varVal);
//				printf("pre: (%s) varVal->ReferenceCount = %i\n",
//					Node->Assign.Dest->Variable.Name,
//					varVal->ReferenceCount);
			}
			#endif
			value = AST_ExecuteNode_BinOp(Block->Script, Node, Node->Assign.Operation, varVal, ret);
			if(value == ERRPTR)	return ERRPTR;

			if(ret)	SpiderScript_DereferenceValue(ret);
			#if 0
			if(varVal)	SpiderScript_DereferenceValue(varVal);
			#else
			if(varVal && varVal->ReferenceCount == 1) {
				SpiderScript_ReferenceValue(varVal);
//				printf("post: varVal->ReferenceCount = %i\n", varVal->ReferenceCount);
				break;	// If varVal was non-null, it has been updated by _BinOp
			}
			#endif
			// Else, it was NULL, so has to be assigned
			ret = value;
		}
		
		// Set the variable value
		if( Variable_SetValue( Block, Node->Assign.Dest, ret ) ) {
			SpiderScript_DereferenceValue( ret );
			return ERRPTR;
		}
		break;
	
	// Post increment/decrement
	case NODETYPE_POSTINC:
	case NODETYPE_POSTDEC:
		{
			tSpiderValue	*varVal, *value;
			static tSpiderValue	one = {
				.Type = SS_DATATYPE_INTEGER,
				.ReferenceCount = 1,
				{.Integer = 1}
				};
			
			// TODO: Support assigning to object attributes
			if( Node->UniOp.Value->Type != NODETYPE_VARIABLE ) {
				AST_RuntimeError(Node, "LVALUE of assignment is not a variable");
				return ERRPTR;
			}
		
			// Get values (current variable contents and a static one)
			varVal = Variable_GetValue(Block, Node->UniOp.Value);
			
			if( Node->Type == NODETYPE_POSTDEC )
				value = AST_ExecuteNode_BinOp(Block->Script, Node, NODETYPE_SUBTRACT, varVal, &one);
			else
				value = AST_ExecuteNode_BinOp(Block->Script, Node, NODETYPE_ADD, varVal, &one);
			if( value == ERRPTR )
				return ERRPTR;
			
			ret = varVal;
		
			if( Variable_SetValue( Block, Node->UniOp.Value, value ) ) {
				SpiderScript_DereferenceValue( ret );
				return ERRPTR;
			}
			SpiderScript_DereferenceValue( value );
		}
		break;
	
	// Function Call
	case NODETYPE_METHODCALL:
	case NODETYPE_FUNCTIONCALL:
	case NODETYPE_CREATEOBJECT:
		// Logical block (used to allocate `params`)
		{
			const char	*namespaces[] = {NULL};	// TODO: Default namespaces?
			tSpiderValue	*params[Node->FunctionCall.NumArgs];
			i = 0;
			
			// Get arguments
			for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling)
			{
				params[i] = AST_ExecuteNode(Block, node);
				if( params[i] == ERRPTR ) {
					while(i--)	SpiderScript_DereferenceValue(params[i]);
					ret = ERRPTR;
					goto _return;
				}
				i ++;
			}

			// TODO: Check for cached function reference			

			// Call the function
			if( Node->Type == NODETYPE_CREATEOBJECT )
			{
				ret = SpiderScript_CreateObject(Block->Script,
					Node->FunctionCall.Name,
					namespaces,
					Node->FunctionCall.NumArgs, params
					);
			}
			else if( Node->Type == NODETYPE_METHODCALL )
			{
				tSpiderValue *obj = AST_ExecuteNode(Block, Node->FunctionCall.Object);
				if( !obj || obj == ERRPTR || obj->Type != SS_DATATYPE_OBJECT ) {
					AST_RuntimeError(Node->FunctionCall.Object,
						"Type Mismatch - Required SS_DATATYPE_OBJECT for method call");
					while(i--)	SpiderScript_DereferenceValue(params[i]);
					ret = ERRPTR;
					break;
				}
				ret = SpiderScript_ExecuteMethod(Block->Script,
					obj->Object, Node->FunctionCall.Name,
					Node->FunctionCall.NumArgs, params
					);
				SpiderScript_DereferenceValue(obj);
			}
			else
			{
				ret = SpiderScript_ExecuteFunction(Block->Script,
					Node->FunctionCall.Name,
					namespaces,
					Node->FunctionCall.NumArgs, params,
					NULL
					);
			}

			
			// Dereference parameters
			while(i--)	SpiderScript_DereferenceValue(params[i]);
			
			// falls out
		}
		break;
	
	// Conditional
	case NODETYPE_IF:
		ret = AST_ExecuteNode(Block, Node->If.Condition);
		if( ret == ERRPTR )	break;
		if( SpiderScript_IsValueTrue(ret) ) {
			tmpobj = AST_ExecuteNode(Block, Node->If.True);
		}
		else {
			tmpobj = AST_ExecuteNode(Block, Node->If.False);
		}
		SpiderScript_DereferenceValue(ret);
		if( tmpobj == ERRPTR )	return ERRPTR;
		SpiderScript_DereferenceValue(tmpobj);
		ret = NULL;
		break;
	
	// Loop
	case NODETYPE_LOOP:
		// Initialise
		ret = AST_ExecuteNode(Block, Node->For.Init);
		if(ret == ERRPTR)	break;
		
		// Check initial condition
		if( !Node->For.bCheckAfter )
		{
			SpiderScript_DereferenceValue(ret);
		
			ret = AST_ExecuteNode(Block, Node->For.Condition);
			if(ret == ERRPTR)	return ERRPTR;
			if(!SpiderScript_IsValueTrue(ret)) {
				SpiderScript_DereferenceValue(ret);
				ret = NULL;
				break;
			}
		}
	
		// Perform loop
		for( ;; )
		{
			SpiderScript_DereferenceValue(ret);
			
			// Code
			ret = AST_ExecuteNode(Block, Node->For.Code);
			if(ret == ERRPTR)	return ERRPTR;
			SpiderScript_DereferenceValue(ret);
			
			if(Block->BreakTarget)
			{
				if( Block->BreakTarget[0] == '\0' || strcmp(Block->BreakTarget, Node->For.Tag) == 0 )
				{
					// Ours
					free((void*)Block->BreakTarget);	Block->BreakTarget = NULL;
					if( Block->BreakType == NODETYPE_CONTINUE ) {
						// Continue, just keep going
					}
					else
						break;
				}
				else
					break;	// Break out of this loop
			}
			
			// Increment
			ret = AST_ExecuteNode(Block, Node->For.Increment);
			if(ret == ERRPTR)	return ERRPTR;
			SpiderScript_DereferenceValue(ret);
			
			// Check condition
			ret = AST_ExecuteNode(Block, Node->For.Condition);
			if(ret == ERRPTR)	return ERRPTR;
			if(!SpiderScript_IsValueTrue(ret))	break;
		}
		SpiderScript_DereferenceValue(ret);
		ret = NULL;
		break;
	
	// Return
	case NODETYPE_RETURN:
		ret = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(ret == ERRPTR)	break;
		Block->RetVal = ret;	// Return value set
		ret = NULL;	// the `return` statement does not return a value
		break;
	
	case NODETYPE_BREAK:
	case NODETYPE_CONTINUE:
		Block->BreakTarget = strdup(Node->Variable.Name);
		Block->BreakType = Node->Type;
		break;
	
	// Define a variable
	case NODETYPE_DEFVAR:
		if( Node->DefVar.InitialValue ) {
			tmpobj = AST_ExecuteNode(Block, Node->DefVar.InitialValue);
			if(tmpobj == ERRPTR)	return ERRPTR;
		}
		else {
			tmpobj = NULL;
		}
		// TODO: Handle arrays
		ret = NULL;
		if( Variable_Define(Block, Node->DefVar.DataType, Node->DefVar.Name, tmpobj) == ERRPTR )
			ret = ERRPTR;
		SpiderScript_DereferenceValue(tmpobj);
		break;
	
	// Scope
	case NODETYPE_SCOPE:
		{
		tSpiderNamespace	*ns;
		
		// Set current namespace if unset
		if( !Block->CurNamespace )
			Block->CurNamespace = Block->BaseNamespace;
		
		// Empty string means use the root namespace
		if( Node->Scope.Name[0] == '\0' )
		{
			ns = &Block->Script->Variant->RootNamespace;
		}
		else
		{
			// Otherwise scan the current namespace for the element
			for( ns = Block->CurNamespace->FirstChild; ns; ns = ns->Next )
			{
				if( strcmp(ns->Name, Node->Scope.Name) == 0 )
					break;
			}
		}
		if(!ns) {
			AST_RuntimeError(Node, "Unknown namespace '%s'", Node->Scope.Name);
			ret = ERRPTR;
			break;
		}
		Block->CurNamespace = ns;
	
		// TODO: Check type of child node (Scope, Constant or Function)	
	
		ret = AST_ExecuteNode(Block, Node->Scope.Element);
		}
		break;
	
	// Variable
	case NODETYPE_VARIABLE:
		ret = Variable_GetValue( Block, Node );
		break;
	
	// Element of an Object
	case NODETYPE_ELEMENT:
		tmpobj = AST_ExecuteNode( Block, Node->Scope.Element );
		if(tmpobj == ERRPTR)	return ERRPTR;
		if( !tmpobj || tmpobj->Type != SS_DATATYPE_OBJECT )
		{
			AST_RuntimeError(Node->Scope.Element, "Unable to dereference a non-object");
			ret = ERRPTR;
			break ;
		}
		
		for( i = 0; i < tmpobj->Object->Type->NAttributes; i ++ )
		{
			if( strcmp(Node->Scope.Name, tmpobj->Object->Type->AttributeDefs[i].Name) == 0 )
			{
				ret = tmpobj->Object->Attributes[i];
				SpiderScript_ReferenceValue(ret);
				break;
			}
		}
		if( i == tmpobj->Object->Type->NAttributes )
		{
			AST_RuntimeError(Node->Scope.Element, "Unknown attribute '%s' of class '%s'",
				Node->Scope.Name, tmpobj->Object->Type->Name);
			ret = ERRPTR;
		}
		break;

	// Cast a value to another
	case NODETYPE_CAST:
		{
		tmpobj = AST_ExecuteNode(Block, Node->Cast.Value);
		if(tmpobj == ERRPTR) return ERRPTR;
		ret = SpiderScript_CastValueTo( Node->Cast.DataType, tmpobj );
		SpiderScript_DereferenceValue(tmpobj);
		}
		break;

	// Index into an array
	case NODETYPE_INDEX:
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);	// Array
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);	// Offset
		if(op2 == ERRPTR) {
			SpiderScript_DereferenceValue(op1);
			return ERRPTR;
		}
		
		if( !op2 || op2->Type != SS_DATATYPE_INTEGER )
		{
			if( !Block->Script->Variant->bImplicitCasts ) {
				AST_RuntimeError(Node, "Array index is not an integer");
				ret = ERRPTR;
				break;
			}
			else {
				tmpobj = SpiderScript_CastValueTo(SS_DATATYPE_INTEGER, op2);
				SpiderScript_DereferenceValue(op2);
				op2 = tmpobj;
			}
		}
	
		if( !op1 )
		{
			SpiderScript_DereferenceValue(op2);
			AST_RuntimeError(Node, "Indexing NULL value");
			ret = ERRPTR;
			break;
		}

		ret = AST_ExecuteNode_Index(Block->Script, Node, op1, op2->Integer, NULL);

		SpiderScript_DereferenceValue(op1);
		SpiderScript_DereferenceValue(op2);
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		// TODO: Scan namespace for constant name
		AST_RuntimeError(Node, "TODO - Runtime Constants");
		ret = ERRPTR;
		break;
	
	// Constant Values
	case NODETYPE_STRING:
	case NODETYPE_INTEGER:
	case NODETYPE_REAL:
		ret = &Node->Constant;
		SpiderScript_ReferenceValue(ret);
		break;
	case NODETYPE_NULL:
		ret = NULL;
		break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALNOT:	// Logical NOT (!)
		op1 = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(op1 == ERRPTR)	return ERRPTR;
		ret = SpiderScript_CreateInteger( !SpiderScript_IsValueTrue(op1) );
		SpiderScript_DereferenceValue(op1);
		break;
	case NODETYPE_LOGICALAND:	// Logical AND (&&)
	case NODETYPE_LOGICALOR:	// Logical OR (||)
	case NODETYPE_LOGICALXOR:	// Logical XOR (^^)
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			SpiderScript_DereferenceValue(op1);
			return ERRPTR;
		}
		
		switch( Node->Type )
		{
		case NODETYPE_LOGICALAND:
			ret = SpiderScript_CreateInteger( SpiderScript_IsValueTrue(op1) && SpiderScript_IsValueTrue(op2) );
			break;
		case NODETYPE_LOGICALOR:
			ret = SpiderScript_CreateInteger( SpiderScript_IsValueTrue(op1) || SpiderScript_IsValueTrue(op2) );
			break;
		case NODETYPE_LOGICALXOR:
			ret = SpiderScript_CreateInteger( SpiderScript_IsValueTrue(op1) ^ SpiderScript_IsValueTrue(op2) );
			break;
		default:	break;
		}
		
		// Free intermediate objects
		SpiderScript_DereferenceValue(op1);
		SpiderScript_DereferenceValue(op2);
		break;
	
	// General Unary Operations
	case NODETYPE_BWNOT:	// Bitwise NOT (~)
	case NODETYPE_NEGATE:	// Negation (-)
		op1 = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(op1 == ERRPTR)	return ERRPTR;
		ret = AST_ExecuteNode_UniOp(Block->Script, Node, Node->Type, op1);
		SpiderScript_DereferenceValue(op1);
		break;
	
	// General Binary Operations
	case NODETYPE_ADD:
	case NODETYPE_SUBTRACT:
	case NODETYPE_MULTIPLY:
	case NODETYPE_DIVIDE:
	case NODETYPE_MODULO:
	case NODETYPE_BWAND:
	case NODETYPE_BWOR:
	case NODETYPE_BWXOR:
	case NODETYPE_BITSHIFTLEFT:
	case NODETYPE_BITSHIFTRIGHT:
	case NODETYPE_BITROTATELEFT:
	case NODETYPE_EQUALS:
	case NODETYPE_NOTEQUALS:
	case NODETYPE_LESSTHAN:
	case NODETYPE_GREATERTHAN:
	case NODETYPE_LESSTHANEQUAL:
	case NODETYPE_GREATERTHANEQUAL:
		// Get operands
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			SpiderScript_DereferenceValue(op1);
			return ERRPTR;
		}
		
		ret = AST_ExecuteNode_BinOp(Block->Script, Node, Node->Type, op1, op2);
		
		// Free intermediate objects
		SpiderScript_DereferenceValue(op1);
		SpiderScript_DereferenceValue(op2);
		break;
	
	//default:
	//	ret = NULL;
	//	AST_RuntimeError(Node, "BUG - SpiderScript AST_ExecuteNode Unimplemented %i", Node->Type);
	//	break;
	}
_return:
	// Reset namespace when no longer needed
	if( Node->Type != NODETYPE_SCOPE )
		Block->CurNamespace = NULL;

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
#endif

tSpiderValue *AST_ExecuteNode_UniOp(tSpiderScript *Script, tAST_Node *Node, int Operation, tSpiderValue *Value)
{
	tSpiderValue	*ret;
	#if 0
	if( Value->Type == SS_DATATYPE_OBJECT )
	{
		const char	*fcnname;
		switch(Operation)
		{
		case NODETYPE_NEGATE:	fcnname = "-ve";	break;
		case NODETYPE_BWNOT:	fcnname = "~";	break;
		default:	fcnname = NULL;	break;
		}
		
		if( fcnname )
		{
			ret = Object_ExecuteMethod(Value->Object, fcnname, );
			if( ret != ERRPTR )
				return ret;
		}
	}
	#endif
	switch(Value->Type)
	{
	// Integer Operations
	case SS_DATATYPE_INTEGER:
		if( Value->ReferenceCount == 1 )
			SpiderScript_ReferenceValue(ret = Value);
		else
			ret = SpiderScript_CreateInteger(0);
		switch(Operation)
		{
		case NODETYPE_NEGATE:	ret->Integer = -Value->Integer;	break;
		case NODETYPE_BWNOT:	ret->Integer = ~Value->Integer;	break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,UniOP,Integer unknown op %i", Operation);
			SpiderScript_DereferenceValue(ret);
			ret = ERRPTR;
			break;
		}
		break;
	// Real number Operations
	case SS_DATATYPE_REAL:
		switch(Operation)
		{
		case NODETYPE_NEGATE:	ret = SpiderScript_CreateInteger( -Value->Real );	break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,UniOP,Real unknown op %i", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	
	default:
		AST_RuntimeError(NULL, "Invalid operation (%i) on type (%i)", Operation, Value->Type);
		ret = ERRPTR;
		break;
	}
	
	return ret;
}

tSpiderValue *AST_ExecuteNode_BinOp(tSpiderScript *Script, tAST_Node *Node, int Operation, tSpiderValue *Left, tSpiderValue *Right)
{
	tSpiderValue	*preCastValue = Right;
	tSpiderValue	*ret;
	
	// Convert types
	if( Left && Right && Left->Type != Right->Type )
	{
		#if 0
		// Object types
		// - Operator overload functions
		if( Left->Type == SS_DATATYPE_OBJECT )
		{
			const char	*fcnname;
			switch(Operation)
			{
			case NODETYPE_ADD:	fcnname = "+";	break;
			case NODETYPE_SUBTRACT:	fcnname = "-";	break;
			case NODETYPE_MULTIPLY:	fcnname = "*";	break;
			case NODETYPE_DIVIDE:	fcnname = "/";	break;
			case NODETYPE_MODULO:	fcnname = "%";	break;
			case NODETYPE_BWAND:	fcnname = "&";	break;
			case NODETYPE_BWOR: 	fcnname = "|";	break;
			case NODETYPE_BWXOR:	fcnname = "^";	break;
			case NODETYPE_BITSHIFTLEFT:	fcnname = "<<";	break;
			case NODETYPE_BITSHIFTRIGHT:fcnname = ">>";	break;
			case NODETYPE_BITROTATELEFT:fcnname = "<<<";	break;
			default:	fcnname = NULL;	break;
			}
			
			if( fcnname )
			{
				ret = Object_ExecuteMethod(Left->Object, fcnname, Right);
				if( ret != ERRPTR )
					return ret;
				// Fall through and try casting (which will usually fail)
			}
		}
		#endif
		
		// If implicit casts are allowed, convert Right to Left's type
		if(Script->Variant->bImplicitCasts)
		{
			Right = SpiderScript_CastValueTo(Left->Type, Right);
			if(Right == ERRPTR)
				return ERRPTR;
		}
		// If statically typed, this should never happen, but catch it anyway
		else {
			AST_RuntimeError(Node, "Implicit cast not allowed (from %i to %i)", Right->Type, Left->Type);
			return ERRPTR;
		}
	}
	
	// NULL Check
	if( Left == NULL || Right == NULL ) {
		if(Right && Right != preCastValue)	free(Right);
		return NULL;
	}

	// Catch comparisons
	switch(Operation)
	{
	case NODETYPE_EQUALS:
	case NODETYPE_NOTEQUALS:
	case NODETYPE_LESSTHAN:
	case NODETYPE_GREATERTHAN:
	case NODETYPE_LESSTHANEQUAL:
	case NODETYPE_GREATERTHANEQUAL: {
		 int	cmp;
		ret = NULL;
		// Do operation
		switch(Left->Type)
		{
		// - String Compare (does a strcmp, well memcmp)
		case SS_DATATYPE_STRING:
			// Call memcmp to do most of the work
			cmp = memcmp(
				Left->String.Data, Right->String.Data,
				(Left->String.Length < Right->String.Length) ? Left->String.Length : Right->String.Length
				);
			// Handle reaching the end of the string
			if( cmp == 0 ) {
				if( Left->String.Length == Right->String.Length )
					cmp = 0;
				else if( Left->String.Length < Right->String.Length )
					cmp = 1;
				else
					cmp = -1;
			}
			break;
		
		// - Integer Comparisons
		case SS_DATATYPE_INTEGER:
			if( Left->Integer == Right->Integer )
				cmp = 0;
			else if( Left->Integer < Right->Integer )
				cmp = -1;
			else
				cmp = 1;
			break;
		// - Real Number Comparisons
		case SS_DATATYPE_REAL:
			cmp = (Left->Real - Right->Real) / Right->Real * 10000;	// < 0.1% difference is equality
			break;
		default:
			AST_RuntimeError(Node, "TODO - Comparison of type %i", Left->Type);
			ret = ERRPTR;
			break;
		}
		
		// Error check
		if( ret != ERRPTR )
		{
			if(Left->ReferenceCount == 1 && Left->Type != SS_DATATYPE_STRING)
				SpiderScript_ReferenceValue(ret = Left);
			else
				ret = SpiderScript_CreateInteger(0);
			
			// Create return
			switch(Operation)
			{
			case NODETYPE_EQUALS:	ret->Integer = (cmp == 0);	break;
			case NODETYPE_NOTEQUALS:	ret->Integer = (cmp != 0);	break;
			case NODETYPE_LESSTHAN:	ret->Integer = (cmp < 0);	break;
			case NODETYPE_GREATERTHAN:	ret->Integer = (cmp > 0);	break;
			case NODETYPE_LESSTHANEQUAL:	ret->Integer = (cmp <= 0);	break;
			case NODETYPE_GREATERTHANEQUAL:	ret->Integer = (cmp >= 0);	break;
			default:
				AST_RuntimeError(Node, "Exec,CmpOp unknown op %i", Operation);
				SpiderScript_DereferenceValue(ret);
				ret = ERRPTR;
				break;
			}
		}
		if(Right && Right != preCastValue)	free(Right);
		return ret;
		}

	// Fall through and sort by type instead
	default:
		break;
	}
	
	// Do operation
	switch(Left->Type)
	{
	// String Concatenation
	case SS_DATATYPE_STRING:
		switch(Operation)
		{
		case NODETYPE_ADD:	// Concatenate
			ret = SpiderScript_StringConcat(Left, Right);
			break;
		// TODO: Support python style 'i = %i' % i ?
		// Might do it via a function call
		// Implement it via % with an array, but getting past the cast will be fun
//		case NODETYPE_MODULUS:
//			break;
		// TODO: Support string repititions
//		case NODETYPE_MULTIPLY:
//			break;

		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,String unknown op %i", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	// Integer Operations
	case SS_DATATYPE_INTEGER:
		if( Left->ReferenceCount == 1 )
			SpiderScript_ReferenceValue(ret = Left);
		else
			ret = SpiderScript_CreateInteger(0);
		switch(Operation)
		{
		case NODETYPE_ADD:	ret->Integer = Left->Integer + Right->Integer;	break;
		case NODETYPE_SUBTRACT:	ret->Integer = Left->Integer - Right->Integer;	break;
		case NODETYPE_MULTIPLY:	ret->Integer = Left->Integer * Right->Integer;	break;
		case NODETYPE_DIVIDE:	ret->Integer = Left->Integer / Right->Integer;	break;
		case NODETYPE_MODULO:	ret->Integer = Left->Integer % Right->Integer;	break;
		case NODETYPE_BWAND:	ret->Integer = Left->Integer & Right->Integer;	break;
		case NODETYPE_BWOR: 	ret->Integer = Left->Integer | Right->Integer;	break;
		case NODETYPE_BWXOR:	ret->Integer = Left->Integer ^ Right->Integer;	break;
		case NODETYPE_BITSHIFTLEFT: ret->Integer = Left->Integer << Right->Integer;	break;
		case NODETYPE_BITSHIFTRIGHT:ret->Integer = Left->Integer >> Right->Integer;	break;
		case NODETYPE_BITROTATELEFT:
			ret->Integer = (Left->Integer << Right->Integer) | (Left->Integer >> (64-Right->Integer));
			break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,Integer unknown op %i", Operation);
			SpiderScript_DereferenceValue(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	// Real Numbers
	case SS_DATATYPE_REAL:
		if( Left->ReferenceCount == 1 )
			SpiderScript_ReferenceValue(ret = Left);
		else
			ret = SpiderScript_CreateReal(0);
		switch(Operation)
		{
		case NODETYPE_ADD:	ret->Real = Left->Real + Right->Real;	break;
		case NODETYPE_SUBTRACT:	ret->Real = Left->Real - Right->Real;	break;
		case NODETYPE_MULTIPLY:	ret->Real = Left->Real * Right->Real;	break;
		case NODETYPE_DIVIDE:	ret->Real = Left->Real / Right->Real;	break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,Real unknown op %i", Operation);
			SpiderScript_DereferenceValue(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	default:
		AST_RuntimeError(Node, "BUG - Invalid operation (%i) on type (%i)", Operation, Left->Type);
		ret = ERRPTR;
		break;
	}
	
	if(Right && Right != preCastValue)	free(Right);
	
	return ret;
}

tSpiderValue *AST_ExecuteNode_Index(tSpiderScript *Script, tAST_Node *Node,
	tSpiderValue *Array, int Index, tSpiderValue *SaveValue)
{
	// Quick sanity check
	if( !Array )
	{
		AST_RuntimeError(Node, "Indexing NULL, not a good idea");
		return ERRPTR;
	}

	// Array?
	if( SS_GETARRAYDEPTH(Array->Type) )
	{
		if( Index < 0 || Index >= Array->Array.Length ) {
			AST_RuntimeError(Node, "Array index out of bounds %i not in (0, %i]",
				Index, Array->Array.Length);
			return ERRPTR;
		}
		
		if( SaveValue )
		{
			if( SaveValue->Type != SS_DOWNARRAY(Array->Type) ) {
				// TODO: Implicit casting
				AST_RuntimeError(Node, "Type mismatch assiging to array element");
				return ERRPTR;
			}
			SpiderScript_DereferenceValue( Array->Array.Items[Index] );
			Array->Array.Items[Index] = SaveValue;
			SpiderScript_ReferenceValue( Array->Array.Items[Index] );
			return NULL;
		}
		else
		{
			SpiderScript_ReferenceValue( Array->Array.Items[Index] );
			return Array->Array.Items[Index];
		}
	}
	else
	{
		AST_RuntimeError(Node, "TODO - Implement indexing on non-arrays (type = %x)",
			Array->Type);
		return ERRPTR;
	}
}

#if USE_AST_EXEC
/**
 * \brief Define a variable
 * \param Block	Current block state
 * \param Type	Type of the variable
 * \param Name	Name of the variable
 * \return Boolean Failure
 */
tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name, tSpiderValue *Value)
{
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
	var->Object = Value;
	if(Value)	SpiderScript_ReferenceValue(Value);
	strcpy(var->Name, Name);
	
	if(prev)	prev->Next = var;
	else	Block->FirstVar = var;
	
	//printf("Defined variable %s (%i)\n", Name, Type);
	
	return var;
}

tAST_Variable *Variable_Lookup(tAST_BlockState *Block, tAST_Node *VarNode, int CreateType)
{	
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
		tAST_BlockState	*bs;
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
				var = Variable_Define(Block, CreateType, VarNode->Variable.Name, NULL);
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
}

/**
 * \brief Set the value of a variable
 * \return Boolean Failure
 */
int Variable_SetValue(tAST_BlockState *Block, tAST_Node *VarNode, tSpiderValue *Value)
{
	tAST_Variable	*var;
	
	var = Variable_Lookup(Block, VarNode, (Value ? Value->Type : SS_DATATYPE_UNDEF));
	
	if( !var )	return -1;
	
	if( !Block->Script->Variant->bDyamicTyped && (Value && var->Type != Value->Type) )
	{
		AST_RuntimeError(VarNode, "Type mismatch assigning to '%s'",
			VarNode->Variable.Name);
		return -2;
	}

//	printf("Assign %p to '%s'\n", Value, var->Name);
	SpiderScript_ReferenceValue(Value);
	SpiderScript_DereferenceValue(var->Object);
	var->Object = Value;
	return 0;
}

/**
 * \brief Get the value of a variable
 */
tSpiderValue *Variable_GetValue(tAST_BlockState *Block, tAST_Node *VarNode)
{
	tAST_Variable	*var = Variable_Lookup(Block, VarNode, 0);
	
	if( !var )	return ERRPTR;
	
	SpiderScript_ReferenceValue(var->Object);
	return var->Object;
}

/**
 * \brief Destorys a variable
 */
void Variable_Destroy(tAST_Variable *Variable)
{
//	printf("Variable_Destroy: (%p'%s')\n", Variable, Variable->Name);
	SpiderScript_DereferenceValue(Variable->Object);
	free(Variable);
}
#endif


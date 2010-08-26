/*
 * Acess2 Init
 * - Script AST Manipulator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

// === CODE ===
tAST_Script *AST_NewScript(void)
{
	tAST_Script	*ret = malloc( sizeof(tAST_Script) );
	
	ret->Functions = NULL;
	ret->LastFunction = NULL;
	
	return ret;
}

/**
 * \brief Append a function to a script
 */
tAST_Function *AST_AppendFunction(tAST_Script *Script, const char *Name)
{
	tAST_Function	*ret;
	
	ret = malloc( sizeof(tAST_Function) + strlen(Name) + 1 );
	ret->Next = NULL;
	strcpy(ret->Name, Name);
	ret->Code = NULL;
	ret->Arguments = NULL;
	
	if(Script->LastFunction == NULL) {
		Script->Functions = Script->LastFunction = ret;
	}
	else {
		Script->LastFunction->Next = ret;
		Script->LastFunction = ret;
	}
	
	return ret;
}

void AST_AppendFunctionArg(tAST_Function *Function, tAST_Node *Node)
{
	if( !Function->Arguments ) {
		Function->Arguments_Last = Function->Arguments = Node;
	}
	else {
		Function->Arguments_Last->NextSibling = Node;
		Function->Arguments_Last = Node;
	}
}

/**
 * \brief Set the code for a function
 */
void AST_SetFunctionCode(tAST_Function *Function, tAST_Node *Root)
{
	Function->Code = Root;
}

/**
 * \name Node Manipulation
 * \{
 */
/**
 * \brief Free a node and all subnodes
 */
void AST_FreeNode(tAST_Node *Node)
{
	tAST_Node	*node;
	
	if(!Node)	return ;
	
	switch(Node->Type)
	{
	// Block of code
	case NODETYPE_BLOCK:
		for( node = Node->Block.FirstChild; node; )
		{
			tAST_Node	*savedNext = node->NextSibling;
			AST_FreeNode(node);
			node = savedNext;
		}
		break;
	
	// Function Call
	case NODETYPE_FUNCTIONCALL:
		for( node = Node->FunctionCall.FirstArg; node; )
		{
			tAST_Node	*savedNext = node->NextSibling;
			AST_FreeNode(node);
			node = savedNext;
		}
		break;
	
	// If node
	case NODETYPE_IF:
		AST_FreeNode(Node->If.Condition);
		AST_FreeNode(Node->If.True);
		AST_FreeNode(Node->If.False);
		break;
	
	// Looping Construct (For loop node)
	case NODETYPE_LOOP:
		AST_FreeNode(Node->For.Init);
		AST_FreeNode(Node->For.Condition);
		AST_FreeNode(Node->For.Increment);
		AST_FreeNode(Node->For.Code);
		break;
	
	// Asignment
	case NODETYPE_ASSIGN:
		AST_FreeNode(Node->Assign.Dest);
		AST_FreeNode(Node->Assign.Value);
		break;
	
	// Casting
	case NODETYPE_CAST:
		AST_FreeNode(Node->Cast.Value);
		break;
	
	// Define a variable
	case NODETYPE_DEFVAR:
		for( node = Node->DefVar.LevelSizes; node; )
		{
			tAST_Node	*savedNext = node->NextSibling;
			AST_FreeNode(node);
			node = savedNext;
		}
		break;
	
	// Unary Operations
	case NODETYPE_RETURN:
		AST_FreeNode(Node->UniOp.Value);
		break;
	
	// Binary Operations
	case NODETYPE_INDEX:
	case NODETYPE_ADD:
	case NODETYPE_SUBTRACT:
	case NODETYPE_MULTIPLY:
	case NODETYPE_DIVIDE:
	case NODETYPE_MODULO:
	case NODETYPE_BITSHIFTLEFT:
	case NODETYPE_BITSHIFTRIGHT:
	case NODETYPE_BITROTATELEFT:
	case NODETYPE_BWAND:	case NODETYPE_LOGICALAND:
	case NODETYPE_BWOR: 	case NODETYPE_LOGICALOR:
	case NODETYPE_BWXOR:	case NODETYPE_LOGICALXOR:
	case NODETYPE_EQUALS:
	case NODETYPE_LESSTHAN:
	case NODETYPE_GREATERTHAN:
		AST_FreeNode( Node->BinOp.Left );
		AST_FreeNode( Node->BinOp.Right );
		break;
	
	// Node types with no children
	case NODETYPE_NOP:	break;
	case NODETYPE_VARIABLE:	break;
	case NODETYPE_CONSTANT:	break;
	case NODETYPE_STRING:	break;
	case NODETYPE_INTEGER:	break;
	case NODETYPE_REAL:	break;
	}
	free( Node );
}

tAST_Node *AST_NewCodeBlock(void)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_BLOCK;
	ret->Block.FirstChild = NULL;
	ret->Block.LastChild = NULL;
	
	return ret;
}

void AST_AppendNode(tAST_Node *Parent, tAST_Node *Child)
{
	Child->NextSibling = NULL;
	switch( Parent->Type )
	{
	case NODETYPE_BLOCK:
		if(Parent->Block.FirstChild == NULL) {
			Parent->Block.FirstChild = Parent->Block.LastChild = Child;
		}
		else {
			Parent->Block.LastChild->NextSibling = Child;
			Parent->Block.LastChild = Child;
		}
		break;
	case NODETYPE_DEFVAR:
		if(Parent->DefVar.LevelSizes == NULL) {
			Parent->DefVar.LevelSizes = Parent->DefVar.LevelSizes_Last = Child;
		}
		else {
			Parent->DefVar.LevelSizes_Last->NextSibling = Child;
			Parent->DefVar.LevelSizes_Last = Child;
		}
		break;
	default:
		fprintf(stderr, "BUG REPORT: AST_AppendNode on an invalid node type (%i)\n", Parent->Type);
		break;
	}
}

tAST_Node *AST_NewIf(tAST_Node *Condition, tAST_Node *True, tAST_Node *False)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_IF;
	ret->If.Condition = Condition;
	ret->If.True = True;
	ret->If.False = False;
	return ret;
}

tAST_Node *AST_NewLoop(tAST_Node *Init, int bPostCheck, tAST_Node *Condition, tAST_Node *Increment, tAST_Node *Code)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_LOOP;
	ret->For.Init = Init;
	ret->For.bCheckAfter = !!bPostCheck;
	ret->For.Condition = Condition;
	ret->For.Increment = Increment;
	ret->For.Code = Code;
	return ret;
}

tAST_Node *AST_NewAssign(int Operation, tAST_Node *Dest, tAST_Node *Value)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_ASSIGN;
	ret->Assign.Operation = Operation;
	ret->Assign.Dest = Dest;
	ret->Assign.Value = Value;
	
	return ret;
}

tAST_Node *AST_NewCast(int Target, tAST_Node *Value)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_CAST;
	ret->Cast.DataType = Target;
	ret->Cast.Value = Value;
	
	return ret;
}

tAST_Node *AST_NewBinOp(int Operation, tAST_Node *Left, tAST_Node *Right)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = Operation;
	ret->BinOp.Left = Left;
	ret->BinOp.Right = Right;
	
	return ret;
}

/**
 */
tAST_Node *AST_NewUniOp(int Operation, tAST_Node *Value)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = Operation;
	ret->UniOp.Value = Value;
	
	return ret;
}

/**
 * \brief Create a new string node
 */
tAST_Node *AST_NewString(const char *String, int Length)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) + Length + 1 );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_STRING;
	ret->String.Length = Length;
	memcpy(ret->String.Data, String, Length);
	ret->String.Data[Length] = '\0';
	
	return ret;
}

/**
 * \brief Create a new integer node
 */
tAST_Node *AST_NewInteger(uint64_t Value)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_INTEGER;
	ret->Integer = Value;
	return ret;
}

/**
 * \brief Create a new variable reference node
 */
tAST_Node *AST_NewVariable(const char *Name)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) + strlen(Name) + 1 );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_VARIABLE;
	strcpy(ret->Variable.Name, Name);
	return ret;
}

/**
 * \brief Create a new variable definition node
 */
tAST_Node *AST_NewDefineVar(int Type, const char *Name)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) + strlen(Name) + 1 );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_DEFVAR;
	ret->DefVar.DataType = Type;
	ret->DefVar.LevelSizes = NULL;
	strcpy(ret->DefVar.Name, Name);
	return ret;
}

/**
 * \brief Create a new runtime constant reference node
 */
tAST_Node *AST_NewConstant(const char *Name)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) + strlen(Name) + 1 );
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_CONSTANT;
	strcpy(ret->Variable.Name, Name);
	return ret;
}

/**
 * \brief Create a function call node
 * \note Argument list is manipulated using AST_AppendFunctionCallArg
 */
tAST_Node *AST_NewFunctionCall(const char *Name)
{
	tAST_Node	*ret = malloc( sizeof(tAST_Node) + strlen(Name) + 1 );
	
	ret->NextSibling = NULL;
	//ret->Line = giLineNumber;
	ret->Type = NODETYPE_FUNCTIONCALL;
	ret->FunctionCall.FirstArg = NULL;
	ret->FunctionCall.LastArg = NULL;
	strcpy(ret->FunctionCall.Name, Name);
	return ret;
}

/**
 * \brief Append an argument to a function call
 */
void AST_AppendFunctionCallArg(tAST_Node *Node, tAST_Node *Arg)
{
	if( Node->Type != NODETYPE_FUNCTIONCALL )	return ;
	
	if(Node->FunctionCall.LastArg) {
		Node->FunctionCall.LastArg->NextSibling = Arg;
		Node->FunctionCall.LastArg = Arg;
	}
	else {
		Node->FunctionCall.FirstArg = Arg;
		Node->FunctionCall.LastArg = Arg;
	}
}

/**
 * \}
 */

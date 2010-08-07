/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

#define ERRPTR	((void*)((intptr_t)0-1))

// === PROTOTYPES ===
void	Object_Dereference(tSpiderVariable *Object);
void	Object_Reference(tSpiderVariable *Object);
tSpiderVariable	*Object_CreateInteger(uint64_t Value);
tSpiderVariable	*Object_CreateReal(double Value);
tSpiderVariable	*Object_CreateString(int Length, const char *Data);
tSpiderVariable	*Object_CastTo(int Type, tSpiderVariable *Source);
 int	Object_IsTrue(tSpiderVariable *Value);

void	Variable_SetValue(tSpiderScript *Script, const char *Name, tSpiderVariable *Value);
tSpiderVariable	*Variable_GetValue(tSpiderScript *Script, const char *Name);

// === CODE ===
/**
 * \brief Dereference a created object
 */
void Object_Dereference(tSpiderVariable *Object)
{
	Object->ReferenceCount --;
	if( Object->ReferenceCount == 0 )	free(Object);
}

void Object_Reference(tSpiderVariable *Object)
{
	Object->ReferenceCount ++;
}

/**
 * \brief Create an integer object
 */
tSpiderVariable	*Object_CreateInteger(uint64_t Value)
{
	tSpiderVariable	*ret = malloc( sizeof(tSpiderVariable) );
	ret->Type = SS_DATATYPE_INTEGER;
	ret->ReferenceCount = 1;
	ret->Integer = Value;
	return ret;
}

/**
 * \brief Create an real number object
 */
tSpiderVariable	*Object_CreateReal(double Value)
{
	tSpiderVariable	*ret = malloc( sizeof(tSpiderVariable) );
	ret->Type = SS_DATATYPE_REAL;
	ret->ReferenceCount = 1;
	ret->Real = Value;
	return ret;
}

/**
 * \brief Create an string object
 */
tSpiderVariable	*Object_CreateString(int Length, const char *Data)
{
	tSpiderVariable	*ret = malloc( sizeof(tSpiderVariable) + Length + 1 );
	ret->Type = SS_DATATYPE_STRING;
	ret->ReferenceCount = 1;
	ret->String.Length = Length;
	memcpy(ret->String.Data, Data, Length);
	ret->String.Data[Length] = '\0';
	return ret;
}

/**
 */
tSpiderVariable	*Object_CastTo(int Type, tSpiderVariable *Source)
{
	tSpiderVariable	*ret;
	// Check if anything needs to be done
	if( Source->Type == Type ) {
		Object_Reference(Source);
		return Source;
	}
	
	switch(Type)
	{
	case SS_DATATYPE_UNDEF:
	case SS_DATATYPE_NULL:
	case SS_DATATYPE_ARRAY:
		fprintf(stderr, "Object_CastTo - Invalid cast to %i\n", Type);
		return ERRPTR;
	
	case SS_DATATYPE_INTEGER:
		ret = malloc(sizeof(tSpiderVariable));
		ret->Type = SS_DATATYPE_INTEGER;
		ret->ReferenceCount = 1;
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	break;	// Handled above
		case SS_DATATYPE_STRING:	ret->Integer = atoi(Source->String.Data);	break;
		case SS_DATATYPE_REAL:	ret->Integer = Source->Real;	break;
		default:
			fprintf(stderr, "Object_CastTo - Invalid cast from %i\n", Source->Type);
			break;
		}
		break;
	}
	
	return ret;
}

/**
 * \brief Condenses a value down to a boolean
 */
int Object_IsTrue(tSpiderVariable *Value)
{
	switch(Value->Type)
	{
	case SS_DATATYPE_UNDEF:
	case SS_DATATYPE_NULL:
		return 0;
	
	case SS_DATATYPE_INTEGER:
		return !!Value->Integer;
	
	case SS_DATATYPE_REAL:
		return (-.5f < Value->Real && Value->Real < 0.5f);
	
	case SS_DATATYPE_STRING:
		return Value->String.Length > 0;
	
	case SS_DATATYPE_OBJECT:
		return Value->Object != NULL;
	
	case SS_DATATYPE_ARRAY:
		return Value->Array.Length > 0;
	}
	return 0;
}

tSpiderVariable *AST_ExecuteNode(tSpiderScript *Script, tAST_Node *Node)
{
	tAST_Node	*node;
	tSpiderVariable	*ret, *tmpvar;
	tSpiderVariable	*op1, *op2;	// Binary operations
	 int	cmp;	// Used in comparisons
	
	switch(Node->Type)
	{
	// No Operation
	case NODETYPE_NOP:	ret = NULL;	break ;
	
	// Code block
	case NODETYPE_BLOCK:
		ret = NULL;
		for(node = Node->Block.FirstChild; node; node = node->NextSibling )
		{
			if(node->Type == NODETYPE_RETURN) {
				ret = AST_ExecuteNode(Script, node);
				break ;
			}
			else {
				tmpvar = AST_ExecuteNode(Script, node);
				if(tmpvar == ERRPTR)	return ERRPTR;	// Error check
				if(tmpvar)	Object_Dereference(tmpvar);	// Free unused value
			}
		}
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		if( Node->Assign.Dest->Type != NODETYPE_VARIABLE ) {
			fprintf(stderr, "Syntax error: LVALUE of assignment is not a variable\n");
			return ERRPTR;
		}
		ret = AST_ExecuteNode(Script, Node->Assign.Value);
		// TODO: Apply operation
		Variable_SetValue( Script, Node->Assign.Dest->Variable.Name, ret );
		break;
	
	case NODETYPE_FUNCTIONCALL:
		// TODO: Find a function from the export list in variant
		//SpiderScript_ExecuteMethod(Script, Node->FunctionCall.Name
		ret = ERRPTR;
		fprintf(stderr, "TODO: Implement function calls\n");
		break;
	
	// Return's special handling happens elsewhere
	case NODETYPE_RETURN:
		ret = AST_ExecuteNode(Script, Node->UniOp.Value);
		break;
	
	// Variable
	case NODETYPE_VARIABLE:	ret = Variable_GetValue( Script, Node->Variable.Name );	break;
	
	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:	ret = ERRPTR;	break;
	// Constant Values
	case NODETYPE_STRING:	ret = Object_CreateString( Node->String.Length, Node->String.Data );	break;
	case NODETYPE_INTEGER:	ret = Object_CreateInteger( Node->Integer );	break;
	case NODETYPE_REAL: 	ret = Object_CreateReal( Node->Real );	break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALAND:	// Logical AND (&&)
	case NODETYPE_LOGICALOR:	// Logical OR (||)
	case NODETYPE_LOGICALXOR:	// Logical XOR (^^)
		op1 = AST_ExecuteNode(Script, Node->BinOp.Left);
		op2 = AST_ExecuteNode(Script, Node->BinOp.Right);
		switch( Node->Type )
		{
		case NODETYPE_LOGICALAND:
			ret = Object_CreateInteger( Object_IsTrue(op1) && Object_IsTrue(op2) );
			break;
		case NODETYPE_LOGICALOR:
			ret = Object_CreateInteger( Object_IsTrue(op1) || Object_IsTrue(op2) );
			break;
		case NODETYPE_LOGICALXOR:
			ret = Object_CreateInteger( Object_IsTrue(op1) ^ Object_IsTrue(op2) );
			break;
		default:	break;
		}
		break;
	
	// Comparisons
	case NODETYPE_EQUALS:
	case NODETYPE_LESSTHAN:
	case NODETYPE_GREATERTHAN:
		op1 = AST_ExecuteNode(Script, Node->BinOp.Left);
		op2 = AST_ExecuteNode(Script, Node->BinOp.Right);
		
		// No conversion done for NULL
		// TODO: Determine if this will ever be needed
		if( op1->Type == SS_DATATYPE_NULL )
		{
			// NULLs always typecheck
			ret = Object_CreateInteger(op2->Type == SS_DATATYPE_NULL);
			break;
		}
		
		// Convert types
		if( op1->Type != op2->Type ) {
			// If dynamically typed, convert op2 to op1's type
			if(Script->Variant->bDyamicTyped)
			{
				tmpvar = op2;
				op2 = Object_CastTo(op1->Type, op2);
				Object_Dereference(tmpvar);
			}
			// If statically typed, this should never happen, but catch it anyway
			else {
				ret = ERRPTR;
				break;
			}
		}
		// Do operation
		switch(op1->Type)
		{
		// - NULL
		case SS_DATATYPE_NULL:	break;
		// - String Compare (does a strcmp, well memcmp)
		case SS_DATATYPE_STRING:
			// Call memcmp to do most of the work
			cmp = memcmp(
				op1->String.Data, op2->String.Data,
				(op1->String.Length < op2->String.Length) ? op1->String.Length : op2->String.Length
				);
			// Handle reaching the end of the string
			if( cmp == 0 ) {
				if( op1->String.Length == op2->String.Length )
					cmp = 0;
				else if( op1->String.Length < op2->String.Length )
					cmp = 1;
				else
					cmp = -1;
			}
			break;
		}
		
		// Free intermediate objects
		Object_Dereference(op1);
		Object_Dereference(op2);
		
		// Create return
		switch(Node->Type)
		{
		case NODETYPE_EQUALS:	ret = Object_CreateInteger(cmp == 0);	break;
		case NODETYPE_LESSTHAN:	ret = Object_CreateInteger(cmp < 0);	break;
		case NODETYPE_GREATERTHAN:	ret = Object_CreateInteger(cmp > 0);	break;
		default:
			fprintf(stderr, "SpiderScript internal error: Exec,CmpOp unknown op %i", Node->Type);
			ret = ERRPTR;
			break;
		}
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
		// Get operands
		op1 = AST_ExecuteNode(Script, Node->BinOp.Left);
		op2 = AST_ExecuteNode(Script, Node->BinOp.Right);
		
		// Convert types
		if( op1->Type != op2->Type ) {
			// If dynamically typed, convert op2 to op1's type
			if(Script->Variant->bDyamicTyped)
			{
				tmpvar = op2;
				op2 = Object_CastTo(op1->Type, op2);
				Object_Dereference(tmpvar);
			}
			// If statically typed, this should never happen, but catch it anyway
			else {
				ret = ERRPTR;
				break;
			}
		}
		
		// Do operation
		switch(op1->Type)
		{
		case SS_DATATYPE_NULL:	break;
		// String Concatenation
		case SS_DATATYPE_STRING:
			switch(Node->Type)
			{
			default:
				fprintf(stderr, "SpiderScript internal error: Exec,BinOP,String unknown op %i", Node->Type);
				ret = ERRPTR;
				break;
			}
			break;
		case SS_DATATYPE_INTEGER:
			switch(Node->Type)
			{
			case NODETYPE_ADD:	ret = Object_CreateInteger( op1->Integer + op2->Integer );	break;
			case NODETYPE_SUBTRACT:	ret = Object_CreateInteger( op1->Integer - op2->Integer );	break;
			case NODETYPE_MULTIPLY:	ret = Object_CreateInteger( op1->Integer * op2->Integer );	break;
			case NODETYPE_DIVIDE:	ret = Object_CreateInteger( op1->Integer / op2->Integer );	break;
			case NODETYPE_MODULO:	ret = Object_CreateInteger( op1->Integer % op2->Integer );	break;
			case NODETYPE_BWAND:	ret = Object_CreateInteger( op1->Integer & op2->Integer );	break;
			case NODETYPE_BWOR: 	ret = Object_CreateInteger( op1->Integer | op2->Integer );	break;
			case NODETYPE_BWXOR:	ret = Object_CreateInteger( op1->Integer ^ op2->Integer );	break;
			case NODETYPE_BITSHIFTLEFT:	ret = Object_CreateInteger( op1->Integer << op2->Integer );	break;
			case NODETYPE_BITSHIFTRIGHT:ret = Object_CreateInteger( op1->Integer >> op2->Integer );	break;
			case NODETYPE_BITROTATELEFT:
				ret = Object_CreateInteger( (op1->Integer << op2->Integer) | (op1->Integer >> (64-op2->Integer)) );
				break;
			default:
				fprintf(stderr, "SpiderScript internal error: Exec,BinOP,Integer unknown op %i", Node->Type);
				ret = ERRPTR;
				break;
			}
			break;
		}
		
		// Free intermediate objects
		Object_Dereference(op1);
		Object_Dereference(op2);
		break;
	
	//default:
	//	ret = NULL;
	//	fprintf(stderr, "ERROR: SpiderScript AST_ExecuteNode Unimplemented %i\n", Node->Type);
	//	break;
	}
	return ret;
}

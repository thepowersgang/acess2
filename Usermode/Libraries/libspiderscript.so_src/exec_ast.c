/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ast.h"

// === PROTOTYPES ===
void	Object_Dereference(tSpiderValue *Object);
void	Object_Reference(tSpiderValue *Object);
tSpiderValue	*SpiderScript_CreateInteger(uint64_t Value);
tSpiderValue	*SpiderScript_CreateReal(double Value);
tSpiderValue	*SpiderScript_CreateString(int Length, const char *Data);
tSpiderValue	*SpiderScript_CastValueTo(int Type, tSpiderValue *Source);
 int	SpiderScript_IsValueTrue(tSpiderValue *Value);
void	SpiderScript_FreeValue(tSpiderValue *Value);
char	*SpiderScript_DumpValue(tSpiderValue *Value);

tSpiderValue	*AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node);
tSpiderValue	*AST_ExecuteNode_BinOp(tAST_BlockState *Block, int Operation, tSpiderValue *Left, tSpiderValue *Right);

tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name);
 int	Variable_SetValue(tAST_BlockState *Block, const char *Name, tSpiderValue *Value);
tSpiderValue	*Variable_GetValue(tAST_BlockState *Block, const char *Name);
void	Variable_Destroy(tAST_Variable *Variable);

void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === CODE ===
/**
 * \brief Dereference a created object
 */
void Object_Dereference(tSpiderValue *Object)
{
	if(!Object)	return ;
	if(Object == ERRPTR)	return ;
	Object->ReferenceCount --;
//	printf("%p Dereferenced (%i)\n", Object, Object->ReferenceCount);
	if( Object->ReferenceCount == 0 ) {
		switch( (enum eSpiderScript_DataTypes) Object->Type )
		{
		case SS_DATATYPE_OBJECT:
			Object->Object->Type->Destructor( Object->Object );
			break;
		case SS_DATATYPE_OPAQUE:
			Object->Opaque.Destroy( Object->Opaque.Data );
			break;
		default:
			break;
		}
		free(Object);
	}
}

void Object_Reference(tSpiderValue *Object)
{
	if(!Object)	return ;
	Object->ReferenceCount ++;
//	printf("%p Referenced (%i)\n", Object, Object->ReferenceCount);
}

/**
 * \brief Create an integer object
 */
tSpiderValue *SpiderScript_CreateInteger(uint64_t Value)
{
	tSpiderValue	*ret = malloc( sizeof(tSpiderValue) );
	ret->Type = SS_DATATYPE_INTEGER;
	ret->ReferenceCount = 1;
	ret->Integer = Value;
	return ret;
}

/**
 * \brief Create an real number object
 */
tSpiderValue *SpiderScript_CreateReal(double Value)
{
	tSpiderValue	*ret = malloc( sizeof(tSpiderValue) );
	ret->Type = SS_DATATYPE_REAL;
	ret->ReferenceCount = 1;
	ret->Real = Value;
	return ret;
}

/**
 * \brief Create an string object
 */
tSpiderValue *SpiderScript_CreateString(int Length, const char *Data)
{
	tSpiderValue	*ret = malloc( sizeof(tSpiderValue) + Length + 1 );
	ret->Type = SS_DATATYPE_STRING;
	ret->ReferenceCount = 1;
	ret->String.Length = Length;
	memcpy(ret->String.Data, Data, Length);
	ret->String.Data[Length] = '\0';
	return ret;
}

/**
 * \brief Concatenate two strings
 */
tSpiderValue *Object_StringConcat(tSpiderValue *Str1, tSpiderValue *Str2)
{
	 int	newLen = 0;
	tSpiderValue	*ret;
	if(Str1)	newLen += Str1->String.Length;
	if(Str2)	newLen += Str2->String.Length;
	ret = malloc( sizeof(tSpiderValue) + newLen + 1 );
	ret->Type = SS_DATATYPE_STRING;
	ret->ReferenceCount = 1;
	ret->String.Length = newLen;
	if(Str1)
		memcpy(ret->String.Data, Str1->String.Data, Str1->String.Length);
	if(Str2) {
		if(Str1)
			memcpy(ret->String.Data+Str1->String.Length, Str2->String.Data, Str2->String.Length);
		else
			memcpy(ret->String.Data, Str2->String.Data, Str2->String.Length);
	}
	ret->String.Data[ newLen ] = '\0';
	return ret;
}

/**
 * \brief Cast one object to another
 * \brief Type	Destination type
 * \brief Source	Input data
 */
tSpiderValue *SpiderScript_CastValueTo(int Type, tSpiderValue *Source)
{
	tSpiderValue	*ret = ERRPTR;
	 int	len = 0;

	if( !Source )	return NULL;
	
	// Check if anything needs to be done
	if( Source->Type == Type ) {
		Object_Reference(Source);
		return Source;
	}
	
	switch( (enum eSpiderScript_DataTypes)Type )
	{
	case SS_DATATYPE_UNDEF:
	case SS_DATATYPE_ARRAY:
	case SS_DATATYPE_OPAQUE:
		AST_RuntimeError(NULL, "Invalid cast to %i", Type);
		return ERRPTR;
	
	case SS_DATATYPE_INTEGER:
		ret = malloc(sizeof(tSpiderValue));
		ret->Type = SS_DATATYPE_INTEGER;
		ret->ReferenceCount = 1;
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	break;	// Handled above
		case SS_DATATYPE_STRING:	ret->Integer = atoi(Source->String.Data);	break;
		case SS_DATATYPE_REAL:	ret->Integer = Source->Real;	break;
		default:
			AST_RuntimeError(NULL, "Invalid cast from %i to Integer", Source->Type);
			free(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	case SS_DATATYPE_STRING:
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	len = snprintf(NULL, 0, "%li", Source->Integer);	break;
		case SS_DATATYPE_REAL:	snprintf(NULL, 0, "%f", Source->Real);	break;
		default:	break;
		}
		ret = malloc(sizeof(tSpiderValue) + len + 1);
		ret->Type = SS_DATATYPE_STRING;
		ret->ReferenceCount = 1;
		ret->String.Length = len;
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	sprintf(ret->String.Data, "%li", Source->Integer);	break;
		case SS_DATATYPE_REAL:	sprintf(ret->String.Data, "%f", Source->Real);	break;
		default:
			AST_RuntimeError(NULL, "Invalid cast from %i to String", Source->Type);
			free(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	default:
		AST_RuntimeError(NULL, "BUG - BUG REPORT: Unimplemented cast target");
		break;
	}
	
	return ret;
}

/**
 * \brief Condenses a value down to a boolean
 */
int SpiderScript_IsValueTrue(tSpiderValue *Value)
{
	if( Value == ERRPTR )	return 0;
	if( Value == NULL )	return 0;
	
	switch( (enum eSpiderScript_DataTypes)Value->Type )
	{
	case SS_DATATYPE_UNDEF:
		return 0;
	
	case SS_DATATYPE_INTEGER:
		return !!Value->Integer;
	
	case SS_DATATYPE_REAL:
		return (-.5f < Value->Real && Value->Real < 0.5f);
	
	case SS_DATATYPE_STRING:
		return Value->String.Length > 0;
	
	case SS_DATATYPE_OBJECT:
		return Value->Object != NULL;
	
	case SS_DATATYPE_OPAQUE:
		return Value->Opaque.Data != NULL;
	
	case SS_DATATYPE_ARRAY:
		return Value->Array.Length > 0;
	default:
		AST_RuntimeError(NULL, "Unknown type %i in SpiderScript_IsValueTrue", Value->Type);
		return 0;
	}
	return 0;
}

/**
 * \brief Free a value
 * \note Just calls Object_Dereference
 */
void SpiderScript_FreeValue(tSpiderValue *Value)
{
	Object_Dereference(Value);
}

/**
 * \brief Dump a value into a string
 * \return Heap string
 */
char *SpiderScript_DumpValue(tSpiderValue *Value)
{
	char	*ret;
	if( Value == ERRPTR )
		return strdup("ERRPTR");
	if( Value == NULL )
		return strdup("null");
	
	switch( (enum eSpiderScript_DataTypes)Value->Type )
	{
	case SS_DATATYPE_UNDEF:	return strdup("undefined");
	
	case SS_DATATYPE_INTEGER:
		ret = malloc( sizeof(Value->Integer)*2 + 3 );
		sprintf(ret, "0x%lx", Value->Integer);
		return ret;
	
	case SS_DATATYPE_REAL:
		ret = malloc( sprintf(NULL, "%f", Value->Real) + 1 );
		sprintf(ret, "%f", Value->Real);
		return ret;
	
	case SS_DATATYPE_STRING:
		ret = malloc( Value->String.Length + 3 );
		ret[0] = '"';
		strcpy(ret+1, Value->String.Data);
		ret[Value->String.Length+1] = '"';
		ret[Value->String.Length+2] = '\0';
		return ret;
	
	case SS_DATATYPE_OBJECT:
		ret = malloc( sprintf(NULL, "{%s *%p}", Value->Object->Type->Name, Value->Object) + 1 );
		sprintf(ret, "{%s *%p}", Value->Object->Type->Name, Value->Object);
		return ret;
	
	case SS_DATATYPE_OPAQUE:
		ret = malloc( sprintf(NULL, "*%p", Value->Opaque.Data) + 1 );
		sprintf(ret, "*%p", Value->Opaque.Data);
		return ret;
	
	case SS_DATATYPE_ARRAY:
		return strdup("Array");
	
	default:
		AST_RuntimeError(NULL, "Unknown type %i in Object_Dump", Value->Type);
		return NULL;
	}
	
}

/**
 * \brief Execute an AST node and return its value
 */
tSpiderValue *AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node)
{
	tAST_Node	*node;
	tSpiderValue	*ret = NULL, *tmpobj;
	tSpiderValue	*op1, *op2;	// Binary operations
	 int	cmp;	// Used in comparisons
	
	switch(Node->Type)
	{
	// No Operation
	case NODETYPE_NOP:	ret = NULL;	break;
	
	// Code block
	case NODETYPE_BLOCK:
		{
			tAST_BlockState	blockInfo;
			blockInfo.FirstVar = NULL;
			blockInfo.RetVal = NULL;
			blockInfo.Parent = Block;
			blockInfo.Script = Block->Script;
			ret = NULL;
			for(node = Node->Block.FirstChild; node && !blockInfo.RetVal; node = node->NextSibling )
			{
				tmpobj = AST_ExecuteNode(&blockInfo, node);
				if(tmpobj == ERRPTR) {	// Error check
					ret = ERRPTR;
					break ;
				}
				if(tmpobj)	Object_Dereference(tmpobj);	// Free unused value
				tmpobj = NULL;
			}
			// Clean up variables
			while(blockInfo.FirstVar)
			{
				tAST_Variable	*nextVar = blockInfo.FirstVar->Next;
				Variable_Destroy( blockInfo.FirstVar );
				blockInfo.FirstVar = nextVar;
			}
			
			if( blockInfo.RetVal )
				Block->RetVal = blockInfo.RetVal;
		}
		
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		if( Node->Assign.Dest->Type != NODETYPE_VARIABLE ) {
			AST_RuntimeError(Node, "LVALUE of assignment is not a variable");
			return ERRPTR;
		}
		ret = AST_ExecuteNode(Block, Node->Assign.Value);
		if(ret == ERRPTR)
			return ERRPTR;
		
		if( Node->Assign.Operation != NODETYPE_NOP )
		{
			tSpiderValue	*varVal = Variable_GetValue(Block, Node->Assign.Dest->Variable.Name);
			tSpiderValue	*value;
			value = AST_ExecuteNode_BinOp(Block, Node->Assign.Operation, varVal, ret);
			if( value == ERRPTR )
				return ERRPTR;
			if(ret)	Object_Dereference(ret);
			Object_Dereference(varVal);
			ret = value;
		}
		
		if( Variable_SetValue( Block, Node->Assign.Dest->Variable.Name, ret ) ) {
			Object_Dereference( ret );
			return ERRPTR;
		}
		break;
	
	// Function Call
	case NODETYPE_FUNCTIONCALL:
		{
			 int	nParams = 0;
			for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling) {
				nParams ++;
			}
			// Logical block (used to allocate `params`)
			{
				tSpiderValue	*params[nParams];
				 int	i=0;
				for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling) {
					params[i] = AST_ExecuteNode(Block, node);
					if( params[i] == ERRPTR ) {
						while(i--)	Object_Dereference(params[i]);
						ret = ERRPTR;
						goto _return;
					}
					i ++;
				}
				
				// Call the function (SpiderScript_ExecuteMethod does the
				// required namespace handling)
				ret = SpiderScript_ExecuteMethod(Block->Script, Node->FunctionCall.Name, nParams, params);
				
				// Dereference parameters
				while(i--)	Object_Dereference(params[i]);
				
				// falls out
			}
		}
		break;
	
	// Conditional
	case NODETYPE_IF:
		ret = AST_ExecuteNode(Block, Node->If.Condition);
		if( SpiderScript_IsValueTrue(ret) ) {
			Object_Dereference(AST_ExecuteNode(Block, Node->If.True));
		}
		else {
			Object_Dereference(AST_ExecuteNode(Block, Node->If.False));
		}
		Object_Dereference(ret);
		ret = NULL;
		break;
	
	// Loop
	case NODETYPE_LOOP:
		ret = AST_ExecuteNode(Block, Node->For.Init);
		if( Node->For.bCheckAfter )
		{
			do {
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Code);
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Increment);
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Condition);
			} while( SpiderScript_IsValueTrue(ret) );
		}
		else
		{
			Object_Dereference(ret);
			ret = AST_ExecuteNode(Block, Node->For.Condition);
			while( SpiderScript_IsValueTrue(ret) ) {
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Code);
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Increment);
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Condition);
			}
		}
		Object_Dereference(ret);
		ret = NULL;
		break;
	
	// Return
	case NODETYPE_RETURN:
		ret = AST_ExecuteNode(Block, Node->UniOp.Value);
		Block->RetVal = ret;	// Return value set
		//Object_Reference(ret);	// Make sure it exists after return
		ret = NULL;	// the `return` statement does not return a value
		break;
	
	// Define a variable
	case NODETYPE_DEFVAR:
		ret = NULL;
		if( Variable_Define(Block, Node->DefVar.DataType, Node->DefVar.Name) == ERRPTR )
			ret = ERRPTR;
		break;
	
	// Variable
	case NODETYPE_VARIABLE:
		ret = Variable_GetValue( Block, Node->Variable.Name );
		break;

	// Cast a value to another
	case NODETYPE_CAST:
		{
		tSpiderValue	*tmp = AST_ExecuteNode(Block, Node->Cast.Value);
		ret = SpiderScript_CastValueTo( Node->Cast.DataType, tmp );
		Object_Dereference(tmp);
		}
		break;

	// Index into an array
	case NODETYPE_INDEX:
		AST_RuntimeError(Node, "TODO - Array Indexing");
		ret = ERRPTR;
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		AST_RuntimeError(Node, "TODO - Runtime Constants");
		ret = ERRPTR;
		break;
	// Constant Values
	case NODETYPE_STRING:	ret = SpiderScript_CreateString( Node->String.Length, Node->String.Data );	break;
	case NODETYPE_INTEGER:	ret = SpiderScript_CreateInteger( Node->Integer );	break;
	case NODETYPE_REAL: 	ret = SpiderScript_CreateReal( Node->Real );	break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALAND:	// Logical AND (&&)
	case NODETYPE_LOGICALOR:	// Logical OR (||)
	case NODETYPE_LOGICALXOR:	// Logical XOR (^^)
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
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
		Object_Dereference(op1);
		Object_Dereference(op2);
		break;
	
	// Comparisons
	case NODETYPE_EQUALS:
	case NODETYPE_LESSTHAN:
	case NODETYPE_GREATERTHAN:
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
			return ERRPTR;
		}
		
		// Convert types
		if( op1->Type != op2->Type ) {
			// If dynamically typed, convert op2 to op1's type
			if(Block->Script->Variant->bDyamicTyped)
			{
				tmpobj = op2;
				op2 = SpiderScript_CastValueTo(op1->Type, op2);
				Object_Dereference(tmpobj);
				if(op2 == ERRPTR) {
					Object_Dereference(op1);
					return ERRPTR;
				}
			}
			// If statically typed, this should never happen, but catch it anyway
			else {
				AST_RuntimeError(Node, "Statically typed implicit cast");
				ret = ERRPTR;
				break;
			}
		}
		// Do operation
		switch(op1->Type)
		{
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
		
		// - Integer Comparisons
		case SS_DATATYPE_INTEGER:
			if( op1->Integer == op2->Integer )
				cmp = 0;
			else if( op1->Integer < op2->Integer )
				cmp = -1;
			else
				cmp = 1;
			break;
		default:
			AST_RuntimeError(Node, "TODO - Comparison of type %i", op1->Type);
			ret = ERRPTR;
			break;
		}
		
		// Free intermediate objects
		Object_Dereference(op1);
		Object_Dereference(op2);
		
		// Error check
		if( ret == ERRPTR )
			break;
		
		// Create return
		switch(Node->Type)
		{
		case NODETYPE_EQUALS:	ret = SpiderScript_CreateInteger(cmp == 0);	break;
		case NODETYPE_LESSTHAN:	ret = SpiderScript_CreateInteger(cmp < 0);	break;
		case NODETYPE_GREATERTHAN:	ret = SpiderScript_CreateInteger(cmp > 0);	break;
		default:
			AST_RuntimeError(Node, "Exec,CmpOp unknown op %i", Node->Type);
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
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
			return ERRPTR;
		}
		
		ret = AST_ExecuteNode_BinOp(Block, Node->Type, op1, op2);
		
		// Free intermediate objects
		Object_Dereference(op1);
		Object_Dereference(op2);
		break;
	
	//default:
	//	ret = NULL;
	//	AST_RuntimeError(Node, "BUG - SpiderScript AST_ExecuteNode Unimplemented %i\n", Node->Type);
	//	break;
	}
_return:
	return ret;
}

tSpiderValue *AST_ExecuteNode_BinOp(tAST_BlockState *Block, int Operation, tSpiderValue *Left, tSpiderValue *Right)
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
		if(Block->Script->Variant->bImplicitCasts)
		{
			Right = SpiderScript_CastValueTo(Left->Type, Right);
			if(Right == ERRPTR)
				return ERRPTR;
		}
		// If statically typed, this should never happen, but catch it anyway
		else {
			AST_RuntimeError(NULL, "Implicit cast not allowed (from %i to %i)\n", Right->Type, Left->Type);
			return ERRPTR;
		}
	}
	
	// NULL Check
	if( Left == NULL || Right == NULL ) {
		if(Right && Right != preCastValue)	free(Right);
		return NULL;
	}
	
	// Do operation
	switch(Left->Type)
	{
	// String Concatenation
	case SS_DATATYPE_STRING:
		switch(Operation)
		{
		case NODETYPE_ADD:	// Concatenate
			ret = Object_StringConcat(Left, Right);
			break;
		default:
			AST_RuntimeError(NULL, "SpiderScript internal error: Exec,BinOP,String unknown op %i", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	// Integer Operations
	case SS_DATATYPE_INTEGER:
		switch(Operation)
		{
		case NODETYPE_ADD:	ret = SpiderScript_CreateInteger( Left->Integer + Right->Integer );	break;
		case NODETYPE_SUBTRACT:	ret = SpiderScript_CreateInteger( Left->Integer - Right->Integer );	break;
		case NODETYPE_MULTIPLY:	ret = SpiderScript_CreateInteger( Left->Integer * Right->Integer );	break;
		case NODETYPE_DIVIDE:	ret = SpiderScript_CreateInteger( Left->Integer / Right->Integer );	break;
		case NODETYPE_MODULO:	ret = SpiderScript_CreateInteger( Left->Integer % Right->Integer );	break;
		case NODETYPE_BWAND:	ret = SpiderScript_CreateInteger( Left->Integer & Right->Integer );	break;
		case NODETYPE_BWOR: 	ret = SpiderScript_CreateInteger( Left->Integer | Right->Integer );	break;
		case NODETYPE_BWXOR:	ret = SpiderScript_CreateInteger( Left->Integer ^ Right->Integer );	break;
		case NODETYPE_BITSHIFTLEFT:	ret = SpiderScript_CreateInteger( Left->Integer << Right->Integer );	break;
		case NODETYPE_BITSHIFTRIGHT:ret = SpiderScript_CreateInteger( Left->Integer >> Right->Integer );	break;
		case NODETYPE_BITROTATELEFT:
			ret = SpiderScript_CreateInteger( (Left->Integer << Right->Integer) | (Left->Integer >> (64-Right->Integer)) );
			break;
		default:
			AST_RuntimeError(NULL, "SpiderScript internal error: Exec,BinOP,Integer unknown op %i\n", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	
	// Real Numbers
	case SS_DATATYPE_REAL:
		switch(Operation)
		{
		default:
			AST_RuntimeError(NULL, "SpiderScript internal error: Exec,BinOP,Real unknown op %i", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	
	default:
		AST_RuntimeError(NULL, "BUG - Invalid operation (%i) on type (%i)", Operation, Left->Type);
		ret = ERRPTR;
		break;
	}
	
	if(Right && Right != preCastValue)	free(Right);
	
	return ret;
}

/**
 * \brief Define a variable
 * \param Block	Current block state
 * \param Type	Type of the variable
 * \param Name	Name of the variable
 * \return Boolean Failure
 */
tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name)
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
	var->Object = NULL;
	strcpy(var->Name, Name);
	
	if(prev)	prev->Next = var;
	else	Block->FirstVar = var;
	
	//printf("Defined variable %s (%i)\n", Name, Type);
	
	return var;
}

/**
 * \brief Set the value of a variable
 * \return Boolean Failure
 */
int Variable_SetValue(tAST_BlockState *Block, const char *Name, tSpiderValue *Value)
{
	tAST_Variable	*var;
	tAST_BlockState	*bs;
	
	for( bs = Block; bs; bs = bs->Parent )
	{
		for( var = bs->FirstVar; var; var = var->Next )
		{
			if( strcmp(var->Name, Name) == 0 )
			{
				if( !Block->Script->Variant->bDyamicTyped
				 && (Value && var->Type != Value->Type) )
				{
					AST_RuntimeError(NULL, "Type mismatch assigning to '%s'", Name);
					return -2;
				}
//				printf("Assign %p to '%s'\n", Value, var->Name);
				Object_Reference(Value);
				Object_Dereference(var->Object);
				var->Object = Value;
				return 0;
			}
		}
	}
	
	if( Block->Script->Variant->bDyamicTyped )
	{
		// Define variable
		var = Variable_Define(Block, Value->Type, Name);
		Object_Reference(Value);
		var->Object = Value;
		return 0;
	}
	else
	{
		AST_RuntimeError(NULL, "Variable '%s' set while undefined", Name);
		return -1;
	}
}

/**
 * \brief Get the value of a variable
 */
tSpiderValue *Variable_GetValue(tAST_BlockState *Block, const char *Name)
{
	tAST_Variable	*var;
	tAST_BlockState	*bs;
	
	for( bs = Block; bs; bs = bs->Parent )
	{
		for( var = bs->FirstVar; var; var = var->Next )
		{
			if( strcmp(var->Name, Name) == 0 ) {
				Object_Reference(var->Object);
				return var->Object;
			}
		}
	}
	
	
	AST_RuntimeError(NULL, "Variable '%s' used undefined", Name);
	
	return ERRPTR;
}

/**
 * \brief Destorys a variable
 */
void Variable_Destroy(tAST_Variable *Variable)
{
//	printf("Variable_Destroy: (%p'%s')\n", Variable, Variable->Name);
	Object_Dereference(Variable->Object);
	free(Variable);
}

void AST_RuntimeError(tAST_Node *Node, const char *Format, ...)
{
	va_list	args;
	
	fprintf(stderr, "ERROR: ");
	va_start(args, Format);
	vfprintf(stderr, Format, args);
	va_end(args);
	fprintf(stderr, "\n");
	
	if(Node)
	{
		fprintf(stderr, "   at %s:%i\n", Node->File, Node->Line);
	}
}

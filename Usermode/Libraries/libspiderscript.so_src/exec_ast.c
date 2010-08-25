/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

// === PROTOTYPES ===
void	Object_Dereference(tSpiderValue *Object);
void	Object_Reference(tSpiderValue *Object);
tSpiderValue	*Object_CreateInteger(uint64_t Value);
tSpiderValue	*Object_CreateReal(double Value);
tSpiderValue	*Object_CreateString(int Length, const char *Data);
tSpiderValue	*Object_CastTo(int Type, tSpiderValue *Source);
 int	Object_IsTrue(tSpiderValue *Value);

tSpiderValue	*AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node);

tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name);
void	Variable_SetValue(tAST_BlockState *Block, const char *Name, tSpiderValue *Value);
tSpiderValue	*Variable_GetValue(tAST_BlockState *Block, const char *Name);
void	Variable_Destroy(tAST_Variable *Variable);

// === CODE ===
/**
 * \brief Dereference a created object
 */
void Object_Dereference(tSpiderValue *Object)
{
	if(!Object)	return ;
	Object->ReferenceCount --;
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
}

/**
 * \brief Create an integer object
 */
tSpiderValue *Object_CreateInteger(uint64_t Value)
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
tSpiderValue *Object_CreateReal(double Value)
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
tSpiderValue *Object_CreateString(int Length, const char *Data)
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
tSpiderValue *Object_CastTo(int Type, tSpiderValue *Source)
{
	tSpiderValue	*ret = ERRPTR;
	// Check if anything needs to be done
	if( Source->Type == Type ) {
		Object_Reference(Source);
		return Source;
	}
	
	switch( (enum eSpiderScript_DataTypes)Type )
	{
	case SS_DATATYPE_UNDEF:
	case SS_DATATYPE_NULL:
	case SS_DATATYPE_ARRAY:
	case SS_DATATYPE_OPAQUE:
		fprintf(stderr, "Object_CastTo - Invalid cast to %i\n", Type);
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
			fprintf(stderr, "Object_CastTo - Invalid cast from %i\n", Source->Type);
			free(ret);
			ret = ERRPTR;
			break;
		}
		break;
	default:
		fprintf(stderr, "BUG REPORT: Unimplemented cast target\n");
		break;
	}
	
	return ret;
}

/**
 * \brief Condenses a value down to a boolean
 */
int Object_IsTrue(tSpiderValue *Value)
{
	switch( (enum eSpiderScript_DataTypes)Value->Type )
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
	
	case SS_DATATYPE_OPAQUE:
		return Value->Opaque.Data != NULL;
	
	case SS_DATATYPE_ARRAY:
		return Value->Array.Length > 0;
	default:
		fprintf(stderr, "Spiderscript internal error: Unknown type %i in Object_IsTrue\n", Value->Type);
		return 0;
	}
	return 0;
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
			blockInfo.Parent = Block;
			blockInfo.Script = Block->Script;
			ret = NULL;
			for(node = Node->Block.FirstChild; node; node = node->NextSibling )
			{
				if(node->Type == NODETYPE_RETURN) {
					ret = AST_ExecuteNode(&blockInfo, node);
					break ;
				}
				else {
					tmpobj = AST_ExecuteNode(&blockInfo, node);
					if(tmpobj == ERRPTR) {	// Error check
						ret = ERRPTR;
						break ;
					}
					if(tmpobj)	Object_Dereference(tmpobj);	// Free unused value
				}
			}
			// Clean up variables
			while(blockInfo.FirstVar)
			{
				tAST_Variable	*nextVar = blockInfo.FirstVar->Next;
				Variable_Destroy( blockInfo.FirstVar );
				blockInfo.FirstVar = nextVar;
			}
		}
		
		break;
	
	// Assignment
	case NODETYPE_ASSIGN:
		if( Node->Assign.Dest->Type != NODETYPE_VARIABLE ) {
			fprintf(stderr, "Syntax error: LVALUE of assignment is not a variable\n");
			return ERRPTR;
		}
		ret = AST_ExecuteNode(Block, Node->Assign.Value);
		if(ret != ERRPTR)
			Variable_SetValue( Block, Node->Assign.Dest->Variable.Name, ret );
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
	
	// Return's special handling happens elsewhere
	case NODETYPE_RETURN:
		ret = AST_ExecuteNode(Block, Node->UniOp.Value);
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
		ret = Object_CastTo(
			Node->Cast.DataType,
			AST_ExecuteNode(Block, Node->Cast.Value)
			);
		break;

	// Index into an array
	case NODETYPE_INDEX:
		fprintf(stderr, "TODO: Array indexing\n");
		ret = ERRPTR;
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		fprintf(stderr, "TODO: Runtime Constants\n");
		ret = ERRPTR;
		break;
	// Constant Values
	case NODETYPE_STRING:	ret = Object_CreateString( Node->String.Length, Node->String.Data );	break;
	case NODETYPE_INTEGER:	ret = Object_CreateInteger( Node->Integer );	break;
	case NODETYPE_REAL: 	ret = Object_CreateReal( Node->Real );	break;
	
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
			if(Block->Script->Variant->bDyamicTyped)
			{
				tmpobj = op2;
				op2 = Object_CastTo(op1->Type, op2);
				Object_Dereference(tmpobj);
				if(op2 == ERRPTR) {
					Object_Dereference(op1);
					return ERRPTR;
				}
			}
			// If statically typed, this should never happen, but catch it anyway
			else {
				fprintf(stderr, "PARSER ERROR: Statically typed implicit cast\n");
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
		default:
			fprintf(stderr, "SpiderScript internal error: TODO: Comparison of type %i\n", op1->Type);
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
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
			return ERRPTR;
		}
		
		// Convert types
		if( op1 && op2 && op1->Type != op2->Type ) {
			// If dynamically typed, convert op2 to op1's type
			if(Block->Script->Variant->bDyamicTyped)
			{
				tmpobj = op2;
				op2 = Object_CastTo(op1->Type, op2);
				Object_Dereference(tmpobj);
				if(op2 == ERRPTR) {
					Object_Dereference(op1);
					return ERRPTR;
				}
			}
			// If statically typed, this should never happen, but catch it anyway
			else {
				fprintf(stderr, "PARSER ERROR: Statically typed implicit cast\n");
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
			case NODETYPE_ADD:	// Concatenate
				ret = Object_StringConcat(op1, op2);
				break;
			default:
				fprintf(stderr, "SpiderScript internal error: Exec,BinOP,String unknown op %i\n", Node->Type);
				ret = ERRPTR;
				break;
			}
			break;
		// Integer Operations
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
				fprintf(stderr, "SpiderScript internal error: Exec,BinOP,Integer unknown op %i\n", Node->Type);
				ret = ERRPTR;
				break;
			}
			break;
		
		// Real Numbers
		case SS_DATATYPE_REAL:
			switch(Node->Type)
			{
			default:
				fprintf(stderr, "SpiderScript internal error: Exec,BinOP,Real unknown op %i\n", Node->Type);
				ret = ERRPTR;
				break;
			}
			break;
		
		default:
			fprintf(stderr, "SpiderScript error: Invalid operation (%i) on type (%i)\n", Node->Type, op1->Type);
			ret = ERRPTR;
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
_return:
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
			fprintf(stderr, "ERROR: Redefinition of variable '%s'\n", Name);
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
 */
void Variable_SetValue(tAST_BlockState *Block, const char *Name, tSpiderValue *Value)
{
	tAST_Variable	*var;
	tAST_BlockState	*bs;
	
	for( bs = Block; bs; bs = bs->Parent )
	{
		for( var = bs->FirstVar; var; var = var->Next )
		{
			if( strcmp(var->Name, Name) == 0 ) {
				if( !Block->Script->Variant->bDyamicTyped
				 && (Value && var->Type != Value->Type) ) {
					fprintf(stderr, "ERROR: Type mismatch assigning to '%s'\n", Name);
					return ;
				}
				Object_Reference(Value);
				Object_Dereference(var->Object);
				var->Object = Value;
				return ;
			}
		}
	}
	
	if( Block->Script->Variant->bDyamicTyped )
	{
		// Define variable
		var = Variable_Define(Block, Value->Type, Name);
		Object_Reference(Value);
		var->Object = Value;
	}
	else
	{
		fprintf(stderr, "ERROR: Variable '%s' set while undefined\n", Name);
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
	
	fprintf(stderr, "ERROR: Variable '%s' used undefined\n", Name);
	
	return ERRPTR;
}

/**
 * \brief Destorys a variable
 */
void Variable_Destroy(tAST_Variable *Variable)
{
	Object_Dereference(Variable->Object);
	free(Variable);
}

/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ast.h"

#define TRACE_VAR_LOOKUPS	0
#define TRACE_NODE_RETURNS	0

// === IMPORTS ===
extern tSpiderFunction	*gpExports_First;

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
tSpiderValue	*AST_ExecuteNode_BinOp(tAST_BlockState *Block, tAST_Node *Node, int Operation, tSpiderValue *Left, tSpiderValue *Right);
tSpiderValue	*AST_ExecuteNode_UniOp(tAST_BlockState *Block, tAST_Node *Node, int Operation, tSpiderValue *Value);

tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name, tSpiderValue *Value);
 int	Variable_SetValue(tAST_BlockState *Block, tAST_Node *VarNode, tSpiderValue *Value);
tSpiderValue	*Variable_GetValue(tAST_BlockState *Block, tAST_Node *VarNode);
void	Variable_Destroy(tAST_Variable *Variable);

void	AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...);
void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === GLOBALS ===
 int	giNextBlockIdent = 1;

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
 * \brief Allocate and initialise a SpiderScript object
 */
tSpiderObject *SpiderScript_AllocateObject(tSpiderObjectDef *Class, int ExtraBytes)
{
	 int	size = sizeof(tSpiderObject) + Class->NAttributes * sizeof(tSpiderValue*) + ExtraBytes;
	tSpiderObject	*ret = malloc(size);
	
	ret->Type = Class;
	ret->ReferenceCount = 1;
	ret->OpaqueData = &ret->Attributes[ Class->NAttributes ];
	memset( ret->Attributes, 0, Class->NAttributes * sizeof(tSpiderValue*) );
	
	return ret;
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
	if( Data )
		memcpy(ret->String.Data, Data, Length);
	else
		memset(ret->String.Data, 0, Length);
	ret->String.Data[Length] = '\0';
	return ret;
}

/**
 * \brief Concatenate two strings
 */
tSpiderValue *Object_StringConcat(const tSpiderValue *Str1, const tSpiderValue *Str2)
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

	if( !Source )
	{
		switch(Type)
		{
		case SS_DATATYPE_INTEGER:	return SpiderScript_CreateInteger(0);
		case SS_DATATYPE_REAL:	return SpiderScript_CreateReal(0);
		case SS_DATATYPE_STRING:	return SpiderScript_CreateString(4, "null");
		}
		return NULL;
	}
	
	// Check if anything needs to be done
	if( Source->Type == Type ) {
		Object_Reference(Source);
		return Source;
	}
	
	#if 0
	{
		printf("Casting %i ", Source->Type);
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	printf("0x%lx", Source->Integer);	break;
		case SS_DATATYPE_STRING:	printf("\"%s\"", Source->String.Data);	break;
		case SS_DATATYPE_REAL:	printf("%f", Source->Real);	break;
		default:	break;
		}
		printf(" to %i\n", Type);
	}
	#endif
	
	#if 0
	if( Source->Type == SS_DATATYPE_OBJECT )
	{
		const char	*name = NULL;
		switch(Type)
		{
		case SS_DATATYPE_INTEGER:	name = "cast Integer";	break;
		case SS_DATATYPE_REAL:  	name = "cast Real";	break;
		case SS_DATATYPE_STRING:	name = "cast String";	break;
		case SS_DATATYPE_ARRAY: 	name = "cast Array";	break;
		default:
			AST_RuntimeError(NULL, "Invalid cast to %i from Object", Type);
			return ERRPTR;
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
	
	switch( (enum eSpiderScript_DataTypes)Type )
	{
	case SS_DATATYPE_UNDEF:
	case SS_DATATYPE_ARRAY:
	case SS_DATATYPE_OPAQUE:
	case SS_DATATYPE_OBJECT:
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
	
	case SS_DATATYPE_REAL:
		ret = malloc(sizeof(tSpiderValue));
		ret->Type = SS_DATATYPE_REAL;
		ret->ReferenceCount = 1;
		switch(Source->Type)
		{
		case SS_DATATYPE_STRING:	ret->Real = atof(Source->String.Data);	break;
		case SS_DATATYPE_INTEGER:	ret->Real = Source->Integer;	break;
		default:
			AST_RuntimeError(NULL, "Invalid cast from %i to Real", Source->Type);
			free(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	case SS_DATATYPE_STRING:
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	len = snprintf(NULL, 0, "%li", Source->Integer);	break;
		case SS_DATATYPE_REAL:	len = snprintf(NULL, 0, "%g", Source->Real);	break;
		default:	break;
		}
		ret = malloc(sizeof(tSpiderValue) + len + 1);
		ret->Type = SS_DATATYPE_STRING;
		ret->ReferenceCount = 1;
		ret->String.Length = len;
		switch(Source->Type)
		{
		case SS_DATATYPE_INTEGER:	sprintf(ret->String.Data, "%li", Source->Integer);	break;
		case SS_DATATYPE_REAL:
			sprintf(ret->String.Data, "%g", Source->Real);	break;
		default:
			AST_RuntimeError(NULL, "Invalid cast from %i to String", Source->Type);
			free(ret);
			ret = ERRPTR;
			break;
		}
		break;
	
	default:
		AST_RuntimeError(NULL, "BUG - BUG REPORT: Unimplemented cast target %i", Type);
		ret = ERRPTR;
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
 * \brief Execute a script function
 * \param Script	Script context to execute in
 * \param Namespace	Namespace to search for the function
 * \param Function	Function name to execute
 * \param NArguments	Number of arguments to pass
 * \param Arguments	Arguments passed
 */
tSpiderValue *SpiderScript_ExecuteFunction(tSpiderScript *Script,
	tSpiderNamespace *Namespace, const char *Function,
	int NArguments, tSpiderValue **Arguments)
{
	 int	bFound = 0;	// Used to keep nesting levels down
	tSpiderValue	*ret = ERRPTR;
	tSpiderFunction	*fcn;
	
	// First: Find the function in the script
	{
		tAST_Function	*astFcn;
		for( astFcn = Script->Script->Functions; astFcn; astFcn = astFcn->Next )
		{
			if( strcmp(astFcn->Name, Function) == 0 )
				break;
		}
		// Execute!
		if(astFcn)
		{
			tAST_BlockState	bs;
			tAST_Node	*arg;
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
			for( arg = astFcn->Arguments; arg; arg = arg->NextSibling, i++ )
			{
				if( i >= NArguments )	break;	// TODO: Return gracefully
				// TODO: Type checks
				Variable_Define(&bs,
					arg->DefVar.DataType, arg->DefVar.Name,
					Arguments[i]);
			}
			
			// Execute function
			ret = AST_ExecuteNode(&bs, astFcn->Code);
			if(ret != ERRPTR)
			{
				Object_Dereference(ret);	// Dereference output of last block statement
				ret = bs.RetVal;	// Set to return value of block
			}
			bFound = 1;
			
			while(bs.FirstVar)
			{
				tAST_Variable	*nextVar = bs.FirstVar->Next;
				Variable_Destroy( bs.FirstVar );
				bs.FirstVar = nextVar;
			}
		}
	}
	
	// Didn't find it in script?
	if(!bFound)
	{
		fcn = NULL;	// Just to allow the below code to be neat
		
		// Second: Scan current namespace
		if( !fcn && Namespace )
		{
			for( fcn = Namespace->Functions; fcn; fcn = fcn->Next )
			{
				if( strcmp( fcn->Name, Function ) == 0 )
					break;
			}
		}
		
		// Third: Search the variant's global exports
		if( !fcn )
		{
			for( fcn = Script->Variant->Functions; fcn; fcn = fcn->Next )
			{
				if( strcmp( fcn->Name, Function ) == 0 )
					break;
			}
		}
		
		// Fourth: Search language exports
		if( !fcn )
		{
			for( fcn = gpExports_First; fcn; fcn = fcn->Next )
			{
				if( strcmp( fcn->Name, Function ) == 0 )
					break;
			}
		}
		
		// Execute!
		if(fcn)
		{
			// TODO: Type Checking
			ret = fcn->Handler( Script, NArguments, Arguments );
			bFound = 1;
		}
	}
	
	// Not found?
	if(!bFound)
	{
		fprintf(stderr, "Undefined reference to function '%s' (ns='%s')\n",
			Function, Namespace->Name);
		return ERRPTR;
	}
	
	return ret;
}

/**
 * \brief Execute an object method function
 * \param Script	Script context to execute in
 * \param Object	Object in which to find the method
 * \param MethodName	Name of method to call
 * \param NArguments	Number of arguments to pass
 * \param Arguments	Arguments passed
 */
tSpiderValue *SpiderScript_ExecuteMethod(tSpiderScript *Script,
	tSpiderObject *Object, const char *MethodName,
	int NArguments, tSpiderValue **Arguments)
{
	tSpiderFunction	*fcn;
	tSpiderValue	this;
	tSpiderValue	*newargs[NArguments+1];
	 int	i;
	
	// TODO: Support program defined objects
	
	// Search for the function
	for( fcn = Object->Type->Methods; fcn; fcn = fcn->Next )
	{
		if( strcmp(fcn->Name, MethodName) == 0 )
			break;
	}
	// Error
	if( !fcn )
	{
		AST_RuntimeError(NULL, "Class '%s' does not have a method '%s'",
			Object->Type->Name, MethodName);
		return ERRPTR;
	}
	
	// Create the "this" argument
	this.Type = SS_DATATYPE_OBJECT;
	this.ReferenceCount = 1;
	this.Object = Object;
	newargs[0] = &this;
	memcpy(&newargs[1], Arguments, NArguments*sizeof(tSpiderValue*));
	
	// Check the type of the arguments
	for( i = 0; fcn->ArgTypes[i]; i ++ )
	{
		if( i >= NArguments ) {
			for( ; fcn->ArgTypes[i]; i ++ )	;
			AST_RuntimeError(NULL, "Argument count mismatch (%i passed, %i expected)",
				NArguments, i);
			return ERRPTR;
		}
		if( Arguments[i] && Arguments[i]->Type != fcn->ArgTypes[i] )
		{
			AST_RuntimeError(NULL, "Argument type mismatch (%i, expected %i)",
				Arguments[i]->Type, fcn->ArgTypes[i]);
			return ERRPTR;
		}
	}
	
	// Call handler
	return fcn->Handler(Script, NArguments+1, newargs);
}

/**
 * \brief Execute a script function
 * \param Script	Script context to execute in
 * \param Function	Function name to execute
 * \param NArguments	Number of arguments to pass
 * \param Arguments	Arguments passed
 */
tSpiderValue *SpiderScript_CreateObject(tSpiderScript *Script,
	tSpiderNamespace *Namespace, const char *ClassName,
	int NArguments, tSpiderValue **Arguments)
{
	 int	bFound = 0;	// Used to keep nesting levels down
	tSpiderValue	*ret = ERRPTR;
	tSpiderObjectDef	*class;
	
	// First: Find the function in the script
	// TODO: Implement script-defined classes
	#if 0
	{
		tAST_Function	*astClass;
		for( astClass = Script->Script->Classes; astClass; astClass = astClass->Next )
		{
			if( strcmp(astClass->Name, ClassName) == 0 )
				break;
		}
		// Execute!
		if(astClass)
		{
			tAST_BlockState	bs;
			tAST_Node	*arg;
			 int	i = 0;
			
			// Build a block State
			bs.FirstVar = NULL;
			bs.RetVal = NULL;
			bs.Parent = NULL;
			bs.BaseNamespace = &Script->Variant->RootNamespace;
			bs.CurNamespace = NULL;
			bs.Script = Script;
			bs.Ident = giNextBlockIdent ++;
			
			for( arg = astFcn->Arguments; arg; arg = arg->NextSibling, i++ )
			{
				if( i >= NArguments )	break;	// TODO: Return gracefully
				// TODO: Type checks
				Variable_Define(&bs,
					arg->DefVar.DataType, arg->DefVar.Name,
					Arguments[i]);
			}
			
			// Execute function
			ret = AST_ExecuteNode(&bs, astFcn->Code);
			if( ret != ERRPTR )
			{
				Object_Dereference(ret);	// Dereference output of last block statement
				ret = bs.RetVal;	// Set to return value of block
			}
			bFound = 1;
			
			while(bs.FirstVar)
			{
				tAST_Variable	*nextVar = bs.FirstVar->Next;
				Variable_Destroy( bs.FirstVar );
				bs.FirstVar = nextVar;
			}
		}
	}
	#endif
	
	// Didn't find it in script?
	if(!bFound)
	{
		class = NULL;	// Just to allow the below code to be neat
		
		//if( !Namespace )
		//	Namespace = &Script->Variant->RootNamespace;
		
		// Second: Scan current namespace
		if( !class && Namespace )
		{
			for( class = Namespace->Classes; class; class = class->Next )
			{
				if( strcmp( class->Name, ClassName ) == 0 )
					break;
			}
		}
		
		#if 0
		// Third: Search the variant's global exports
		if( !class )
		{
			for( class = Script->Variant->Classes; class; class = fcn->Next )
			{
				if( strcmp( class->Name, Function ) == 0 )
					break;
			}
		}
		#endif
		
		#if 0
		// Fourth: Search language exports
		if( !class )
		{
			for( class = gpExports_First; class; class = fcn->Next )
			{
				if( strcmp( class->Name, ClassName ) == 0 )
					break;
			}
		}
		#endif
		
		// Execute!
		if(class)
		{
			tSpiderObject	*obj;
			// TODO: Type Checking
			
			// Call constructor
			obj = class->Constructor( NArguments, Arguments );
			if( obj == NULL || obj == ERRPTR )
				return (void *)obj;
			
			// Creatue return object
			ret = malloc( sizeof(tSpiderValue) );
			ret->Type = SS_DATATYPE_OBJECT;
			ret->ReferenceCount = 1;
			ret->Object = obj;
			bFound = 1;
		}
	}
	
	// Not found?
	if(!bFound)
	{
		fprintf(stderr, "Undefined reference to class '%s'\n", ClassName);
		return ERRPTR;
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
	 int	cmp;	// Used in comparisons
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
			blockInfo.Ident = giNextBlockIdent ++;
			ret = NULL;
			// Loop over all nodes, or until the return value is set
			for(node = Node->Block.FirstChild; node && !blockInfo.RetVal; node = node->NextSibling )
			{
				ret = AST_ExecuteNode(&blockInfo, node);
				if(ret == ERRPTR)	break;	// Error check
				if(ret != NULL)	Object_Dereference(ret);	// Free unused value
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
			tSpiderValue	*varVal = Variable_GetValue(Block, Node->Assign.Dest);
			tSpiderValue	*value;
			value = AST_ExecuteNode_BinOp(Block, Node, Node->Assign.Operation, varVal, ret);
			if(value == ERRPTR)	return ERRPTR;
			if(ret)	Object_Dereference(ret);
			if(varVal)	Object_Dereference(varVal);
			ret = value;
		}
		
		// Set the variable value
		if( Variable_SetValue( Block, Node->Assign.Dest, ret ) ) {
			Object_Dereference( ret );
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
				value = AST_ExecuteNode_BinOp(Block, Node, NODETYPE_SUBTRACT, varVal, &one);
			else
				value = AST_ExecuteNode_BinOp(Block, Node, NODETYPE_ADD, varVal, &one);
			if( value == ERRPTR )
				return ERRPTR;
			
			ret = varVal;
		
			if( Variable_SetValue( Block, Node->UniOp.Value, value ) ) {
				Object_Dereference( ret );
				return ERRPTR;
			}
			Object_Dereference( value );
		}
		break;
	
	// Function Call
	case NODETYPE_METHODCALL:
	case NODETYPE_FUNCTIONCALL:
	case NODETYPE_CREATEOBJECT:
		// Logical block (used to allocate `params`)
		{
			tSpiderNamespace	*ns = Block->CurNamespace;
			tSpiderValue	*params[Node->FunctionCall.NumArgs];
			i = 0;
			for(node = Node->FunctionCall.FirstArg; node; node = node->NextSibling)
			{
				params[i] = AST_ExecuteNode(Block, node);
				if( params[i] == ERRPTR ) {
					while(i--)	Object_Dereference(params[i]);
					ret = ERRPTR;
					goto _return;
				}
				i ++;
			}
			
			if( !ns )	ns = Block->BaseNamespace;
			
			// Call the function
			if( Node->Type == NODETYPE_CREATEOBJECT )
			{
				ret = SpiderScript_CreateObject(Block->Script,
					ns,
					Node->FunctionCall.Name,
					Node->FunctionCall.NumArgs, params
					);
			}
			else if( Node->Type == NODETYPE_METHODCALL )
			{
				tSpiderValue *obj = AST_ExecuteNode(Block, Node->FunctionCall.Object);
				if( !obj || obj == ERRPTR || obj->Type != SS_DATATYPE_OBJECT ) {
					AST_RuntimeError(Node->FunctionCall.Object,
						"Type Mismatch - Required SS_DATATYPE_OBJECT for method call");
					while(i--)	Object_Dereference(params[i]);
					ret = ERRPTR;
					break;
				}
				ret = SpiderScript_ExecuteMethod(Block->Script,
					obj->Object, Node->FunctionCall.Name,
					Node->FunctionCall.NumArgs, params
					);
				Object_Dereference(obj);
			}
			else
			{
				ret = SpiderScript_ExecuteFunction(Block->Script,
					ns, Node->FunctionCall.Name,
					Node->FunctionCall.NumArgs, params
					);
			}

			
			// Dereference parameters
			while(i--)	Object_Dereference(params[i]);
			
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
		Object_Dereference(ret);
		if( tmpobj == ERRPTR )	return ERRPTR;
		Object_Dereference(tmpobj);
		ret = NULL;
		break;
	
	// Loop
	case NODETYPE_LOOP:
		ret = AST_ExecuteNode(Block, Node->For.Init);
		if(ret == ERRPTR)	break;
		if( Node->For.bCheckAfter )
		{
			do {
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Code);
				if(ret == ERRPTR)	return ERRPTR;
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Increment);
				if(ret == ERRPTR)	return ERRPTR;
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Condition);
				if(ret == ERRPTR)	return ERRPTR;
			} while( SpiderScript_IsValueTrue(ret) );
		}
		else
		{
			Object_Dereference(ret);
			ret = AST_ExecuteNode(Block, Node->For.Condition);
			if(ret == ERRPTR)	return ERRPTR;
			while( SpiderScript_IsValueTrue(ret) ) {
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Code);
				if(ret == ERRPTR)	return ERRPTR;
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Increment);
				if(ret == ERRPTR)	return ERRPTR;
				Object_Dereference(ret);
				ret = AST_ExecuteNode(Block, Node->For.Condition);
				if(ret == ERRPTR)	return ERRPTR;
			}
		}
		Object_Dereference(ret);
		ret = NULL;
		break;
	
	// Return
	case NODETYPE_RETURN:
		ret = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(ret == ERRPTR)	break;
		Block->RetVal = ret;	// Return value set
		ret = NULL;	// the `return` statement does not return a value
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
		ret = NULL;
		if( Variable_Define(Block, Node->DefVar.DataType, Node->DefVar.Name, tmpobj) == ERRPTR )
			ret = ERRPTR;
		Object_Dereference(tmpobj);
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
				Object_Reference(ret);
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
		Object_Dereference(tmpobj);
		}
		break;

	// Index into an array
	case NODETYPE_INDEX:
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);	// Array
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);	// Offset
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
			return ERRPTR;
		}
		
		if( !op1 || op1->Type != SS_DATATYPE_ARRAY )
		{
			// TODO: Implement "operator []" on objects
			AST_RuntimeError(Node, "Indexing non-array");
			ret = ERRPTR;
			break;
		}
		
		if( (!op2 || op2->Type != SS_DATATYPE_INTEGER) && !Block->Script->Variant->bImplicitCasts ) {
			AST_RuntimeError(Node, "Array index is not an integer");
			ret = ERRPTR;
			break;
		}
		
		if( !op2 || op2->Type != SS_DATATYPE_INTEGER )
		{
			tmpobj = SpiderScript_CastValueTo(SS_DATATYPE_INTEGER, op2);
			Object_Dereference(op2);
			op2 = tmpobj;
		}
		
		if( op2->Integer >= op1->Array.Length ) {
			AST_RuntimeError(Node, "Array index out of bounds %i >= %i",
				op2->Integer, op1->Array.Length);
			ret = ERRPTR;
			break;
		}
		
		ret = op1->Array.Items[ op2->Integer ];
		Object_Reference(ret);
		
		Object_Dereference(op1);
		Object_Dereference(op2);
		break;

	// TODO: Implement runtime constants
	case NODETYPE_CONSTANT:
		// TODO: Scan namespace for function
		AST_RuntimeError(Node, "TODO - Runtime Constants");
		ret = ERRPTR;
		break;
	
	// Constant Values
	case NODETYPE_STRING:
	case NODETYPE_INTEGER:
	case NODETYPE_REAL:
		ret = &Node->Constant;
		Object_Reference(ret);
		break;
	
	// --- Operations ---
	// Boolean Operations
	case NODETYPE_LOGICALNOT:	// Logical NOT (!)
		op1 = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(op1 == ERRPTR)	return ERRPTR;
		ret = SpiderScript_CreateInteger( !SpiderScript_IsValueTrue(op1) );
		Object_Dereference(op1);
		break;
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
	case NODETYPE_LESSTHANEQUAL:
	case NODETYPE_GREATERTHANEQUAL:
		op1 = AST_ExecuteNode(Block, Node->BinOp.Left);
		if(op1 == ERRPTR)	return ERRPTR;
		op2 = AST_ExecuteNode(Block, Node->BinOp.Right);
		if(op2 == ERRPTR) {
			Object_Dereference(op1);
			ret = ERRPTR;
			break;
		}
		
		if( !op1 || !op2 ) {
			AST_RuntimeError(Node, "NULL Comparison (%p and %p)", op1, op2);
			if(op1)	Object_Dereference(op1);
			if(op2)	Object_Dereference(op2);
			ret = SpiderScript_CreateInteger( !op1 && !op2 );
			break;
		}
		
		// Convert types
		if( op1->Type != op2->Type ) {
			// If dynamically typed, convert op2 to op1's type
			if(Block->Script->Variant->bImplicitCasts)
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
				AST_RuntimeError(Node, "Statically typed implicit cast %i <op> %i",
					op1->Type, op2->Type);
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
		// - Real Number Comparisons
		case SS_DATATYPE_REAL:
			cmp = (op1->Real - op2->Real) / op2->Real * 10000;	// < 0.1% difference is equality
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
		case NODETYPE_LESSTHANEQUAL:	ret = SpiderScript_CreateInteger(cmp <= 0);	break;
		case NODETYPE_GREATERTHANEQUAL:	ret = SpiderScript_CreateInteger(cmp >= 0);	break;
		default:
			AST_RuntimeError(Node, "Exec,CmpOp unknown op %i", Node->Type);
			ret = ERRPTR;
			break;
		}
		break;
	
	// General Unary Operations
	case NODETYPE_BWNOT:	// Bitwise NOT (~)
	case NODETYPE_NEGATE:	// Negation (-)
		op1 = AST_ExecuteNode(Block, Node->UniOp.Value);
		if(op1 == ERRPTR)	return ERRPTR;
		ret = AST_ExecuteNode_UniOp(Block, Node, Node->Type, op1);
		Object_Dereference(op1);
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
		
		ret = AST_ExecuteNode_BinOp(Block, Node, Node->Type, op1, op2);
		
		// Free intermediate objects
		Object_Dereference(op1);
		Object_Dereference(op2);
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

tSpiderValue *AST_ExecuteNode_UniOp(tAST_BlockState *Block, tAST_Node *Node, int Operation, tSpiderValue *Value)
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
			// Fall through and try casting (which will usually fail)
		}
	}
	#endif
	switch(Value->Type)
	{
	// Integer Operations
	case SS_DATATYPE_INTEGER:
		switch(Operation)
		{
		case NODETYPE_NEGATE:	ret = SpiderScript_CreateInteger( -Value->Integer );	break;
		case NODETYPE_BWNOT:	ret = SpiderScript_CreateInteger( ~Value->Integer );	break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,UniOP,Integer unknown op %i", Operation);
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

tSpiderValue *AST_ExecuteNode_BinOp(tAST_BlockState *Block, tAST_Node *Node, int Operation, tSpiderValue *Left, tSpiderValue *Right)
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
			AST_RuntimeError(Node, "Implicit cast not allowed (from %i to %i)", Right->Type, Left->Type);
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
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,String unknown op %i", Operation);
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
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,Integer unknown op %i", Operation);
			ret = ERRPTR;
			break;
		}
		break;
	
	// Real Numbers
	case SS_DATATYPE_REAL:
		switch(Operation)
		{
		case NODETYPE_ADD:	ret = SpiderScript_CreateReal( Left->Real + Right->Real );	break;
		case NODETYPE_SUBTRACT:	ret = SpiderScript_CreateReal( Left->Real - Right->Real );	break;
		case NODETYPE_MULTIPLY:	ret = SpiderScript_CreateReal( Left->Real * Right->Real );	break;
		case NODETYPE_DIVIDE:	ret = SpiderScript_CreateReal( Left->Real / Right->Real );	break;
		default:
			AST_RuntimeError(Node, "SpiderScript internal error: Exec,BinOP,Real unknown op %i", Operation);
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
	if(Value)	Object_Reference(Value);
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
	tAST_Variable	*var = Variable_Lookup(Block, VarNode, Value->Type);
	
	if( !var )	return -1;
	
	if( !Block->Script->Variant->bDyamicTyped && (Value && var->Type != Value->Type) )
	{
		AST_RuntimeError(VarNode, "Type mismatch assigning to '%s'",
			VarNode->Variable.Name);
		return -2;
	}

//	printf("Assign %p to '%s'\n", Value, var->Name);
	Object_Reference(Value);
	Object_Dereference(var->Object);
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
	
	Object_Reference(var->Object);
	return var->Object;
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

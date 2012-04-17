/*
* SpiderScript Library
* by John Hodge (thePowersGang)
* 
* bytecode_makefile.c
* - Generate a bytecode file
*/
#include <stdlib.h>
#include "common.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// === IMPORTS ===
extern tSpiderFunction	*gpExports_First;
extern tSpiderValue	*AST_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Arguments);
extern tSpiderValue	*Bytecode_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Args);

// === PROTOTYPES ===
void	AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...);
void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === CODE ===
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
	
	// First: Find the function in the script
	if( !Namespace )
	{
		tScript_Function	*fcn;
		for( fcn = Script->Functions; fcn; fcn = fcn->Next )
		{
			if( strcmp(fcn->Name, Function) == 0 )
				break;
		}
		// Execute!
		if(fcn)
		{
			if( fcn->BCFcn )
				ret = Bytecode_ExecuteFunction(Script, fcn, NArguments, Arguments);
			else
				ret = AST_ExecuteFunction(Script, fcn, NArguments, Arguments);
			bFound = 1;
			return ret;
		}
	}
	
	// Didn't find it in script?
	if(!bFound)
	{
		tSpiderFunction	*fcn;
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
				SpiderScript_DereferenceValue(ret);	// Dereference output of last block statement
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

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

#define BC_NS_SEPARATOR	'@'

// === IMPORTS ===
extern tSpiderFunction	*gpExports_First;
extern tSpiderNamespace	gExportNamespaceRoot;
extern tSpiderValue	*AST_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Arguments);
extern tSpiderValue	*Bytecode_ExecuteFunction(tSpiderScript *Script, tScript_Function *Fcn, int NArguments, tSpiderValue **Args);

// === PROTOTYPES ===
void	AST_RuntimeMessage(tAST_Node *Node, const char *Type, const char *Format, ...);
void	AST_RuntimeError(tAST_Node *Node, const char *Format, ...);

// === CODE ===
void *SpiderScript_int_GetNamespace(tSpiderScript *Script, tSpiderNamespace *RootNamespace,
	const char *BasePath, const char *ItemPath,
	const char **ItemName
	)
{
	 int	len;
	const char	*name;
	const char	*end;
	 int	bTriedBase;
	
	tSpiderNamespace	*lastns, *ns;

	// Prepend the base namespace
	if( BasePath ) {
		name = BasePath;
		bTriedBase = 0;
	}
	else {
		bTriedBase = 1;
		name = ItemPath;
	}
	
	// Scan
	lastns = RootNamespace;
	do {
		end = strchr(name, BC_NS_SEPARATOR);
		if(!end) {
			if( !bTriedBase )
				len = strlen(name);
			else
				break;
		}
		else {
			len = end - name;
		}

		// Check for this level
		for( ns = lastns->FirstChild; ns; ns = ns->Next )
		{
			printf("%p %.*s == %s\n", lastns, len, name, ns->Name);
			if( strncmp(name, ns->Name, len) == 0 && ns->Name[len] == 0 )
				break ;
		}
		
		if(!ns)	return NULL;

		if(!end && !bTriedBase) {
			end = ItemPath - 1;	// -1 to counter (name = end + 1)
			bTriedBase = 1;
		}

		lastns = ns;
		name = end + 1;
	} while( end );

	*ItemName = name;

	return lastns;
}

tSpiderFunction *SpiderScript_int_GetNativeFunction(tSpiderScript *Script, tSpiderNamespace *RootNamespace,
	const char *BasePath, const char *FunctionPath)
{
	tSpiderNamespace *ns;
	const char *name;
	tSpiderFunction	*fcn;

	ns = SpiderScript_int_GetNamespace(Script, RootNamespace, BasePath, FunctionPath, &name);
	if(!ns)	return NULL;

	for( fcn = ns->Functions; fcn; fcn = fcn->Next )
	{
		if( strcmp(name, fcn->Name) == 0 )
			return fcn;
	}
	
	return NULL;
}

tSpiderObjectDef *SpiderScript_int_GetNativeClass(tSpiderScript *Script, tSpiderNamespace *RootNamespace,
	const char *BasePath, const char *ClassPath
	)
{
	tSpiderNamespace *ns;
	const char *name;
	tSpiderObjectDef *class;

	ns = SpiderScript_int_GetNamespace(Script, RootNamespace, BasePath, ClassPath, &name);
	if(!ns)	return NULL;
	
	for( class = ns->Classes; class; class = class->Next )
	{
		if( strcmp(name, class->Name) == 0 )
			return class;
	}

	return NULL;
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
	const char *Function,
	const char *DefaultNamespaces[],
	int NArguments, tSpiderValue **Arguments,
	void **FunctionIdent
	)
{
	tSpiderValue	*ret = ERRPTR;
	tSpiderFunction	*fcn = NULL;
	 int	i;

	// Scan list, Last item should always be NULL, so abuse that to check non-prefixed	
	for( i = 0; i == 0 || (DefaultNamespaces && DefaultNamespaces[i-1]); i ++ )
	{
		const char *ns = DefaultNamespaces ? DefaultNamespaces[i] : NULL;
		fcn = SpiderScript_int_GetNativeFunction(Script, &Script->Variant->RootNamespace, ns, Function);
		if( fcn )	break;

		fcn = SpiderScript_int_GetNativeFunction(Script, &gExportNamespaceRoot, ns, Function);
		if( fcn )	break;
	
		// TODO: Script namespacing
	}

	// Search the variant's global exports
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
	
	// Find the function in the script?
	// TODO: Script namespacing
	if( !fcn && strchr(Function, BC_NS_SEPARATOR) == NULL )
	{
		tScript_Function	*sfcn;
		for( sfcn = Script->Functions; sfcn; sfcn = sfcn->Next )
		{
			if( strcmp(sfcn->Name, Function) == 0 )
				break;
		}
		// Execute!
		if(sfcn)
		{
			if( sfcn->BCFcn )
				ret = Bytecode_ExecuteFunction(Script, sfcn, NArguments, Arguments);
			else
				ret = AST_ExecuteFunction(Script, sfcn, NArguments, Arguments);

			if( FunctionIdent ) {
				*FunctionIdent = sfcn;
				// Abuses alignment requirements on almost all platforms
				*(intptr_t*)FunctionIdent |= 1;
			}

			return ret;
		}
	}
	
	if(fcn)
	{
		// Execute!
		// TODO: Type Checking
		ret = fcn->Handler( Script, NArguments, Arguments );
	
		if( FunctionIdent )
			*FunctionIdent = fcn;		

		return ret;
	}
	else
	{
		fprintf(stderr, "Undefined reference to function '%s'\n", Function);
		return ERRPTR;
	}
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
	const char *ClassPath, const char *DefaultNamespaces[],
	int NArguments, tSpiderValue **Arguments)
{
	tSpiderValue	*ret = ERRPTR;
	tSpiderObjectDef	*class;
	 int	i;	

	// Scan list, Last item should always be NULL, so abuse that to check non-prefixed	
	for( i = 0; i == 0 || DefaultNamespaces[i-1]; i ++ )
	{
		const char *ns = DefaultNamespaces[i];
		class = SpiderScript_int_GetNativeClass(Script, &Script->Variant->RootNamespace, ns, ClassPath);
		if( class )	break;

		class = SpiderScript_int_GetNativeClass(Script, &gExportNamespaceRoot, ns, ClassPath);
		if( class )	break;
		
		// TODO: Language defined classes
	}
		
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
		
		return ret;
	}
	else	// Not found?
	{
		fprintf(stderr, "Undefined reference to class '%s'\n", ClassPath);
		return ERRPTR;
	}
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

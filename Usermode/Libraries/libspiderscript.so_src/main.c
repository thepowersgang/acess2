/*
 * Acess2 - SpiderScript
 * Interpreter Library
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <spiderscript.h>
#include "ast.h"

// === IMPORTS ===
extern tAST_Script	*Parse_Buffer(tSpiderVariant *Variant, char *Buffer);
extern tSpiderFunction	*gpExports_First;
extern tAST_Variable *Variable_Define(tAST_BlockState *Block, int Type, const char *Name);
extern void	Variable_SetValue(tAST_BlockState *Block, const char *Name, tSpiderValue *Value);

// === CODE ===
/**
 * \brief Library Entry Point
 */
int SoMain()
{
	return 0;
}

/**
 * \brief Parse a script
 */
tSpiderScript *SpiderScript_ParseFile(tSpiderVariant *Variant, const char *Filename)
{
	char	*data;
	 int	fLen;
	FILE	*fp;
	tSpiderScript	*ret;
	
	fp = fopen(Filename, "r");
	if( !fp ) {
		return NULL;
	}
	
	// Create the script
	ret = malloc(sizeof(tSpiderScript));
	ret->Variant = Variant;
	
	fseek(fp, 0, SEEK_END);
	fLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// Allocate and read data
	data = malloc(fLen + 1);
	if(!data)	return NULL;
	fread(data, fLen, 1, fp);
	data[fLen] = '\0';
	
	fclose(fp);
	
	ret->CurNamespace = NULL;
	ret->Script = Parse_Buffer(Variant, data);
	if( ret->Script == NULL ) {
		free(data);
		free(ret);
		return NULL;
	}
	
	free(data);
	
	return ret;
}

/**
 * \brief Execute a script function
 * \param Script	Script context to execute in
 * \param Function	Function name to execute
 * \param NArguments	Number of arguments to pass
 * \param Arguments	Arguments passed
 */
tSpiderValue *SpiderScript_ExecuteMethod(tSpiderScript *Script,
	const char *Function, int NArguments, tSpiderValue **Arguments)
{
	char	*trueName = NULL;
	 int	bFound = 0;	// Used to keep nesting levels down
	tSpiderValue	*ret = ERRPTR;
	
	// Handle namespaces
	if( Function[0] == '.' ) {
		trueName = (char*)&Function[1];
	}
	else if( !Script->CurNamespace ) {
		trueName = (char*)Function;
	}
	else {
		 int	len = strlen(Script->CurNamespace) + 1 + strlen(Function);
		trueName = malloc( len + 1 );
		strcpy(trueName, Script->CurNamespace);
		strcat(trueName, ".");
		strcat(trueName, Function);
	}
	
	// First: Find the function in the script
	{
		tAST_Function	*fcn = Script->Script->Functions;
		for( ; fcn; fcn = fcn->Next ) {
			if( strcmp(fcn->Name, trueName) == 0 )
				break;
		}
		// Execute!
		if(fcn) {
			tAST_BlockState	bs;
			bs.FirstVar = NULL;
			bs.RetVal = NULL;
			bs.Parent = NULL;
			bs.Script = Script;
			{
				tAST_Node	*arg;
				 int	i = 0;
				for( arg = fcn->Arguments; arg; arg = arg->NextSibling, i++ )
				{
					// TODO: Type checks
					Variable_Define(&bs, arg->DefVar.DataType, arg->DefVar.Name);
					if( i >= NArguments )	break;	// TODO: Return gracefully
					Variable_SetValue(&bs, arg->DefVar.Name, Arguments[i]);
				}
			}
			ret = AST_ExecuteNode(&bs, fcn->Code);
			Object_Dereference(ret);
			ret = bs.RetVal;
			bFound = 1;
		}
	}
	
	// Didn't find it in script?
	if(!bFound)
	{
		tSpiderFunction	*fcn;
		// Second: Search the variant's exports
		for( fcn = Script->Variant->Functions; fcn; fcn = fcn->Next )
		{
			if( strcmp( fcn->Name, trueName ) == 0 )
				break;
		}
		// Execute!
		if(fcn) {
			// TODO: Type Checking
			ret = fcn->Handler( Script, NArguments, Arguments );
			bFound = 1;
		}
	}
	
	// Not in variant exports? Search the language internal ones
	if(!bFound)
	{
		tSpiderFunction	*fcn;
		// Third: Search language exports
		for( fcn = gpExports_First; fcn; fcn = fcn->Next )
		{
			if( strcmp( fcn->Name, trueName ) == 0 )
				break;
		}
		// Execute!
		if(fcn) {
			ret = fcn->Handler( Script, NArguments, Arguments );
			bFound = 1;
		}
	}
	
	// Not found?
	if(!bFound)
	{
		fprintf(stderr, "Undefined reference to '%s'\n", trueName);
	}
	
	if( trueName != Function && trueName != &Function[1] )
		free(trueName);
	
	return ret;
	
}

/**
 * \brief Free a script
 */
void SpiderScript_Free(tSpiderScript *Script)
{
	tAST_Function	*fcn = Script->Script->Functions;
	tAST_Function	*nextFcn;
	tAST_Node	*var, *nextVar;
	
	// Free functions
	while(fcn) {
		AST_FreeNode( fcn->Code );
		
		var = fcn->Arguments;
		while(var)
		{
			nextVar = var->NextSibling;
			AST_FreeNode( var );
			var = nextVar;
		}
		
		nextFcn = fcn->Next;
		free( fcn );
		fcn = nextFcn;
	}
	
	// TODO: Pass this off to AST for a proper cleanup
	free(Script->Script);
	
	free(Script);
}

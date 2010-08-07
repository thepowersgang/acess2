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
	
	ret = malloc(sizeof(tSpiderScript));
	ret->Variant = Variant;
	
	fseek(fp, 0, SEEK_END);
	fLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	data = malloc(fLen);
	if(!data)	return NULL;
	fread(data, fLen, 1, fp);
	
	fclose(fp);
	
	ret->Script = Parse_Buffer(Variant, data);
	
	free(data);
	
	return ret;
}

/**
 * \brief Execute a script function
 * \todo Arguments?
 */
tSpiderVariable *SpiderScript_ExecuteMethod(tSpiderScript *Script, const char *Function)
{
	tAST_Function	*fcn = Script->Script->Functions;
	
	// Find the function
	for( ; fcn; fcn = fcn->Next ) {
		if( strcmp(fcn->Name, Function) == 0 )
			break;
	}
	if(!fcn)	return NULL;
	
	// Execute!
	return AST_ExecuteNode(Script, fcn->Code);
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
}

/*
 * Acess2 - SpiderScript
 * - Script Exports (Lang. Namespace)
 */
#define _GNU_SOURCE	// HACK!
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <spiderscript.h>

// === PROTOTYPES ===
tSpiderValue	*Exports_sizeof(tSpiderScript *Script, int NArgs, tSpiderValue **Args);
tSpiderValue	*Exports_array(tSpiderScript *Script, int NArgs, tSpiderValue **Args);
tSpiderValue	*Exports_Lang_Strings_Split(tSpiderScript *Script, int NArgs, tSpiderValue **Args);
tSpiderValue	*Exports_Lang_Struct(tSpiderScript *Script, int NArgs, tSpiderValue **Args);

// === GLOBALS ===
tSpiderFunction	gExports_Lang_Strings_Split = {
	.Name = "Split",
	.Handler = Exports_Lang_Strings_Split,
	.ReturnType = SS_MAKEARRAY(SS_DATATYPE_STRING),
	.ArgTypes = {SS_DATATYPE_STRING, SS_DATATYPE_STRING, -1}
};
tSpiderNamespace	gExports_NS_Lang_Strings = {
	.Name = "Strings",
	.Functions = &gExports_Lang_Strings_Split
	};

tSpiderFunction	gExports_Lang_Struct = {
	.Name = "Struct",
	.Handler = Exports_Lang_Struct,
	.ReturnType = SS_DATATYPE_OPAQUE,
	.ArgTypes = {SS_DATATYPE_STRING, -1}
};
// - Lang Namespace
tSpiderNamespace	gExports_NS_Lang = {
	.Name = "Lang",
	.Functions = &gExports_Lang_Struct,
	.FirstChild = &gExports_NS_Lang_Strings
	};
tSpiderNamespace	gExportNamespaceRoot = {
	.FirstChild = &gExports_NS_Lang
};

// -- Global Functions
tSpiderFunction	gExports_array = {
	.Next = NULL,
	.Name = "array",
	.Handler = Exports_array,
	.ReturnType = SS_DATATYPE_DYNAMIC,
	.ArgTypes = {SS_DATATYPE_INTEGER, -1}
};
tSpiderFunction	gExports_sizeof = {
	.Next = &gExports_array,
	.Name = "sizeof",
	.Handler = Exports_sizeof,
	.ReturnType = SS_DATATYPE_INTEGER,
	.ArgTypes = {SS_DATATYPE_UNDEF, -1}
};
tSpiderFunction	*gpExports_First = &gExports_sizeof;

// === CODE ===
tSpiderValue *Exports_sizeof(tSpiderScript *Script, int NArgs, tSpiderValue **Args)
{
	if(NArgs != 1 || !Args[0])	return NULL;

	switch( Args[0]->Type )
	{
	case SS_DATATYPE_STRING:
		return SpiderScript_CreateInteger(Args[0]->String.Length);
	case SS_DATATYPE_ARRAY:
		return SpiderScript_CreateInteger(Args[0]->Array.Length);
	default:
		return NULL;
	}
}

tSpiderValue *Exports_array(tSpiderScript *Script, int NArgs, tSpiderValue **Args)
{
	if(NArgs != 2)	return ERRPTR;
	if(!Args[0] || !Args[1])	return ERRPTR;
	
	if(Args[0]->Type != SS_DATATYPE_INTEGER || Args[1]->Type != SS_DATATYPE_INTEGER)
		return ERRPTR;

	 int	type = Args[1]->Integer;
	 int	size = Args[0]->Integer;

	if( type != SS_DATATYPE_ARRAY )
	{
		if( !SS_GETARRAYDEPTH(type) ) {
			// ERROR - This should never happen
			return ERRPTR;
		}
		type = SS_DOWNARRAY(type);
	}

	return SpiderScript_CreateArray(type, size);
}

tSpiderValue *Exports_Lang_Strings_Split(tSpiderScript *Script, int NArgs, tSpiderValue **Args)
{
	 int	len, ofs, slen;
	void	*haystack, *end;
	 int	nSubStrs = 0;
	tSpiderValue	**strings = NULL;
	tSpiderValue	*ret;

	// Error checking
	if( NArgs != 2 )
		return ERRPTR;
	if( !Args[0] || !Args[1] )
		return ERRPTR;
	if( Args[0]->Type != SS_DATATYPE_STRING )
		return ERRPTR;
	if( Args[1]->Type != SS_DATATYPE_STRING )
		return ERRPTR;

	// Split the string
	len = Args[0]->String.Length;
	haystack = Args[0]->String.Data;
	ofs = 0;
	do {
		end = memmem(haystack + ofs, len - ofs, Args[1]->String.Data, Args[1]->String.Length);
		if( end )
			slen = end - (haystack + ofs);
		else
			slen = len - ofs;
		
		strings = realloc(strings, (nSubStrs+1)*sizeof(tSpiderValue*));
		strings[nSubStrs] = SpiderScript_CreateString(slen, haystack + ofs);
		nSubStrs ++;

		ofs += slen + Args[1]->String.Length;
	} while(end);

	// Create output array
	ret = SpiderScript_CreateArray(SS_DATATYPE_STRING, nSubStrs);
	memcpy(ret->Array.Items, strings, nSubStrs*sizeof(tSpiderValue*));
	free(strings);

	return ret;
}

tSpiderValue *Exports_Lang_Struct(tSpiderScript *Script, int NArgs, tSpiderValue **Args)
{
	 int	i;
	printf("Exports_Lang_Struct: (Script=%p, NArgs=%i, Args=%p)\n", Script, NArgs, Args);
	
	for( i = 0; i < NArgs; i ++ )
	{
		printf(" Args[%i] = {Type: %i, ", i, Args[i]->Type);
		switch(Args[i]->Type)
		{
		case SS_DATATYPE_INTEGER:
			printf(" Integer: 0x%lx", Args[i]->Integer);
			break;
		case SS_DATATYPE_REAL:
			printf(" Real: %f", Args[i]->Real);
			break;
		case SS_DATATYPE_STRING:
			printf(" Length: %i, Data = '%s'", Args[i]->String.Length, Args[i]->String.Data);
			break;
		default:
			break;
		}
		printf("}\n");
	}
	
	return NULL;
}

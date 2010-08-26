/*
 * Acess2 - SpiderScript
 * - Script Exports (Lang. Namespace)
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <spiderscript.h>

// === PROTOTYPES ===
tSpiderValue	*Exports_Lang_Struct(tSpiderScript *Script, int NArgs, tSpiderValue **Args);

// === GLOBALS ===
tSpiderFunction	gExports_Lang_Struct = {NULL,"Lang.Struct", Exports_Lang_Struct, {SS_DATATYPE_STRING,-1}};
tSpiderFunction	*gpExports_First = &gExports_Lang_Struct;

// === CODE ===
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

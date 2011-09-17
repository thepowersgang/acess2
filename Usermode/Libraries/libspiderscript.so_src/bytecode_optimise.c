/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * bytecode_gen.c
 * - Generate bytecode
 */
#include <stdlib.h>
#include "bytecode_ops.h"

// Patterns:
// TODO: Figure out what optimisations can be done

int Bytecode_OptimizeFunction(tBC_Function *Function)
{
	for( op = Function->Operations; op; op = op->Next, idx ++ )
	{
	}
}

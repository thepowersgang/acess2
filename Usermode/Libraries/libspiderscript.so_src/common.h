/*
 * SpiderScript
 * - By John Hodge (thePowersGang)
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <spiderscript.h>

typedef struct sScript_Function	tScript_Function;
typedef struct sScript_Arg	tScript_Arg;

struct sSpiderScript
{
	tSpiderVariant	*Variant;
	tScript_Function	*Functions;
	tScript_Function	*LastFunction;
	char	*CurNamespace;	//!< Current namespace prefix (NULL = Root) - No trailing .
};

struct sScript_Arg
{
	 int	Type;
	char	*Name;
};

struct sScript_Function
{
	tScript_Function	*Next;
	// char	*Namespace;
	char	*Name;

	 int	ReturnType;
	
	struct sAST_Node	*ASTFcn;
	struct sBC_Function	*BCFcn;

	 int	ArgumentCount;
	tScript_Arg	Arguments[];
};

#endif


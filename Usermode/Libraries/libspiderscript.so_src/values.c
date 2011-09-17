/*
 * SpiderScript Library
 * by John Hodge (thePowersGang)
 * 
 * values.c
 * - Manage tSpiderValue objects
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "spiderscript.h"

// === IMPORTS ===
extern void	AST_RuntimeError(void *Node, const char *Format, ...);

// === PROTOTYPES ===
void	SpiderScript_DereferenceValue(tSpiderValue *Object);
void	SpiderScript_ReferenceValue(tSpiderValue *Object);
tSpiderValue	*SpiderScript_CreateInteger(uint64_t Value);
tSpiderValue	*SpiderScript_CreateReal(double Value);
tSpiderValue	*SpiderScript_CreateString(int Length, const char *Data);
tSpiderValue	*SpiderScript_CastValueTo(int Type, tSpiderValue *Source);
 int	SpiderScript_IsValueTrue(tSpiderValue *Value);
void	SpiderScript_FreeValue(tSpiderValue *Value);
char	*SpiderScript_DumpValue(tSpiderValue *Value);

// === CODE ===
/**
 * \brief Dereference a created object
 */
void SpiderScript_DereferenceValue(tSpiderValue *Object)
{
	if(!Object || Object == ERRPTR)	return ;
	Object->ReferenceCount --;
	if( Object->ReferenceCount == 0 )
	{
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

/**
 * \brief Reference a value
 */
void SpiderScript_ReferenceValue(tSpiderValue *Object)
{
	if(!Object || Object == ERRPTR)	return ;
	Object->ReferenceCount ++;
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
tSpiderValue *SpiderScript_StringConcat(const tSpiderValue *Str1, const tSpiderValue *Str2)
{
	 int	newLen = 0;
	tSpiderValue	*ret;
	
	if( Str1 && Str1->Type != SS_DATATYPE_STRING)
		return NULL;
	if( Str2 && Str2->Type != SS_DATATYPE_STRING)
		return NULL;
	
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
		SpiderScript_DereferenceValue(Source);
		return Source;
	}
	
	// Debug
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
	
	// Object casts
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
		AST_RuntimeError(NULL, "Invalid cast to %i", Type);
		return ERRPTR;
	case SS_DATATYPE_OBJECT:
		// TODO: 
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
	SpiderScript_DereferenceValue(Value);
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



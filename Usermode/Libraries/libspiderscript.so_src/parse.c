/*
 * Acess2 - SpiderScript
 * - Parser
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <spiderscript.h>
#define WANT_TOKEN_STRINGS	1
#include "tokens.h"
#include "ast.h"

#define DEBUG	0

// === PROTOTYPES ===
tAST_Script	*Parse_Buffer(tSpiderVariant *Variant, char *Buffer);
tAST_Node	*Parse_DoCodeBlock(tParser *Parser);
tAST_Node	*Parse_DoBlockLine(tParser *Parser);
tAST_Node	*Parse_GetVarDef(tParser *Parser, int Type);

tAST_Node	*Parse_DoExpr0(tParser *Parser);	// Assignment
tAST_Node	*Parse_DoExpr1(tParser *Parser);	// Boolean Operators
tAST_Node	*Parse_DoExpr2(tParser *Parser);	// Comparison Operators
tAST_Node	*Parse_DoExpr3(tParser *Parser);	// Bitwise Operators
tAST_Node	*Parse_DoExpr4(tParser *Parser);	// Bit Shifts
tAST_Node	*Parse_DoExpr5(tParser *Parser);	// Arithmatic
tAST_Node	*Parse_DoExpr6(tParser *Parser);	// Mult & Div

tAST_Node	*Parse_DoParen(tParser *Parser);	// Parenthesis (Always Last)
tAST_Node	*Parse_DoValue(tParser *Parser);	// Values

tAST_Node	*Parse_GetString(tParser *Parser);
tAST_Node	*Parse_GetNumeric(tParser *Parser);
tAST_Node	*Parse_GetVariable(tParser *Parser);
tAST_Node	*Parse_GetIdent(tParser *Parser);

void	SyntaxAssert(tParser *Parser, int Have, int Want);

#define TODO(Parser, message...) do {\
	fprintf(stderr, "TODO: "message);\
	longjmp(Parser->JmpTarget, -1);\
}while(0)

// === CODE ===
/**
 * \brief Parse a buffer into a syntax tree
 */
tAST_Script	*Parse_Buffer(tSpiderVariant *Variant, char *Buffer)
{
	tParser	parser = {0};
	tParser *Parser = &parser;	//< Keeps code consistent
	tAST_Script	*ret;
	tAST_Node	*mainCode;
	char	*name;
	tAST_Function	*fcn;
	 int	type;
	
	#if DEBUG >= 2
	printf("Parse_Buffer: (Variant=%p, Buffer=%p)\n", Variant, Buffer);
	#endif
	
	// Initialise parser
	parser.LastToken = -1;
	parser.NextToken = -1;
	parser.CurLine = 1;
	parser.BufStart = Buffer;
	parser.CurPos = Buffer;
	
	ret = AST_NewScript();
	mainCode = AST_NewCodeBlock();
	
	// Give us an error fallback
	if( setjmp( parser.JmpTarget ) != 0 )
	{
		AST_FreeNode( mainCode );
		return NULL;
	}
	
	// Parse the file!
	while(Parser->Token != TOK_EOF)
	{
		switch( GetToken(Parser) )
		{
		case TOK_EOF:
			break;
		
		// Typed variables/functions
		case TOKEN_GROUP_TYPES:
			{
			 int	tok, type;
			TOKEN_GET_DATATYPE(type, Parser->Token);
			
			tok = GetToken(Parser);
			// Define a function (pass on to the other function definition code)
			if( tok == TOK_IDENT ) {
				goto defFcn;
			}
			// Define a variable
			else if( tok == TOK_VARIABLE ) {
				AST_AppendNode( mainCode, Parse_GetVarDef(Parser, type) );
				SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);
			}
			else {
				fprintf(stderr, "ERROR: Unexpected %s, expected TOK_IDENT or TOK_VARIABLE\n",
					csaTOKEN_NAMES[tok]);
			}
			}
			break;
		
		// Define a function
		case TOK_RWD_FUNCTION:
			if( !Variant->bDyamicTyped ) {
				fprintf(stderr, "ERROR: Dynamic functions are invalid in static mode\n");
				longjmp(Parser->JmpTarget, -1);
			}
			type = SS_DATATYPE_DYNAMIC;
			SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT );
		defFcn:
			name = strndup( Parser->TokenStr, Parser->TokenLen );
			fcn = AST_AppendFunction( ret, name );
			#if DEBUG
			printf("DefFCN %s\n", name);
			#endif
			free(name);
			
			// Get arguments
			SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN );
			if( LookAhead(Parser) != TOK_PAREN_CLOSE )
			{
				do {
					type = SS_DATATYPE_DYNAMIC;
					GetToken(Parser);
					// Non dynamic typed variants must use data types
					if( !Variant->bDyamicTyped ) {
						TOKEN_GET_DATATYPE(type, Parser->Token);
						GetToken(Parser);
					}
					AST_AppendFunctionArg(fcn, Parse_GetVarDef(Parser, type)); 
				}	while(GetToken(Parser) == TOK_COMMA);
			}
			else
				GetToken(Parser);
			SyntaxAssert(Parser, Parser->Token, TOK_PAREN_CLOSE );
			
			AST_SetFunctionCode( fcn, Parse_DoCodeBlock(Parser) );
			break;
		
		default:
			PutBack(Parser);
			AST_AppendNode( mainCode, Parse_DoBlockLine(Parser) );
			break;
		}
	}
	
	fcn = AST_AppendFunction( ret, "" );
	AST_SetFunctionCode( fcn, mainCode );
	
	//printf("---- %p parsed as SpiderScript ----\n", Buffer);
	
	return ret;
}

/**
 * \brief Parse a block of code surrounded by { }
 */
tAST_Node *Parse_DoCodeBlock(tParser *Parser)
{
	tAST_Node	*ret;
	
	// Check if we are being called for a one-liner
	if( GetToken(Parser) != TOK_BRACE_OPEN ) {
		PutBack(Parser);
		return Parse_DoBlockLine(Parser);
	}
	
	ret = AST_NewCodeBlock();
	
	while( LookAhead(Parser) != TOK_BRACE_CLOSE )
	{
		AST_AppendNode( ret, Parse_DoBlockLine(Parser) );
	}
	GetToken(Parser);	// Omnomnom
	return ret;
}

/**
 * \brief Parse a line in a block
 */
tAST_Node *Parse_DoBlockLine(tParser *Parser)
{
	tAST_Node	*ret;
	
	//printf("Parse_DoBlockLine: Line %i\n", Parser->CurLine);
	
	switch(LookAhead(Parser))
	{
	// Empty statement
	case TOK_SEMICOLON:
		GetToken(Parser);
		return NULL;
	
	// Return from a method
	case TOK_RWD_RETURN:
		GetToken(Parser);
		ret = AST_NewUniOp(Parser, NODETYPE_RETURN, Parse_DoExpr0(Parser));
		break;
	
	// Control Statements
	case TOK_RWD_IF:
		{
		tAST_Node	*cond, *true, *false = NULL;
		GetToken(Parser);	// eat the if
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);
		cond = Parse_DoExpr0(Parser);	// Get condition
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
		true = Parse_DoCodeBlock(Parser);
		if( LookAhead(Parser) == TOK_RWD_ELSE ) {
			GetToken(Parser);
			false = Parse_DoCodeBlock(Parser);
		}
		ret = AST_NewIf(Parser, cond, true, false);
		}
		return ret;
	
	case TOK_RWD_FOR:
		{
		tAST_Node	*init=NULL, *cond=NULL, *inc=NULL, *code;
		GetToken(Parser);	// Eat 'for'
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_OPEN);
		
		if(LookAhead(Parser) != TOK_SEMICOLON)
			init = Parse_DoExpr0(Parser);
		
		SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);
		if(LookAhead(Parser) != TOK_SEMICOLON)
			cond = Parse_DoExpr0(Parser);
		
		SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON);
		if(LookAhead(Parser) != TOK_SEMICOLON)
			inc = Parse_DoExpr0(Parser);
		
		SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
		
		code = Parse_DoCodeBlock(Parser);
		ret = AST_NewLoop(Parser, init, 0, cond, inc, code);
		}
		return ret;
	
	case TOK_RWD_DO:
	case TOK_RWD_WHILE:
		TODO(Parser, "Implement do and while\n");
		break;
	
	// Define Variables
	case TOKEN_GROUP_TYPES:
		{
			 int	type;
			
			switch(GetToken(Parser))
			{
			case TOK_RWD_INTEGER:	type = SS_DATATYPE_INTEGER;	break;
			case TOK_RWD_OBJECT:	type = SS_DATATYPE_OBJECT;	break;
			case TOK_RWD_REAL:	type = SS_DATATYPE_REAL;	break;
			case TOK_RWD_STRING:	type = SS_DATATYPE_STRING;	break;
			}
			
			SyntaxAssert(Parser, GetToken(Parser), TOK_VARIABLE);
			
			ret = Parse_GetVarDef(Parser, type);
		}
		break;
	
	// Default
	default:
		//printf("exp0\n");
		ret = Parse_DoExpr0(Parser);
		break;
	}
	
	SyntaxAssert(Parser, GetToken(Parser), TOK_SEMICOLON );
	return ret;
}

/**
 * \brief Get a variable definition
 */
tAST_Node *Parse_GetVarDef(tParser *Parser, int Type)
{
	char	name[Parser->TokenLen];
	tAST_Node	*ret;
	
	SyntaxAssert(Parser, Parser->Token, TOK_VARIABLE);
	
	// copy the name (trimming the $)
	memcpy(name, Parser->TokenStr+1, Parser->TokenLen-1);
	name[Parser->TokenLen-1] = 0;
	// Define the variable
	ret = AST_NewDefineVar(Parser, Type, name);
	// Handle arrays
	while( LookAhead(Parser) == TOK_SQUARE_OPEN )
	{
		GetToken(Parser);
		AST_AppendNode(ret, Parse_DoExpr0(Parser));
		SyntaxAssert(Parser, GetToken(Parser), TOK_SQUARE_CLOSE);
	}
	return ret;
}

/**
 * \brief Assignment Operations
 */
tAST_Node *Parse_DoExpr0(tParser *Parser)
{
	tAST_Node	*ret = Parse_DoExpr1(Parser);

	// Check Assignment
	switch(LookAhead(Parser))
	{
	case TOK_ASSIGN:
		GetToken(Parser);		// Eat Token
		ret = AST_NewAssign(Parser, NODETYPE_NOP, ret, Parse_DoExpr0(Parser));
		break;
	#if 0
	case TOK_DIV_EQU:
		GetToken(Parser);		// Eat Token
		ret = AST_NewAssign(Parser, NODETYPE_DIVIDE, ret, Parse_DoExpr0(Parser));
		break;
	case TOK_MULT_EQU:
		GetToken(Parser);		// Eat Token
		ret = AST_NewAssign(Parser, NODETYPE_MULTIPLY, ret, Parse_DoExpr0(Parser));
		break;
	#endif
	default:
		#if DEBUG >= 2
		printf("Parse_DoExpr0: Parser->Token = %i\n", Parser->Token);
		#endif
		break;
	}
	return ret;
}

/**
 * \brief Logical/Boolean Operators
 */
tAST_Node *Parse_DoExpr1(tParser *Parser)
{
	tAST_Node	*ret = Parse_DoExpr2(Parser);
	
	switch(GetToken(Parser))
	{
	case TOK_LOGICAND:
		ret = AST_NewBinOp(Parser, NODETYPE_LOGICALAND, ret, Parse_DoExpr1(Parser));
		break;
	case TOK_LOGICOR:
		ret = AST_NewBinOp(Parser, NODETYPE_LOGICALOR, ret, Parse_DoExpr1(Parser));
		break;
	case TOK_LOGICXOR:
		ret = AST_NewBinOp(Parser, NODETYPE_LOGICALXOR, ret, Parse_DoExpr1(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

// --------------------
// Expression 2 - Comparison Operators
// --------------------
tAST_Node *Parse_DoExpr2(tParser *Parser)
{
	tAST_Node	*ret = Parse_DoExpr3(Parser);

	// Check token
	switch(GetToken(Parser))
	{
	case TOK_EQUALS:
		ret = AST_NewBinOp(Parser, NODETYPE_EQUALS, ret, Parse_DoExpr2(Parser));
		break;
	case TOK_LT:
		ret = AST_NewBinOp(Parser, NODETYPE_LESSTHAN, ret, Parse_DoExpr2(Parser));
		break;
	case TOK_GT:
		ret = AST_NewBinOp(Parser, NODETYPE_GREATERTHAN, ret, Parse_DoExpr2(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

/**
 * \brief Bitwise Operations
 */
tAST_Node *Parse_DoExpr3(tParser *Parser)
{
	tAST_Node	*ret = Parse_DoExpr4(Parser);

	// Check Token
	switch(GetToken(Parser))
	{
	case TOK_OR:
		ret = AST_NewBinOp(Parser, NODETYPE_BWOR, ret, Parse_DoExpr3(Parser));
		break;
	case TOK_AND:
		ret = AST_NewBinOp(Parser, NODETYPE_BWAND, ret, Parse_DoExpr3(Parser));
		break;
	case TOK_XOR:
		ret = AST_NewBinOp(Parser, NODETYPE_BWXOR, ret, Parse_DoExpr3(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}
	return ret;
}

// --------------------
// Expression 4 - Shifts
// --------------------
tAST_Node *Parse_DoExpr4(tParser *Parser)
{
	tAST_Node *ret = Parse_DoExpr5(Parser);

	switch(GetToken(Parser))
	{
	case TOK_SHL:
		ret = AST_NewBinOp(Parser, NODETYPE_BITSHIFTLEFT, ret, Parse_DoExpr5(Parser));
		break;
	case TOK_SHR:
		ret = AST_NewBinOp(Parser, NODETYPE_BITSHIFTRIGHT, ret, Parse_DoExpr5(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}

	return ret;
}

// --------------------
// Expression 5 - Arithmatic
// --------------------
tAST_Node *Parse_DoExpr5(tParser *Parser)
{
	tAST_Node *ret = Parse_DoExpr6(Parser);

	switch(GetToken(Parser))
	{
	case TOK_PLUS:
		ret = AST_NewBinOp(Parser, NODETYPE_ADD, ret, Parse_DoExpr5(Parser));
		break;
	case TOK_MINUS:
		ret = AST_NewBinOp(Parser, NODETYPE_SUBTRACT, ret, Parse_DoExpr5(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}

	return ret;
}

// --------------------
// Expression 6 - Multiplcation & Division
// --------------------
tAST_Node *Parse_DoExpr6(tParser *Parser)
{
	tAST_Node *ret = Parse_DoParen(Parser);

	switch(GetToken(Parser))
	{
	case TOK_MUL:
		ret = AST_NewBinOp(Parser, NODETYPE_MULTIPLY, ret, Parse_DoExpr6(Parser));
		break;
	case TOK_DIV:
		ret = AST_NewBinOp(Parser, NODETYPE_DIVIDE, ret, Parse_DoExpr6(Parser));
		break;
	default:
		PutBack(Parser);
		break;
	}

	return ret;
}


// --------------------
// 2nd Last Expression - Parens
// --------------------
tAST_Node *Parse_DoParen(tParser *Parser)
{
	#if DEBUG >= 2
	printf("Parse_DoParen: (Parser=%p)\n", Parser);
	#endif
	if(LookAhead(Parser) == TOK_PAREN_OPEN)
	{
		tAST_Node	*ret;
		 int	type;
		GetToken(Parser);
		
		// TODO: Handle casts here
		switch(LookAhead(Parser))
		{
		case TOKEN_GROUP_TYPES:
			GetToken(Parser);
			TOKEN_GET_DATATYPE(type, Parser->Token);
			SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
			ret = AST_NewCast(Parser, type, Parse_DoParen(Parser));
			break;
		default:		
			ret = Parse_DoExpr0(Parser);
			SyntaxAssert(Parser, GetToken(Parser), TOK_PAREN_CLOSE);
			break;
		}
		return ret;
	}
	else
		return Parse_DoValue(Parser);
}

// --------------------
// Last Expression - Value
// --------------------
tAST_Node *Parse_DoValue(tParser *Parser)
{
	 int	tok = LookAhead(Parser);

	#if DEBUG >= 2
	printf("Parse_DoValue: tok = %i\n", tok);
	#endif

	switch(tok)
	{
	case TOK_STR:	return Parse_GetString(Parser);
	case TOK_INTEGER:	return Parse_GetNumeric(Parser);
	case TOK_IDENT:	return Parse_GetIdent(Parser);
	case TOK_VARIABLE:	return Parse_GetVariable(Parser);

	default:
		fprintf(stderr, "Syntax Error: Unexpected %s on line %i, Expected TOK_T_VALUE\n",
			csaTOKEN_NAMES[tok], Parser->CurLine);
		longjmp( Parser->JmpTarget, -1 );
	}
}

/**
 * \brief Get a string
 */
tAST_Node *Parse_GetString(tParser *Parser)
{
	tAST_Node	*ret;
	 int	i, j;
	GetToken( Parser );
	
	{
		char	data[ Parser->TokenLen - 2 ];
		j = 0;
		
		for( i = 1; i < Parser->TokenLen - 1; i++ )
		{
			if( Parser->TokenStr[i] == '\\' ) {
				i ++;
				switch( Parser->TokenStr[i] )
				{
				case 'n':	data[j++] = '\n';	break;
				case 'r':	data[j++] = '\r';	break;
				default:
					// TODO: Octal Codes
					// TODO: Error/Warning?
					break;
				}
			}
			else {
				data[j++] = Parser->TokenStr[i];
			}
		}
		
		// TODO: Parse Escape Codes
		ret = AST_NewString( Parser, data, j );
	}
	return ret;
}

/**
 * \brief Get a numeric value
 */
tAST_Node *Parse_GetNumeric(tParser *Parser)
{
	uint64_t	value = 0;
	char	*pos;
	GetToken( Parser );
	pos = Parser->TokenStr;
	//printf("pos = %p, *pos = %c\n", pos, *pos);
		
	if( *pos == '0' )
	{
		pos ++;
		if(*pos == 'x') {
			pos ++;
			for( ;; pos++)
			{
				value *= 16;
				if( '0' <= *pos && *pos <= '9' ) {
					value += *pos - '0';
					continue;
				}
				if( 'A' <= *pos && *pos <= 'F' ) {
					value += *pos - 'A' + 10;
					continue;
				}
				if( 'a' <= *pos && *pos <= 'f' ) {
					value += *pos - 'a' + 10;
					continue;
				}
				break;
			}
		}
		else {
			while( '0' <= *pos && *pos <= '7' ) {
				value = value*8 + *pos - '0';
				pos ++;
			}
		}
	}
	else {
		while( '0' <= *pos && *pos <= '9' ) {
			value = value*10 + *pos - '0';
			pos ++;
		}
	}
	
	return AST_NewInteger( Parser, value );
}

/**
 * \brief Get a variable
 */
tAST_Node *Parse_GetVariable(tParser *Parser)
{
	tAST_Node	*ret;
	SyntaxAssert( Parser, GetToken(Parser), TOK_VARIABLE );
	{
		char	name[Parser->TokenLen];
		memcpy(name, Parser->TokenStr+1, Parser->TokenLen-1);
		name[Parser->TokenLen-1] = 0;
		ret = AST_NewVariable( Parser, name );
		#if DEBUG >= 2
		printf("Parse_GetVariable: name = '%s'\n", name);
		#endif
	}
	// Handle array references
	while( LookAhead(Parser) == TOK_SQUARE_OPEN )
	{
		GetToken(Parser);
		ret = AST_NewBinOp(Parser, NODETYPE_INDEX, ret, Parse_DoExpr0(Parser));
		SyntaxAssert(Parser, GetToken(Parser), TOK_SQUARE_CLOSE);
	}
	return ret;
}

/**
 * \brief Get an identifier (constant or function call)
 */
tAST_Node *Parse_GetIdent(tParser *Parser)
{
	tAST_Node	*ret = NULL;
	char	*name;
	SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT );
	name = strndup( Parser->TokenStr, Parser->TokenLen );
	
	#if 0
	while( GetToken(Parser) == TOK_SCOPE )
	{
		ret = AST_NewScopeDereference( Parser, ret, name );
		SyntaxAssert(Parser, GetToken(Parser), TOK_IDENT );
		name = strndup( Parser->TokenStr, Parser->TokenLen );
	}
	PutBack(Parser);
	#endif
	
	if( GetToken(Parser) == TOK_PAREN_OPEN )
	{
		#if DEBUG >= 2
		printf("Parse_GetIdent: Calling '%s'\n", name);
		#endif
		// Function Call
		ret = AST_NewFunctionCall( Parser, name );
		// Read arguments
		if( GetToken(Parser) != TOK_PAREN_CLOSE )
		{
			PutBack(Parser);
			do {
				#if DEBUG >= 2
				printf(" Parse_GetIdent: Argument\n");
				#endif
				AST_AppendFunctionCallArg( ret, Parse_DoExpr0(Parser) );
			} while(GetToken(Parser) == TOK_COMMA);
			SyntaxAssert( Parser, Parser->Token, TOK_PAREN_CLOSE );
			#if DEBUG >= 2
			printf(" Parse_GetIdent: All arguments parsed\n");
			#endif
		}
	}
	else {
		// Runtime Constant / Variable (When implemented)
		#if DEBUG >= 2
		printf("Parse_GetIdent: Referencing '%s'\n", name);
		#endif
		PutBack(Parser);
		ret = AST_NewConstant( Parser, name );
	}
	
	free(name);
	return ret;
}

/**
 * \brief Check for an error
 */
void SyntaxAssert(tParser *Parser, int Have, int Want)
{
	if(Have != Want) {
		fprintf(stderr, "ERROR: SyntaxAssert Failed, Expected %s(%i), got %s(%i) on line %i\n",
			csaTOKEN_NAMES[Want], Want, csaTOKEN_NAMES[Have], Have, Parser->CurLine);
		longjmp(Parser->JmpTarget, -1);
	}
}


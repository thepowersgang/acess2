/*
 * Acess2 - SpiderScript
 * - Parser
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <spiderscript.h>
#include "tokens.h"
#include "ast.h"

// === PROTOTYPES ===
tAST_Script	*Parse_Buffer(tSpiderVariant *Variant, char *Buffer);
tAST_Node	*Parse_DoCodeBlock(tParser *Parser);
tAST_Node	*Parse_DoExpr(tParser *Parser);

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

void	SyntaxAssert(int Have, int Want);

// === CODE ===
/**
 * \brief Parse a buffer into a syntax tree
 */
tAST_Script	*Parse_Buffer(tSpiderVariant *Variant, char *Buffer)
{
	tParser	parser = {0};
	tParser *Parser = &parser;	//< Keeps code consitent
	tAST_Script	*ret;
	tAST_Node	*mainCode;
	char	*name;
	tAST_Function	*fcn;
	
	// Initialise parser
	parser.BufStart = Buffer;
	parser.CurPos = Buffer;
	
	ret = AST_NewScript();
	mainCode = AST_NewCodeBlock();
	
	for(;;)
	{
		switch( GetToken(Parser) )
		{
		case TOK_RWD_FUNCTION:			
			// Define a function
			SyntaxAssert( GetToken(Parser), TOK_IDENT );
			name = strndup( Parser->TokenStr, Parser->TokenLen );
			fcn = AST_AppendFunction( ret, name );
			free(name);
			
			SyntaxAssert( GetToken(Parser), TOK_PAREN_OPEN );
			// TODO: Arguments
			SyntaxAssert( GetToken(Parser), TOK_PAREN_CLOSE );
			
			AST_SetFunctionCode( fcn, Parse_DoCodeBlock(Parser) );
			break;
		
		default:
			PutBack(&parser);
			AST_AppendNode( mainCode, Parse_DoExpr(Parser) );
			break;
		}
	}
	
	fcn = AST_AppendFunction( ret, "" );
	AST_SetFunctionCode( fcn, mainCode );
	
	return ret;
}

/**
 * \brief Parse a block of code surrounded by { }
 */
tAST_Node *Parse_DoCodeBlock(tParser *Parser)
{
	tAST_Node	*ret;
	SyntaxAssert( GetToken(Parser), TOK_BRACE_OPEN );
	
	ret = AST_NewCodeBlock();
	
	while( GetToken(Parser) != TOK_BRACE_CLOSE )
	{
		AST_AppendNode( ret, Parse_DoExpr(Parser) );
		SyntaxAssert( GetToken(Parser), TOK_SEMICOLON );
	}
	return ret;
}

/**
 * \brief Parse an expression
 */
tAST_Node *Parse_DoExpr(tParser *Parser)
{
	return Parse_DoExpr0(Parser);
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
		ret = AST_NewAssign(NODETYPE_NOP, ret, Parse_DoExpr0(Parser));
		break;
	#if 0
	case TOK_DIV_EQU:
		GetToken(Parser);		// Eat Token
		ret = AST_NewAssignOp(ret, NODETYPE_DIVIDE, DoExpr0(Parser));
		break;
	case TOK_MULT_EQU:
		GetToken(Parser);		// Eat Token
		ret = AST_NewAssignOp(ret, NODETYPE_MULTIPLY, DoExpr0(Parser));
		break;
	#endif
	default:	break;
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
		ret = AST_NewBinOp(NODETYPE_LOGICALAND, ret, Parse_DoExpr1(Parser));
		break;
	case TOK_LOGICOR:
		ret = AST_NewBinOp(NODETYPE_LOGICALOR, ret, Parse_DoExpr1(Parser));
		break;
	case TOK_LOGICXOR:
		ret = AST_NewBinOp(NODETYPE_LOGICALXOR, ret, Parse_DoExpr1(Parser));
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
		ret = AST_NewBinOp(NODETYPE_EQUALS, ret, Parse_DoExpr2(Parser));
		break;
	case TOK_LT:
		ret = AST_NewBinOp(NODETYPE_LESSTHAN, ret, Parse_DoExpr2(Parser));
		break;
	case TOK_GT:
		ret = AST_NewBinOp(NODETYPE_GREATERTHAN, ret, Parse_DoExpr2(Parser));
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
		ret = AST_NewBinOp(NODETYPE_BWOR, ret, Parse_DoExpr3(Parser));
		break;
	case TOK_AND:
		ret = AST_NewBinOp(NODETYPE_BWAND, ret, Parse_DoExpr3(Parser));
		break;
	case TOK_XOR:
		ret = AST_NewBinOp(NODETYPE_BWXOR, ret, Parse_DoExpr3(Parser));
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
		ret = AST_NewBinOp(NODETYPE_BITSHIFTLEFT, ret, Parse_DoExpr5(Parser));
		break;
	case TOK_SHR:
		ret = AST_NewBinOp(NODETYPE_BITSHIFTRIGHT, ret, Parse_DoExpr5(Parser));
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
		ret = AST_NewBinOp(NODETYPE_ADD, ret, Parse_DoExpr5(Parser));
		break;
	case TOK_MINUS:
		ret = AST_NewBinOp(NODETYPE_SUBTRACT, ret, Parse_DoExpr5(Parser));
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
		ret = AST_NewBinOp(NODETYPE_MULTIPLY, ret, Parse_DoExpr6(Parser));
		break;
	case TOK_DIV:
		ret = AST_NewBinOp(NODETYPE_DIVIDE, ret, Parse_DoExpr6(Parser));
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
	if(LookAhead(Parser) == TOK_PAREN_OPEN)
	{
		tAST_Node	*ret;
		GetToken(Parser);
		ret = Parse_DoExpr0(Parser);
		SyntaxAssert(GetToken(Parser), TOK_PAREN_CLOSE);
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

	switch(tok)
	{
	case TOK_STR:	return Parse_GetString(Parser);
	case TOK_INTEGER:	return Parse_GetNumeric(Parser);
	case TOK_IDENT:	return Parse_GetIdent(Parser);
	case TOK_VARIABLE:	return Parse_GetVariable(Parser);

	default:
		//ParseError2( tok, TOK_T_VALUE );
		return NULL;
	}
}

/**
 * \brief Get a string
 */
tAST_Node *Parse_GetString(tParser *Parser)
{
	tAST_Node	*ret;
	GetToken( Parser );
	// TODO: Parse Escape Codes
	ret = AST_NewString( Parser->TokenStr+1, Parser->TokenLen-2 );
	return ret;
}

/**
 * \brief Get a numeric value
 */
tAST_Node *Parse_GetNumeric(tParser *Parser)
{
	uint64_t	value;
	GetToken( Parser );
	value = atoi( Parser->TokenStr );
	return AST_NewInteger( value );
}

/**
 * \brief Get a variable
 */
tAST_Node *Parse_GetVariable(tParser *Parser)
{
	char	*name;
	tAST_Node	*ret;
	SyntaxAssert( GetToken(Parser), TOK_VARIABLE );
	name = strndup( Parser->TokenStr+1, Parser->TokenLen-1 );
	ret = AST_NewVariable( name );
	free(name);
	return ret;
}

/**
 * \brief Get an identifier (constand or function call)
 */
tAST_Node *Parse_GetIdent(tParser *Parser)
{
	tAST_Node	*ret;
	char	*name;
	SyntaxAssert( GetToken(Parser), TOK_IDENT );
	name = strndup( Parser->TokenStr, Parser->TokenLen );
	
	if( GetToken(Parser) == TOK_PAREN_OPEN )
	{
		// Function Call
		ret = AST_NewFunctionCall( name );
		// Read arguments
		if( GetToken(Parser) != TOK_PAREN_CLOSE )
		{
			PutBack(Parser);
			do {
				AST_AppendFunctionCallArg( ret, Parse_DoExpr0(Parser) );
			} while(GetToken(Parser) == TOK_COMMA);
			SyntaxAssert( Parser->Token, TOK_PAREN_CLOSE );
		}
	}
	else {
		// Runtime Constant
		PutBack(Parser);
		ret = AST_NewConstant( name );
	}
	
	free(name);
	return ret;
}

/**
 * \brief Check for an error
 */
void SyntaxAssert(int Have, int Want)
{
	if(Have != Want) {
		fprintf(stderr, "ERROR: Expected %i, got %i\n", Want, Have);
		//longjmp(jmpTarget, 1);
		return;
	}
}


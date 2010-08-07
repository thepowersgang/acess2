/*
 * Acess2 init
 * - Script Lexer
 */
#include "tokens.h"
#include <stdlib.h>

// === PROTOTYPES ===
 int	is_ident(char ch);
 int	isdigit(char ch);
 int	isspace(char ch);
 int	GetToken(tParser *File);

// === CONSTANTS ===
const struct {
	const  int	Value;
	const char	*Name;
} csaReservedWords[] = {
	{TOK_RWD_FUNCTION, "function"},
	{TOK_RWD_INTEGER, "integer"},
	{TOK_RWD_REAL, "string"}
};

// === CODE ===
/**
 * \brief Read a token from a buffer
 * \param File	Parser state
 */
int GetToken(tParser *File)
{
	 int	ret;
	
	if( File->NextToken != -1 ) {
		File->Token = File->NextToken;
		File->TokenStr = File->NextTokenStr;
		File->TokenLen = File->NextTokenLen;
		File->NextToken = -1;
		return File->Token;
	}
	
	// Clear whitespace (including comments)
	for( ;; )
	{
		// Whitespace
		while( isspace( *File->CurPos ) )
			File->CurPos ++;
		
		// # Line Comments
		if( *File->CurPos == '#' ) {
			while( *File->CurPos && *File->CurPos != '\n' )
				File->CurPos ++;
			continue ;
		}
		
		// C-Style Line Comments
		if( *File->CurPos == '/' && File->CurPos[1] == '/' ) {
			while( *File->CurPos && *File->CurPos != '\n' )
				File->CurPos ++;
			continue ;
		}
		
		// C-Style Block Comments
		if( *File->CurPos == '/' && File->CurPos[1] == '*' ) {
			File->CurPos += 2;
			while( *File->CurPos && !(File->CurPos[-1] == '*' && *File->CurPos == '/') )
				File->CurPos ++;
			continue ;
		}
		
		// No more "whitespace"
		break;
	}
	
	// Save previous tokens (speeds up PutBack and LookAhead)
	File->LastToken = File->Token;
	File->LastTokenStr = File->TokenStr;
	File->LastTokenLen = File->TokenLen;
	
	// Read token
	File->TokenStr = File->CurPos;
	switch( *File->CurPos++ )
	{
	// Operations
	case '/':	ret = TOK_DIV;	break;
	case '*':	ret = TOK_MUL;	break;
	case '+':	ret = TOK_PLUS;	break;
	case '-':
		if( *File->CurPos == '>' ) {
			File->CurPos ++;
			ret = TOK_ELEMENT;
		}
		else
			ret = TOK_MINUS;
		break;
	
	// Strings
	case '"':
		File->TokenStr ++;
		while( *File->CurPos && !(*File->CurPos == '"' && *File->CurPos != '\\') )
			File->CurPos ++;
		ret = TOK_STR;
		break;
	
	// Brackets
	case '(':	ret = TOK_PAREN_OPEN;	break;
	case ')':	ret = TOK_PAREN_CLOSE;	break;
	case '{':	ret = TOK_BRACE_OPEN;	break;
	case '}':	ret = TOK_BRACE_CLOSE;	break;
	case '[':	ret = TOK_SQUARE_OPEN;	break;
	case ']':	ret = TOK_SQUARE_CLOSE;	break;
	
	// Core symbols
	case ';':	ret = TOK_SEMICOLON;	break;
	case '.':	ret = TOK_SCOPE;	break;
	
	// Equals
	case '=':
		// Comparison Equals
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_EQUALS;
			break;
		}
		// Assignment Equals
		ret = TOK_ASSIGN;
		break;
	
	// Variables
	// \$[0-9]+ or \$[_a-zA-Z][_a-zA-Z0-9]*
	case '$':
		File->TokenStr ++;
		// Numeric Variable
		if( isdigit( *File->CurPos ) ) {
			while( isdigit(*File->CurPos) )
				File->CurPos ++;
		}
		// Ident Variable
		else {
			while( is_ident(*File->CurPos) )
				File->CurPos ++;
		}
		ret = TOK_VARIABLE;
		break;
	
	// Default (Numbers and Identifiers)
	default:
		// Numbers
		if( isdigit(*File->CurPos) )
		{
			while( isdigit(*File->CurPos) )
				File->CurPos ++;
			ret = TOK_INTEGER;
			break;
		}
	
		// Identifier
		if( is_ident(*File->CurPos) )
		{
			// Identifier
			while( is_ident(*File->CurPos) || isdigit(*File->CurPos) )
				File->CurPos ++;
			
			ret = TOK_IDENT;
			break;
		}
		// Syntax Error
		ret = 0;
		break;
	}
	// Return
	File->Token = ret;
	File->TokenLen = File->CurPos - File->TokenStr;
	return ret;
}

void PutBack(tParser *File)
{
	if( File->LastToken == -1 ) {
		// ERROR:
		return ;
	}
	// Save
	File->NextToken = File->Token;
	File->NextTokenStr = File->TokenStr;
	File->NextTokenLen = File->TokenLen;
	// Restore
	File->Token = File->LastToken;
	File->TokenStr = File->LastTokenStr;
	File->TokenLen = File->LastTokenLen;
	File->CurPos = File->NextTokenStr;
	// Invalidate
	File->LastToken = -1;
}

int LookAhead(tParser *File)
{
	 int	ret = GetToken(File);
	PutBack(File);
	return ret;
}

// --- Helpers ---
/**
 * \brief Check for ident characters
 * \note Matches Regex [a-zA-Z_]
 */
int is_ident(char ch)
{
	if('a' <= ch && ch <= 'z')	return 1;
	if('Z' <= ch && ch <= 'Z')	return 1;
	if(ch == '_')	return 1;
	if(ch < 0)	return 1;
	return 0;
}

int isdigit(char ch)
{
	if('0' <= ch && ch <= '9')	return 1;
	return 0;
}

int isspace(char ch)
{
	if(' ' == ch)	return 1;
	if('\t' == ch)	return 1;
	if('\b' == ch)	return 1;
	if('\n' == ch)	return 1;
	if('\r' == ch)	return 1;
	return 0;
}

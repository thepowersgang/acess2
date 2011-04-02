/*
 * SpiderScript
 * - Script Lexer
 */
#include "tokens.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Make the scope character ('.') be a symbol, otherwise it's just
// a ident character
#define USE_SCOPE_CHAR	0

#define DEBUG	0

#define ARRAY_SIZE(x)	((sizeof(x))/(sizeof((x)[0])))

// === PROTOTYPES ===
 int	is_ident(char ch);
 int	isdigit(int ch);
 int	isspace(int ch);
 int	GetToken(tParser *File);

// === CONSTANTS ===
const struct {
	const  int	Value;
	const char	*Name;
} csaReservedWords[] = {
	{TOK_RWD_FUNCTION, "function"},
	
	{TOK_RWD_RETURN, "return"},
	{TOK_RWD_NEW, "new"},
	
	{TOK_RWD_IF, "if"},
	{TOK_RWD_ELSE, "else"},
	{TOK_RWD_DO, "do"},
	{TOK_RWD_WHILE, "while"},
	{TOK_RWD_FOR, "for"},
	
	{TOK_RWD_VOID, "void"},
	{TOK_RWD_OBJECT, "Object"},
	{TOK_RWD_OPAQUE, "Opaque"},
	{TOK_RWD_INTEGER, "Integer"},
	{TOK_RWD_REAL, "Real"},
	{TOK_RWD_STRING, "String"}
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
		// Save Last
		File->LastToken = File->Token;
		File->LastTokenStr = File->TokenStr;
		File->LastTokenLen = File->TokenLen;
		File->LastLine = File->CurLine;
		// Restore Next
		File->Token = File->NextToken;
		File->TokenStr = File->NextTokenStr;
		File->TokenLen = File->NextTokenLen;
		File->CurLine = File->NextLine;
		// Set State
		File->CurPos = File->TokenStr + File->TokenLen;
		File->NextToken = -1;
		{
			char	buf[ File->TokenLen + 1];
			memcpy(buf, File->TokenStr, File->TokenLen);
			buf[File->TokenLen] = 0;
			#if DEBUG
			printf(" GetToken: FAST Return %i (%i long) (%s)\n", File->Token, File->TokenLen, buf);
			#endif
		}
		return File->Token;
	}
	
	//printf("  GetToken: File=%p, File->CurPos = %p\n", File, File->CurPos);
	
	// Clear whitespace (including comments)
	for( ;; )
	{
		// Whitespace
		while( isspace( *File->CurPos ) )
		{
			//printf("whitespace 0x%x, line = %i\n", *File->CurPos, File->CurLine);
			if( *File->CurPos == '\n' )
				File->CurLine ++;
			File->CurPos ++;
		}
		
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
			File->CurPos += 2;	// Eat the '/*'
			while( *File->CurPos && !(File->CurPos[-1] == '*' && *File->CurPos == '/') )
			{
				if( *File->CurPos == '\n' )	File->CurLine ++;
				File->CurPos ++;
			}
			File->CurPos ++;	// Eat the '/'
			continue ;
		}
		
		// No more "whitespace"
		break;
	}
	
	// Save previous tokens (speeds up PutBack and LookAhead)
	File->LastToken = File->Token;
	File->LastTokenStr = File->TokenStr;
	File->LastTokenLen = File->TokenLen;
	File->LastLine = File->CurLine;
	
	// Read token
	File->TokenStr = File->CurPos;
	switch( *File->CurPos++ )
	{
	case '\0':	ret = TOK_EOF;	break;
	
	// Operations
	case '^':
		if( *File->CurPos == '^' ) {
			File->CurPos ++;
			ret = TOK_LOGICXOR;
			break;
		}
		ret = TOK_XOR;
		break;
	
	case '|':
		if( *File->CurPos == '|' ) {
			File->CurPos ++;
			ret = TOK_LOGICOR;
			break;
		}
		ret = TOK_OR;
		break;
	
	case '&':
		if( *File->CurPos == '&' ) {
			File->CurPos ++;
			ret = TOK_LOGICAND;
			break;
		}
		ret = TOK_AND;
		break;
	
	case '/':
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_ASSIGN_DIV;
			break;
		}
		ret = TOK_DIV;
		break;
	case '*':
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_ASSIGN_MUL;
			break;
		}
		ret = TOK_MUL;
		break;
	case '+':
		if( *File->CurPos == '+' ) {
			File->CurPos ++;
			ret = TOK_INCREMENT;
			break;
		}
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_ASSIGN_PLUS;
			break;
		}
		ret = TOK_PLUS;
		break;
	case '-':
		if( *File->CurPos == '-' ) {
			File->CurPos ++;
			ret = TOK_DECREMENT;
			break;
		}
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_ASSIGN_MINUS;
			break;
		}
		if( *File->CurPos == '>' ) {
			File->CurPos ++;
			ret = TOK_ELEMENT;
			break;
		}
		ret = TOK_MINUS;
		break;
	
	// Strings
	case '"':
		while( *File->CurPos && !(*File->CurPos == '"' && *File->CurPos != '\\') )
			File->CurPos ++;
		if( *File->CurPos )
		{
			File->CurPos ++;
			ret = TOK_STR;
		}
		else
			ret = TOK_EOF;
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
	case ',':	ret = TOK_COMMA;	break;
	#if USE_SCOPE_CHAR
	case '.':	ret = TOK_SCOPE;	break;
	#endif
	
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
	
	// Less-Than
	case '<':
		// Less-Than or Equal
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_LTE;
			break;
		}
		ret = TOK_LT;
		break;
	
	// Greater-Than
	case '>':
		// Greater-Than or Equal
		if( *File->CurPos == '=' ) {
			File->CurPos ++;
			ret = TOK_GTE;
			break;
		}
		ret = TOK_GT;
		break;
	
	// Logical NOT
	case '!':
		ret = TOK_LOGICNOT;
		break;
	// Bitwise NOT
	case '~':
		ret = TOK_BWNOT;
		break;
	
	// Variables
	// \$[0-9]+ or \$[_a-zA-Z][_a-zA-Z0-9]*
	case '$':
		// Numeric Variable
		if( isdigit( *File->CurPos ) ) {
			while( isdigit(*File->CurPos) )
				File->CurPos ++;
		}
		// Ident Variable
		else {
			while( is_ident(*File->CurPos) || isdigit(*File->CurPos) )
				File->CurPos ++;
		}
		ret = TOK_VARIABLE;
		break;
	
	// Default (Numbers and Identifiers)
	default:
		File->CurPos --;
		
		// Numbers
		if( isdigit(*File->CurPos) )
		{
			ret = TOK_INTEGER;
			if( *File->CurPos == '0' && File->CurPos[1] == 'x' )
			{
				File->CurPos += 2;
				while(('0' <= *File->CurPos && *File->CurPos <= '9')
				   || ('A' <= *File->CurPos && *File->CurPos <= 'F')
				   || ('a' <= *File->CurPos && *File->CurPos <= 'f') )
				{
					File->CurPos ++;
				}
			}
			else
			{
				while( isdigit(*File->CurPos) )
					File->CurPos ++;
				
//				printf("*File->CurPos = '%c'\n", *File->CurPos);
				
				// Decimal
				if( *File->CurPos == '.' )
				{
					ret = TOK_REAL;
					File->CurPos ++;
					while( isdigit(*File->CurPos) )
						File->CurPos ++;
				}
				// Exponent
				if( *File->CurPos == 'e' || *File->CurPos == 'E' )
				{
					ret = TOK_REAL;
					File->CurPos ++;
					if(*File->CurPos == '-' || *File->CurPos == '+')
						File->CurPos ++;
					while( isdigit(*File->CurPos) )
						File->CurPos ++;
				}
				
//				printf(" ret = %i\n", ret);
			}
			break;
		}
	
		// Identifier
		if( is_ident(*File->CurPos) )
		{
			ret = TOK_IDENT;
			
			// Identifier
			while( is_ident(*File->CurPos) || isdigit(*File->CurPos) )
				File->CurPos ++;
			
			// This is set later too, but we use it below
			File->TokenLen = File->CurPos - File->TokenStr;
			
			// Check if it's a reserved word
			{
				char	buf[File->TokenLen + 1];
				 int	i;
				memcpy(buf, File->TokenStr, File->TokenLen);
				buf[File->TokenLen] = 0;
				for( i = 0; i < ARRAY_SIZE(csaReservedWords); i ++ )
				{
					if(strcmp(csaReservedWords[i].Name, buf) == 0) {
						ret = csaReservedWords[i].Value;
						break ;
					}
				}
			}
			// If there's no match, just keep ret as TOK_IDENT
			
			break;
		}
		// Syntax Error
		ret = TOK_INVAL;
		
		fprintf(stderr, "Syntax Error: Unknown symbol '%c'\n", *File->CurPos);
		longjmp(File->JmpTarget, 1);
		
		break;
	}
	// Return
	File->Token = ret;
	File->TokenLen = File->CurPos - File->TokenStr;
	
	#if DEBUG
	{
		char	buf[ File->TokenLen + 1];
		memcpy(buf, File->TokenStr, File->TokenLen);
		buf[File->TokenLen] = 0;
		//printf("  GetToken: File->CurPos = %p\n", File->CurPos);
		printf(" GetToken: Return %i (%i long) (%s)\n", ret, File->TokenLen, buf);
	}
	#endif
	return ret;
}

void PutBack(tParser *File)
{
	if( File->LastToken == -1 ) {
		// ERROR:
		fprintf(stderr, "INTERNAL ERROR: Putback when LastToken==-1\n");
		longjmp( File->JmpTarget, -1 );
		return ;
	}
	#if DEBUG
	printf(" PutBack: Was on %i\n", File->Token);
	#endif
	// Save
	File->NextLine = File->CurLine;
	File->NextToken = File->Token;
	File->NextTokenStr = File->TokenStr;
	File->NextTokenLen = File->TokenLen;
	// Restore
	File->CurLine = File->LastLine;
	File->Token = File->LastToken;
	File->TokenStr = File->LastTokenStr;
	File->TokenLen = File->LastTokenLen;
	File->CurPos = File->NextTokenStr;
	// Invalidate
	File->LastToken = -1;
}

int LookAhead(tParser *File)
{
	// TODO: Should I save the entire state here?
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
	if('A' <= ch && ch <= 'Z')	return 1;
	if(ch == '_')	return 1;
	#if !USE_SCOPE_CHAR
	if(ch == '.')	return 1;
	#endif
	if(ch < 0)	return 1;
	return 0;
}

int isdigit(int ch)
{
	if('0' <= ch && ch <= '9')	return 1;
	return 0;
}

int isspace(int ch)
{
	if(' ' == ch)	return 1;
	if('\t' == ch)	return 1;
	if('\b' == ch)	return 1;
	if('\n' == ch)	return 1;
	if('\r' == ch)	return 1;
	return 0;
}

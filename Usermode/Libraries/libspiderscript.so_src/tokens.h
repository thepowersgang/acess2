/*
 */
#ifndef _TOKENS_H_
#define _TOKENS_H_

// === TYPES ===
typedef struct
{	
	// Lexer State
	char	*BufStart;
	char	*CurPos;
	
	 int	LastToken, LastTokenLen;
	char	*LastTokenStr;
	
	 int	NextToken, NextTokenLen;
	char	*NextTokenStr;
	
	 int	Token, TokenLen;
	char	*TokenStr;
}	tParser;

// === FUNCTIONS ===
 int	GetToken(tParser *File);
void	PutBack(tParser *File);
 int	LookAhead(tParser *File);

// === CONSTANTS ===
enum eTokens
{
	TOK_INVAL,
	TOK_EOF,
	
	TOK_STR,
	TOK_INTEGER,
	TOK_VARIABLE,
	TOK_IDENT,
	
	TOK_RWD_FUNCTION,
	TOK_RWD_STRING,
	TOK_RWD_INTEGER,
	TOK_RWD_REAL,
	
	TOK_ASSIGN,
	TOK_SEMICOLON,
	TOK_COMMA,
	TOK_SCOPE,
	TOK_ELEMENT,
	
	TOK_EQUALS,
	TOK_LT,	TOK_LTE,
	TOK_GT,	TOK_GTE,
	
	TOK_DIV,	TOK_MUL,
	TOK_PLUS,	TOK_MINUS,
	TOK_SHL,	TOK_SHR,
	TOK_LOGICAND,	TOK_LOGICOR,	TOK_LOGICXOR,
	TOK_AND,	TOK_OR,	TOK_XOR,
	
	TOK_PAREN_OPEN,
	TOK_PAREN_CLOSE,
	TOK_BRACE_OPEN,
	TOK_BRACE_CLOSE,
	TOK_SQUARE_OPEN,
	TOK_SQUARE_CLOSE,
	
	TOK_LAST
};

#endif

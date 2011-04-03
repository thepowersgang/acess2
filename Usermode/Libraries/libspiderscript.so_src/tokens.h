/*
 */
#ifndef _TOKENS_H_
#define _TOKENS_H_

#include <setjmp.h>

// Make the scope character ('.') be a symbol, otherwise it's just
// a ident character
#define USE_SCOPE_CHAR	1

// === TYPES ===
typedef struct
{	
	// Lexer State
	const char	*BufStart;
	const char	*CurPos;
	
	char	*Filename;
	
	 int	LastLine;
	 int	LastToken, LastTokenLen;
	const char	*LastTokenStr;
	
	 int	NextLine;
	 int	NextToken, NextTokenLen;
	const char	*NextTokenStr;
	
	 int	CurLine;
	 int	Token, TokenLen;
	const char	*TokenStr;
	
	jmp_buf	JmpTarget;
	 int	ErrorHit;
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
	
	// Primitives
	TOK_STR,
	TOK_INTEGER,
	TOK_REAL,
	TOK_VARIABLE,
	TOK_IDENT,
	
	// Reserved Words
	// - Definitions
	TOK_RWD_FUNCTION,
	TOK_RWD_NAMESPACE,
	// - Control Flow
	TOK_RWD_NEW,
	TOK_RWD_RETURN,
	TOK_RWD_BREAK,
	TOK_RWD_CONTINUE,
	// - Blocks
	TOK_RWD_IF,
	TOK_RWD_ELSE,
	TOK_RWD_DO,
	TOK_RWD_WHILE,
	TOK_RWD_FOR,
	// - Types
	TOK_RWD_VOID,
	TOK_RWD_OBJECT,
	TOK_RWD_OPAQUE,
	TOK_RWD_STRING,
	TOK_RWD_INTEGER,
	TOK_RWD_REAL,
	
	// 
	TOK_ASSIGN,
	TOK_SEMICOLON,
	TOK_COMMA,
	TOK_SCOPE,
	TOK_ELEMENT,
	
	// Comparisons
	TOK_EQUALS,
	TOK_LT,	TOK_LTE,
	TOK_GT,	TOK_GTE,
	
	// Operations
	TOK_BWNOT,	TOK_LOGICNOT,
	TOK_DIV,	TOK_MUL,
	TOK_PLUS,	TOK_MINUS,
	TOK_SHL,	TOK_SHR,
	TOK_LOGICAND,	TOK_LOGICOR,	TOK_LOGICXOR,
	TOK_AND,	TOK_OR,	TOK_XOR,
	
	// Assignment Operations
	TOK_INCREMENT,  	TOK_DECREMENT,
	TOK_ASSIGN_DIV, 	TOK_ASSIGN_MUL,
	TOK_ASSIGN_PLUS,	TOK_ASSIGN_MINUS,
	TOK_ASSIGN_SHL, 	TOK_ASSIGN_SHR,
	TOK_ASSIGN_LOGICAND, 	TOK_ASSIGN_LOGICOR,	TOK_ASSIGN_LOGXICOR,
	TOK_ASSIGN_AND, 	TOK_ASSIGN_OR,	TOK_ASSIGN_XOR,
	
	TOK_PAREN_OPEN, 	TOK_PAREN_CLOSE,
	TOK_BRACE_OPEN, 	TOK_BRACE_CLOSE,
	TOK_SQUARE_OPEN,	TOK_SQUARE_CLOSE,
	
	TOK_LAST
};

#define TOKEN_GROUP_TYPES	TOK_RWD_VOID:\
	case TOK_RWD_OBJECT:\
	case TOK_RWD_OPAQUE:\
	case TOK_RWD_INTEGER:\
	case TOK_RWD_STRING:\
	case TOK_RWD_REAL
#define TOKEN_GROUP_TYPES_STR	"TOK_RWD_VOID, TOK_RWD_OBJECT, TOK_RWD_OPAQUE, TOK_RWD_INTEGER, TOK_RWD_STRING or TOK_RWD_REAL"

#define TOKEN_GET_DATATYPE(_type, _tok) do { switch(_tok) {\
	case TOK_RWD_VOID:  _type = SS_DATATYPE_UNDEF;	break;\
	case TOK_RWD_INTEGER:_type = SS_DATATYPE_INTEGER;	break;\
	case TOK_RWD_OPAQUE: _type = SS_DATATYPE_OPAQUE;	break;\
	case TOK_RWD_OBJECT: _type = SS_DATATYPE_OBJECT;	break;\
	case TOK_RWD_REAL:   _type = SS_DATATYPE_REAL;	break;\
	case TOK_RWD_STRING: _type = SS_DATATYPE_STRING;	break;\
	default:_type=SS_DATATYPE_UNDEF;fprintf(stderr,\
	"ERROR: Unexpected %s, expected "TOKEN_GROUP_TYPES_STR"\n",csaTOKEN_NAMES[Parser->Token]);\
	break;\
	} } while(0)

# if WANT_TOKEN_STRINGS
const char * const csaTOKEN_NAMES[] = {
	"TOK_INVAL",
	"TOK_EOF",
	
	"TOK_STR",
	"TOK_INTEGER",
	"TOK_REAL",
	"TOK_VARIABLE",
	"TOK_IDENT",
	
	"TOK_RWD_FUNCTION",
	"TOK_RWD_NAMESPACE",
	
	"TOK_RWD_NEW",
	"TOK_RWD_RETURN",
	"TOK_RWD_BREAK",
	"TOK_RWD_CONTINUE",
	
	"TOK_RWD_IF",
	"TOK_RWD_ELSE",
	"TOK_RWD_DO",
	"TOK_RWD_WHILE",
	"TOK_RWD_FOR",
	
	"TOK_RWD_VOID",
	"TOK_RWD_OBJECT",
	"TOK_RWD_OPAUQE",
	"TOK_RWD_STRING",
	"TOK_RWD_INTEGER",
	"TOK_RWD_REAL",
	
	"TOK_ASSIGN",
	"TOK_SEMICOLON",
	"TOK_COMMA",
	"TOK_SCOPE",
	"TOK_ELEMENT",
	
	"TOK_EQUALS",
	"TOK_LT",	"TOK_LTE",
	"TOK_GT",	"TOK_GTE",
	
	"TOK_BWNOT",	"TOK_LOGICNOT",
	"TOK_DIV",	"TOK_MUL",
	"TOK_PLUS",	"TOK_MINUS",
	"TOK_SHL",	"TOK_SHR",
	"TOK_LOGICAND",	"TOK_LOGICOR",	"TOK_LOGICXOR",
	"TOK_AND",	"TOK_OR",	"TOK_XOR",
	
	"TOK_INCREMENT",  	"TOK_DECREMENT",
	"TOK_ASSIGN_DIV",	"TOK_ASSIGN_MUL",
	"TOK_ASSIGN_PLUS",	"TOK_ASSIGN_MINUS",
	"TOK_ASSIGN_SHL",	"TOK_ASSIGN_SHR",
	"TOK_ASSIGN_LOGICAND",	"TOK_ASSIGN_LOGICOR",	"TOK_ASSIGN_LOGICXOR",
	"TOK_ASSIGN_AND",	"TOK_ASSIGN_OR",	"TOK_ASSIGN_XOR",
	
	"TOK_PAREN_OPEN",	"TOK_PAREN_CLOSE",
	"TOK_BRACE_OPEN",	"TOK_BRACE_CLOSE",
	"TOK_SQUARE_OPEN",	"TOK_SQUARE_CLOSE",
	
	"TOK_LAST"
};
# endif

#endif

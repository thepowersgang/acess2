/*
 */
#ifndef _AST_H_
#define _AST_H_

#include <spiderscript.h>
#include "tokens.h"

typedef enum eAST_NodeTypes	tAST_NodeType;
typedef struct sAST_Script	tAST_Script;
typedef struct sAST_Function	tAST_Function;
typedef struct sAST_Node	tAST_Node;
typedef struct sAST_BlockState	tAST_BlockState;
typedef struct sAST_Variable	tAST_Variable;

/**
 * \brief Node Types
 */
enum eAST_NodeTypes
{
	NODETYPE_NOP,
	
	NODETYPE_BLOCK,	//!< Node Block
	
	NODETYPE_VARIABLE,	//!< Variable
	NODETYPE_CONSTANT,	//!< Runtime Constant
	NODETYPE_STRING,	//!< String Constant
	NODETYPE_INTEGER,	//!< Integer Constant
	NODETYPE_REAL,	//!< Real Constant
	
	NODETYPE_DEFVAR,	//!< Define a variable (Variable)
	NODETYPE_SCOPE,	//!< Dereference a Namespace/Class static
	NODETYPE_ELEMENT,	//!< Reference a class attribute
	NODETYPE_CAST,	//!< Cast a value to another (Uniop)
	
	NODETYPE_RETURN,	//!< Return from a function (reserved word)
	NODETYPE_BREAK, 	//!< Break out of a loop
	NODETYPE_CONTINUE,	//!< Next loop iteration
	NODETYPE_ASSIGN,	//!< Variable assignment operator
	NODETYPE_POSTINC,	//!< Post-increment (i++) - Uniop
	NODETYPE_POSTDEC,	//!< Post-decrement (i--) - Uniop
	NODETYPE_FUNCTIONCALL,	//!< Call a function
	NODETYPE_METHODCALL,	//!< Call a class method
	NODETYPE_CREATEOBJECT,	//!< Create an object
	
	NODETYPE_IF,	//!< Conditional
	NODETYPE_LOOP,	//!< Looping Construct
	
	NODETYPE_INDEX,	//!< Index into an array
	
	NODETYPE_LOGICALNOT,	//!< Logical NOT operator
	NODETYPE_LOGICALAND,	//!< Logical AND operator
	NODETYPE_LOGICALOR, 	//!< Logical OR operator
	NODETYPE_LOGICALXOR,	//!< Logical XOR operator
	
	NODETYPE_EQUALS,	//!< Comparison Equals
	NODETYPE_LESSTHAN,	//!< Comparison Less Than
	NODETYPE_LESSTHANEQUAL,	//!< Comparison Less Than or Equal
	NODETYPE_GREATERTHAN,	//!< Comparison Greater Than
	NODETYPE_GREATERTHANEQUAL,	//!< Comparison Greater Than or Equal
	
	NODETYPE_BWNOT,	//!< Bitwise NOT
	NODETYPE_BWAND,	//!< Bitwise AND
	NODETYPE_BWOR,	//!< Bitwise OR
	NODETYPE_BWXOR,	//!< Bitwise XOR
	
	NODETYPE_BITSHIFTLEFT,	//!< Bitwise Shift Left (Grow)
	NODETYPE_BITSHIFTRIGHT,	//!< Bitwise Shift Right (Shrink)
	NODETYPE_BITROTATELEFT,	//!< Bitwise Rotate Left (Grow)
	
	NODETYPE_NEGATE,	//!< Negagte
	NODETYPE_ADD,	//!< Add
	NODETYPE_SUBTRACT,	//!< Subtract
	NODETYPE_MULTIPLY,	//!< Multiply
	NODETYPE_DIVIDE,	//!< Divide
	NODETYPE_MODULO,	//!< Modulus
};

struct sSpiderScript
{
	tSpiderVariant	*Variant;
	tAST_Script	*Script;
	char	*CurNamespace;	//!< Current namespace prefix (NULL = Root) - No trailing .
};

struct sAST_Script
{
	// TODO: Namespaces and Classes
	tAST_Function	*Functions;
	tAST_Function	*LastFunction;
};

struct sAST_Function
{
	tAST_Function	*Next;	//!< Next function in list
	 int	ReturnType;
	tAST_Node	*Code;	//!< Function Code
	tAST_Node	*Arguments;	// HACKJOB (Only NODETYPE_DEFVAR is allowed)
	tAST_Node	*Arguments_Last;
	char	Name[];	//!< Function Name
};

struct sAST_Node
{
	tAST_Node	*NextSibling;
	tAST_NodeType	Type;
	
	const char	*File;
	 int	Line;
	
	void	*BlockState;	//!< BlockState pointer (for cache integrity)
	 int	BlockIdent;	//!< Ident (same as above)
	void	*ValueCache;	//!< Cached value / pointer
	
	union
	{
		struct {
			tAST_Node	*FirstChild;
			tAST_Node	*LastChild;
		}	Block;
		
		struct {
			 int	Operation;
			tAST_Node	*Dest;
			tAST_Node	*Value;
		}	Assign;
		
		struct {
			tAST_Node	*Value;
		}	UniOp;
		
		struct {
			tAST_Node	*Left;
			tAST_Node	*Right;
		}	BinOp;
		
		struct {
			tAST_Node	*Object;
			tAST_Node	*FirstArg;
			tAST_Node	*LastArg;
			 int	NumArgs;
			char	Name[];
		}	FunctionCall;
		
		struct {
			tAST_Node	*Condition;
			tAST_Node	*True;
			tAST_Node	*False;
		}	If;
		
		struct {
			tAST_Node	*Init;
			 int	bCheckAfter;
			tAST_Node	*Condition;
			tAST_Node	*Increment;
			tAST_Node	*Code;
			char	Tag[];
		}	For;
		
		/**
		 * \note Used for \a NODETYPE_VARIABLE and \a NODETYPE_CONSTANT
		 */
		struct {
			char	_unused;	// Shut GCC up
			char	Name[];
		}	Variable;
		
		struct {
			tAST_Node	*Element;
			char	Name[];
		}	Scope;	// Used by NODETYPE_SCOPE and NODETYPE_ELEMENT
		
		struct {
			 int	DataType;
			tAST_Node	*LevelSizes;
			tAST_Node	*LevelSizes_Last;
			tAST_Node	*InitialValue;
			char	Name[];
		}	DefVar;
		
		struct {
			 int	DataType;
			 tAST_Node	*Value;
		}	Cast;
		
		// Used for NODETYPE_REAL, NODETYPE_INTEGER and NODETYPE_STRING
		tSpiderValue	Constant;
	};
};

/**
 * \brief Code Block state (stores local variables)
 */
struct sAST_BlockState
{
	tAST_BlockState	*Parent;
	tSpiderScript	*Script;	//!< Script
	tAST_Variable	*FirstVar;	//!< First variable in the list
	tSpiderValue	*RetVal;
	tSpiderNamespace	*BaseNamespace;	//!< Base namespace (for entire block)
	tSpiderNamespace	*CurNamespace;	//!< Currently selected namespace
	 int	Ident;	//!< ID number used for variable lookup caching
	const char	*BreakTarget;
	 int	BreakType;
};

struct sAST_Variable
{
	tAST_Variable	*Next;
	 int	Type;	// Only used for static typing
	tSpiderValue	*Object;
	char	Name[];
};

// === FUNCTIONS ===
extern tAST_Script	*AST_NewScript(void);
extern size_t	AST_WriteScript(void *Buffer, tAST_Script *Script);
extern size_t	AST_WriteNode(void *Buffer, size_t Offset, tAST_Node *Node);

extern tAST_Function	*AST_AppendFunction(tAST_Script *Script, const char *Name, int ReturnType);
extern void	AST_AppendFunctionArg(tAST_Function *Function, tAST_Node *Arg);
extern void	AST_SetFunctionCode(tAST_Function *Function, tAST_Node *Root);

extern tAST_Node	*AST_NewNop(tParser *Parser);

extern tAST_Node	*AST_NewString(tParser *Parser, const char *String, int Length);
extern tAST_Node	*AST_NewInteger(tParser *Parser, int64_t Value);
extern tAST_Node	*AST_NewReal(tParser *Parser, double Value);
extern tAST_Node	*AST_NewVariable(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewDefineVar(tParser *Parser, int Type, const char *Name);
extern tAST_Node	*AST_NewConstant(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewClassElement(tParser *Parser, tAST_Node *Object, const char *Name);

extern tAST_Node	*AST_NewFunctionCall(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewCreateObject(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewMethodCall(tParser *Parser, tAST_Node *Object, const char *Name);
extern void	AST_AppendFunctionCallArg(tAST_Node *Node, tAST_Node *Arg);

extern tAST_Node	*AST_NewCodeBlock(tParser *Parser);
extern void	AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);

extern tAST_Node	*AST_NewIf(tParser *Parser, tAST_Node *Condition, tAST_Node *True, tAST_Node *False);
extern tAST_Node	*AST_NewLoop(tParser *Parser, const char *Tag, tAST_Node *Init, int bPostCheck, tAST_Node *Condition, tAST_Node *Increment, tAST_Node *Code);

extern tAST_Node	*AST_NewAssign(tParser *Parser, int Operation, tAST_Node *Dest, tAST_Node *Value);
extern tAST_Node	*AST_NewCast(tParser *Parser, int Target, tAST_Node *Value);
extern tAST_Node	*AST_NewBinOp(tParser *Parser, int Operation, tAST_Node *Left, tAST_Node *Right);
extern tAST_Node	*AST_NewUniOp(tParser *Parser, int Operation, tAST_Node *Value);
extern tAST_Node	*AST_NewBreakout(tParser *Parser, int Type, const char *DestTag);
extern tAST_Node	*AST_NewScopeDereference(tParser *Parser, const char *Name, tAST_Node *Child);

extern void	AST_FreeNode(tAST_Node *Node);

// exec_ast.h
extern void	Object_Dereference(tSpiderValue *Object);
extern void	Object_Reference(tSpiderValue *Object);
extern tSpiderValue	*AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node);

#endif

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
	NODETYPE_CAST,	//!< Cast a value to another (Uniop)
	
	NODETYPE_RETURN,	//!< Return from a function (reserved word)
	NODETYPE_ASSIGN,	//!< Variable assignment operator
	NODETYPE_FUNCTIONCALL,	//!< Call a function
	
	NODETYPE_IF,	//!< Conditional
	NODETYPE_LOOP,	//!< Looping Construct
	
	NODETYPE_INDEX,	//!< Index into an array
	
	NODETYPE_LOGICALAND,	//!< Logical AND operator
	NODETYPE_LOGICALOR, 	//!< Logical OR operator
	NODETYPE_LOGICALXOR,	//!< Logical XOR operator
	
	NODETYPE_EQUALS,	//!< Comparison Equals
	NODETYPE_LESSTHAN,	//!< Comparison Less Than
	NODETYPE_GREATERTHAN,	//!< Comparison Greater Than
	
	NODETYPE_BWAND,	//!< Bitwise AND
	NODETYPE_BWOR,	//!< Bitwise OR
	NODETYPE_BWXOR,	//!< Bitwise XOR
	
	NODETYPE_BITSHIFTLEFT,	//!< Bitwise Shift Left (Grow)
	NODETYPE_BITSHIFTRIGHT,	//!< Bitwise Shift Right (Shrink)
	NODETYPE_BITROTATELEFT,	//!< Bitwise Rotate Left (Grow)
	
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
	tAST_Function	*Functions;
	tAST_Function	*LastFunction;
};

struct sAST_Function
{
	tAST_Function	*Next;	//!< Next function in list
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
			tAST_Node	*FirstArg;
			tAST_Node	*LastArg;
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
		}	For;
		
		/**
		 * \note Used for \a NODETYPE_VARIABLE and \a NODETYPE_CONSTANT
		 */
		struct {
			char	_unused;	// Shut GCC up
			char	Name[];
		}	Variable;
		
		struct {
			 int	DataType;
			 int	Depth;
			tAST_Node	*LevelSizes;
			tAST_Node	*LevelSizes_Last;
			char	Name[];
		}	DefVar;
		
		struct {
			 int	DataType;
			 tAST_Node	*Value;
		}	Cast;
		
		struct {
			 int	Length;
			char	Data[];
		}	String;
		
		uint64_t	Integer;
		double	Real;
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

extern tAST_Function	*AST_AppendFunction(tAST_Script *Script, const char *Name);
extern void	AST_AppendFunctionArg(tAST_Function *Function, tAST_Node *Arg);
extern void	AST_SetFunctionCode(tAST_Function *Function, tAST_Node *Root);
extern tAST_Node	*AST_NewString(tParser *Parser, const char *String, int Length);
extern tAST_Node	*AST_NewInteger(tParser *Parser, uint64_t Value);
extern tAST_Node	*AST_NewVariable(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewDefineVar(tParser *Parser, int Type, const char *Name);
extern tAST_Node	*AST_NewConstant(tParser *Parser, const char *Name);
extern tAST_Node	*AST_NewFunctionCall(tParser *Parser, const char *Name);
extern void	AST_AppendFunctionCallArg(tAST_Node *Node, tAST_Node *Arg);

extern tAST_Node	*AST_NewCodeBlock(void);
extern void	AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);

extern tAST_Node	*AST_NewIf(tParser *Parser, tAST_Node *Condition, tAST_Node *True, tAST_Node *False);

extern tAST_Node	*AST_NewAssign(tParser *Parser, int Operation, tAST_Node *Dest, tAST_Node *Value);
extern tAST_Node	*AST_NewCast(tParser *Parser, int Target, tAST_Node *Value);
extern tAST_Node	*AST_NewBinOp(tParser *Parser, int Operation, tAST_Node *Left, tAST_Node *Right);
extern tAST_Node	*AST_NewUniOp(tParser *Parser, int Operation, tAST_Node *Value);

extern void	AST_FreeNode(tAST_Node *Node);

// exec_ast.h
extern void	Object_Dereference(tSpiderValue *Object);
extern void	Object_Reference(tSpiderValue *Object);
extern tSpiderValue	*AST_ExecuteNode(tAST_BlockState *Block, tAST_Node *Node);

#endif

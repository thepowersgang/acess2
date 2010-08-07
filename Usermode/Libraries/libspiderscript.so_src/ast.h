/*
 */
#ifndef _AST_H_
#define _AST_H_

#include <spiderscript.h>

typedef struct sAST_Script	tAST_Script;
typedef struct sAST_Function	tAST_Function;
typedef struct sAST_Node	tAST_Node;
typedef enum eAST_NodeTypes	tAST_NodeType;

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
	
	NODETYPE_RETURN,	//!< Return from a function (reserved word)
	NODETYPE_ASSIGN,	//!< Variable assignment operator
	NODETYPE_FUNCTIONCALL,	//!< Call a function
	
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
};

struct sAST_Script
{
	tAST_Function	*Functions;
	tAST_Function	*LastFunction;
};

struct sAST_Function
{
	tAST_Function	*Next;
	char	*Name;
	tAST_Node	*Code;
	tAST_Node	*Arguments;	// HACKJOB (Only NODETYPE_VARIABLE is allowed)
};

struct sAST_Node
{
	tAST_Node	*NextSibling;
	tAST_NodeType	Type;
	
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
			 int	Length;
			char	Data[];
		}	String;
		
		struct {
			tAST_Node	*FirstArg;
			tAST_Node	*LastArg;
			char	Name[];
		}	FunctionCall;
		
		/**
		 * \note Used for \a NODETYPE_VARIABLE and \a NODETYPE_CONSTANT
		 */
		struct {
			char	_unused;	// Shut GCC up
			char	Name[];
		}	Variable;
		
		uint64_t	Integer;
		double	Real;
	};
};

// === FUNCTIONS ===
tAST_Script	*AST_NewScript(void);

tAST_Function	*AST_AppendFunction(tAST_Script *Script, const char *Name);
void	AST_AppendFunctionArg(tAST_Function *Function, int Type, tAST_Node *Arg);
void	AST_SetFunctionCode(tAST_Function *Function, tAST_Node *Root);

tAST_Node	*AST_NewString(const char *String, int Length);
tAST_Node	*AST_NewInteger(uint64_t Value);
tAST_Node	*AST_NewVariable(const char *Name);
tAST_Node	*AST_NewConstant(const char *Name);
tAST_Node	*AST_NewFunctionCall(const char *Name);
void	AST_AppendFunctionCallArg(tAST_Node *Node, tAST_Node *Arg);

tAST_Node	*AST_NewCodeBlock(void);
void	AST_AppendNode(tAST_Node *Parent, tAST_Node *Child);
tAST_Node	*AST_NewAssign(int Operation, tAST_Node *Dest, tAST_Node *Value);
tAST_Node	*AST_NewBinOp(int Operation, tAST_Node *Left, tAST_Node *Right);

void	AST_FreeNode(tAST_Node *Node);

tSpiderVariable	*AST_ExecuteNode(tSpiderScript *Script, tAST_Node *Node);

#endif

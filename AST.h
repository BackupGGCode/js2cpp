// AST.h - Abstract Syntax Trees
#ifndef AST_H
#define AST_H

#pragma warning(disable:4786)

#include "jslex.h"
#include <stdio.h>

namespace js2cpp
{

	class aScope;

	class AST
	{
	public:
		AST(const Token &token);
		~AST();

		Token	token;			// reference token
		AST*	first;			// descendent1
		AST*	second;			// 2
		AST*	third;			// 3

		inline const char* Name(void) const { return token.m_name; }

		void Attach(aScope* scope);
		aScope* Scope(int nUp=0) const;

		void TracePrint(FILE *f, int depth=0);

	private:
		aScope*		scope;
	}; // AST

	// access functions, to insulate from specifics of parse trees
	inline const char* Name(AST* node) { return node->token.m_name; }
	inline TokenType& Type(AST* node) { return node->token.m_type; }

	AST*& LoopBody(AST* loop_node);		// body of a loop (any loop)
	AST*& LoopExpr(AST* loop_node);		// controlling expr of a loop
	AST*& ThenPart(AST* if_node);		// then-part of an if statement
	AST*& ElsePart(AST* if_node);		// else-part of an if statement
	// functions:
	AST*& FuncIdent(AST* func_node);	// name (ident node) of a function
	AST*& Formals(AST* func_node);		// formal param list of a function
	AST*& FuncBody(AST* func_node);		// body of a function
	AST*& LHS(AST* assignment);
	AST*& RHS(AST* assignment);
	bool IsPrefix(AST* expr);			// unary operator node
	bool IsPostfix(AST* expr);
	AST*& LeftOperand(AST* expr);		// binary ops, and postfix unary ops
	AST*& RightOperand(AST* expr);		// binary ops and prefix unary ops

} // namespace

#endif

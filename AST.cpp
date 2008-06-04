// AST.cpp - Abstract Syntax Trees, implementation details
//
#pragma warning(disable:4786)

#include "AST.h"
#include "scope.h"
#include "log.h"
#include <assert.h>
#include <string.h>
#include <string>
using namespace std;


namespace js2cpp {

	///////////////////////////////////////////////////////////////////
	// AST - Abstract Syntax Tree

	AST::AST(const Token &t)
	: token(t), first(NULL), second(NULL), third(NULL), scope(NULL)
	{
	}

	AST::~AST()
	{
		delete first;
		delete second;
		delete third;
	}


	///////////////////////////////////////////////////////////////////
	// access functions, to insulate from oddities of trees

	aScope* AST::Scope(int nUp) const
	{
		assert(scope);
		aScope* s = scope;
		while (nUp--) {
			assert(s);
			if (!s) break;
			s = s->Parent();
		}
		return s;
	}

	void AST::Attach(aScope* s)
	{
		assert(!scope);
		scope = s;
	} // Attach

	AST*& LoopBody(AST* loop_node)
	{
		// body of a loop (any loop)
		assert(Type(loop_node)==tWHILE ||
	           Type(loop_node)==tDO ||
			   Type(loop_node)==tFOR);
		return loop_node->second;
	}

	AST*& LoopExpr(AST* loop_node)
	{
		// controlling expr of a loop
		assert(Type(loop_node)==tWHILE ||
	           Type(loop_node)==tDO ||
			   Type(loop_node)==tFOR);
		return loop_node->first;
	}

	AST*& ThenPart(AST* if_node)
	{
		// then-part of an if statement
		assert(Type(if_node)==tIF);
		return if_node->second;
	}

	AST*& ElsePart(AST* if_node)
	{
		// else-part of an if statement
		assert(Type(if_node)==tIF);
		return if_node->third;
	}

	AST*& FuncIdent(AST* func_node)
	{
		assert(Type(func_node)==tFUNCTION || Type(func_node)==tFUNEX);
		return func_node->first;
	}

	AST*& Formals(AST* func_node)
	{
		assert(Type(func_node)==tFUNCTION || Type(func_node)==tFUNEX);
		return func_node->second;
	}

	AST*& FuncBody(AST* func_node)
	{
		assert(Type(func_node)==tFUNCTION || Type(func_node)==tFUNEX);
		return func_node->third;
	}

	AST*& LHS(AST* expr)
	{
		assert(isAssOp(Type(expr)));
		return expr->first;
	}

	AST*& RHS(AST* expr)
	{
		assert(isAssOp(Type(expr)));
		return expr->second;
	}

	bool IsPrefix(AST* expr)
	{
		assert(isUnaryOp(Type(expr)));
		return (expr->second != NULL);
	}

	bool IsPostfix(AST* expr)
	{
		assert(isUnaryOp(Type(expr)));
		return (expr->first != NULL);
	}

	AST*& LeftOperand(AST* expr)
	{
		return expr->first;
	}

	AST*& RightOperand(AST* expr)
	{
		return expr->second;
	}

} // namespace js2cpp

// codegen.h - Code Generator

#ifndef CODEGEN_H
#define CODEGEN_H

#include "errcodes.h"
#include "stringtab.h"
#include "scope.h"

namespace js2cpp {

	class AST;
	class CodeSink;

class CodeGenerator
{
public:
	CodeGenerator(CodeSink* pcode, ErrorSink* perr);
	~CodeGenerator();

	void Program(AST* tree);

private:
	void TopLevelStatements(AST* tree);
	void TopLevelDefs(AST* tree);
	void InitializeVars(AST* tree);
	void EmitFunctionBody(AST* def);
	void Statements(AST* tree);
	void Block(AST* tree);
	void Statement(AST* tree);
	void RefExpr(AST* tree);
	void ExprValue(AST* tree);
	void EmitCall(AST* tree);
	void EmitFuncVal(AST *fun);
	void BindFormals(AST* tree);
	void DeclareLocalStruct(AST* def);
	void EmitLocals(AST* tree, bool bHeap);
	void ExprList(AST* tree);
	void ForLoop(AST* tree);

	const char* FuncName(AST* fun);
	int ArgCount(AST* fun);

	void DeclareFunctionClass(AST* fun);

	void emitString(const char *s);

	void emit(const char *pz);
	void emitf(const char *f,...);
	void indent(void);
	void dedent(void);

	void Error(ErrorCode e, AST* tree, const char *msg);

private:
	CodeSink*	m_psink;
	ErrorSink&	err;

	char		spaces[256];
	char*		indent_str;
	StringTable	nameTable;		// table of names, allows them to be unique.

	aScope*		local_scope;	// current scope being compiled, if any
	aScope*		global_scope;

	aScope* ActiveScope(void) const { return local_scope ? local_scope : global_scope; }

};

class CodeSink
{
public:
	virtual void emit(const char *) = 0;
	void emitf(const char *,...);

}; // ErrorSink

} // namespace

#endif

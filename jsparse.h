// jsparse.h - Javascript parser

#include "jslex.h"
#include <stdio.h>
#include "errcodes.h"
#include "AST.h"

namespace js2cpp
{

	class AST;

	class GlobalText : public SourceText
	{
	public:
		GlobalText(const char* s);
		virtual const char *Title();
		virtual bool ReadLine(char *buffer, int buflen);
	private:
		const char*		source;
	};

	class Parser
	{
	public:
		Parser(Lexer* plex, ErrorSink* perr);
		~Parser();

		AST* Parse(void);
		// Parse the lexical stream and return an
		// abstract syntax tree.

	private:
		Lexer&		lex;		// pointer to current token stream
		GlobalText	srcGlob;	// source text, global defs
		ErrorSink&	err;
		Token		token;		// current token
		bool		bNewline;	// current token starts a new line.
		Token		peeked[6];	// token look-ahead.
		int			nPeeked;	// number of peeked tokens
		int			nLitFuncs;	// counts literal functions

		bool Advance(void);
		// Read the next token

		bool Advance(TokenType tt);
		// Advance over token of specified type,
		// or report error.

		const Token& Peek(int i);
		// token i positions ahead of current token
		// Peek(0) returns the current token.

		// Check for a semicolon (or '}')
		// Interpolates one if there is a line break!
		bool SoftSemicolon(void);

		AST* Statements(void);
		AST* Statement(void);
		AST* Function(void);
		AST* FunctionLiteral(void);
		AST* FormalParams(void);
		AST* Block(void);
		AST* VarDecl(void);
		AST* VarExpr(void);
		AST* Expression(void);
		AST* ExprList(void);
		AST* AssignmentExpr(void);
		AST* ConditionalExpr(void);
		AST* Term(void);
		AST* ObjectLiteral(void);
		AST* ArrayLiteral(void);
		AST* ElementList(void);

		// Scoping of variables - done during parse
		//
		aScope*		scope[8];		// scopes
		int			nesting;		// scope nesting depth (0 in global scope)

		void InitGlobalScope(void);
		void StartScope(const char *name);	// enter a new scope
		void EndScope(AST* node);
		// close off and clean up a scope
		// attach it to the specified node
		// Export any refs to the surrounding scope

		void DeclareExternal(const char* extname);
		void DeclareVariable(const char* varname, AST* init);
		void DeclareFunction(const char* fname, AST* fun);
		void DeclareLiteralFunction(AST* def);
		void Reference(const char* id);

		// Reporting syntax errors and warnings
		//
		void Warning(ErrorCode w);
		// Issue a warning at the current token
		void Error(ErrorCode e, const char *note = "");
		// Report an error at the current token

	}; // Parser

} // namespace

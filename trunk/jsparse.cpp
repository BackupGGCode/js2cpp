// jsparse.cpp - Javascript parser
//
#pragma warning(disable:4786)

#include "jsparse.h"
#include "AST.h"
#include "scope.h"
#include <stdio.h>
#include <assert.h>

#define LENGTH(a) (sizeof(a)/sizeof((a)[0]))

namespace js2cpp {
//
//
	const char* global_text =
"extern var alert,undefined;\n"
"extern var Object, Function, Array, String, Boolean, Number, Date, RegExp;\n"
"extern var Error, EvalError, RangeError, ReferenceError, SyntaxError, TypeError, URIError;\n"
"extern var Math;\n"
;

	GlobalText::GlobalText(const char* s)
		: source(s)
	{
	}

	bool GlobalText::ReadLine(char* buffer, int buflen)
	{
		if (0==*source) {
			return false;
		}
		while (*source && buflen > 1) {
			if ((*buffer++ = *source++)=='\n') {
				break;
			}
		}
		*buffer = 0;
		return true;
	}

	const char* GlobalText::Title(void)
	{
		return "*predefined*";
	}

	Parser::Parser(Lexer *plexer, ErrorSink *perr)
		: lex(*plexer), err(*perr), nPeeked(0),
		  nesting(0),
		  nLitFuncs(0),
		  srcGlob(global_text)
	{
	}

	Parser::~Parser()
	{
	}

	AST* Parser::Parse(void)
	{
		// set up the global object/scope:
		InitGlobalScope();
		lex.Include(&srcGlob);
		// suck in the first token:
		Advance();
		// parse statements
		AST* t = Statements();
		EndScope(t);
		Advance(tEOF);
		return t;
	} // Parse

	bool Parser::Advance(void)
	{
		int lastline = token.m_line;
		if (nPeeked > 0) {
			// take from the peek queue
			token = peeked[0];
			for (int i = 1; i < nPeeked; i++) {
				peeked[i-1] = peeked[i];
			}
			nPeeked--;
		} else {
			lex.GetToken(token);
		}
		bNewline = (token.m_line != lastline);
		return true;
	}

	const Token& Parser::Peek(int i)
	{
		if (i<=0) {
			return token;
		}
		while (nPeeked < i) {
			lex.GetToken(peeked[nPeeked]);
			nPeeked++;
		}
		return peeked[i-1];
	} // Peek

	bool Parser::Advance(TokenType tt)
	{
		if (token.m_type != tt) {
			Error(E_EXPECTED, Name(tt));
			return false;
		}
		return Advance();
	}

	bool Parser::SoftSemicolon(void)
	{
		if (token.m_type==tSEMI) {
			Advance();
			return true;
		}
		if (bNewline || token.m_type==tRBRACE || token.m_type==tEOF) {
			fprintf(stderr, "==> inferred semicolon\n");
			return true;
		}
		return false;
	}

	void Parser::Warning(ErrorCode w)
	{
		const char *msg;
		switch (w) {
		case W_EXTRA_SEMI:
			msg = "Extra semicolon";
			break;
		default:
			msg = "Spike! missing text for warning code!";
			break;
		}
		err.Warning(token.m_sourceName, token.m_line, token.m_ichar, w, msg);
	}

	void Parser::Error(ErrorCode e, const char *note)
	{
		char szMsg[256];
		switch (e) {
		case E_EXPECTED:
			_snprintf(szMsg, LENGTH(szMsg),
				"Expected %s at %s", note, token.m_name);
			break;
		default:
			_snprintf(szMsg, LENGTH(szMsg), "Spike! No text for parse error %d!", (int)e);
			break;
		}
		err.Error(token.m_sourceName, token.m_line, token.m_ichar, e, szMsg);
	} // Error


	/////////////////////////////////////////////////////////////////
	// Name scoping

	void Parser::InitGlobalScope(void)
	{
		scope[0] = NULL;
		nesting = 0;		// in the outer scope
		StartScope("global_object");
	}

	void Parser::StartScope(const char* name)
	{
		nesting++;
		scope[nesting] = new aScope(name, scope[nesting-1]);

	}

	void Parser::EndScope(AST* tree)
	{
		tree->Attach(scope[nesting]);
		scope[nesting]->End();
		nesting--;
	}

	void Parser::DeclareExternal(const char* varname)
	{
		Binding binding;
		binding.MakeExtern();
		scope[nesting]->Bind(varname, binding);
	}

	void Parser::DeclareVariable(const char* varname, AST* init)
	{
		scope[nesting]->DeclareVariable(varname, init);
	}

	void Parser::DeclareFunction(const char* fname, AST* fun)
	{
		scope[nesting]->DeclareFunction(fname, fun);
	}

	void Parser::DeclareLiteralFunction(AST* def)
	{
		scope[nesting]->DeclareLiteralFunction(def);
	}

	void Parser::Reference(const char* id)
	{
		scope[nesting]->Reference(id);
	}

	/////////////////////////////////////////////////////////////////
	// Parsing specialists

	AST* Parser::Statements(void)
	{
		AST* r = new AST(token);	// parent node of list
		// Change node-type to StatementList
		Type(r) = tSTATLIST;

		AST* list = r;				// most recent node
        while (token.m_type != tEOF && token.m_type != tRBRACE) {
			AST* s = Statement();
			if (!s) {
				break;
			}
			list->first = s;
			if (token.m_type==tRBRACE || token.m_type==tEOF) {
				break;
			}
			// Create a new statement-list node
			list = list->third = new AST(r->token);
			Type(list) = tSTATLIST;
        }
        return r;
	} // Statements

	AST* Parser::Statement(void)
	{
		AST *r = NULL;

		// Is this a labelled statement?
		while (token.m_type==tIDENT && Peek(1).m_type==tCOLON) {
			// Yes. Yes, it is.
			// build a tree with the label
			AST *label = new AST(token); Advance();
			// make a tree with operator ':'
			r = new AST(token); Advance();
			r->first = label;
			r->second = Statement();
			return r;
        }

		switch (token.m_type) {
		case tBREAK:
		case tCONTINUE:
			r = new AST(token); Advance();
			if (SoftSemicolon()) {
				break;
			}
			if (token.m_type == tIDENT) {
				r->first = new AST(token); Advance();
			}
			if (!SoftSemicolon()) {
				Error(E_EXPECTED, ";");
			}
			break;

		case tCASE:
			r = new AST(token); Advance();
			r->first = Expression();
			if (!Advance(tCOLON)) {
				Error(E_EXPECTED, ":");
			}
			break;

		case tDEFAULT:
			r = new AST(token); Advance();
			if (!Advance(tCOLON)) {
				Error(E_EXPECTED, ":");
			}
			break;

		case tDO:
			r = new AST(token); Advance();
			LoopBody(r) = Statement();
			if (!Advance(tWHILE)) {
				Error(E_EXPECTED, "while");
				break;
			}
			if (!Advance(tLPAREN)) {
				Error(E_EXPECTED, "(");
				break;
			}
			LoopExpr(r) = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			break;

		case tFOR:
			{
				r = new AST(token); Advance();
				if (token.m_type != tLPAREN) {
					Error(E_EXPECTED, "(");
					break;
				}
				// create the iteration block, eat the '('
				AST* h = new AST(token); Advance();
				h->first = VarExpr();		// init OR var in object
				if (Type(h->first) == tIN) {
					// we're done with the iteration spec
				} else {
					// Otherwise, we expect init;test;incr
					if (!Advance(tSEMI)) {
						Error(E_EXPECTED, ";");
						break;
					}
					h->second = Expression();		// test
					if (!Advance(tSEMI)) {
						Error(E_EXPECTED, ";");
						break;
					}
					h->third = Expression();		// increment
				}
				if (!Advance(tRPAREN)) {
					Error(E_EXPECTED, ")");
				}
				LoopExpr(r) = h;
				LoopBody(r) = Statement();
			}
			break;

		case tFUNCTION:
			// function definition
			r = Function();
			break;

		case tIF:
			r = new AST(token); Advance();
			if (!Advance(tLPAREN)) {
				Error(E_EXPECTED, "(");
				break;
			}
			r->first = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			ThenPart(r) = Statement();
			if (token.m_type == tELSE) {
				Advance();
				ElsePart(r) = Statement();
			}
			break;

		case tRETURN:
			r = new AST(token); Advance();
			if (token.m_type != tSEMI && !bNewline) {
				r->first = Expression();
			}
			if (!SoftSemicolon()) {
				Error(E_EXPECTED, ";");
			}
			break;

		case tSWITCH:
			r = new AST(token); Advance();
			if (!Advance(tLPAREN)) {
				Error(E_EXPECTED, "(");
				break;
			}
			r->first = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			r->second = Block();
			break;

		case tTHROW:
			r = new AST(token); Advance();
			r->first = Expression();
			if (!SoftSemicolon()) {
				Error(E_EXPECTED, ";");
			}
			break;

		case tTRY:
			r = new AST(token); Advance();
			r->first = Block();
			if (token.m_type == tCATCH) {
				// catch block
				AST *c = new AST(token); Advance();
				if (!Advance(tLPAREN)) {
					Error(E_EXPECTED, "(");
					break;
				}
				if (token.m_type != tIDENT) {
					Error(E_EXPECTED, "<identifier>");
					break;
				}
				c->first = new AST(token); Advance();
				if (!Advance(tRPAREN)) {
					Error(E_EXPECTED, ")");
				}
				c->second = Block();
				r->second = c;
			}
			if (token.m_type == tFINALLY) {
				// catch block
				Advance();
				r->third = Block();
			}
			break;

		case tVAR:
			r = VarDecl();
			if (!SoftSemicolon()) {
				Error(E_EXPECTED, ";");
			}
			break;

		case tWHILE:
			r = new AST(token); Advance();
			if (!Advance(tLPAREN)) {
				Error(E_EXPECTED, "(");
				break;
			}
			LoopExpr(r) = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			LoopBody(r) = Statement();
			break;

		case tWITH:
			r = new AST(token); Advance();
			if (!Advance(tLPAREN)) {
				Error(E_EXPECTED, "(");
				break;
			}
			r->first = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			r->second = Statement();
			break;

		case tLBRACE:
			// compound statement
			r = Block();
			break;

		case tSEMI:
			// empty statement
			r = new AST(token); Advance();
			break;

		case tIDENT:
			if (0==strcmp(token.m_name,"extern")) {
				if (Peek(1).m_type==tVAR) {
					// special pseudo-keyword
					AST* tag = new AST(token); Advance();			// eat the 'extern'
					// return what looks like an empty 'var' declaration:
					r = new AST(token);	Advance(tVAR);		// eat the 'var'
					while (token.m_type==tIDENT) {
						DeclareExternal(token.m_name);
						Advance(tIDENT);
						if (token.m_type != tCOMMA) {
							break;
						}
						Advance(tCOMMA);
					}
					if (!SoftSemicolon()) {
						Error(E_EXPECTED, ";");
					}
					break;
				}
			}
			//** Fall thru **
		default:
			// Presumably some kind of expression
			// (Which usually means assignment or function call.)
			r = Expression();
			if (!SoftSemicolon()) {
				Error(E_EXPECTED, ";");
				Token bad = token;
				bad.m_type = tINVALID;
				r = new AST(bad);
				while (!bNewline && token.m_type!=tSEMI && token.m_type!=tRBRACE && token.m_type!=tEOF) {
					Advance();
				}
			}
			break;
		} // switch

		return r;
	} // Statement

	AST* Parser::Function(void)
	{
		// Parse function definition
		AST *r = new AST(token); Advance(tFUNCTION);
		if (token.m_type != tIDENT) {
			Error(E_EXPECTED, Name(tIDENT));
		} else {
			const char* name = token.m_name;
			StartScope(name);
			// function
			r->first = new AST(token); Advance();
			Formals(r) = FormalParams();
			FuncBody(r) = Block();
			EndScope(r);
			DeclareFunction(name, r);
		}
		return r;
	}

	AST* Parser::FunctionLiteral(void)
	{
		// TODO: ECMAScript syntax allows optional identifier
		// which is *not* bound in the containing scope, but can
		// be used for recursion in the body!
		// Kind of a mini-scope with just one id in it.
		assert(token.m_type==tFUNCTION);
		AST *r = new AST(token); Advance(tFUNCTION);
		// mark the tree node as a FunctionExpression, not a Function:
		Type(r) = tFUNEX;
		if (token.m_type==tIDENT) {
			// Allow optional name, but ignore it.
			// TODO: handle per ECMA
			Advance();
		}
		StartScope("<literal function>");
		Formals(r) = FormalParams();
		FuncBody(r) = Block();
		EndScope(r);
		DeclareLiteralFunction(r);
		return r;
	}

	AST* Parser::FormalParams(void)
	{
		AST *r = new AST(token);
		if (Advance(tLPAREN)) {
			AST *list = r;
			while (token.m_type == tIDENT) {
				DeclareVariable(token.m_name, NULL);
				list->first = new AST(token); Advance();
				if (token.m_type != tCOMMA) {
					break;
				}
				list = list->third = new AST(token); Advance();
			}
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
		} else {
			Error(E_EXPECTED, "(");
		}
		return r;
	} // FormalParams

	AST* Parser::VarDecl(void)
	{	// Parse the construct beginning with 'var'
		// but *not* including any terminating ';'
		AST* r = new AST(token); Advance();
		AST* vardecl = r;
next_var:
		if (token.m_type != tIDENT) {
			Error(E_EXPECTED, "<identifier>");
			return r;
		}
		vardecl->first = new AST(token); Advance();	// get the var-name
		if (token.m_type == tASSIGN) {
			// initialization clause
			Advance();
			vardecl->second = ConditionalExpr();		// get expr excluding ','
		}
		// Record that this variable was declared in the current scope
		DeclareVariable(Name(vardecl->first), vardecl->second);
		if (token.m_type == tCOMMA) {
			// more to come
			vardecl = vardecl->third = new AST(token); Advance();
			goto next_var;
		}
		return r;
	} // VarDecl

	AST* Parser::Block(void)
	{
		AST *r = new AST(token);
		if (token.m_type != tLBRACE) {
			Error(E_EXPECTED, "{");
		} else {
			Advance(tLBRACE);
			AST *list = r;
			if (token.m_type != tRBRACE) {
				while (true) {
					AST* s = Statement();
					list->first = s;
					if (token.m_type == tRBRACE ||
						token.m_type == tEOF) {
						break;
					}
					list = list->third = new AST(token);
				}
			}
			if (!Advance(tRBRACE)) {
				Error(E_EXPECTED, "}");
			}
		}
		return r;
	} // Block

	AST* Parser::Term(void)
	// one operand of a binary expression
	{
		AST *r = NULL;
		switch (token.m_type) {
		case tPLUSPLUS:				// ++ prefix
		case tMINUSMINUS:			// -- prefix
		case tMINUS:				// unary minus
		case tPLUS:					// unary plus
		case tWIGGLE:				// ~ bitwise not
		case tBANG:					// ! logical not
		case tDELETE:
		case tTYPEOF:
		case tVOID:
			r = new AST(token); Advance();
			// Prefix operators, operand goes in 'second' position
			r->second = Term();
			assert(IsPrefix(r));
			break;

		case tNEW:
			r = new AST(token); Advance();
			r->second = Term();
			assert(IsPrefix(r));
			break;

		case tLPAREN:
			Advance();
			r = Expression();
			if (!Advance(tRPAREN)) {
				Error(E_EXPECTED, ")");
			}
			break;

		case tLBRACE:
			// object-constructor
			r = ObjectLiteral();
			break;

		case tLBRACKET:
			r = ArrayLiteral();
			break;

		case tIDENT:
			Reference(token.m_name);
 			r = new AST(token); Advance();
			break;

		case tNUMBER:
		case tSTRING:
		case tREGEX:
		case tTHIS:
		case tTRUE:
		case tFALSE:
		case tNULL:
			r = new AST(token); Advance();
			break;

		case tFUNCTION:
			// function literal (js 1.2)
			r = FunctionLiteral();
			break;

		default:
			Error(E_EXPECTED, "<expression>");
			return new AST(token); Advance();
		} // switch
		// we found a 
postfixing:
		switch (token.m_type) {
		case tPLUSPLUS:				// ++ postfix
		case tMINUSMINUS:			// -- postfix
			{
				AST* operand = r;
				r = new AST(token); Advance();
				LeftOperand(r) = operand;
			}
			goto postfixing;

		case tLBRACKET:
			{ // array index
				AST *a = new AST(token); Advance();
				a->first = r;
				a->second = ExprList();
				if (!Advance(tRBRACKET)) {
					Error(E_EXPECTED, "]");
				}
				r = a;
			}
			goto postfixing;

		case tDOT:
			{
				AST* left = r;
				r = new AST(token); Advance();				// eat the dot
				if (token.m_type != tIDENT) {
					Error(E_EXPECTED, "<identifier>");
				} else {
					LeftOperand(r) = left;
					RightOperand(r) = new AST(token); Advance();
				}
			}
			goto postfixing;

		case tLPAREN:
			// function call, arguments follow.
			// '(' acts like binary operator
			// r holds
			{
				AST* left = r;
				r = new AST(token); Advance();	// eat the '('
				LeftOperand(r) = left;			// function designator
				if (token.m_type != tRPAREN) {
					// argument list
					RightOperand(r) = ExprList();
				}
				if (!Advance(tRPAREN)) {
					Error(E_EXPECTED, ")");
				}
			}
			goto postfixing;

		default:
			break;
		} // switch

		return r;
	} // Term

	AST* Parser::Expression(void)
	{
		return ExprList();
	} // Expression

	AST* Parser::ExprList(void)
	{
		AST *a = ConditionalExpr();
		if (token.m_type == tCOMMA) {
			AST *l = new AST(token);
			Advance();
			l->first = a;
			l->second = ExprList();
			a = l;
		}
		return a;
	} // ExprList

	AST* Parser::ConditionalExpr(void)
	{
		AST* a = AssignmentExpr();
		if (token.m_type == tFROGMARK) {
			AST *r = new AST(token);
			Advance();
			r->first = a;
			r->second = AssignmentExpr();
			if (!Advance(tCOLON)) {
				Error(E_EXPECTED, ":");
			} else {
				r->third = AssignmentExpr();
			}
			a = r;
		}
		return a;
	} // ConditionalExpr

	// priority stack for operator-operand associativity
	//
	bool is_higher_priority(TokenType a, TokenType b)
	{
		switch (a) {
			// precedence 13
		case tSPLAT:
		case tDIV:
		case tREM:
			return true;
		case tPLUS:
		case tMINUS:
			return b >= tPLUS;
		case tSL:
		case tSRSX:
		case tSRZX:
			return b >= tSL;
		case tLT:
		case tLTE:
		case tGT:
		case tGTE:
		case tINSTANCEOF:
		case tIN:
			return b >= tLT;
			// precedence 9
		case tEQUALEQUAL:
		case tNOTEQUAL:
		case tIDENTITY:
		case tNONIDENTITY:
			return b >= tEQUALEQUAL;
		case tAND:
		case tXOR:
		case tOR:
		case tLAND:
		case tLOR:
			return b >= a;
		case tFROGMARK:
		case tCOLON:
			return b > tCOLON;
		case tASSIGN:
		case tASSPLUS:
		case tASSMINUS:
		case tASSDIV:
		case tASSMUL:
		case tASSREM:
		case tASSSL:
		case tASSSRSX:
		case tASSSRZX:
		case tASSAND:
		case tASSXOR:
		case tASSOR:
		case tASSLAND:
		case tASSLOR:
			return b > tASSLOR;
			
		case tCOMMA:
			return b==tCOMMA;
		} // switch
		assert(false);
		return true;
	} // is_higher_priority

	class PrecedenceStack
	{
		// push alternating operands and operators onto this stack
		// when you push a new operator op2, as long as the top
		// of the stack is L op1 R and op1 has a higher precedence
		// than op2, then (L op1 R) is combined into one operand.
		// When there are no more operators, the stuff on the stack
		// is combined to form the final expression.
	public:
		PrecedenceStack() { sp = 0; }

		void push(Token &token) {
			while (sp >= 2 && is_higher_priority(stack[sp-2]->token.m_type, token.m_type)) {
				fold();
			}
			stack[sp++] = new AST(token);
		}

		void push(AST* expr) { stack[sp++] = expr; }

		AST* result(void) {
			while (sp > 1) { fold(); }
			assert(sp==1);
			return stack[--sp];
		}

	private:
		AST* stack[100];
		int sp;

		void fold(void) {
			AST* t = stack[sp-2];
			t->first = stack[sp-3];
			t->second = stack[sp-1];
			sp -= 3;
			stack[sp++] = t;
		}
	};

	AST* Parser::AssignmentExpr(void)
	{
		PrecedenceStack stack;

		stack.push(Term());
		while (token.m_type >= tSPLAT && token.m_type < tCOMMA && token.m_type != tFROGMARK) {
			// ordinary binary operator
			stack.push(token); Advance();
			stack.push(Term());
		}
		return stack.result();
	} // AssignmentExpr


	AST* Parser::VarExpr(void)
	{
		// TODO: Make this really match ECMA.
		// parse an expr-list
		// OR a vardecl: var <id> [= <expr] {, <id> [= <expr>]}
		// OR a thing of the form <lhs> in <expr>
		// OR a thing of the form var <id> in <expr>
		// Needless to say, this is only used in parsing a for-loop control expression
		if (token.m_type != tVAR) {
			// parse an expression, which can include assignments & commas.
			return Expression();
		}
		// var <id> [= <expr>] {, <id> [= <expr>]}  OR
		// var <id> in <expr>
		if (Peek(1).m_type == tIDENT && Peek(2).m_type == tIN) {
			// var <id> in <expr>
			// create a (VAR <id>) node
			AST* varnode = new AST(token); Advance();		// pick up the 'var'
			varnode->first = new AST(token); Advance();		// <ident>
			// return an (IN (VAR <id>) <expr>) node
			AST* r = new AST(token), Advance();				// make an 'in' node
			r->first = varnode;
			r->second = Expression();		// the 'object' expression
			return r;
		}
		return VarDecl();
	} // VarExpr

	AST* Parser::ObjectLiteral(void)
	{
		// assume there's a { there.
		AST *r = new AST(token); Advance();
		AST *list = r;
		while (token.m_type==tIDENT) {
			// plug in property-name
			list->first = new AST(token); Advance();
			if (!Advance(tCOLON)) {
				Error(E_EXPECTED, ":");
				break;
			}
			list->second = ConditionalExpr();
			if (token.m_type != tCOMMA) {
				break;
			}
			list = list->third = new AST(token);
			Advance();
		} // while
		if (!Advance(tRBRACE)) {
			Error(E_EXPECTED, "}");
		}
		return r;
	} // ObjectLiteral

	AST* Parser::ElementList(void)
	{
		AST* list = NULL;
		if (token.m_type!=tRBRACKET) {
			if (token.m_type==tCOMMA) {
				list = new AST(token); Advance();
				list->second = ElementList();
			} else {
				AST* e = ConditionalExpr();
				list = new AST(token);
				Type(list) = tCOMMA;
				list->first = e;
				if (token.m_type==tCOMMA) {
					Advance();
					list->second = ElementList();
				}
			}
		}
		return list;
	} // ElementList

	AST* Parser::ArrayLiteral(void)
	{
		AST *r = new AST(token); Advance();	// eat the '['
		Type(r) = tARRAYLIT;
		if (token.m_type!=tRBRACKET) {
			r->first = ElementList();
		}
		if (!Advance(tRBRACKET)) {
			Error(E_EXPECTED, "]");
		}
		return r;
	} // ArrayLiteral

} // namespace

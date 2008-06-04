// codegen.cpp - Code Generator
#pragma warning(disable:4786)

#include "codegen.h"
#include "AST.h"
#include "scope.h"
#include "jscpprt.h"
#include "log.h"
#include <assert.h>
#include "memory.h"
#include <stdarg.h>
#include <stdio.h>
#include <new>
#include <string>

namespace js2cpp {

	int ListLength(AST* list)
	{
		if (!list) {
			return 0;
		}
		if (Type(list)==tCOMMA) {
			int n = 1;
			if (list->second) {
				n += ListLength(list->second);
			}
			return n;
		} else {
			return 1;
		}
	} // ListLength

	int CodeGenerator::ArgCount(AST* fun)
	{
		// Return the number of arguments expected by function fun
		assert(Type(fun)==tFUNCTION || Type(fun)==tFUNEX);
		return ListLength(Formals(fun));
	}

	const char* CodeGenerator::FuncName(AST* fun)
	{
		// Return the name of the 'body' (executable C++ function)
		// that implements function fun.
		if (Type(fun)==tFUNEX) {
			char name[256];
			sprintf(name, "LitFunc_%x", (unsigned long)fun);
			return nameTable.Intern(name)->m_name;
		}
		if (Type(fun)==tFUNCTION) {
			return FuncIdent(fun)->Name();
		}
		assert(false);
		return "unknown_";
	}

	CodeGenerator::CodeGenerator(CodeSink* pcode, ErrorSink* perr)
	: m_psink(pcode),
	  err(*perr)
	{
		memset(spaces, ' ', sizeof spaces - 1);
		indent_str = spaces + (sizeof spaces) - 1;
		*indent_str = (char)0;
	}

	CodeGenerator::~CodeGenerator()
	{
	}

	void CodeGenerator::Error(ErrorCode e, AST* tree, const char *msg)
	{
		Token& token = tree->token;

		err.Error(token.m_sourceName, token.m_line, token.m_ichar, e, msg);
	} // Error
	


	void CodeGenerator::indent(void)
	{
		indent_str -= 2;
	}

	void CodeGenerator::dedent(void)
	{
		indent_str += 2;
	}

	void CodeGenerator::emit(const char *pz)
	{
		m_psink->emit(pz);
	}

	void CodeGenerator::emitf(const char *pz,...)
	{
		static char buffer[8192];
		va_list arg_ptr;
		
		va_start( arg_ptr, pz );
		if (_vsnprintf(buffer, sizeof buffer, pz, arg_ptr) >= 0) {
			m_psink->emit(buffer);
		} else {
			// presumably buffer overflow
			const int BIGSIZE = 65536;
			char *bigbuf = new char[BIGSIZE];
			if (!bigbuf) {
				throw std::bad_alloc();
			}
			if (_vsnprintf(bigbuf, BIGSIZE, pz, arg_ptr) < 0) {
				delete[] bigbuf;
				throw "CodeGenerator::emitf buffer overflow";
			}
			m_psink->emit(bigbuf);
			delete[] bigbuf;
		}
		va_end(arg_ptr);

	}

	void CodeGenerator::Program(AST* tree)
	{
		emit("// js2cpp code generator\n");
		emit("#include \"jscpprt.h\"\n");
		emit("\n");
		emit("// definitions\n\n");
		local_scope = NULL;
		global_scope = tree->Scope();

		TopLevelDefs(tree);
		emit("\n");
		emit("int jsmain_(...)\n");
		emit("{\n");
		indent();
		emit("// dynamic global initialization\n");
		InitializeVars(tree);
		emit("\n");
		TopLevelStatements(tree);
		emitf("%sreturn 0;\n", indent_str);
		dedent();
		emit("} // jsmain_\n\n");
		emit("//------- end of module\n");
	}


	void CodeGenerator::TopLevelDefs(AST* tree)
	{
		if (!tree || Type(tree)==tINVALID) {
			return;
		}
		// Pump out user-defined globals, both explicit and
		// implicitly defined.
		aScope* scope = tree->Scope();
		Bindings& decls = scope->Declarations();

		for (Bindings::iterator ii=decls.begin(); ii!=decls.end(); ++ii) {
			Binding& binding = (*ii).second;
			const char* id = (const char*)(*ii).first;
			emit(indent_str);
			if (binding.isReference()) {
				// referenced but never defined
				// TODO: Reference to such a variable before it is set
				// is supposed to throw an exception.
				emitf("value_ %s;\n", id);
			} else if (binding.isExtern()) {
				// External variable
				emitf("extern value_ %s;\n", id);
			} else if (binding.isVar()) {
				// Local (global) variable
				emitf("value_ %s;\n", id);
			} else if (binding.isFunction()) {
				// Function
				// do a tricky declare & initialize.
				AST* fun = binding.Definition();
				// Note - literal functions are handled separately.
				// forward or extern the body:
				if (FuncBody(fun)) {
					// It has a body, forward declare it:
					DeclareFunctionClass(fun);
				} else {
					// No body, must be extern?
					assert(false);
				}
				// create function object:
				const char* fname = FuncName(fun);
				emit(indent_str);
				emitf("%s_foc_ %s_func_;\n", fname, fname);
				// declare variable that holds function value:
				emit(indent_str);
				emitf("value_ %s(&%s_func_);\n", id, fname);
			} else {
				assert(false);
			}
		}

		// Emit function definitions
		emit("\n// literal functions\n");
		TreeSet& lits = scope->LiteralFunctions();
		for (TreeSet::iterator jj=lits.begin(); jj!=lits.end(); ++jj) {
			AST* fun = (*jj);
			DeclareFunctionClass(fun);
			// Declare the func_ object:
			const char* fname = FuncName(fun);
			emitf("%s_foc_ %s_func_;\n\n", fname, fname);
			// Emit the code for the function:
			EmitFunctionBody(fun);
			emit("\n");
		}

		emit("// named functions\n\n");
		for (ii=decls.begin(); ii!=decls.end(); ++ii) {
			Binding binding = (*ii).second;
			if (binding.isFunction()) {
				AST* def = binding.Definition();
				if (FuncBody(def)) {
					// emit function definition!
					emit("\n");
					EmitFunctionBody(def);
					emit("\n");
				}
			}
		}
	} // TopLevelDefs

	void CodeGenerator::InitializeVars(AST* tree)
	{
	} // InitializeVars

	void CodeGenerator::DeclareFunctionClass(AST* fun)
	{
		const char* fname = FuncName(fun);
		emit("\n");		// just for readability
		aScope* scope = fun->Scope();
		int depth = scope->Depth();		// levels of nesting
		// Forward declare the class that represents the literal function
		emitf("class %s_foc_ : public func_ {\n", fname);
		emitf("public:\n");
		// emit 'defining scope' links for each NLNG scope:
		for (int d = 1; d < depth; d++) {
			emitf("  %s_locals_& nlng%d_;\n", scope->AtDepth(d)->Name(), d);
		}
		// Create constructor for func_ objects for this function:
		emitf("  %s_foc_(", fname);
		for (d = 1; d < depth; d++) {
			if (d > 1) {
				emit(",");
			}
			emitf("%s_locals_* pl%d_", scope->AtDepth(d)->Name(), d);
		}
		emit(")");
		for (d = 1; d < depth; d++) {
			if (d==1) {
				emit(" : ");
			} else {
				emit(",");
			}
			emitf("nlng%d_(*pl%d_)", d, d);
		}
		emitf(" { length=%d; }\n", ArgCount(fun));
		emitf("  virtual value_ call(value_,int,...);\n");
		emitf("};\n");
		emit("\n");
	}

	void CodeGenerator::EmitFunctionBody(AST* def)
	{
		if (!def || Type(def)==tINVALID) {
			return;
		}

		// Declare the class that holds the local variables
		DeclareLocalStruct(def);

		// Does this function have any nested functions?
		bool bHasNestedFns = false;
		aScope* scope = def->Scope();
		if (scope) {
			Bindings& decls = scope->Declarations();
			for (Bindings::iterator ii=decls.begin(); ii!=decls.end(); ++ii) {
				const char* id = (const char*)(*ii).first;
				Binding binding = (*ii).second;
				if (binding.isFunction()) {
					// nested function def, I don't know *what* to do with
					// this yet!
					bHasNestedFns = true;
					AST* nestedFn = binding.Definition();
					Log("Function %s contains function %s\n", FuncName(def), FuncName(nestedFn));
					DeclareFunctionClass(nestedFn);
					EmitFunctionBody(nestedFn);
				}
			} // for
			TreeSet& lits = scope->LiteralFunctions();
			for (TreeSet::iterator jj=lits.begin(); jj!=lits.end(); ++jj) {
				bHasNestedFns = true;
				AST* nestedFn = (*jj);
				Log("Function %s contains literal function %s\n", FuncName(def), FuncName(nestedFn));
				// Forward declare the class that represents the literal function
				DeclareFunctionClass(nestedFn);
				EmitFunctionBody(nestedFn);
			}
		}
		// If any nested functions, allocate our locals on the heap:
		bool bHeapLocals = bHasNestedFns;

		AST* formals = Formals(def);
		AST* body = FuncBody(def);
		if (body) {
			const char* name = FuncName(def);
			emit(indent_str);
			// Begin the actual function body
			emitf("value_ %s_foc_::call(value_ this_,int nargs_,...) {\n", name);
			indent();
			// push into scope of function
			aScope* old_local_scope = local_scope;
			local_scope = def->Scope();
			Log("Inside scope %s\n", local_scope->Name());

			EmitLocals(def, bHeapLocals);
			BindFormals(formals);
			emit("\n");
			Statements(body);
			// Throw out a 'safety' return in case
			// the body falls thru without returning a value:
			emitf("%sreturn undefined;\n", indent_str);
			dedent();
			emitf("%s} // call\n\n", indent_str);

			// pop out to surrounding scope, if any
			Log("Leaving scope %s\n", local_scope->Name());
			local_scope = old_local_scope;
			if (local_scope) {
				Log("Now in scope %s\n", local_scope->Name());
			} else {
				Log("Now in global scope\n");
			}
		}
	} // EmitFunction

	void CodeGenerator::DeclareLocalStruct(AST* def)
	{
		if (!def || Type(def)==tINVALID) {
			return;
		}

		emit(indent_str);
		emitf("class %s_locals_ {\n", FuncName(def));
		emitf("%spublic:\n", indent_str);

		aScope* scope = def->Scope();
		Bindings& decls = scope->Declarations();
		for (Bindings::iterator ii=decls.begin(); ii!=decls.end(); ++ii) {
			const char* id = (const char*)(*ii).first;
			Binding binding = (*ii).second;
			if (binding.isVar() || binding.isFunction()) {
				emit(indent_str);
				emitf("value_ %s;\n", id);
			}
		} // for
		emitf("%s};\n\n", indent_str);
	} // DeclareLocalStruct

	void CodeGenerator::EmitLocals(AST* tree, bool bHeap)
	{
		if (!tree || Type(tree)==tINVALID) {
			return;
		}

		const char* fname = FuncName(tree);
		if (bHeap) {
			// allocate locals on heap
			emit(indent_str);
			emitf("%s_locals_& locals_ = *(new %s_locals_);\n", fname, fname);
		} else {
			// allocate locals on stack
			emit(indent_str);
			emitf("%s_locals_ locals_;\n", fname);
		}

		aScope* scope = tree->Scope();
		Bindings& decls = scope->Declarations();
		for (Bindings::iterator ii=decls.begin(); ii!=decls.end(); ++ii) {
			const char* id = (const char*)(*ii).first;
			Binding binding = (*ii).second;
			if (binding.isFunction()) {
				// nested function def
				AST* nestedfn = binding.Definition();
				emit(indent_str);
				emitf("locals_.%s = ", FuncName(nestedfn));
				EmitFuncVal(nestedfn);
				emit(";\n");
			}
		} // for
	} // EmitLocals

	void CodeGenerator::RefExpr(AST* tree)
	{
		if (!tree || Type(tree)==tINVALID) {
			return;
		}

		switch (Type(tree)) {
		case tDOT:
			emit("(");
			ExprValue(LeftOperand(tree));
			// TODO: intern any string handed to .ref
			emitf(").dotref(\"%s\")", RightOperand(tree)->Name());
			break;

		case tLBRACKET:
			// indexing operation
			emit("(");
			ExprValue(tree->first);
			emit(").atref(");
			ExprValue(tree->second);
			emit(")");
			break;

		default:
			ExprValue(tree);
			break;
		} // switch
	} // RefExpr

	void CodeGenerator::ExprValue(AST* tree)
	{
		// Emit code that computes the value of an expression tree.
		if (!tree || Type(tree)==tINVALID) {
			return;
		}

		switch (Type(tree)) {
		case tINVALID:
			return;

		case tNUMBER:
			emitf("value_(%s)", tree->token.m_name);
			break;

		case tIDENT:
			// Other parts take care of getting variables declared.
			{
				Binding decl;
				aScope* owner;
				if (ActiveScope()->FindDeclaration(tree->token.m_name, decl, owner)) {
					if (owner == local_scope) {
						emit("locals_.");
					} else if (owner == global_scope) {
						// no prefix needed, just a bare reference
					} else {
						// NLNG reference
						emitf("nlng%d_.", owner->Depth());
					}
				} else {
					// implicitly declared global
					// TODO: need code to throw exception if uninitialized
					// for now, fall thru.
				}
				emit(tree->token.m_name);
			}
			break;

		case tTRUE:
			emit("true_");
			break;

		case tFALSE:
			emit("false_");
			break;

		case tTHIS:
			emit("this_");
			break;

		case tNULL:
			emit("null_");
			break;

		case tSTRING:
			emit("value_(");
			emitString(tree->token.m_name);
			emit(")");
			break;

		case tREGEX:
			emitf("rx_(\"%s\")", tree->token.m_name);
			break;

		case tFUNEX:
			// function literal!
			EmitFuncVal(tree);
			break;

		case tARRAYLIT:
			// array literal
			{
				emit(indent_str);
				emitf("MakeArray_(%d", ListLength(tree->first));
				AST* list = tree->first;
				while (list) {
					emit(",");
					assert(Type(list)==tCOMMA);
					emitf("\n%s", indent_str);
					if (list->first) {
						ExprValue(list->first);
					} else {
						emit("undefined");
					}
					list = list->second;
				} // while
				emit(");\n");
			}
			break;

		case tVAR:
			// variable declaration
			// null var expressions are possible:
			if (tree->first) {
				assert(Type(tree->first)==tIDENT);
				if (tree->second) {
					// Process as an assignment!
					ExprValue(tree->first);
					emit("=");
					ExprValue(tree->second);
				}
			}
			break;

		case tASSIGN:
		case tASSPLUS:
		case tASSMINUS:
		case tASSDIV:
		case tASSMUL:
		case tASSREM:
		case tASSSL:
		case tASSSRSX:
		case tASSAND:
		case tASSXOR:
		case tASSOR:
			RefExpr(LHS(tree));
			emit(tree->token.m_name);
			ExprValue(RHS(tree));
			break;

		case tLPAREN:
			// function call
			EmitCall(tree);
			break;

		case tPLUS:
			if (tree->first) {
				ExprValue(tree->first);
				emit(tree->token.m_name);
			}
			ExprValue(tree->second);
			break;

		case tMINUS:
			if (tree->first) {
				ExprValue(tree->first);
			}
			emit(tree->token.m_name);
			ExprValue(tree->second);
			break;

		case tCOMMA:
		case tLT:
		case tLTE:
		case tNOTEQUAL:
		case tGT:
		case tGTE:
		case tEQUALEQUAL:
		case tSPLAT:
		case tOR:
		case tLOR:
		case tAND:
		case tLAND:
		case tXOR:
		case tREM:
		case tDIV:
			emit("(");
			ExprValue(LeftOperand(tree));
			emit(")");
			emit(Name(tree));
			emit("(");
			ExprValue(RightOperand(tree));
			emit(")");
			break;

		case tNONIDENTITY:
			emit("!");
		case tIDENTITY:
			emit("identical_(");
			ExprValue(LeftOperand(tree));
			emit(",");
			ExprValue(RightOperand(tree));
			emit(")");
			break;

		case tDOT:
			emit("(");
			ExprValue(LeftOperand(tree));
			emitf(").dot(\"%s\")", RightOperand(tree)->Name());
			break;

		case tLBRACKET:
			// array indexing
			emit("(");
			ExprValue(tree->first);
			emit(").at(");
			ExprValue(tree->second);
			emit(")");
			break;

		case tPLUSPLUS:
			if (IsPrefix(tree)) {
				emit("preinc_(");
				RefExpr(RightOperand(tree));
				emit(")");
			} else {
				emit("postinc_(");
				RefExpr(LeftOperand(tree));
				emit(")");
			}
			break;

		case tMINUSMINUS:
			if (IsPrefix(tree)) {
				emit("predec_(");
				RefExpr(RightOperand(tree));
				emit(")");
			} else {
				emit("postdec_(");
				RefExpr(LeftOperand(tree));
				emit(")");
			}
			break;

		case tNEW:
			{
				assert(IsPrefix(tree));
				// Get constructor expression:
				AST* op = RightOperand(tree);		// new is a prefix operator
				// operand can be an Expression or a call
				AST* cons;
				AST* args;
				if (Type(op)==tLPAREN) {
					// new cons(args)
					cons = op->first;
					args = op->second;
				} else {
					// new cons
					cons = op;
					args = NULL;
				}
				ExprValue(cons);
				emit(".toFunc()->call");
				emitf("(value_(new obj_),%d", ListLength(args));
				if (args) {
					emit(",");
					ExprList(args);
				}
				emit(")");
			}
			break;

		case tWIGGLE:
		case tBANG:
			// prefix form is only form
			assert(IsPrefix(tree));
			emit(tree->token.m_name);
			emit("(");
			ExprValue(RightOperand(tree));
			emit(")");
			break;

		case tTYPEOF:
			emit("(");
			ExprValue(RightOperand(tree));
			emit(").typeof()");
			break;

		case tVOID:
			// the void prefix operator
			// Evaluate the operand for it's value
			// then return undefined.
			emit("((");
			ExprValue(RightOperand(tree));
			emit("),undefined)");
			break;

		default:
			emitf("%s(", tree->token.m_name);
			if (tree->first) {
				ExprValue(tree->first);
			}
			if (tree->second) {
				if (tree->first) {
					emit(",");
				}
				ExprValue(tree->second);
			}
			if (tree->third) {
				if (tree->first || tree->second) {
					emit(",");
				}
				ExprValue(tree->third);
			}
			emit(")");
			break;
		} // switch
	}

	void CodeGenerator::ExprList(AST* list)
	{
		if (Type(list)==tCOMMA) {
			ExprValue(list->first);
			if (list->second) {
				emit(",");
				ExprList(list->second);
			}
		} else {
			ExprValue(list);
		}
	} // ExprList

	void CodeGenerator::EmitCall(AST *tree)
	{
		AST* args = tree->second;
		AST* func = tree->first;
		if (!func) {
			return;
		}
		if (Type(func)==tDOT) {
			// ah-ha, a named method call
			// generate (left).dotcall(method-name-string,base,#args,arg0,arg1,...)
			const char* id = RightOperand(func)->Name();
			emit("(");
			ExprValue(LeftOperand(func));
			emitf(").dotcall(\"%s\",", id);
		} else if (Type(func)==tLBRACKET) {
			// indexed call
			emit("(");
			ExprValue(func->first);
			emit(").eltcall(");
			ExprValue(func->second);
			emit(",");
		} else {
			// function call (with this === global obj)
			emit("(");
			ExprValue(tree->first);
			emit(").toFunc()->call(global_,");
		}
		emitf("%d", ListLength(args));
		if (args) {
			emit(",");
			ExprList(args);
		}
		emit(")");
	} // EmitCall

	void CodeGenerator::EmitFuncVal(AST *fun)
	{
		// fun can be a function literal, or the def
		// of a named function
		assert(Type(fun)==tFUNCTION || Type(fun)==tFUNEX);
		// level 0 (non-nested) functions have global (static)
		// func_ objects.
		// Nested functions need to have their func_ objects
		// constructed on demand, on the heap.
		const char* fname = FuncName(fun);
		// How many functions surround this one:
		int nesting = fun->Scope()->Depth()-1;
		if (nesting==0) {
			// top-level function, has a static func_ object
			emitf("value_((func_*)&%s_func_)", fname);
		} else {
			// nested function, have to dynamically construct
			// the func_ object, and pass it pointers to all
			// defining scopes.
			emitf("value_(new %s_foc_(", fname);
			for (int d = 1; d < nesting; d++) {
				emitf("&nlng%d_,", d);
			}
			emit("&locals_))");
		}
	} // EmitFuncVal

	void CodeGenerator::BindFormals(AST *forms)
	{
		// Note: formals are declared as uninitialized local vars
		AST* list = forms;
		// walk the list initializing the formals
		int i = 0;
		while (list) {
			if (list->first) {
				const char* name = Name(list->first);
				emit(indent_str);emitf("if (nargs_ > %d) {\n", i);
				emit(indent_str);emitf("  locals_.%s = ((value_*)(&nargs_+1))[%d];\n", name, i);
				emit(indent_str);emit("} else {\n");
				emit(indent_str);emitf("  locals_.%s = undefined;\n", name);
				emit(indent_str);emitf("}\n");
				i++;
			}
			list = list->third;
		}
	} // BindFormals

	void CodeGenerator::TopLevelStatements(AST* tree)
	{
		while (tree) {
			if (Type(tree)==tINVALID) {
				return;
			}

			assert(Type(tree)==tSTATLIST);
			AST* stat = tree->first;
			if (stat) {
				switch (Type(stat)) {
				case tFUNCTION:
					break;
				default:
					Statement(stat);
					break;
				} // switch
			}
			tree = tree->third;
		} // while
	}

	void CodeGenerator::Statements(AST* tree)
	{
		assert(tree==NULL ||
			   Type(tree)==tSTATLIST ||
			   Type(tree)==tLBRACE);

		while (tree) {
			if (Type(tree)==tINVALID) {
				return;
			}
			if (tree->first) {
				Statement(tree->first);
			}
			tree = tree->third;
		}
	} // Statements

	void CodeGenerator::Block(AST* tree)
	{	// emit the code for tree, inside braces
		emitf("%s{\n", indent_str);
		if (tree) {
			indent();
			if (Type(tree)==tSTATLIST ||
				Type(tree)==tLBRACE) {
				Statements(tree);
			} else {
				Statement(tree);
			}
			dedent();
		}
		emitf("%s}\n", indent_str);
	} // Block

	void CodeGenerator::Statement(AST* tree)
	{
		if (!tree || Type(tree)==tINVALID) {
			return;
		}

		switch (Type(tree)) {
		case tSTATLIST:
		case tLBRACE:
			Block(tree);
			break;

		case tBREAK:
			// tree->first is label, if any
			if (tree->first) {
				// break the specified (labeled) loop
				emitf("%sgoto break_%s;\n", indent_str, tree->first->token.m_name);
			} else {
				// break the inner look
				emitf("%sbreak;\n", indent_str);
			}
			break;

		case tCOLON:
			// labeled statement  L: S
			// emit the statement:
			Statement(tree->second);
			// emit a break-label:
			emitf("%sbreak_%s:\n", indent_str, tree->first->token.m_name);
			break;

		case tCONTINUE:
			// tree->first is label, if any.
			if (tree->first) {
				// continue the specified loop
				// TODO: check that label actually applies to some surrounding statement.
				// TODO: emit the necessary label!
				emitf("%sgoto %s_continue;\n", indent_str, tree->first->token.m_name);
			} else {
				// continue the inner loop
				emitf("%continue;\n", indent_str);
			}
			break;

		case tFUNCTION:
			// Nested function definition!
			emitf("%s//nested function %s()...\n", indent_str, tree->first->token.m_name);
			break;

		case tFOR:
			ForLoop(tree);
			break;

		case tIF:
			emitf("%sif (", indent_str);
			ExprValue(tree->first);
			emit(") {\n");
			// emit the then part
			indent();
			Statement(tree->second);
			dedent();
			if (tree->third) {
				emitf("%s} else {\n", indent_str);
				indent();
				Statement(tree->third);
				dedent();
			}
			emitf("%s}\n", indent_str);
			break;

		case tRETURN:
			emitf("%sreturn ", indent_str);
			if (tree->first) {
				ExprValue(tree->first);
			} else {
				emit("undefined");
			}
			emit(";\n");
			break;

		case tSEMI:
			break;

		case tWHILE:
			emitf("%swhile (", indent_str);
			ExprValue(tree->first);
			emit(")\n");
			Block(tree->second);
			break;

		case tVAR:
			// Variable declarations are executable
			while (tree) {
				if (tree->first && tree->second) {
					// ident = expr
					emit(indent_str);
					RefExpr(tree->first);
					emit("=");
					ExprValue(tree->second);
					emit(";\n");
				}
				tree = tree->third;
			}
			break;

		default:
			// must be an expression
			emit(indent_str);
			ExprValue(tree);
			emit(";\n");
			break;
		} // switch
	} // Statement

	void CodeGenerator::ForLoop(AST* tree)
	{
		// there are two flavors of for - traditional C-style
		// and for (x in e)
		// tree->first is the iteration-controlling header
		// tree->second is the loop body
		{
			AST* h = tree->first;
			if (h->third) {
				// traditional C-style
				emitf("%sfor (", indent_str);
				ExprValue(h->first);
				emit(";");
				ExprValue(h->second);
				emit(";");
				ExprValue(h->third);
				emit(")\n");
			} else {
				// for (x in e)
			}
			Block(tree->second);
		}
	} // ForLoop

	void CodeGenerator::emitString(const char *s)
	{
		if (*s == '\"') {
			// quoted with double-quotes, compatible
			// with C++ string literals already.
			// TODO: completely?
			// A. Obviously not, since JS strings are UTF-16...
			emit(s);
		} else {
			// single-quoted
			assert(*s=='\'');
			char *buf = new char[1+strlen(s)*6], *t=buf;
			if (!buf) {
				throw std::bad_alloc();
			}
			*t++ = '\"'; s++;	// replace single with double quote at start.
			while (*s) {
				char ch = *s++;
				if (ch=='\\') {
					// escape sequence
					// pick up first char of sequence
					ch = *s++;
					switch (ch) {
					case '\'':
						// escaped single-quote, unescape it.
						*t++ = ch;
						break;
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
					case 'v':
					case '\\':
						// these are js standard, and are
						// C++ standards too
						*t++ = '\\';
						*t++ = ch;
						break;
					case 'x':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						// hex sequence or octal sequence,
						// copy thru and cross our fingers
						*t++ = '\\';
						*t++ = ch;
						break;
					case 'u':
						// unicode escape
						{
							int v;
							sscanf(s, "%x", &v);
							s += 4;
							sprintf(t, "\\x%04x", v);
							t += strlen(t);
						}
						break;
					default:
						// non-standard escape, replace
						// with just the char
						sprintf(t, "\\%03o", (unsigned)ch);
						t += strlen(t);
						break;
					} // switch
				} else {
					if (ch=='\"') {
						// naked double-quote
						// escape it
						*t++ = '\\';
					}
					*t++ = ch;
				}
			} // while
			// replace the closing single-quote
			// with a double-quote:
			t[-1] = '\"';
			t[0] = 0;
			emit(buf);
			delete[] buf;
		}
	} // emitString

} // namespace
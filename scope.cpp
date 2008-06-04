// scope.cpp - implementation of Scope class
//
#include "scope.h"
#include "log.h"
#include <assert.h>

namespace js2cpp {

	// Binding of name to definition
	AST* Binding::Definition(void) const
	{
		assert(type==BOUND_FUNCTION);
		return value;
	} // Definition

	void Binding::MakeExtern(void)
	{
		type=BOUND_EXTERN;
	}

	void Binding::MakeVar(AST* init)
	{
		type=BOUND_VARIABLE;
		value = init;
	}

	void Binding::MakeFunction(AST* fun)
	{
		type=BOUND_FUNCTION;
		value = fun;
	}

	void Binding::MakeReference(void)
	{
		type=BOUND_REFERENCE;
	}

	// Scope of names
	aScope::~aScope()
	{
	}

	void aScope::Bind(const char* name, Binding bind)
	{
		decls[name] = bind;
	}

	void aScope::DeclareVariable(const char* vname, AST* init)
	{
		// init is optional initializer
		// add a variable declaration to this scope.
		Log("%s declared in %s\n", vname, name);
		Binding binding;
		binding.MakeVar(init);
		Bind(vname, binding);
	}

	void aScope::DeclareFunction(const char* fname, AST* fun)
	{
		// add a literal function to this scope
		Log("function %s added to scope %s\n", fname, name);
		Binding binding;
		binding.MakeFunction(fun);
		Bind(fname, binding);
	}


	void aScope::DeclareLiteralFunction(AST* fun)
	{
		// add a literal function to this scope
		Log("literal function added to scope %s\n", name);
		litfuncs.insert(fun);
	}

	void aScope::Reference(const char* id)
	{
		Log("use: %s in %s\n", id, name);
		Binding binding;
		if (decls.find(id) == decls.end()) {
			// first reference to this name in this scope
			binding.MakeReference();
			decls[id] = binding;
		}
	} // Reference

	bool aScope::FindDeclaration(const char* id, Binding& decl, aScope* &owner)
	// Look up the declaration of a name in this scope-chain.
	// Returns true if a declaration is found, false otherwise.
	{
		assert(this);
		assert(name);
		Bindings::iterator ii = decls.find(id);
		if (ii != decls.end()) {
			decl = (*ii).second;
			if (decl.isDeclaration()) {
				owner = this;
				return true;
			}
		}
		if (!parent) {
			return false;
		}
		// Look in parent scope (Tail recursion!)
		return parent->FindDeclaration(id, decl, owner);
	}

	void aScope::Start(void)
	{
	}

	void aScope::End(void)
	// End of a scope, surround = surrounding scope, if any (otherwise NULL)
	{
		Log("ending scope %s\n", this->name);
		if (parent) {
			for (Bindings::iterator ii=decls.begin(); ii!=decls.end(); ++ii) {
				const char* id = (const char*)(*ii).first;
				Binding binding = (*ii).second;
				if (binding.isReference()) {
					// referenced but never defined in this scope,
					// export to surrounding scope
					Log("export ref: %s to %s\n", id, parent->Name());
					parent->Reference(id);
				}

			}
		}
	}

	aScope* aScope::AtDepth(int d)
	{
		assert(d <= depth);
		if (d==depth) {
			return this;
		}
		return parent->AtDepth(d);
	} // AtDepth

} // namespace
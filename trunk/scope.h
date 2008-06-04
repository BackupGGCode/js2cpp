// scope.h - the Scope class
//
#ifndef SCOPE_H
#define SCOPE_H

#include "AST.h"
#include <map>
#include <set>

namespace js2cpp
{

	class AST;

	class Binding
	{
	public:
		Binding() : type(BOUND_REFERENCE), value(NULL) {}
		
		typedef enum {
			BOUND_REFERENCE,
			BOUND_VARIABLE,
			BOUND_EXTERN,
			BOUND_FUNCTION,
		} Type;
	
		void MakeExtern(void);
		void MakeFunction(AST* fun);
		void MakeVar(AST* init);
		void MakeReference(void);

		bool isExtern(void) const { return type==BOUND_EXTERN; }
		bool isFunction(void) const { return type==BOUND_FUNCTION; }
		bool isVar(void) const { return type==BOUND_VARIABLE; }
		bool isDeclaration(void) const { return type !=BOUND_REFERENCE; }
		bool isReference(void) const { return type==BOUND_REFERENCE; }

		AST* Initialiser(void) const;		// variables only
		AST* Definition(void) const;		// functions only

	private:
		AST*			value;
		Type			type;
	};

	typedef std::map<const char*,Binding> Bindings;
	typedef std::set<AST*> TreeSet;

	class aScope
	{
	public:
		aScope(const char* s, aScope* up) : name(s), parent(up), depth(parent ? parent->depth+1 : 0) {}
		~aScope();

		void Start(void);
		void End(void);

		const char* Name(void) const { return name; }
		int Depth(void) const { return depth; }
		aScope* Parent(void) const { return parent; }
		aScope* AtDepth(int d);

		void Bind(const char* name, Binding bind);

		void DeclareExternal(const char* extname);
		void DeclareVariable(const char* varname, AST* init);
		void DeclareFunction(const char* fname, AST* fun);
		void DeclareLiteralFunction(AST* fun);

		void Reference(const char* id);

		bool FindDeclaration(const char* name, Binding& decl, aScope* &owner);
		// Look up the declaration of a name in this scope-chain.
		// Returns true if a declaration is found, false otherwise.

		Bindings& Declarations(void) { return decls; }
		TreeSet& LiteralFunctions(void) { return litfuncs; }

	private:
		aScope*		parent;		// containing scope
		int			depth;		// nesting depth (0=global)
		const char*	name;		// name of this scope (for errors & logging)
		Bindings	decls;		// identifiers declared in a scope
		TreeSet		litfuncs;	// literal functions
	}; // aScope

} // namespace

#endif

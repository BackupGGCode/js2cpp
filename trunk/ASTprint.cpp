// ASTprint.cpp
#pragma warning(disable:4786)

#include "ASTprint.h"
#include "AST.h"

namespace js2cpp {
//
//

	void TreePrint(FILE *f, AST *tree, int depth)
	{
tail_recursion:
		if (!tree) {
			fprintf(f, "%*cNULL", depth-1, ' ');
			return;
		}

		TokenType type = Type(tree);
		const char *name = Name(type);
		if (type >= tIDENT && type <= tREGEX) {
			fprintf(f, "%*c%s=%s\n", depth-1, ' ', name, Name(tree));
		} else {
			fprintf(f, "%*c%s\n", depth-1, ' ', name);
		}
		if (tree->first) {
			TreePrint(f, tree->first, depth+3);
		} else if (tree->second || tree->third) {
			fprintf(f, "%*cNULL\n", depth+3-1, ' ');
		}
		if (tree->second) {
			TreePrint(f, tree->second, depth+3);
		} else if (tree->third) {
			fprintf(f, "%*cNULL\n", depth+3-1, ' ');
		}
		if (tree->third) {
			if (Type(tree)==tSTATLIST) {
				tree = tree->third;
				goto tail_recursion;
			} else {
				TreePrint(f, tree->third, depth+3);
			}
		}
	} // TreePrint

} // namespace
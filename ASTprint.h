// ASTprint.h - AST printing

#ifndef ASTPRINT_H
#define ASTPRINT_H

#include <stdio.h>

namespace js2cpp {

class AST;

void TreePrint(FILE *f, AST *tree, int depth=0);

} // namespace

#endif


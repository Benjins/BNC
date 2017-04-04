#include <stdio.h>

#include "AST.cpp"
#include "semantics.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	String code = ReadStringFromFile("test1.bnc");

	AST ast;
	ast.ConstructFromString(code);

	FixUpOperators(&ast.nodes.Back());
	DisplayTree(&ast.nodes.Back());

	SemanticContext sc;
	DoSemantics(&ast, &sc);
	
	return 0;
}


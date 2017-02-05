#include <stdio.h>

#include "AST.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	
	//printf("Yo.");

	AST ast;
	ast.ConstructFromString("{ xc : int32 ; x = 2 * 3; }");

	//DisplayTree(&ast.nodes.Back());

	//printf("Fixing up ops...\n\n");

	FixUpOperators(&ast.nodes.Back());
	DisplayTree(&ast.nodes.Back());
	
	return 0;
}


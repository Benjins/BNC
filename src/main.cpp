#include <stdio.h>

#include "AST.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	
	//printf("Yo.");

	AST ast;
	ast.ConstructFromString("1 * (2 + 3) * 4 + 5;");

	DisplayTree(&ast.nodes.Back());

	printf("Fixing up ops...\n\n");

	FixUpOperators(&ast.nodes.Back());
	DisplayTree(&ast.nodes.Back());
	
	return 0;
}


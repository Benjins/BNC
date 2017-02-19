#include <stdio.h>

#include "AST.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	
	//printf("Yo.");

	AST ast;
	ast.ConstructFromString("{ 1 + a.b * c.d == 4 ; if 1 == 2 { print (5 * 6 + 7 * 8 == a * 5 + b * 6 + 3) ; } }");
	//ast.ConstructFromString("{ v : Ter ;}");

	//DisplayTree(&ast.nodes.Back());

	//printf("\n\n---Fixing up ops---\n\n");

	FixUpOperators(&ast.nodes.Back());

	DisplayTree(&ast.nodes.Back());
	
	return 0;
}


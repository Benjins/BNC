#include <stdio.h>

#include "AST.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	
	//printf("Yo.");

	String code = ReadStringFromFile("test1.bnc");

	AST ast;
	ast.ConstructFromString(code);
	//ast.ConstructFromString("{ v : Ter ;}");

	//DisplayTree(&ast.nodes.Back());

	DisplayTree(&ast.nodes.Back());

	printf("\n\n---Fixing up ops---\n\n");

	FixUpOperators(&ast.nodes.Back());
	printf("\n");
	DisplayTree(&ast.nodes.Back());
	
	return 0;
}


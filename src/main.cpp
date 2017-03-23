#include <stdio.h>

#include "AST.cpp"
#include "../CppUtils/vector.cpp"
#include "../CppUtils/assert.cpp"
#include "../CppUtils/strings.cpp"
#include "../CppUtils/lexer.cpp"

int main(int argc, char** argv){
	
	//printf("Yo.");

	AST ast;
	ast.ConstructFromString(
		"vec3 :: struct { x : float = 0; y : float = 0; z : float = 0; } "
		"vec2 :: struct {x : float; y : float;}"
		"pow :: (b : int, e : int = 2 * b + 1 * 4) -> int { return b * e; }");
	//ast.ConstructFromString("{ v : Ter ;}");

	//DisplayTree(&ast.nodes.Back());

	//printf("\n\n---Fixing up ops---\n\n");

	FixUpOperators(&ast.nodes.Back());

	DisplayTree(&ast.nodes.Back());
	
	return 0;
}


#ifndef BACKEND_H
#define BACKEND_H

#pragma once

#include "bytecode.h"
#include "AST.h"
#include "semantics.h"

bool CompileASTExpressionToByteCode(ASTNode* node, SemanticContext* sc, Vector<int>* outCode);

BNCBytecodeValue CompileTimeInterpretASTExpression(ASTNode* node, SemanticContext* sc);

void OutputASTToCCode(ASTNode* node, SemanticContext* sc, FILE* fileHandle, bool writeVarDeclInit = true);

#endif

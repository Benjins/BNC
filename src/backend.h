#ifndef BACKEND_H
#define BACKEND_H

#pragma once

#include "bytecode.h"
#include "AST.h"

bool CompileASTExpressionToByteCode(ASTNode* node, Vector<int>* outCode);

BNCBytecodeValue CompileTimeInterpretASTExpression(ASTNode* node);

void OutputASTToCCode(ASTNode* node, FILE* fileHandle);

#endif

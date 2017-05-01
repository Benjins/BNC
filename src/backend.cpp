#include "backend.h"

bool OutputStructDeclarations(SemanticContext* sc, AST* ast, FILE* fileHandle) {
	BNS_VEC_FOREACH(sc->definedStructs) {
		fprintf(fileHandle, "struct %.*s;\n", BNS_LEN_START(ptr->name));
	}

	Vector<StructDef> structsToDefine = sc->definedStructs;
	while (structsToDefine.count > 0) {
		bool madeProgress = false;
		BNS_VEC_FOREACH(structsToDefine) {
			bool canBeDefined = true;
			BNS_VEC_FOREACH_NAME(ptr->fieldDecls, fieldPtr) {
				TypeInfo fType = sc->knownTypes.data[fieldPtr->typeIndex];
				if (fType.type == TypeInfo::UE_StructTypeInfo) {
					BNS_VEC_FOREACH_NAME(structsToDefine, defPtr) {
						if (defPtr->idx == sc->definedStructs.data[fType.AsStructTypeInfo().index].idx) {
							canBeDefined = false;
							break;
						}
					}
				}

				if (!canBeDefined) { break; }
			}

			if (canBeDefined) {

				fprintf(fileHandle, "typedef struct {\n");
				BNS_VEC_FOREACH_NAME(ptr->fieldDecls, declPtr) {
					ASTNode* node = &ast->nodes.data[declPtr->idx];
					fprintf(fileHandle, "\t");
					OutputASTToCCode(node, sc, fileHandle, false);
					fprintf(fileHandle, ";\n");
				}
				fprintf(fileHandle, "} %.*s;\n", BNS_LEN_START(ptr->name));

				StructDef temp = *ptr;
				*ptr = structsToDefine.data[structsToDefine.count - 1];
				structsToDefine.data[structsToDefine.count - 1] = temp;
				structsToDefine.PopBack();
				ptr--;
				madeProgress = true;
			}
		}

		if (!madeProgress) {
			printf("Error, could not make progress in struct definition graph.\n");
			ASSERT(false);
			return false;
		}
	}

	return true;
}

void OutputASTToCCode(ASTNode* node, SemanticContext* sc, FILE* fileHandle, bool writeVarDeclInit /*= true*/) {
	ASSERT(node != nullptr);

	switch (node->type) {
	case ANT_IntegerLiteral: { fprintf(fileHandle, "%d", node->IntegerLiteral_value.val); } break;
	case ANT_FloatLiteral:   { fprintf(fileHandle, "%f", node->FloatLiteral_value.val); } break;
	case ANT_StringLiteral:  { fprintf(fileHandle, "\"%.*s\"", BNS_LEN_START(node->StringLiteral_value.repr)); } break;
	case ANT_BoolLiteral:    { fprintf(fileHandle, "%s", node->BoolLiteral_value.val ? "true" : "false"); } break;

	case ANT_TypeSimple: {
		OutputASTToCCode(&node->ast->nodes.data[node->TypeSimple_value.name], sc, fileHandle);
	} break;

	case ANT_TypePointer: {
		OutputASTToCCode(&node->ast->nodes.data[node->TypePointer_value.childType], sc, fileHandle);
		fprintf(fileHandle, "*");
	} break;

	case ANT_VariableDecl: {
		ASTNode* typeNode = &node->ast->nodes.data[node->VariableDecl_value.type];
		ASTNode* varNode  = &node->ast->nodes.data[node->VariableDecl_value.varName];
		ASTNode* initNode = nullptr;
		if (node->VariableDecl_value.initValue >= 0) {
			initNode = &node->ast->nodes.data[node->VariableDecl_value.initValue];
		}

		OutputASTToCCode(typeNode, sc, fileHandle);
		fprintf(fileHandle, " ");
		OutputASTToCCode(varNode, sc, fileHandle);

		if (writeVarDeclInit && initNode != nullptr) {
			fprintf(fileHandle, " = ");
			OutputASTToCCode(initNode, sc, fileHandle);
		}
	} break;

	case ANT_StructDefinition: {
		/*
		ASTNode* name = &node->ast->nodes.data[node->StructDefinition_value.structName];

		fprintf(fileHandle, "struct ");
		OutputASTToCCode(name, sc, fileHandle);
		fprintf(fileHandle, " {\n");

		BNS_VEC_FOREACH(node->StructDefinition_value.fieldDecls) {
			ASTNode* field = &node->ast->nodes.data[*ptr];
			OutputASTToCCode(field, sc, fileHandle);
			fprintf(fileHandle, ";\n");
		}

		fprintf(fileHandle, " };\n");
		*/
		// We already did this above.
	} break;

	case ANT_FunctionDefinition: {
		OutputASTToCCode(&node->ast->nodes.data[node->FunctionDefinition_value.returnType], sc, fileHandle);
		fprintf(fileHandle, " ");
		OutputASTToCCode(&node->ast->nodes.data[node->FunctionDefinition_value.name], sc, fileHandle);
		fprintf(fileHandle, "(");
		bool first = true;
		BNS_VEC_FOREACH(node->FunctionDefinition_value.params) {
			if (!first) {
				fprintf(fileHandle, ", ");
			}

			ASTNode* param = &node->ast->nodes.data[*ptr];
			OutputASTToCCode(param, sc, fileHandle);

			first = false;
		}
		fprintf(fileHandle, ")\n");

		ASTNode* body = &node->ast->nodes.data[node->FunctionDefinition_value.bodyScope];
		OutputASTToCCode(body, sc, fileHandle);
	} break;

	case ANT_ArrayAccess: { 
		ASTNode* arrNode = &node->ast->nodes.data[node->ArrayAccess_value.arr];
		ASTNode* idxNode = &node->ast->nodes.data[node->ArrayAccess_value.index];

		OutputASTToCCode(arrNode, sc, fileHandle);
		fprintf(fileHandle, "[");
		OutputASTToCCode(arrNode, sc, fileHandle);
		fprintf(fileHandle, "]");
	} break;

	case ANT_BinaryOp: {
		ASTNode* lNode = &node->ast->nodes.data[node->BinaryOp_value.left];
		ASTNode* rNode = &node->ast->nodes.data[node->BinaryOp_value.right];

		OutputASTToCCode(lNode, sc, fileHandle);
		fprintf(fileHandle, "%s", node->BinaryOp_value.op);
		OutputASTToCCode(rNode, sc, fileHandle);
	} break;

	case ANT_UnaryOp: {
		if (StrEqual(node->UnaryOp_value.op, "^")) {
			fprintf(fileHandle, node->UnaryOp_value.isPre ? "*" : "&");
		}
		else {
			fprintf(fileHandle, "%s", node->UnaryOp_value.op);
			OutputASTToCCode(&node->ast->nodes.data[node->UnaryOp_value.val], sc, fileHandle);
		}

		ASTNode* valnode = &node->ast->nodes.data[node->UnaryOp_value.val];
		OutputASTToCCode(valnode, sc, fileHandle);
	} break;

	case ANT_FunctionCall: {
		ASTNode* func = &node->ast->nodes.data[node->FunctionCall_value.func];
		OutputASTToCCode(func, sc, fileHandle);

		fprintf(fileHandle, "(");
		bool isFirst = true;
		BNS_VEC_FOREACH(node->FunctionCall_value.args) {
			ASTNode* arg = &node->ast->nodes.data[*ptr];
			if (!isFirst) {
				fprintf(fileHandle, ", ");
			}

			OutputASTToCCode(arg, sc, fileHandle);

			isFirst = false;
		}
		fprintf(fileHandle, ")");
	} break;

	case ANT_Identifier: {
		fprintf(fileHandle, "%.*s", BNS_LEN_START(node->Identifier_value.name));
	} break;

	case ANT_Statement: {
		ASTNode* stmt = &node->ast->nodes.data[node->Statement_value.root];
		OutputASTToCCode(stmt, sc, fileHandle);
		fprintf(fileHandle, ";\n");
	} break;

	case ANT_ReturnStatement: {
		fprintf(fileHandle, "return ");
		ASTNode* retVal = &node->ast->nodes.data[node->ReturnStatement_value.retVal];
		OutputASTToCCode(retVal, sc, fileHandle);
	} break;

	case ANT_Scope: {
		fprintf(fileHandle, "{\n");
		BNS_VEC_FOREACH(node->Scope_value.statements) {
			ASTNode* stmt = &node->ast->nodes.data[*ptr];
			OutputASTToCCode(stmt, sc, fileHandle);
		}
		fprintf(fileHandle, "}\n");
	} break;

	case ANT_Root: {
		OutputStructDeclarations(sc, node->ast, fileHandle);
		BNS_VEC_FOREACH(node->Root_value.topLevelStatements) {
			ASTNode* stmt = &node->ast->nodes.data[*ptr];
			OutputASTToCCode(stmt, sc, fileHandle);
		}
	} break;

	default: {
		ASSERT(false);
	} break;
	}
}

bool CompileASTExpressionToByteCode(ASTNode* node, SemanticContext* sc, Vector<int>* outCode) {
	switch (node->type) {
	case ANT_IntegerLiteral: {
		outCode->PushBack(BNCBI_IntLit);
		outCode->PushBack(node->IntegerLiteral_value.val);
	} break;

	case ANT_FloatLiteral: {
		outCode->PushBack(BNCBI_FloatLit);
		outCode->PushBack(*(int*)&node->FloatLiteral_value.val);
	} break;

	case ANT_BinaryOp: {
		ASTNode* left  = &node->ast->nodes.data[node->BinaryOp_value.left];
		ASTNode* right = &node->ast->nodes.data[node->BinaryOp_value.right];
		CompileASTExpressionToByteCode(left,  sc, outCode);
		CompileASTExpressionToByteCode(right, sc, outCode);

		if (StrEqual(node->BinaryOp_value.op, "+")) {
			outCode->PushBack(BNCBI_Add);
		}
		else if (StrEqual(node->BinaryOp_value.op, "-")) {
			outCode->PushBack(BNCBI_Sub);
		}
		else if (StrEqual(node->BinaryOp_value.op, "*")) {
			outCode->PushBack(BNCBI_Mul);
		}
		else if (StrEqual(node->BinaryOp_value.op, "/")) {
			outCode->PushBack(BNCBI_Div);
		}
		else {
			ASSERT(false);
		}
	} break;

	default: { ASSERT(false); return false; }
	}

	return true;
}

BNCBytecodeValue CompileTimeInterpretASTExpression(ASTNode* node, SemanticContext* sc) {
	Vector<int> code;
	CompileASTExpressionToByteCode(node, sc, &code);
	BNCBytecodeVMState state;
	return ExecuteBytecode(code.data, code.count, &state);
}


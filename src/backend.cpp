#include "backend.h"

bool CompileASTExpressionToByteCode(ASTNode* node, Vector<int>* outCode) {
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
		CompileASTExpressionToByteCode(left,  outCode);
		CompileASTExpressionToByteCode(right, outCode);

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

BNCBytecodeValue CompileTimeInterpretASTExpression(ASTNode* node) {
	Vector<int> code;
	CompileASTExpressionToByteCode(node, &code);
	BNCBytecodeVMState state;
	return ExecuteBytecode(code.data, code.count, &state);
}


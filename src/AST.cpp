#include "AST.h"
#include "../CppUtils/lexer.h"

ASTIndex ASTNode::GetIndex() {
	return this - ast->nodes.data;
}

void AST::ConstructFromString(const String& str) {
	Vector<SubString> toks = LexString(str);
	ConstructFromTokens(toks);
}

void AST::ConstructFromTokens(const Vector<SubString>& toks) {
	TokenStream stream;
	stream.toks = toks.data;
	stream.tokCount = toks.count;
	stream.ast = this;

	ParseTokenStream(&stream);
}

void ParseTokenStream(TokenStream* stream) {
	while (stream->index < stream->tokCount) {
		if (!ParseStatement(stream)) {
			break;
		}
	}
}

bool ParseStatement(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseValue(stream)) {
		if (stream->index < stream->tokCount) {
			if (ExpectAndEatWord(stream, ";")) {
				ASTIndex root = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_Statement;
				node->Statement_value.root = root;

				FRAME_SUCCES();
			}
		}
	}

	return false;
}

bool ParseIntLiteral(TokenStream* stream) {
	const SubString& tok = stream->CurrTok();
	bool isValid = true;
	for (int i = 0; i < tok.length; i++) {
		if (!IsNumeric(tok.start[i])) {
			isValid = false;
			break;
		}
	}

	if (isValid) {
		stream->index++;
		ASTNode* node = stream->ast->addNode();
		node->type = ANT_IntegerLiteral;
		node->IntegerLiteral_value.repr = tok;
		node->IntegerLiteral_value.val = Atoi(tok.start);
	}

	return isValid;
}

const char* binaryOperators[] = {
	"*", "-", "+", "/"
};

bool ParseBinaryOp(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseSingleValue(stream)) {
		ASTIndex left = stream->ast->GetCurrIdx();
		int opIdx = -1;
		if (ExpectAndEatOneOfWords(stream, binaryOperators, BNS_ARRAY_COUNT(binaryOperators), &opIdx)) {
			if (ParseValue(stream)) {
				ASTIndex right = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_BinaryOp;
				node->BinaryOp_value.op = binaryOperators[opIdx];
				node->BinaryOp_value.left = left;
				node->BinaryOp_value.right = right;

				FRAME_SUCCES();
			}
		}
	}

	return false;
}

bool ParseUnaryOp(TokenStream* stream) {
	return false;
}

bool ParseValue(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseFunctionCall(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseBinaryOp(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseSingleValue(stream)) {
		FRAME_SUCCES();
	}
	else {
		if (ExpectAndEatWord(stream, "(")) {
			if (ParseValue(stream)) {
				if (ExpectAndEatWord(stream, ")")) {
					FRAME_SUCCES();
				}
			}
		}
	}

	return false;
}

bool ParseSingleValue(TokenStream* stream) {
	if (ParseIdentifier(stream)) {
		return true;
	}
	else if (ParseIntLiteral(stream)) {
		return true;
	}

	return false;
}

bool ParseFunctionCall(TokenStream* stream) {
	return false;
}

bool ExpectAndEatWord(TokenStream* stream, const char* str) {
	if (stream->index >= stream->tokCount) {
		return false;
	}

	if (stream->CurrTok() == str) {
		stream->index++;
		return true;
	}

	return false;
}

bool ExpectAndEatOneOfWords(TokenStream* stream, const char** strs, const int count, int* outIdx) {
	if (stream->index >= stream->tokCount - 1) {
		return false;
	}

	for (int i = 0; i < count; i++) {
		if (stream->CurrTok() == strs[i]) {
			*outIdx = i;
			stream->index++;
			return true;
		}
	}

	return false;
}

bool ParseIdentifier(TokenStream* stream) {
	if (stream->index >= stream->tokCount - 1) {
		return false;
	}

	const SubString& tok = stream->CurrTok();

	if (*tok.start == '_' || IsAlpha(*tok.start)) {
		bool isValid = true;
		for (int i = 1; i < tok.length; i++) {
			char c = tok.start[i];
			if (!IsAlpha(c) && !IsNumeric(c) && c != '_') {
				isValid = false;
				break;
			}
		}

		if (isValid) {
			stream->index++;

			ASTNode* node = stream->ast->addNode();
			node->type = ANT_Identifier;
			node->Identifier_value.name = tok;

			return true;
		}
	}

	return false;
}

void DisplayTree(ASTNode* node, int indentation /*= 0*/) {
#define INDENT(x) for (int i = 0; i < x; i++) {printf("    ");}
	switch (node->type) {
	case ANT_BinaryOp: {
		INDENT(indentation);
		printf("Binary Op: '%s'\n", node->BinaryOp_value.op);
		DisplayTree(&node->ast->nodes.data[node->BinaryOp_value.left], indentation + 1);
		DisplayTree(&node->ast->nodes.data[node->BinaryOp_value.right], indentation + 1);
	} break;

	case ANT_IntegerLiteral: {
		INDENT(indentation);
		printf("Int lit: %d\n", node->IntegerLiteral_value.val);
	} break;

	case ANT_Statement: {
		INDENT(indentation);
		printf("Statement:\n");
		ASTNode* root = &node->ast->nodes.data[node->Statement_value.root];
		DisplayTree(root, indentation);
	} break;

	default: {
		printf("Something went wrong...\n");
		ASSERT(false);
	} break;
	}
}

const BinaryOperator* GetInfoForOp(const char* op) {
	for (int i = 0; i < BNS_ARRAY_COUNT(binOpInfo); i++) {
		if (StrEqual(op, binOpInfo[i].op)) {
			return &binOpInfo[i];
		}
	}

	return nullptr;
}

void SwapBinaryOperators(ASTNode* parent, ASTNode* child) {
	ASSERT(parent->type == ANT_BinaryOp);
	ASSERT(child->type == ANT_BinaryOp);

	ASTIndex childIdx = child->GetIndex();
	if (parent->BinaryOp_value.left == childIdx) {
		// Actually, not sure if this happens?
		ASSERT(false);
	}
	else if (parent->BinaryOp_value.right == childIdx) {
		{
			AST_BinaryOp tmp = parent->BinaryOp_value;
			parent->BinaryOp_value = child->BinaryOp_value;
			child->BinaryOp_value = tmp;
		}

		ASTIndex parentLeft = parent->BinaryOp_value.left;
		parent->BinaryOp_value.left = child->GetIndex();
		child->BinaryOp_value.right = parentLeft;
	}
	else {
		ASSERT(false);
	}
}

void FixUpOperators(ASTNode* node) {
	switch (node->type) {
	case ANT_BinaryOp: {
		ASTNode* left = &node->ast->nodes.data[node->BinaryOp_value.left];
		ASTNode* right = &node->ast->nodes.data[node->BinaryOp_value.right];

		const BinaryOperator* opInfo = GetInfoForOp(node->BinaryOp_value.op);

		int idx = node->GetIndex();

		//printf("Before (%d):\n", idx);
		//DisplayTree(right);

		//FixUpOperators(left);
		//FixUpOperators(right);

		//printf("After(%d):\n", idx);
		//DisplayTree(right);

		if (left->type == ANT_BinaryOp) {
			// If the precedence is higher
			const BinaryOperator* leftInfo = GetInfoForOp(left->BinaryOp_value.op);
			if (opInfo->precedence < leftInfo->precedence) {
				SwapBinaryOperators(node, left);
				FixUpOperators(node);
			}
			else {
				FixUpOperators(left);
			}
		}

		if (right->type == ANT_BinaryOp) {
			// If the precedence is higher, or its equal and left-assoc (so most)
			const BinaryOperator* rightInfo = GetInfoForOp(right->BinaryOp_value.op);
			if (opInfo->precedence < rightInfo->precedence
				|| (opInfo->precedence == rightInfo->precedence && opInfo->assoc == OA_Left)) {
				SwapBinaryOperators(node, right);
				FixUpOperators(node);
			}
			else {
				FixUpOperators(right);
			}
		}
	} break;

	case ANT_Statement: {
		ASTNode* root = &node->ast->nodes.data[node->Statement_value.root];
		FixUpOperators(root);
	}

	default: {
	} break;
	}
}



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
	bool needsSemicolon = true;
	if (ParseVariableAssign(stream)) {
	}
	else if (ParseVariableDecl(stream)) {

	}
	else if (ParseValue(stream)) {
	}
	else if (ParseScope(stream)) {
		needsSemicolon = false;
	}
	else {
		return false;
	}

	if (!needsSemicolon) {
		FRAME_SUCCES();
	}
	else if (stream->index < stream->tokCount) {
		if (ExpectAndEatWord(stream, ";")) {
			ASTIndex root = stream->ast->GetCurrIdx();

			ASTNode* node = stream->ast->addNode();
			node->type = ANT_Statement;
			node->Statement_value.root = root;

			FRAME_SUCCES();
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
	"*", "-", "+", "/", "."
};

static_assert(BNS_ARRAY_COUNT(binaryOperators) == BNS_ARRAY_COUNT(binOpInfo), "Binary operator info");

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

const char* unaryOperators[] = {
	"!", "-", "^", "&"
};

bool ParseUnaryOp(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	int opIdx = -1;
	if (ExpectAndEatOneOfWords(stream, unaryOperators, BNS_ARRAY_COUNT(unaryOperators), &opIdx)) {
		if (ParseSingleValue(stream)) {
			ASTIndex val = stream->ast->GetCurrIdx();

			ASTNode* node = stream->ast->addNode();
			node->type = ANT_UnaryOp;
			node->UnaryOp_value.op = unaryOperators[opIdx];
			node->UnaryOp_value.val = val;

			FRAME_SUCCES();
		}
	}

	return false;
}

bool ParseValue(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseBinaryOp(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseSingleValue(stream)) {
		FRAME_SUCCES();
	}

	return false;
}

bool ParseSingleValue(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseIntLiteral(stream)) {
		FRAME_SUCCES();
	}
	else if (ExpectAndEatWord(stream, "(")) {
		if (ParseValue(stream)) {
			if (ExpectAndEatWord(stream, ")")) {
				ASTIndex idx = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_Parentheses;
				node->Parentheses_value.val = idx;

				FRAME_SUCCES();
			}
		}
	}
	else if (ParseUnaryOp(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseFunctionCall(stream)){
		FRAME_SUCCES();
	}
	else if (ParseIdentifier(stream)) {
		FRAME_SUCCES();
	}

	return false;
}

bool ParseType(TokenStream* stream) {
	if (ParseIdentifier(stream)) {
		ASTIndex identIdx = stream->ast->GetCurrIdx();

		ASTNode* node = stream->ast->addNode();
		node->type = ANT_TypeSimple;
		node->TypeSimple_value.name = identIdx;

		return true;
	}

	return false;
}

bool ParseGenericType(TokenStream* stream) {
	return false;
}

bool ParseArrayType(TokenStream* stream) {
	return false;
}

bool ParsePointerType(TokenStream* stream) {
	return false;
}

bool ParseFunctionCall(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseIdentifier(stream)) {
		ASTIndex callIdx = stream->ast->GetCurrIdx();
		if (ExpectAndEatWord(stream, "(")) {
			Vector<ASTIndex> argIndices;
			while (true) {
				if (ParseValue(stream)) {
					argIndices.PushBack(stream->ast->GetCurrIdx());

					if (ExpectAndEatWord(stream, ",")) {
						// Do nothing I guess?
					}
					else if (CheckNextWord(stream, ")")) {
						break;
					}
					else {
						return false;
					}
				}
				else {
					break;
				}
			}

			if (ExpectAndEatWord(stream, ")")) {
				ASTNode* node = stream->ast->addNode();
				node->type = ANT_FunctionCall;
				node->FunctionCall_value.func = callIdx;
				node->FunctionCall_value.args = argIndices;

				FRAME_SUCCES();
			}
		}
		else {
			// We got a var or something
			// TODO: Other checks to make?
			FRAME_SUCCES();
		}
	}

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

bool CheckNextWord(TokenStream* stream, const char* str) {
	if (stream->index >= stream->tokCount) {
		return false;
	}

	if (stream->CurrTok() == str) {
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

const char* reservedWords[] = {
	"if",
	"while"
};

bool IsReservedWord(const SubString& tok) {
	for (int i = 0; i < BNS_ARRAY_COUNT(reservedWords); i++) {
		if (tok == reservedWords[i]) {
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
			if (!IsReservedWord(tok)) {
				stream->index++;

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_Identifier;
				node->Identifier_value.name = tok;

				return true;
			}
		}
	}

	return false;
}

bool ParseVariableDecl(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseIdentifier(stream)) {
		ASTIndex varIdx = stream->ast->GetCurrIdx();
		if (ExpectAndEatWord(stream, ":")) {
			if (ParseType(stream)) {
				ASTIndex typeIdx = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_VariableDecl;
				node->VariableDecl_value.varName = varIdx;
				node->VariableDecl_value.type = typeIdx;

				FRAME_SUCCES();
			}
		}
	}

	return false;
}

bool ParseScope(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ExpectAndEatWord(stream, "{")) {
		Vector<ASTIndex> statements;
		while (true) {
			if (ParseStatement(stream)) {
				ASTIndex stmtIdx = stream->ast->GetCurrIdx();
				statements.PushBack(stmtIdx);
			}
			else if (CheckNextWord(stream, "}")) {
				break;
			}
			else {
				return false;
			}
		}

		if (ExpectAndEatWord(stream, "}")) {
			ASTNode* node = stream->ast->addNode();
			node->type = ANT_Scope;
			node->Scope_value.statements = statements;

			FRAME_SUCCES();
		}
	}

	return false;
}

bool ParseVariableAssign(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ParseValue(stream)) {
		ASTIndex varIdx = stream->ast->GetCurrIdx();
		if (ExpectAndEatWord(stream, "=")) {
			if (ParseValue(stream)) {
				ASTIndex valIdx = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_VariableAssign;
				node->VariableAssign_value.val = valIdx;
				node->VariableAssign_value.var = varIdx;

				FRAME_SUCCES();
			}
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

	case ANT_Parentheses: {
		ASTNode* val = &node->ast->nodes.data[node->Parentheses_value.val];
		DisplayTree(val, indentation);
	} break;

	case ANT_FunctionCall: {
		INDENT(indentation);
		ASTNode* call = &node->ast->nodes.data[node->FunctionCall_value.func];
		printf("Calling func '%.*s'\n", BNS_LEN_START(call->Identifier_value.name));
		for (int i = 0; i < node->FunctionCall_value.args.count; i++) {
			ASTIndex argIdx = node->FunctionCall_value.args.data[i];
			ASTNode* arg = &node->ast->nodes.data[argIdx];

			INDENT(indentation);
			printf("Arg %d: \n", i);

			DisplayTree(arg, indentation + 1);
		}
	} break;

	case ANT_UnaryOp: {
		INDENT(indentation);
		printf("Unary Op: '%s'\n", node->UnaryOp_value.op);

		ASTNode* val = &node->ast->nodes.data[node->UnaryOp_value.val];
		DisplayTree(val, indentation + 1);
	} break;

	case ANT_VariableAssign: {
		ASTNode* var = &node->ast->nodes.data[node->VariableAssign_value.var];

		INDENT(indentation);
		printf("Assign to var: '%.*s'\n", BNS_LEN_START(var->Identifier_value.name));

		ASTNode* val = &node->ast->nodes.data[node->VariableAssign_value.val];
		DisplayTree(val, indentation + 1);
	} break;

	case ANT_VariableDecl: {
		ASTNode* varName = &node->ast->nodes.data[node->VariableDecl_value.varName];

		INDENT(indentation);
		printf("Declaring var: '%.*s'\n", BNS_LEN_START(varName->Identifier_value.name));
		INDENT(indentation);
		printf("With type\n");

		ASTNode* type = &node->ast->nodes.data[node->VariableDecl_value.type];
		DisplayTree(type, indentation + 1);
	} break;

	case ANT_FieldAccess: {
		INDENT(indentation);
		printf("Field access:\n");

		ASTNode* val   = &node->ast->nodes.data[node->FieldAccess_value.val];
		ASTNode* field = &node->ast->nodes.data[node->FieldAccess_value.field];

		INDENT(indentation);
		printf("Accessing '%.*s'\n", BNS_LEN_START(field->Identifier_value.name));

		DisplayTree(val, indentation + 1);
	} break;

	case ANT_Identifier: {
		INDENT(indentation);
		printf("Identifier '%.*s'\n", BNS_LEN_START(node->Identifier_value.name));
	} break;

	case ANT_Scope: {
		INDENT(indentation);
		printf("Scope.\n");

		for (int i = 0; i < node->Scope_value.statements.count; i++) {
			ASTIndex statementIdx = node->Scope_value.statements.data[i];
			ASTNode* stmt = &node->ast->nodes.data[statementIdx];

			DisplayTree(stmt, indentation + 1);
		}
	} break;

	case ANT_TypeSimple: {
		ASTNode* type = &node->ast->nodes.data[node->TypeSimple_value.name];
		INDENT(indentation);
		printf("Type '%.*s'\n", BNS_LEN_START(type->Identifier_value.name));
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
		else if (left->type == ANT_UnaryOp) {
			if (opInfo->precedence < unaryOpPrecedence) {
				AST_BinaryOp tmp = node->BinaryOp_value;
				node->UnaryOp_value = left->UnaryOp_value;
				left->BinaryOp_value = tmp;
				
				left->BinaryOp_value.left = node->UnaryOp_value.val;
				node->UnaryOp_value.val = left->GetIndex();

				node->type = ANT_UnaryOp;
				left->type = ANT_BinaryOp;

				node = left;
			}
		}

		if (right->type == ANT_BinaryOp) {
			// If the precedence is higher, or its equal and left-assoc (so most)
			const BinaryOperator* rightInfo = GetInfoForOp(right->BinaryOp_value.op);
			if (opInfo->precedence < rightInfo->precedence) {
				SwapBinaryOperators(node, right);
				FixUpOperators(node);
			}
			else if (opInfo->precedence == rightInfo->precedence && opInfo->assoc == OA_Left) {
				SwapBinaryOperators(node, right);
				FixUpOperators(right);
			}
			else {
				FixUpOperators(right);
			}
		}
	} break;

	case ANT_Statement: {
		ASTNode* root = &node->ast->nodes.data[node->Statement_value.root];
		FixUpOperators(root);
	} break;

	case ANT_Parentheses: {
		ASTNode* val = &node->ast->nodes.data[node->Parentheses_value.val];
		FixUpOperators(val);
	} break;

	case ANT_UnaryOp: {
		ASTNode* val = &node->ast->nodes.data[node->UnaryOp_value.val];
		FixUpOperators(val);
	} break;

	case ANT_FieldAccess: {
		ASTNode* val = &node->ast->nodes.data[node->FieldAccess_value.val];
		FixUpOperators(val);
	} break;

	case ANT_VariableAssign: {
		ASTNode* val = &node->ast->nodes.data[node->VariableAssign_value.val];
		FixUpOperators(val);
	} break;

	case ANT_Scope: {
		for (int i = 0; i < node->Scope_value.statements.count; i++) {
			ASTIndex statementIdx = node->Scope_value.statements.data[i];
			ASTNode* stmt = &node->ast->nodes.data[statementIdx];

			FixUpOperators(stmt);
		}
	} break;

	case ANT_FunctionCall: {
		for (int i = 0; i < node->FunctionCall_value.args.count; i++) {
			ASTIndex argIdx = node->FunctionCall_value.args.data[i];
			ASTNode* arg = &node->ast->nodes.data[argIdx];

			FixUpOperators(arg);
		}
	} break;

	default: {
	} break;
	}
}


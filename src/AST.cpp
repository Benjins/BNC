#include "AST.h"
#include "../CppUtils/lexer.h"

const BinaryOperator* GetBinaryInfoForOp(const char* op) {
	for (int i = 0; i < BNS_ARRAY_COUNT(binOpInfo); i++) {
		if (StrEqual(op, binOpInfo[i].op)) {
			return &binOpInfo[i];
		}
	}

	return nullptr;
}

const UnaryOperator* GetUnaryInfoForOp(const char* op) {
	for (int i = 0; i < BNS_ARRAY_COUNT(unOpInfo); i++) {
		if (StrEqual(op, unOpInfo[i].op)) {
			return &unOpInfo[i];
		}
	}

	return nullptr;
}

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

bool ParseTokenStream(TokenStream* stream) {
	Vector<ASTIndex> topLevelStatements;
	while (stream->index < stream->tokCount) {
		if (!ParseTopLevelStatement(stream)) {
			break;
		}
		else {
			topLevelStatements.PushBack(stream->ast->GetCurrIdx());
		}
	}

	if (stream->index == stream->tokCount) {
		ASTNode* node = stream->ast->addNode();
		node->type = ANT_Root;
		node->Root_value.topLevelStatements = topLevelStatements;

		return true;
	}
	else {
		return false;
	}
}

bool ParseTopLevelStatement(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ParseStructDefinition(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseFunctionDefinition(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseStatement(stream)) {
		FRAME_SUCCES();
	}
	else {
		return false;
	}

}

bool ParseStatement(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	bool needsSemicolon = true;
	if (ParseVariableAssign(stream)) {
	}
	else if (ParseVariableDecl(stream)) {

	}
	else if (ParseIfStatement(stream)) {
		needsSemicolon = false;
	}
	else if (ParseScope(stream)) {
		needsSemicolon = false;
	}
	else if (ParseReturnStatement(stream)) {
		// TODO: Should this require a semi-colon in the function, or back here?
	}
	else if (ParseValue(stream)) {

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

bool ParseStringLiteral(TokenStream* stream) {
	const SubString& tok = stream->CurrTok();
	if (*tok.start == '"') {
		stream->index++;
		ASTNode* node = stream->ast->addNode();
		node->type = ANT_StringLiteral;
		node->StringLiteral_value.repr = tok;

		return true;
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
	PUSH_STREAM_FRAME(stream);

	int opIdx = -1;
	if (ExpectAndEatOneOfWords(stream, unaryOperators, BNS_ARRAY_COUNT(unaryOperators), &opIdx)) {
		if (const UnaryOperator* op = GetUnaryInfoForOp(unaryOperators[opIdx])) {
			if (op->pos & UOP_Pre) {
				if (ParseSingleValue(stream)) {
					ASTIndex val = stream->ast->GetCurrIdx();

					ASTNode* node = stream->ast->addNode();
					node->type = ANT_UnaryOp;
					node->UnaryOp_value.op = unaryOperators[opIdx];
					node->UnaryOp_value.val = val;
					node->UnaryOp_value.isPre = true;

					FRAME_SUCCES();
				}
			}
		}
	}
	else if (ParseSingleValueNoUnary(stream)) {
		ASTIndex val = stream->ast->GetCurrIdx();
		if (ExpectAndEatOneOfWords(stream, unaryOperators, BNS_ARRAY_COUNT(unaryOperators), &opIdx)) {
			if (const UnaryOperator* op = GetUnaryInfoForOp(unaryOperators[opIdx])) {
				if (op->pos & UOP_Post) {
					ASTNode* node = stream->ast->addNode();
					node->type = ANT_UnaryOp;
					node->UnaryOp_value.op = unaryOperators[opIdx];
					node->UnaryOp_value.val = val;

					FRAME_SUCCES();
				}
			}
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

bool ParseSingleValueCommon(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseIntLiteral(stream)) {
		FRAME_SUCCES();
	}
	if (ParseStringLiteral(stream)) {
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

	return false;
}

bool ParseSingleValueNoUnary(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseSingleValueCommon(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseFunctionCall(stream)) {
		FRAME_SUCCES();
	}
	else if (ParseIdentifier(stream)) {
		FRAME_SUCCES();
	}

	return false;
}

bool ParseSingleValue(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);
	if (ParseSingleValueCommon(stream)) {
		FRAME_SUCCES();
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
	//if (ParseFunctionCall(stream)) {
	//	ANT_TypeGeneric
	//	stream->ast->nodes;
	//}
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

				if (ExpectAndEatWord(stream, "=")) {
					if (ParseValue(stream)) {
						ASTIndex valIdx = stream->ast->GetCurrIdx();

						ASTNode* node = stream->ast->addNode();
						node->type = ANT_VariableDecl;
						node->VariableDecl_value.varName = varIdx;
						node->VariableDecl_value.type = typeIdx;
						node->VariableDecl_value.initValue = valIdx;

						FRAME_SUCCES();
					}
				}
				else {

					ASTNode* node = stream->ast->addNode();
					node->type = ANT_VariableDecl;
					node->VariableDecl_value.varName = varIdx;
					node->VariableDecl_value.type = typeIdx;
					node->VariableDecl_value.initValue = -1;

					FRAME_SUCCES();
				}
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

bool ParseIfStatement(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ExpectAndEatWord(stream, "if")) {
		if (ParseValue(stream)) {
			ASTIndex condIdx = stream->ast->GetCurrIdx();
			if (ParseScope(stream)) {
				ASTIndex scopeIdx = stream->ast->GetCurrIdx();

				ASTNode* node = stream->ast->addNode();
				node->type = ANT_IfStatement;
				node->IfStatement_value.condition = condIdx;
				node->IfStatement_value.bodyScope = scopeIdx;

				FRAME_SUCCES();
			}
		}
	}

	return false;
}

bool ParseReturnStatement(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ExpectAndEatWord(stream, "return")) {
		if (ParseValue(stream)) {
			ASTIndex valIdx = stream->ast->GetCurrIdx();
			ASTNode* node = stream->ast->addNode();
			node->type = ANT_ReturnStatement;
			node->ReturnStatement_value.retVal = valIdx;

			FRAME_SUCCES();
		}
	}

	return false;
}

bool ParseStructDefinition(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ParseIdentifier(stream)) {
		ASTIndex structNameIdx = stream->ast->GetCurrIdx();
		Vector<ASTIndex> fieldIndices;
		if (ExpectAndEatWord(stream, "::")) {
			if (ExpectAndEatWord(stream, "struct")) {
				if (ExpectAndEatWord(stream, "{")) {
					while (true) {
						if (ParseVariableDecl(stream)) {
							if (ExpectAndEatWord(stream, ";")) {
								fieldIndices.PushBack(stream->ast->GetCurrIdx());
							}
						}
						else if (ExpectAndEatWord(stream, "}")) {
							break;
						}
						else {
							return false;
						}
					}

					ASTNode* node = stream->ast->addNode();
					node->type = ANT_StructDefinition;
					node->StructDefinition_value.structName = structNameIdx;
					node->StructDefinition_value.fieldDecls = fieldIndices;

					FRAME_SUCCES();
				}
			}
		}
	}

	return false;
}

bool ParseFunctionDefinition(TokenStream* stream) {
	PUSH_STREAM_FRAME(stream);

	if (ParseIdentifier(stream)) {
		ASTIndex funcNameIdx = stream->ast->GetCurrIdx();
		if (ExpectAndEatWord(stream, "::")) {
			if (ExpectAndEatWord(stream, "(")) {
				Vector<ASTIndex> parameters;
				bool success = true;
				while (true) {
					if (parameters.count == 0) {
						if (ParseVariableDecl(stream)) {
							parameters.PushBack(stream->ast->GetCurrIdx());
						}
						else if (CheckNextWord(stream, ")")) {
							break;
						}
						else {
							success = false;
							break;
						}
					}
					else if (ExpectAndEatWord(stream, ",")){
						if (ParseVariableDecl(stream)) {
							parameters.PushBack(stream->ast->GetCurrIdx());
						}
						else {
							success = false;
							break;
						}
					}
					else if (CheckNextWord(stream, ")")) {
						break;
					}
					else {
						success = false;
						break;
					}
				}

				if (success) {
					if (ExpectAndEatWord(stream, ")")) {
						if (ExpectAndEatWord(stream, "->")) {
							if (ParseType(stream)) {
								ASTIndex retType = stream->ast->GetCurrIdx();

								if (ParseScope(stream)) {
									ASTIndex bodyScope = stream->ast->GetCurrIdx();

									ASTNode* node = stream->ast->addNode();
									node->type = ANT_FunctionDefinition;
									node->FunctionDefinition_value.name = funcNameIdx;
									node->FunctionDefinition_value.params = parameters;
									node->FunctionDefinition_value.returnType = retType;
									node->FunctionDefinition_value.bodyScope = bodyScope;

									FRAME_SUCCES();
								}
							}
						}
					}
				}
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

	case ANT_StringLiteral: {
		INDENT(indentation);
		printf("String lit:%.*s\n", BNS_LEN_START(node->StringLiteral_value.repr));
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
		printf("Unary Op: '%s' (%s)\n", node->UnaryOp_value.op, (node->UnaryOp_value.isPre ? "pre" : "post"));

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

		if (node->VariableDecl_value.initValue != -1) {
			ASTNode* initVal = &node->ast->nodes.data[node->VariableDecl_value.initValue];

			INDENT(indentation);
			printf("Initial Value:\n");
			DisplayTree(initVal, indentation + 1);
		}
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

	case ANT_IfStatement: {
		INDENT(indentation);
		printf("If statement.\n");
		INDENT(indentation);
		printf("Condition:\n");

		{
			ASTIndex condIndex = node->IfStatement_value.condition;
			ASTNode* cond = &node->ast->nodes.data[condIndex];

			DisplayTree(cond, indentation + 1);
		}

		INDENT(indentation);
		printf("Body:\n");
		{
			ASTIndex bodyIndex = node->IfStatement_value.bodyScope;
			ASTNode* body = &node->ast->nodes.data[bodyIndex];

			DisplayTree(body, indentation + 1);
		}

	} break;

	case ANT_TypeSimple: {
		ASTNode* type = &node->ast->nodes.data[node->TypeSimple_value.name];
		INDENT(indentation);
		printf("Type '%.*s'\n", BNS_LEN_START(type->Identifier_value.name));
	} break;

	case ANT_StructDefinition: {
		{
			ASTNode* name = &node->ast->nodes.data[node->StructDefinition_value.structName];
			INDENT(indentation);
			printf("Struct:\n");
			DisplayTree(name, indentation + 1);
		}

		INDENT(indentation);
		printf("Fields:\n");
		for (int i = 0; i < node->StructDefinition_value.fieldDecls.count; i++) {
			ASTNode* field = &node->ast->nodes.data[node->StructDefinition_value.fieldDecls.data[i]];
			DisplayTree(field, indentation + 1);
		}
	} break;

	case ANT_FunctionDefinition: {
		INDENT(indentation);
		printf("Function:\n");
		{
			ASTNode* name = &node->ast->nodes.data[node->FunctionDefinition_value.name];
			DisplayTree(name, indentation + 1);
		}

		INDENT(indentation);
		printf("Parameters:\n");
		for (int i = 0; i < node->FunctionDefinition_value.params.count; i++) {
			ASTIndex paramIdx = node->FunctionDefinition_value.params.data[i];
			ASTNode* param = &node->ast->nodes.data[paramIdx];
			DisplayTree(param, indentation + 1);
		}

		INDENT(indentation);
		printf("Returns:\n");
		{
			ASTNode* retType = &node->ast->nodes.data[node->FunctionDefinition_value.returnType];
			DisplayTree(retType, indentation + 1);
		}

		INDENT(indentation);
		printf("Body:\n");
		{
			ASTNode* body = &node->ast->nodes.data[node->FunctionDefinition_value.bodyScope];
			DisplayTree(body, indentation + 1);
		}

	} break;

	case ANT_Root: {
		for (int i = 0; i < node->Root_value.topLevelStatements.count; i++) {
			ASTNode* stmt = &node->ast->nodes.data[node->Root_value.topLevelStatements.data[i]];
			DisplayTree(stmt, indentation);
		}
	} break;

	case ANT_ReturnStatement: {
		INDENT(indentation);
		printf("Return:\n");
		ASTNode* retVal = &node->ast->nodes.data[node->ReturnStatement_value.retVal];
		DisplayTree(retVal, indentation + 1);
	} break;

	default: {
		printf("Something went wrong...\n");
		ASSERT(false);
	} break;
	}
}

void SwapBinaryOperators(ASTNode* parent, ASTNode* child) {
	ASSERT(parent->type == ANT_BinaryOp);
	ASSERT(child->type == ANT_BinaryOp);

	ASTIndex childIdx = child->GetIndex();
	if (parent->BinaryOp_value.left == childIdx) {
		{
			AST_BinaryOp tmp = parent->BinaryOp_value;
			parent->BinaryOp_value = child->BinaryOp_value;
			child->BinaryOp_value = tmp;
		}

		ASTIndex parentRight = parent->BinaryOp_value.right;
		parent->BinaryOp_value.right = child->GetIndex();
		child->BinaryOp_value.left = parentRight;
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

void FixUpOperators(ASTNode* node, ASTNode* root = nullptr) {
	switch (node->type) {
	case ANT_BinaryOp: {
		ASTNode* left = &node->ast->nodes.data[node->BinaryOp_value.left];
		ASTNode* right = &node->ast->nodes.data[node->BinaryOp_value.right];

		FixUpOperators(left);
		FixUpOperators(right);

		const BinaryOperator* opInfo = GetBinaryInfoForOp(node->BinaryOp_value.op);

		int idx = node->GetIndex();

		bool didFixup = false;

		if (left->type == ANT_BinaryOp) {
			// If the precedence is higher
			const BinaryOperator* leftInfo = GetBinaryInfoForOp(left->BinaryOp_value.op);
			if (opInfo->precedence < leftInfo->precedence) {
				SwapBinaryOperators(node, left);
				FixUpOperators(node, root);
				didFixup = true;
			}
			else {
				FixUpOperators(left, root);
			}
		}
		else if (left->type == ANT_UnaryOp && left->UnaryOp_value.isPre) {
			if (opInfo->precedence < unaryOpPrecedence) {
				AST_BinaryOp tmp = node->BinaryOp_value;
				node->UnaryOp_value = left->UnaryOp_value;
				left->BinaryOp_value = tmp;
				
				left->BinaryOp_value.left = node->UnaryOp_value.val;
				node->UnaryOp_value.val = left->GetIndex();

				node->type = ANT_UnaryOp;
				left->type = ANT_BinaryOp;

				didFixup = true;
				node = left;
				opInfo = GetBinaryInfoForOp(node->BinaryOp_value.op);
			}
		}

		if (right->type == ANT_BinaryOp) {
			// If the precedence is higher, or its equal and left-assoc (so most)
			const BinaryOperator* rightInfo = GetBinaryInfoForOp(right->BinaryOp_value.op);
			if (opInfo->precedence < rightInfo->precedence) {
				ASTIndex oldNodeIdx = idx;
				SwapBinaryOperators(node, right);
				ASTNode* oldNode = &node->ast->nodes.data[oldNodeIdx];
				FixUpOperators(oldNode, root);
			}
			else if (opInfo->precedence == rightInfo->precedence && opInfo->assoc == OA_Left) {
				SwapBinaryOperators(node, right);
				FixUpOperators(node, root);
				didFixup = true;
			}
			else {
				FixUpOperators(right, root);
			}
		}
		else if (right->type == ANT_UnaryOp && !right->UnaryOp_value.isPre) {
			if (opInfo->precedence < unaryOpPrecedence) {

				AST_BinaryOp tmp = node->BinaryOp_value;
				node->UnaryOp_value = right->UnaryOp_value;
				right->BinaryOp_value = tmp;

				right->BinaryOp_value.right = node->UnaryOp_value.val;
				node->UnaryOp_value.val = right->GetIndex();

				node->type = ANT_UnaryOp;
				right->type = ANT_BinaryOp;
				didFixup = true;
				node = right;
			}
		}
		if (didFixup) {
			FixUpOperators(&node->ast->nodes.data[idx], root);
		}
	} break;

	case ANT_Statement: {
		ASTNode* rootNode = &node->ast->nodes.data[node->Statement_value.root];
		FixUpOperators(rootNode, root);
	} break;

	case ANT_Parentheses: {
		ASTNode* val = &node->ast->nodes.data[node->Parentheses_value.val];
		FixUpOperators(val, root);
	} break;

	case ANT_UnaryOp: {
		ASTNode* val = &node->ast->nodes.data[node->UnaryOp_value.val];

		bool didFixup = false;
		if (node->UnaryOp_value.isPre && val->type == ANT_BinaryOp) {
			const BinaryOperator* opInfo = GetBinaryInfoForOp(val->BinaryOp_value.op);
			if (opInfo->precedence > unaryOpPrecedence) {
				ASTIndex oldNode  = node->GetIndex();
				ASTIndex oldVal   = val->GetIndex();
				ASTIndex oldLeft = val->BinaryOp_value.left;
				ASTIndex oldRight = val->BinaryOp_value.right;
				const char* oldUnOp  = node->UnaryOp_value.op;
				const char* oldBinOp = val->BinaryOp_value.op;

				node->type = ANT_BinaryOp;
				node->BinaryOp_value.left = oldVal;
				node->BinaryOp_value.right = oldRight;
				node->BinaryOp_value.op = oldBinOp;
				val->type = ANT_UnaryOp;
				val->UnaryOp_value.isPre = true;
				val->UnaryOp_value.op = oldUnOp;
				val->UnaryOp_value.val = oldLeft;

				didFixup = true;

				FixUpOperators(node, root);
			}
		}

		if (!didFixup) {
			FixUpOperators(val, root);
		}
	} break;

	case ANT_FieldAccess: {
		ASTNode* val = &node->ast->nodes.data[node->FieldAccess_value.val];
		FixUpOperators(val, root);
	} break;

	case ANT_VariableAssign: {
		ASTNode* val = &node->ast->nodes.data[node->VariableAssign_value.val];
		FixUpOperators(val, root);
	} break;

	case ANT_VariableDecl: {
		ASTNode* val = &node->ast->nodes.data[node->VariableDecl_value.initValue];
		FixUpOperators(val, root);
	} break;

	case ANT_Scope: {
		for (int i = 0; i < node->Scope_value.statements.count; i++) {
			ASTIndex statementIdx = node->Scope_value.statements.data[i];
			ASTNode* stmt = &node->ast->nodes.data[statementIdx];

			FixUpOperators(stmt, root);
		}
	} break;

	case ANT_FunctionCall: {
		for (int i = 0; i < node->FunctionCall_value.args.count; i++) {
			ASTIndex argIdx = node->FunctionCall_value.args.data[i];
			ASTNode* arg = &node->ast->nodes.data[argIdx];

			FixUpOperators(arg, root);
		}
	} break;

	case ANT_IfStatement: {
		{
			ASTIndex condIndex = node->IfStatement_value.condition;
			ASTNode* cond = &node->ast->nodes.data[condIndex];

			FixUpOperators(cond, root);
		}

		{
			ASTIndex bodyIndex = node->IfStatement_value.bodyScope;
			ASTNode* body = &node->ast->nodes.data[bodyIndex];

			FixUpOperators(body, root);
		}
	} break;

	case ANT_StructDefinition: {
		{
			ASTIndex nameIdx = node->StructDefinition_value.structName;
			ASTNode* name = &node->ast->nodes.data[nameIdx];

			FixUpOperators(name, root);
		}

		for (int i = 0; i < node->StructDefinition_value.fieldDecls.count; i++) {
			ASTIndex fieldIdx = node->StructDefinition_value.fieldDecls.data[i];
			ASTNode* field = &node->ast->nodes.data[fieldIdx];

			FixUpOperators(field, root);
		}

	} break;

	case ANT_FunctionDefinition: {
		{
			ASTNode* body = &node->ast->nodes.data[node->FunctionDefinition_value.bodyScope];
			FixUpOperators(body, root);
		}

		for (int i = 0; i < node->FunctionDefinition_value.params.count; i++) {
			ASTNode* param = &node->ast->nodes.data[node->FunctionDefinition_value.params.data[i]];
			FixUpOperators(param, root);
		}
	} break;

	case ANT_Root: {
		for (int i = 0; i < node->Root_value.topLevelStatements.count; i++) {
			ASTIndex stmtIdx = node->Root_value.topLevelStatements.data[i];
			ASTNode* stmt = &node->ast->nodes.data[stmtIdx];

			FixUpOperators(stmt, node);
		}

	} break;

	case ANT_ReturnStatement: {
		ASTIndex valIdx = node->ReturnStatement_value.retVal;
		ASTNode* val = &node->ast->nodes.data[valIdx];

		FixUpOperators(val, root);
	} break;

	default: {
	} break;
	}
}


#ifndef AST_H
#define AST_H

#pragma once

#include "../CppUtils/strings.h"
#include "../CppUtils/vector.h"

enum ASTNodeType {
	ANT_Invalid = -1,
	ANT_StructField,
	ANT_StructDef,
	ANT_FunctionDef,
	ANT_Parameter,
	ANT_VariableDecl,
	ANT_Identifier,
	ANT_ArrayAccess,
	ANT_TypeArray,
	ANT_TypeGeneric,
	ANT_TypePointer,
	ANT_Statement,
	ANT_IfStatement,
	ANT_FunctionCall,
	ANT_StringLiteral,
	ANT_IntegerLiteral,
	ANT_FloatLiteral,
	ANT_UnaryOp,
	ANT_BinaryOp,
	ANT_Count
};

typedef int ASTIndex;

struct AST_StructField{
	ASTIndex def;
};

struct AST_StructDef {
	SubString name;
};

struct AST_FunctionDef {
	SubString name;
	Vector<ASTIndex> params;
	ASTIndex returnType;
	Vector<ASTIndex> bodyStatements;
};

struct ANT_Parameter {
	ASTIndex type;
	ASTIndex name;
};

struct AST_VariableDecl{
	ASTIndex type;
	SubString varName;
};

struct AST_Identifier {
	SubString name;
};

struct AST_ArrayAccess {
	ASTIndex arr;
	ASTIndex index;
};

struct AST_TypeSimple{
	SubString name;
};
struct AST_TypeArray{
	ASTIndex childType;
	ASTIndex length;
};

struct AST_TypeGeneric{
	ASTIndex childType;
	Vector<ASTIndex> args;
};
struct AST_TypePointer {
	ASTIndex childType;
	int indirectionLevel;
};

struct AST_Statement {
	ASTIndex root;
};

struct AST_IfStatement{
	ASTIndex condition;
	Vector<ASTIndex> body;
};

struct AST_WhileStatement {
	// TODO
};

struct AST_FunctionCall{
	ASTIndex func;
	Vector<ASTIndex> args;
};

struct AST_StringLiteral {
	SubString repr;
	SubString val;
};

struct AST_IntegerLiteral {
	SubString repr;
	int val;
};

struct AST_FloatLiteral{
	SubString repr;
	float val;
};

struct AST_UnaryOp{
	const char* op;
	ASTIndex val;
};

struct AST_BinaryOp{
	const char* op;
	ASTIndex left;
	ASTIndex right;
};

struct AST;

struct ASTNode {
	AST* ast;
	ASTNodeType type;

	ASTNode() {
		ast = nullptr;
		type = ANT_Invalid;

		int startOfUnion = BNS_OFFSET_OF(ASTNode, type) + sizeof(type);
		int unionSize = sizeof(ASTNode) - startOfUnion;
		// FFFFFFFFFFFFFF
		MemSet(((char*)this) + startOfUnion, 0, unionSize);
	}

	union {
		AST_BinaryOp       BinaryOp_value;
		AST_UnaryOp        UnaryOp_value;
		AST_FunctionCall   FunctionCall_value;
		AST_IntegerLiteral IntegerLiteral_value;
		AST_Identifier     Identifier_value;
		AST_Statement      Statement_value;
	};

	ASTIndex GetIndex();

	~ASTNode() {

	}
};

struct AST {
	Vector<ASTNode> nodes;

	ASTNode* addNode() {
		ASTNode& node = nodes.EmplaceBack();
		node.ast = this;
		return &node;
	}

	ASTIndex GetCurrIdx() {
		return nodes.count - 1;
	}

	void ConstructFromString(const String& str);
	void ConstructFromTokens(const Vector<SubString>& toks);
};

struct TokenStreamFrame {
	int tokIndex;
	int nodeCount;
};

struct TokenStream {
	SubString* toks;
	int tokCount;
	int index;

	AST* ast;

	Vector<TokenStreamFrame> frames;

	TokenStream() {
		index = 0;
		tokCount = 0;
		toks = nullptr;
		ast = nullptr;
	}

	void PushFrame() {
		TokenStreamFrame frame;
		frame.nodeCount = ast->nodes.count;
		frame.tokIndex = index;
		frames.PushBack(frame);
	}

	void PopFrame() {
		TokenStreamFrame frame = frames.Back();
		frames.PopBack();

		ASSERT(frame.nodeCount <= ast->nodes.count);
		// TODO: cleaner destruct
		ast->nodes.count = frame.nodeCount;

		ASSERT(frame.tokIndex <= index);
		index = frame.tokIndex;
	}

	const SubString& CurrTok() {
		return toks[index];
	}
};

struct __PushPopFrame {
	TokenStream* stream;
	__PushPopFrame(TokenStream* _stream) {
		stream = _stream;
		_stream->PushFrame();
	}

	~__PushPopFrame() {
		if (stream != nullptr) {
			stream->PopFrame();
		}
	}
};

#define PUSH_STREAM_FRAME(stream) __PushPopFrame _frame_stream(stream)
#define FRAME_SUCCES() _frame_stream.stream = nullptr; return true

void ParseTokenStream(TokenStream* stream);

bool ParseIntLiteral(TokenStream* stream);
bool ParseBinaryOp(TokenStream* stream);
bool ParseUnaryOp(TokenStream* stream);
bool ParseValue(TokenStream* stream);
bool ParseFunctionCall(TokenStream* stream);
bool ParseSingleValue(TokenStream* stream);
bool ParseIdentifier(TokenStream* stream);
bool ParseStatement(TokenStream* stream);

bool ExpectAndEatWord(TokenStream* stream, const char* str);
bool ExpectAndEatOneOfWords(TokenStream* stream, const char** strs, const int count, int* outIdx);

inline bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool IsNumeric(char c) {
	return (c >= '0' && c <= '9');
}

enum OperatorAssociativity {
	OA_Left,
	OA_Right
};

struct BinaryOperator {
	const char* op;
	OperatorAssociativity assoc;
	int precedence;
};

const BinaryOperator binOpInfo[] = {
	{ "+", OA_Left, 6 },
	{ "-", OA_Left, 6 },
	{ "*", OA_Left, 5 },
	{ "/", OA_Left, 5 }
};

void DisplayTree(ASTNode* node, int indentation = 0);

#endif
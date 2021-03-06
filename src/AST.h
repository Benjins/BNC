#ifndef AST_H
#define AST_H

#pragma once

#include "../CppUtils/strings.h"
#include "../CppUtils/vector.h"

enum ASTNodeType {
	ANT_Invalid = -1,
	ANT_StructField,
	ANT_StructDefinition,
	ANT_FunctionDefinition,
	ANT_Parameter,
	ANT_VariableDecl,
	ANT_VariableAssign,
	ANT_Identifier,
	ANT_ArrayAccess,
	ANT_TypeArray,
	ANT_TypeGeneric,
	ANT_TypePointer,
	ANT_TypeSimple,
	ANT_Statement,
	ANT_Scope,
	ANT_FieldAccess,
	ANT_IfStatement,
	ANT_FunctionCall,
	ANT_StringLiteral,
	ANT_IntegerLiteral,
	ANT_FloatLiteral,
	ANT_BoolLiteral,
	ANT_UnaryOp,
	ANT_BinaryOp,
	ANT_Parentheses,
	ANT_ReturnStatement,
	ANT_Root,
	ANT_Count
};

typedef int ASTIndex;

struct AST_FunctionDefinition {
	ASTIndex name;
	Vector<ASTIndex> params;
	ASTIndex returnType;
	ASTIndex bodyScope;
};

struct ANT_Parameter {
	ASTIndex type;
	ASTIndex name;
};

struct AST_VariableDecl {
	ASTIndex type;
	ASTIndex varName;
	ASTIndex initValue;
};

struct AST_VariableAssign {
	ASTIndex var;
	ASTIndex val;
};

struct AST_Identifier {
	SubString name;
};

struct AST_ArrayAccess {
	ASTIndex arr;
	ASTIndex index;
};

struct AST_FieldAccess {
	ASTIndex val;
	ASTIndex field;
};

struct AST_TypeSimple{
	ASTIndex name;
};

#define ARRAY_DYNAMIC_LEN -1

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
};

struct AST_Statement {
	ASTIndex root;
};

struct AST_Scope {
	Vector<ASTIndex> statements;
};

struct AST_IfStatement{
	ASTIndex condition;
	ASTIndex bodyScope;
};

struct AST_ReturnStatement {
	ASTIndex retVal;
};

struct AST_WhileStatement {
	ASTIndex condition;
	ASTIndex bodyScope;
};

struct AST_FunctionCall{
	ASTIndex func;
	Vector<ASTIndex> args;
};

struct AST_StringLiteral {
	SubString repr;
};

struct AST_IntegerLiteral {
	SubString repr;
	int val;
};

struct AST_FloatLiteral{
	SubString repr;
	float val;
};

struct AST_BoolLiteral{
	SubString repr;
	bool val;
};

struct AST_UnaryOp{
	const char* op;
	ASTIndex val;
	bool isPre;
};

struct AST_BinaryOp{
	const char* op;
	ASTIndex left;
	ASTIndex right;
};

struct AST_Parentheses {
	ASTIndex val;
};

struct AST_StructDefinition {
	ASTIndex structName;
	Vector<ASTIndex> fieldDecls;
};

struct AST_Root {
	Vector<ASTIndex> topLevelStatements;
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
		// fffffuuuuuuhhhhhh
		MemSet(((char*)this) + startOfUnion, 0, unionSize);
	}

	union {
		AST_BinaryOp           BinaryOp_value;
		AST_UnaryOp            UnaryOp_value;
		AST_FunctionCall       FunctionCall_value;
		AST_IntegerLiteral     IntegerLiteral_value;
		AST_FloatLiteral       FloatLiteral_value;
		AST_StringLiteral      StringLiteral_value;
		AST_BoolLiteral        BoolLiteral_value;
		AST_Identifier         Identifier_value;
		AST_Statement          Statement_value;
		AST_Parentheses        Parentheses_value;
		AST_FieldAccess        FieldAccess_value;
		AST_VariableAssign     VariableAssign_value;
		AST_VariableDecl       VariableDecl_value;
		AST_TypeSimple         TypeSimple_value;
		AST_TypeArray          TypeArray_value;
		AST_TypeGeneric        TypeGeneric_value;
		AST_TypePointer        TypePointer_value;
		AST_Scope              Scope_value;
		AST_IfStatement        IfStatement_value;
		AST_StructDefinition   StructDefinition_value;
		AST_FunctionDefinition FunctionDefinition_value;
		AST_ReturnStatement    ReturnStatement_value;
		AST_ArrayAccess        ArrayAccess_value;
		AST_Root               Root_value;
	};

	ASTIndex GetIndex();

	~ASTNode() {
		// TODO:
		// Should probably have a proper destructor, since we have vectors and stuff...
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

struct __PushPopASTFrame {
	TokenStream* stream;
	bool success;
	__PushPopASTFrame(TokenStream* _stream) {
		stream = _stream;
		_stream->PushFrame();
		success = false;
	}

	~__PushPopASTFrame() {
		if (success) {
			stream->frames.PopBack();
		}
		else {
			stream->PopFrame();
		}
	}
};

#define PUSH_STREAM_FRAME(stream) __PushPopASTFrame _frame_stream(stream)
#define FRAME_SUCCES() _frame_stream.success = true; return true

bool ParseTokenStream(TokenStream* stream);
bool ParseFloatLiteral(TokenStream* stream);
bool ParseIntLiteral(TokenStream* stream);
bool ParseStringLiteral(TokenStream* stream);
bool ParseBoolLiteral(TokenStream* stream);
bool ParseBinaryOp(TokenStream* stream);
bool ParseUnaryOp(TokenStream* stream);
bool ParseValue(TokenStream* stream);
bool ParseFunctionCall(TokenStream* stream);
bool ParseSingleValue(TokenStream* stream);
bool ParseSingleValueNoUnary(TokenStream* stream);
bool ParseIdentifier(TokenStream* stream);
bool ParseStatement(TokenStream* stream);
bool ParseVariableAssign(TokenStream* stream);
bool ParseVariableDecl(TokenStream* stream);
bool ParseType(TokenStream* stream);
bool ParseGenericType(TokenStream* stream);
bool ParseArrayType(TokenStream* stream);
bool ParsePointerType(TokenStream* stream);
bool ParseScope(TokenStream* stream);
bool ParseIfStatement(TokenStream* stream);
bool ParseReturnStatement(TokenStream* stream);
bool ParseStructDefinition(TokenStream* stream);
bool ParseFunctionDefinition(TokenStream* stream);
bool ParseTopLevelStatement(TokenStream* stream);
bool ParseArrayAccess(TokenStream* stream);

bool CheckNextWord(TokenStream* stream, const char* str);
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
	{ "+",  OA_Left, 6 },
	{ "-",  OA_Left, 6 },
	{ "*",  OA_Left, 5 },
	{ "/",  OA_Left, 5 },
	{ ".",  OA_Left, 2 },
	{ "==", OA_Left, 9 },
	{ "<=", OA_Left, 9 },
	{ "<",  OA_Left, 9 },
	{ ">=", OA_Left, 9 },
	{ ".",  OA_Left, 9 },
};


const char* binaryOperators[] = {
	"*", "-", "+", "/", ".", "==", "<=", "<", ">", ">="
};

enum UnaryOperatorPos {
	UOP_Pre = (1 << 0),
	UOP_Post = (1 << 1),
	UOP_Both = UOP_Pre | UOP_Post
};

struct UnaryOperator {
	const char* op;
	UnaryOperatorPos pos;
};

UnaryOperator unOpInfo[] = {
	{ "!",  UOP_Pre },
	{ "-",  UOP_Pre },
	{ "^",  UOP_Both },
	{ "++", UOP_Post },
	{ "--", UOP_Post }
};

const char* unaryOperators[] = {
	"!", "-", "^", "++", "--"
};

static_assert(BNS_ARRAY_COUNT(binaryOperators) == BNS_ARRAY_COUNT(binOpInfo), "Binary operator info");
static_assert(BNS_ARRAY_COUNT(unaryOperators)  == BNS_ARRAY_COUNT(unOpInfo),  "Unary  operator info");

const int unaryOpPrecedence = 3;
const int arrayOpPrecedence = 2;

void DisplayTree(ASTNode* node, int indentation = 0);

#endif
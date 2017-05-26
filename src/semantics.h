#ifndef SEMANTICS_H
#define SEMANTICS_H

#pragma once

#include "../CppUtils/vector.h"
#include "../CppUtils/disc_union.h"
#include "../CppUtils/bitset.h"

#include "AST.h"

enum TypeCheckResult {
	TCR_NoProgress,
	TCR_SomeProgress,
	TCR_Success,
	TCR_Error
};

enum TypeInfoKind {
	TIK_Simple,
	TIK_Struct,
	TIK_Pointer,
	TIK_Array
};

struct BuiltinTypeInfo {
	const char* name;
};

struct StructTypeInfo {
	SubString name;
	int index;
};

struct PointerTypeInfo {
	int subType;
};

struct ArrayTypeInfo {
	int subType;
	int arrayLen;
};

#define DISC_LIST(mac)   \
	mac(BuiltinTypeInfo)  \
	mac(StructTypeInfo)  \
	mac(PointerTypeInfo) \
	mac(ArrayTypeInfo)

// This generates the actual struct body
DEFINE_DISCRIMINATED_UNION(TypeInfo, DISC_LIST)

typedef int TypeIndex;

struct VariableDecl {
	TypeIndex typeIndex;
	SubString name;
	ASTIndex idx;
};

struct FuncDef {
	ASTIndex idx;
	SubString name;
	TypeIndex retType;
	Vector<TypeIndex> argTypes;
	bool doneChecking;
};

struct StructDef {
	ASTIndex idx;
	SubString name;
	Vector<VariableDecl> fieldDecls;
	bool doneChecking;
};

struct ScopeStackFrame {
	int knownTypesCount;
	int varsInScopeCount;
	int definedFunctionsCount;
	int definedStructsCount;
};

struct SemanticContext {
	Vector<TypeInfo> knownTypes;
	Vector<VariableDecl> varsInScope;
	Vector<FuncDef> definedFunctions;
	Vector<StructDef> definedStructs;
	Vector<ASTIndex> globalVarDecls;

	Vector<ScopeStackFrame> scopeFrames;

	BitSet nodesDone;

	void PushScope() {
		ScopeStackFrame frame;
		frame.knownTypesCount       = knownTypes.count;
		frame.varsInScopeCount      = varsInScope.count;
		frame.definedFunctionsCount = definedFunctions.count;
		frame.definedStructsCount   = definedStructs.count;

		scopeFrames.PushBack(frame);
	}

	void PopScope() {
		ScopeStackFrame frame = scopeFrames.Back();
		scopeFrames.PopBack();
#define REMOVE_FROM(field) field . RemoveRange(frame. field ## Count , field .count)
		REMOVE_FROM(knownTypes);
		REMOVE_FROM(varsInScope);
		REMOVE_FROM(definedFunctions);
		REMOVE_FROM(definedStructs);
#undef REMOVE_FROM
	}
};

struct __PushPopSCScope {
	SemanticContext* sc;

	__PushPopSCScope(SemanticContext* _sc) {
		sc = _sc;
		sc->PushScope();
	}

	~__PushPopSCScope() {
		sc->PopScope();
	}
};

#define PUSH_SC_SCOPE(sc) __PushPopSCScope __sc_scope (sc)

void DoSemantics(AST* ast, SemanticContext* sc);

struct IdentifierStruct {
	int structIdx;
};

struct IdentifierFunction {
	int funcIdx;
};

struct IdentifierVariable {
	int varIdx;
};

struct IdentifierUnknown {
};

#define DISC_MAC(mac) \
	mac(IdentifierStruct) \
	mac(IdentifierFunction) \
	mac(IdentifierVariable) \
	mac(IdentifierUnknown)

DEFINE_DISCRIMINATED_UNION(ResolvedIdentifer, DISC_MAC)

#undef DISC_MAC

ResolvedIdentifer ResolveIdentifier(const SubString& str, SemanticContext* sc);

#endif

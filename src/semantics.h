#ifndef SEMANTICS_H
#define SEMANTICS_H

#pragma once

#include "../CppUtils/vector.h"

#include "AST.h"

struct TypeInfo {
	const char* name;
};

typedef int TypeIndex;

struct VariableDecl {
	TypeIndex typeIndex;
	SubString name;
	ASTIndex idx;
};

struct FuncDef {
	ASTIndex idx;
};

struct StructDef {
	ASTIndex idx;
	SubString name;
	Vector<VariableDecl> fieldDecls;
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

	Vector<ScopeStackFrame> scopeFrames;

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

void DoSemantics(AST* ast, SemanticContext* sc);


#endif

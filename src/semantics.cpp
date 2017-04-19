#include "semantics.h"

TypeCheckResult TypeCheckVarDecl(ASTNode* decl, SemanticContext* sc, int* outTypeIdx, bool addToScope = true);
TypeCheckResult TypeCheckStructDef(StructDef* def, SemanticContext* sc, AST* ast);
TypeCheckResult TypeCheckFunctionDef(FuncDef* def, SemanticContext* sc, AST* ast);
TypeCheckResult DoTypeChecking(ASTNode* node, SemanticContext* sc, FuncDef* currFun = nullptr);

FuncDef* GetFuncDefByName(const SubString& name, SemanticContext* sc) {
	BNS_VEC_FOREACH(sc->definedFunctions) {
		if (ptr->name == name) {
			return ptr;
		}
	}

	return nullptr;
}

void InitSemanticContextWithBuiltinTypes(SemanticContext* sc) {
	TypeInfo info;
	BuiltinTypeInfo simple;

#define ADD_SIMPLE_TYPE(tn) simple.name = tn; info = simple; sc->knownTypes.PushBack(info)
	ADD_SIMPLE_TYPE("int");
	ADD_SIMPLE_TYPE("float");
	ADD_SIMPLE_TYPE("bool");
	ADD_SIMPLE_TYPE("string");
#undef ADD_SIMPLE_TYPE
}

int GetSimpleTypeIndex(const SubString& typeName, SemanticContext* sc) {
	BNS_VEC_FOREACH(sc->knownTypes) {
		if (ptr->type == TypeInfo::UE_BuiltinTypeInfo && typeName == ((BuiltinTypeInfo*)&ptr->BuiltinTypeInfo_data)->name) {
			return ptr - sc->knownTypes.data;
		}
		else if (ptr->type == TypeInfo::UE_StructTypeInfo && typeName == ((StructTypeInfo*)&ptr->StructTypeInfo_data)->name) {
			return ptr - sc->knownTypes.data;
		}
	}

	return -1;
}

int GetTypeofVariable(SubString name, SemanticContext* sc) {
	BNS_VEC_FOREACH(sc->varsInScope) {
		if (ptr->name == name) {
			return ptr->typeIndex;
		}
	}

	return -1;
}

int GetTypeIndex(ASTNode* typeNode, SemanticContext* sc) {
	if (typeNode->type == ANT_TypeSimple) {
		ASTNode* typeIdent = &typeNode->ast->nodes.data[typeNode->TypeSimple_value.name];
		ASSERT(typeIdent->type == ANT_Identifier);
		return GetSimpleTypeIndex(typeIdent->Identifier_value.name, sc);
	}
	else {
		// TODO: Pointer, Array, Generic
		ASSERT(false);
		return -1;
	}
}

void DoSemantics(AST* ast, SemanticContext* sc) {
	ASTNode* root = &ast->nodes.Back();

	InitSemanticContextWithBuiltinTypes(sc);

	const Vector<ASTIndex>& topStmts = root->Root_value.topLevelStatements;
	Vector<ASTIndex> globalVarDecls;
	BNS_VEC_FOREACH(topStmts) {
		ASTNode* topStmt = &ast->nodes.data[*ptr];
		if (topStmt->type == ANT_FunctionDefinition) {
			FuncDef def;
			def.idx = *ptr;
			ASTIndex funcNameIdx = ast->nodes.data[*ptr].FunctionDefinition_value.name;
			def.name = ast->nodes.data[funcNameIdx].Identifier_value.name;
			sc->definedFunctions.PushBack(def);
		}
		else if (topStmt->type == ANT_StructDefinition) {
			StructDef def;
			def.idx = *ptr;
			sc->definedStructs.PushBack(def);

			TypeInfo info;
			StructTypeInfo str;
			ASTIndex structNameIdx = ast->nodes.data[*ptr].StructDefinition_value.structName;
			str.name = ast->nodes.data[structNameIdx].Identifier_value.name;
			str.index = sc->definedStructs.count - 1;
			info = str;

			sc->knownTypes.PushBack(info);
		}
		else if (topStmt->type == ANT_Statement) {
			ASTNode* stmt = &ast->nodes.data[topStmt->Statement_value.root];
			if (stmt->type == ANT_VariableDecl) {
				globalVarDecls.PushBack(stmt->GetIndex());
			}
		}
	}

	BNS_VEC_FOREACH(sc->definedStructs) {
		TypeCheckResult res = TypeCheckStructDef(ptr, sc, ast);
		if (res != TCR_Success) {
			printf("Failed to type-check struct.\n");
		}
		else {
			printf("Type checking worked!\n");
		}
	}

	BNS_VEC_FOREACH(globalVarDecls) {
		ASTNode* stmt = &ast->nodes.data[*ptr];
		ASSERT(stmt->type == ANT_VariableDecl);

		int typeIdx;
		TypeCheckResult res = TypeCheckVarDecl(stmt, sc, &typeIdx);
		if (res == TCR_Error) {
			printf("Failed to typecheck var decl\n");
		}
	}

	BNS_VEC_FOREACH(sc->definedFunctions) {
		TypeCheckResult res = TypeCheckFunctionDef(ptr, sc, ast);
		if (res != TCR_Success) {
			printf("Failed to type-check func def.\n");
		}
		else {
			printf("Type checking worked!\n");
		}
	}
}

TypeCheckResult TypeCheckValue(ASTNode* val, SemanticContext* sc, int* outTypeIdx) {
	switch (val->type) {
	case ANT_BinaryOp: {
		int lType, rType;
		ASTNode* left  = &val->ast->nodes.data[val->BinaryOp_value.left];
		ASTNode* right = &val->ast->nodes.data[val->BinaryOp_value.right];
		TypeCheckResult lRes = TypeCheckValue(left,  sc, &lType);
		TypeCheckResult rRes = TypeCheckValue(right, sc, &rType);

		if (lRes == TCR_Success && rRes == TCR_Success && lType == rType) {
			*outTypeIdx = lType;
			return TCR_Success;
		}
		else {
			return TCR_Error;
		}
	} break;

	case ANT_FunctionCall: {
		ASTNode* funcVal = &val->ast->nodes.data[val->FunctionCall_value.func];
		ASSERT(funcVal->type == ANT_Identifier);
		FuncDef* def = GetFuncDefByName(funcVal->Identifier_value.name, sc);

		if (val->FunctionCall_value.args.count != def->argTypes.count) {
			// Arity mismatch
			return TCR_Error;
		}

		for (int i = 0; i < val->FunctionCall_value.args.count; i++) {
			ASTIndex argIndex = val->FunctionCall_value.args.data[i];
			ASTNode* argNode = &val->ast->nodes.data[argIndex];
			int argTypeIdx;
			TypeCheckResult res = TypeCheckValue(argNode, sc, &argTypeIdx);
			if (res != TCR_Success) {
				return TCR_Error;
			}

			if (argTypeIdx != def->argTypes.data[i]) {
				return TCR_Error;
			}
		}

		*outTypeIdx = def->retType;
		return TCR_Success;

	} break;
	
	case ANT_Parentheses: {
		ASTNode* innerVal = &val->ast->nodes.data[val->Parentheses_value.val];
		return TypeCheckValue(innerVal, sc, outTypeIdx);
	} break;

	case ANT_Identifier: {
		TypeIndex idx = GetTypeofVariable(val->Identifier_value.name, sc);
		if (idx == -1) {
			return TCR_Error;
		}

		*outTypeIdx = idx;
		return TCR_Success;
	} break;

	case ANT_UnaryOp: {
		int lType, rType;
		ASTNode* subVal = &val->ast->nodes.data[val->UnaryOp_value.val];
		TypeCheckResult sRes = TypeCheckValue(subVal, sc, &lType);

		if (sRes == TCR_Success) {
			*outTypeIdx = lType;
			return TCR_Success;
		}
		else {
			return TCR_Error;
		}
	} break;

	case ANT_BoolLiteral: {
		*outTypeIdx = GetSimpleTypeIndex(STATIC_TO_SUBSTRING("bool"), sc);
		return TCR_Success;
	};

	case ANT_StringLiteral: {
		*outTypeIdx = GetSimpleTypeIndex(STATIC_TO_SUBSTRING("string"), sc);
		return TCR_Success;
	};

	case ANT_IntegerLiteral: {
		*outTypeIdx = GetSimpleTypeIndex(STATIC_TO_SUBSTRING("int"), sc);
		return TCR_Success;
	};

	case ANT_FloatLiteral: {
		*outTypeIdx = GetSimpleTypeIndex(STATIC_TO_SUBSTRING("float"), sc);
		return TCR_Success;
	};
	}

	return TCR_Error;
}

TypeCheckResult TypeCheckVarDecl(ASTNode* decl, SemanticContext* sc, int* outTypeIdx, bool addToScope /*= true*/) {
	ASSERT(decl->type == ANT_VariableDecl);

	ASTNode* type = &decl->ast->nodes.data[decl->VariableDecl_value.type];
	int varTypeIdx = GetTypeIndex(type, sc);

	if (varTypeIdx < 0) {
		return TCR_Error;
	}

	ASTIndex valNodeIdx = decl->VariableDecl_value.initValue;
	if (valNodeIdx < 0) {
		*outTypeIdx = varTypeIdx;
		if (addToScope) {
			VariableDecl vardecl;
			vardecl.idx = decl - decl->ast->nodes.data;
			vardecl.name = decl->ast->nodes.data[decl->VariableDecl_value.varName].Identifier_value.name;
			vardecl.typeIndex = varTypeIdx;
			sc->varsInScope.PushBack(vardecl);
		}
		return TCR_Success;
	}

	int valTypeIdx;
	ASTNode* val = &decl->ast->nodes.data[valNodeIdx];
	TypeCheckResult vRes = TypeCheckValue(val, sc, &valTypeIdx);
	if (vRes == TCR_Error) {
		return TCR_Error;
	}
	else if (varTypeIdx == valTypeIdx) {
		*outTypeIdx = varTypeIdx;

		if (addToScope) {
			VariableDecl vardecl;
			vardecl.idx = decl - decl->ast->nodes.data;
			vardecl.name = decl->ast->nodes.data[decl->VariableDecl_value.varName].Identifier_value.name;
			vardecl.typeIndex = varTypeIdx;
			sc->varsInScope.PushBack(vardecl);
		}

		return TCR_Success;
	}
	else {
		return TCR_Error;
	}
}

TypeCheckResult TypeCheckStructDef(StructDef* def, SemanticContext* sc, AST* ast) {
	//PUSH_SC_SCOPE(sc);

	ASTNode* defNode = &ast->nodes.data[def->idx];
	ASTNode* nameNode = &ast->nodes.data[defNode->StructDefinition_value.structName];
	def->name = nameNode->Identifier_value.name;

	TypeCheckResult res = TCR_NoProgress;
	bool anyFieldsInProgress = false;
	
	BNS_VEC_FOREACH(defNode->StructDefinition_value.fieldDecls) {
		ASTNode* fieldNode = &ast->nodes.data[*ptr];
		int fieldTypeIdx;
		TypeCheckResult fres = TypeCheckVarDecl(fieldNode, sc, &fieldTypeIdx, false);

		if (fres == TCR_Error) {
			return TCR_Error;
		}
	}

	return TCR_Success;
}

TypeCheckResult TypeCheckFunctionDef(FuncDef* def, SemanticContext* sc, AST* ast) {
	PUSH_SC_SCOPE(sc);

	ASTNode* defNode = &ast->nodes.data[def->idx];

	ASTNode* retNode = &ast->nodes.data[defNode->FunctionDefinition_value.returnType];

	def->retType = GetTypeIndex(retNode, sc);
	if (def->retType < 0) {
		return TCR_Error;
	}

	BNS_VEC_FOREACH(defNode->FunctionDefinition_value.params) {
		int paramTypeIdx;
		TypeCheckResult paramRes = TypeCheckVarDecl(&ast->nodes.data[*ptr], sc, &paramTypeIdx);
		if (paramRes == TCR_Error) {
			return TCR_Error;
		}
		else {
			def->argTypes.PushBack(paramTypeIdx);
		}
	}

	ASTNode* scopeNode = &ast->nodes.data[defNode->FunctionDefinition_value.bodyScope];

	TypeCheckResult scopeRes = DoTypeChecking(scopeNode, sc, def);

	return scopeRes;
}

TypeCheckResult DoTypeChecking(ASTNode* node, SemanticContext* sc, FuncDef* currFun /*= nullptr*/) {
	switch (node->type) {
	case ANT_StructDefinition: {
		// Err..we already did this?
		return TCR_Success;
	};

	case ANT_FunctionDefinition: {
		// Err..we already did this?
		return TCR_Success;	
	} break;

	case ANT_Statement: {
		ASTNode* root = &node->ast->nodes.data[node->Statement_value.root];
		return DoTypeChecking(root, sc, currFun);
	};

	case ANT_ReturnStatement: {
		if (currFun == nullptr) {
			return TCR_Error;
		}

		ASTNode* val = &node->ast->nodes.data[node->ReturnStatement_value.retVal];

		int retType;
		TypeCheckResult res = TypeCheckValue(val, sc, &retType);

		if (res == TCR_Success && retType == currFun->retType){
			return TCR_Success;
		}
		else {
			return TCR_Error;
		}
	} break;

	case ANT_Scope: {
		BNS_VEC_FOREACH(node->Scope_value.statements) {
			ASTNode* stmt = &node->ast->nodes.data[*ptr];
			TypeCheckResult res = DoTypeChecking(stmt, sc, currFun);
			if (res == TCR_Error) {
				return TCR_Error;
			}
		}

		return TCR_Success;
	} break;

	case ANT_VariableDecl: {
		int typeIdx;
		return TypeCheckVarDecl(node, sc, &typeIdx);
	} break;
	}

	ASSERT(false);
	return TCR_Error;
}


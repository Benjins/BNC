#include "semantics.h"

TypeCheckResult TypeCheckVarDecl(ASTNode* decl, SemanticContext* sc, int* outTypeIdx, bool addToScope = true);
TypeCheckResult TypeCheckStructDef(StructDef* def, SemanticContext* sc, AST* ast);
void DoTypeChecking(ASTNode* node, SemanticContext* sc);

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
	BNS_VEC_FOREACH(topStmts) {
		ASTNode* topStmt = &ast->nodes.data[*ptr];
		if (topStmt->type == ANT_VariableDecl) {
			VariableDecl decl;
			decl.idx = *ptr;
			sc->varsInScope.PushBack(decl);
		}
		else if (topStmt->type == ANT_FunctionDefinition) {
			FuncDef def;
			def.idx = *ptr;
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
				VariableDecl decl;
				decl.idx = stmt->GetIndex();
				sc->varsInScope.PushBack(decl);
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

	BNS_VEC_FOREACH(topStmts) {
		ASTNode* topStmt = &ast->nodes.data[*ptr];
		DoTypeChecking(topStmt, sc);
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

	int valTypeIdx;
	ASTNode* val = &decl->ast->nodes.data[decl->VariableDecl_value.initValue];
	TypeCheckValue(val, sc, &valTypeIdx);

	if (varTypeIdx == valTypeIdx) {
		*outTypeIdx = varTypeIdx;
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

void DoTypeChecking(ASTNode* node, SemanticContext* sc) {
	switch (node->type) {
	case ANT_StructDefinition: {
		
	};

	case ANT_Statement: {

	};
	}
}


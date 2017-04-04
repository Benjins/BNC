#include "semantics.h"

void DoTypeChecking(ASTNode* node, SemanticContext* sc);

void DoSemantics(AST* ast, SemanticContext* sc) {
	ASTNode* root = &ast->nodes.Back();

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

	BNS_VEC_FOREACH(topStmts) {
		ASTNode* topStmt = &ast->nodes.data[*ptr];
		DoTypeChecking(topStmt, sc);
	}
}

void DoTypeChecking(ASTNode* node, SemanticContext* sc) {
	switch (node->type) {
	case ANT_StructDefinition: {

	};
	}
}


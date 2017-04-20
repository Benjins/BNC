#ifndef BYTECODE_H
#define BYTECODE_H

#pragma once

#include "../CppUtils/disc_union.h"
#include "../CppUtils/strings.h"
#include "../CppUtils/vector.h"

struct BNCByteCodeVoid {
	char __unused__;
};

struct BNCByteCodeInt {
	int val;

	BNCByteCodeInt(int x = 0) {
		val = x;
	}

	operator int() const { return val; }
};

struct BNCByteCodeFloat {
	float val;

	BNCByteCodeFloat(float x = 0.0f) {
		val = x;
	}

	operator float() const { return val; }
};

#define BNC_VAL(mac)            \
	mac (BNCByteCodeInt)        \
	mac (BNCByteCodeFloat)      \
	mac (String)                \
	mac (BNCByteCodeVoid)

DEFINE_DISCRIMINATED_UNION(BNCBytecodeValue, BNC_VAL)

#define BNC_MATH_OP(op, name)                                                                                      \
BNCBytecodeValue BNS_GLUE_TOKS(BNCValue_, name) (BNCBytecodeValue a, BNCBytecodeValue b) {                         \
	if (a.type == BNCBytecodeValue::UE_BNCByteCodeInt && b.type == BNCBytecodeValue::UE_BNCByteCodeInt) {          \
		BNCBytecodeValue val;                                                                                      \
		val = BNCByteCodeInt(a.AsBNCByteCodeInt() op b.AsBNCByteCodeInt());                                        \
		return val;                                                                                                \
	}                                                                                                              \
	else if (a.type == BNCBytecodeValue::UE_BNCByteCodeFloat && b.type == BNCBytecodeValue::UE_BNCByteCodeFloat) { \
		BNCBytecodeValue val;                                                                                      \
		val = BNCByteCodeFloat(a.AsBNCByteCodeFloat() op b.AsBNCByteCodeFloat());                                  \
		return val;                                                                                                \
	}                                                                                                              \
	else {                                                                                                         \
		ASSERT(false);                                                                                             \
		BNCBytecodeValue val;                                                                                      \
		return val;                                                                                                \
	}                                                                                                              \
}

BNC_MATH_OP(+, Add)
BNC_MATH_OP(-, Sub)
BNC_MATH_OP(*, Mul)
BNC_MATH_OP(/ , Div)

#undef BNC_MATH_OP

struct BNCBytecodeVMState {
	Vector<BNCBytecodeValue> stack;

	void Push(BNCBytecodeValue val) {
		stack.PushBack(val);
	}

	BNCBytecodeValue Pop() {
		ASSERT(stack.count > 0);
		BNCBytecodeValue top = stack.Back();
		stack.PopBack();
		return top;
	}
};

enum BNCBytecodeInstruction {
	BNCBI_Add,
	BNCBI_Mul,
	BNCBI_Sub,
	BNCBI_Div,
	BNCBI_FloatLit,
	BNCBI_IntLit
};

BNCBytecodeValue ExecuteBytecode(int* code, int codeLen, BNCBytecodeVMState* state);

#endif

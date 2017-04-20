#include "bytecode.h"


BNCBytecodeValue ExecuteBytecode(int* code, int codeLen, BNCBytecodeVMState* state) {
	for (int i = 0; i < codeLen; i++) {
		int inst = code[i];
		switch (inst) {
		case BNCBI_Add: { state->Push(BNCValue_Add(state->Pop(), state->Pop())); } break;
		case BNCBI_Sub: { state->Push(BNCValue_Sub(state->Pop(), state->Pop())); } break;
		case BNCBI_Mul: { state->Push(BNCValue_Mul(state->Pop(), state->Pop())); } break;
		case BNCBI_Div: { state->Push(BNCValue_Div(state->Pop(), state->Pop())); } break;

		case BNCBI_IntLit: {
			i++;
			int iVal = *(int*)&code[i];
			BNCBytecodeValue val;
			val = BNCByteCodeInt(iVal);
			state->Push(val);
		} break;

		case BNCBI_FloatLit: {
			i++;
			float fVal = *(float*)&code[i];
			BNCBytecodeValue val;
			val = BNCByteCodeFloat(fVal);
			state->Push(val);
		} break;
		}
	}

	if (state->stack.count == 0) {
		BNCBytecodeValue val;
		BNCByteCodeVoid voidVal;
		val = voidVal;
		return val;
	}
	else if (state->stack.count == 1) {
		return state->Pop();
	}
	else {
		ASSERT(false);

		BNCBytecodeValue val;
		BNCByteCodeVoid voidVal;
		val = voidVal;
		return val;
	}
}



// Game code, medium sweep part 4: more action-script node factories (as
// mid3_node.cpp).
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

// Node* New(s16 id): the "CIsVar" condition node (Mem_Alloc(24, 0, 16), record 16):
// VarName, Operator (an enum: default hash 0x03F26F14, a name list of 6
// holding "Equal", "Larger", "LargerEqual", "Less", "LessEqual",
// "NotEqual"), FValue, IValue, BValue, HValue.
DECOMP(00775BF0, CondIsVar_New_00775BF0) {
    NewNamedNode(ctx, 0x00B1FB0F, 16, {
        {0x00B1FB16, 5, 0, 0, 0},
        {0x00B1FB1E, 2, 0, 0x03F26F14, 1, 6, {0x00B1FB27, 0x00B1FB2D, 0x00B1FB34, 0x00B1FB40, 0x00B1FB45, 0x00B1FB4F}},
        {0x00B1FB58, 3, 1, 0, 2},
        {0x00B1FB5F, 0, 1, 0, 3},
        {0x00B1FB66, 4, 1, 0, 4},
        {0x00B1FB6D, 2, 1, 0, 5}
    }, true);
}

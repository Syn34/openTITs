// The default memory stack (ApeMemStack) that MemState holds.
#include "mem.h"

using namespace sly4;

// ApeMemStack* Mem_GetDefaultStack()
// MemState.heap as it is: 0 until Mem_Alloc / operator new have set it up.
DECOMP(005525FC, Mem_GetDefaultStack) { ret(ctx, ref<MemState>(kMemState).heap.get()); }

// The default memory stack (ApeMemStack) that MemState holds.
#include "mem.h"

namespace sly4::memory {

// MemState.heap as it is: 0 until Mem_Alloc / operator new have set it up.
LinearHeap* Mem_GetDefaultStack() { return mem_state().heap; }

}  // namespace sly4::memory

NATIVE(005525FC, sly4::memory::Mem_GetDefaultStack)

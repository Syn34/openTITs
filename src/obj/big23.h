// Game code, large-function sweep part 23: helpers shared by big23_*.cpp.
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// big23_item.cpp: a gesture list item's step (true when it acted).
struct GestureItem;
bool Item_GestureStep41B200(GestureItem* it, u32 mode);

}  // namespace sly4::obj

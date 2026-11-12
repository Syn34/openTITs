// Game code, large-function sweep part 22: helpers shared by big22_*.cpp.
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// big22_spawn.cpp: the 448-byte spawned physics object's constructor.
struct SpawnedObj;
struct SpawnQuery;
void Spawned_Construct_000B2E18(SpawnedObj* p, SpawnQuery* q);

}  // namespace sly4::obj

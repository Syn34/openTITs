// World_HandleEvent callees, part 16a: small leaves under the hevent15
// functions (scene keep / drop, item hash, component 30, parameter name,
// scene end, layer object by handle).
#include "hevent16.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent16;

namespace {
// Scene handle (*0x00F557F0) +24: u32 cleared to keep it.
constexpr u32 kSceneHold = 24;

// Item record: +320 Item* (+204 u32 hash).
constexpr u32 kRecItem = 320, kItemHash = 204;

// Component type 30 (as found by Obj_FindComponent).
constexpr u32 kCompType30 = 30;

// Param: +20 char name[64].
constexpr u32 kParamName = 20, kParamNameSize = 64;

// Object +176: u16 flags (bit 8 0x100: scene kept, bit 7 0x80: drop on end);
// vt +220 end_scene(Object*, u8 drop).
constexpr u32 kObjSceneFlags = 176, kSceneKept = 0x100, kSceneDrop = 0x80, kObjVtSceneEnd = 220;

// Object +168: id.
constexpr u32 kObjId = 168;
}  // namespace

// void Scene_Keep1AC68(Scene* s) (name?): s +24 = 0.
DECOMP(0001AC68, Scene_Keep1AC68) {
    store<u32>(arg<u32>(ctx, 0) + kSceneHold, 0);
    ctx.gpr[4] = 0;  // li r4, 0 (no known caller reads it)
}

// void Scene_Drop1AC74(Scene* s) (name?): empty (a bare blr).
DECOMP(0001AC74, Scene_Drop1AC74) {}

// u32 Item_Hash39FEBC(ItemRecord* r) (name?): r->item (+320) ? item +204 : 0.
DECOMP(0039FEBC, Item_Hash39FEBC) {
    const u32 item = load<u32>(arg<u32>(ctx, 0) + kRecItem);
    const u32 h = item != 0 ? load<u32>(item + kItemHash) : 0u;
    ctx.gpr[4] = h;  // ori r3, r4, 0: r4 left holding the result (no known caller reads it)
    ret(ctx, h);
}

// Component* Obj_Get0D95EC(Object* o) (name?): Obj_FindComponent(o, 30, 0).
DECOMP(000D95EC, Obj_Get0D95EC) {
    ret(ctx, static_cast<u32>(call(ctx, h16fn::Obj_FindComponent, {arg<u32>(ctx, 0), kCompType30, 0})));
}

// const char* Param_SetName0A78D4(Param* p, const char* s) (name?; symbols.txt
// Param_SetMatrix0A78D4): copies a non-empty s into the 64-byte name at
// p +20 (String_Copy's result returned); otherwise returns s.
DECOMP(000A78D4, Param_SetName0A78D4) {
    const u32 p = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (s == 0 || load<u8>(s) == 0) return ret(ctx, s);
    ret(ctx, static_cast<u32>(call(ctx, h16fn::String_Copy, {p + kParamName, s, kParamNameSize})));
}

// void Obj_SceneEnd0DD02C(Object* o, u8 drop, u8 keep) (name?): keep sets
// +176 bit 8 and stores drop in bit 7; otherwise clears bit 8 and calls
// vt +220 (o, drop). Returns o (keep) or the call's result.
DECOMP(000DD02C, Obj_SceneEnd0DD02C) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u8 drop = static_cast<u8>(a), keep = arg<u8>(ctx, 2);
    const u16 f = load<u16>(o + kObjSceneFlags);
    if (keep != 0) {
        // rlwimi r5, r4, 7, 24, 24: bit 0 of the full argument goes to 0x80.
        store<u16>(o + kObjSceneFlags, static_cast<u16>(((f | kSceneKept) & ~kSceneDrop) | ((a & 1) << 7)));
        return ret(ctx, o);
    }
    store<u16>(o + kObjSceneFlags, static_cast<u16>(f & ~kSceneKept));
    ret(ctx, static_cast<u32>(vcall(ctx, o, kObjVtSceneEnd, {o, drop})));
}

// Object* Obj_FindByHandle42C2B0(u32 handle, u8) (name?): the object of the
// first layer stack entry whose handle (+8, or the object's id +168 when the
// entry's is 0) equals handle; 0 for handle 0 or none. The second argument
// is unused.
DECOMP(0042C2B0, Obj_FindByHandle42C2B0) {
    const u32 h = arg<u32>(ctx, 0);
    if (h == 0) return ret(ctx, 0u);
    const u32 n = load<u32>(kLayerEntryCount);
    u32 e = load<u32>(kLayerEntries);
    for (u32 i = 0; i < n; ++i, e += kLayerEntrySize) {
        const u32 o = load<u32>(e + kEntryObject);
        if (o == 0) continue;
        u32 eh = load<u32>(e + kEntryHandle);
        if (eh == 0) eh = load<u32>(o + kObjId);
        if (eh == h) return ret(ctx, o);
    }
    ret(ctx, 0u);
}

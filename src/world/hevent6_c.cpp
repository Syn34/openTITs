// World_HandleEvent callees, part 6: the binding lists at 0x00F6689C /
// 0x00F668A0, sound release, the file operation starter and the object
// list removal (76..90 instructions).
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent6c {
namespace cfn {
constexpr u32 SmallAlloc = 0x0055238C;          // (size) -> block or 0
constexpr u32 Sound_Free445390 = 0x00445390;    // (SoundMgr*, Sound*) (name?)
constexpr u32 Mem_New = 0x007A3DD8;             // operator new(size)
constexpr u32 FsOp_Init = 0x00570B9C;           // (op, target, kind, options in r6-r8)
constexpr u32 FsOp_Start571A00 = 0x00571A00;    // (op, path, u32, s64 size, options in r7-r9) -> ok (name?)
constexpr u32 FsOp_Destruct = 0x00570DA8;
constexpr u32 Mem_FreePool = 0x0056C730;        // (p, pool, tag)
constexpr u32 Obj_Pos3628B4 = 0x003628B4;       // (vec3* out, Object*) (name?)
constexpr u32 Obj_Place3626E8 = 0x003626E8;     // (Object*, const vec3*, 0) (name?)
}  // namespace cfn

// Bindings: sources at 0x00F6689C ({+36 u32 id, +48 Range*, +52 next}),
// bindings at 0x00F668A0 (32 bytes, vtable 0x00B228A0):
//   +4 source, +8 u32 key, +12 u8 owned, +13 u8, +16 Range* (+4 f32),
//   +20 f32, +24 first handler (+16 next, vfunc +4 release), +28 next.
constexpr u32 kSources = 0x00F6689C;
constexpr u32 kBindings = 0x00F668A0;
constexpr u32 kBindingVtable = 0x00B228A0;
constexpr u32 kBindingSize = 32;
constexpr u32 kDefaultRange = 0x004E2DA8;      // f32 used without a range

// Sounds: +4 emitter handle, +96 u8 3d, +97 u8 playing, +98 u8 kept,
// +99 u8 active, +108 u32 refs, +116 prev, +120 next. Manager list heads:
// +8196 idle, +8200 / +8208 playing (3d / 2d), +8204 3d tail.
constexpr u32 kSndRefs = 108, kSndPrev = 116, kSndNext = 120;
constexpr u32 kMgrIdle = 8196, kMgrPlaying3d = 8200, kMgrTail3d = 8204, kMgrPlaying2d = 8208;

// Object lists by kind: 0x0106C2D4 + 20 * kind, {+0 Object**, +8 u16 count}.
constexpr u32 kObjLists = 0x0106C2D4;
constexpr u32 kObjListStride = 20;
constexpr u32 kKindNone = 5;
constexpr u32 kFsOpKindOpen = 6;
}  // namespace hevent6c
}  // namespace sly4

using namespace sly4::hevent6c;

// void Obj_Set4E3168(const char* source, u32 key) (name?; better Binding_Clear): for every
// binding whose source id matches the name's hash (any with hash 0) and whose
// key matches (any with key 0): releases its handlers (vfunc +4), clears
// +24 / +16 / +20, and an owned (+12) binding releases itself (vfunc +4).
// r3 is left as the original leaves it.
DECOMP(004E3168, Obj_Set4E3168) {
    const u32 key = arg<u32>(ctx, 1);
    const u32 hash = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {arg<u32>(ctx, 0)}));
    u64 r3 = hash;
    for (u32 b = load<u32>(kBindings); b != 0;) {
        const u32 next = load<u32>(b + 28);
        if (hash != 0) {
            r3 = load<u32>(load<u32>(b + 4) + 36);
            if (static_cast<u32>(r3) != hash) {
                b = next;
                continue;
            }
        }
        if (key != 0) {
            r3 = load<u32>(b + 8);
            if (static_cast<u32>(r3) != key) {
                b = next;
                continue;
            }
        }
        for (u32 h = load<u32>(b + 24); h != 0;) {
            const u32 hn = load<u32>(h + 16);
            vcall(ctx, h, 4, {h});
            h = hn;
        }
        r3 = 0;
        const u32 owned = load<u8>(b + 12);
        store<u32>(b + 24, 0);
        store<u32>(b + 16, 0);
        store<u32>(b + 20, 0);
        if (owned != 0) r3 = vcall(ctx, b, 4, {b});
        b = next;
    }
    ctx.gpr[3] = r3;
}

// Binding* Obj_Attach4E2DB0(u32 source_id, u32 key, u8 owned, u8 b) (name?):
// a new binding (SmallAlloc 32) to the source with that id, appended to the
// list at 0x00F668A0, its +16 range and +20 = the range's +4 (or the
// default); 0 when there is no such source or the allocation fails.
DECOMP(004E2DB0, Obj_Attach4E2DB0) {
    const u32 id = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 owned = arg<u32>(ctx, 2), b13 = arg<u32>(ctx, 3);
    u32 src = load<u32>(kSources);
    while (src != 0 && load<u32>(src + 36) != id) src = load<u32>(src + 52);
    if (src == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = static_cast<u32>(call(ctx, cfn::SmallAlloc, {kBindingSize}));
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(b + 8, key);
    store<u32>(b + 0, kBindingVtable);
    store<u32>(b + 16, 0);
    store<u32>(b + 24, 0);
    store<u32>(b + 4, src);
    store<u32>(b + 28, 0);
    u32 last = load<u32>(kBindings);
    store<u8>(b + 12, static_cast<u8>(owned));
    store<u8>(b + 13, static_cast<u8>(b13));
    store<u32>(b + 20, 0);
    if (last != 0) {
        for (u32 n; (n = load<u32>(last + 28)) != 0;) last = n;
        store<u32>(last + 28, b);
        src = load<u32>(b + 4);
    } else {
        store<u32>(kBindings, b);
    }
    const u32 range = load<u32>(src + 48);
    store<u32>(b + 16, range);
    const u32 from = range != 0 ? range + 4 : kDefaultRange;
    store<u32>(b + 20, load<u32>(from));  // lfs / stfs copied as a word
    ctx.gpr[4] = kDefaultRange;           // r4 / f1 as the original leaves them
    ctx.fpr[1] = load<f32>(from);
    ret(ctx, b);
}

// void Audio_Release446384(SoundMgr* m, Sound* s) (name?): drops one
// reference; at zero the sound leaves its list (inactive: the idle list;
// active and not kept: a playing one stops its emitter, then it leaves the
// 3d / 2d playing list) and is freed (0x00445390). Kept sounds stay.
DECOMP(00446384, Audio_Release446384) {
    const u32 m = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    if (s == 0) return;
    const u32 refs = load<u32>(s + kSndRefs);
    if (refs == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(s + kSndRefs, refs - 1);
    if (refs - 1 != 0) {
        ret(ctx, refs - 1);
        return;
    }
    if (load<u8>(s + 99) == 0) {
        const u32 prev = load<u32>(s + kSndPrev);
        if (prev != 0) store<u32>(prev + kSndNext, load<u32>(s + kSndNext));
        else store<u32>(m + kMgrIdle, load<u32>(s + kSndNext));
        const u32 next = load<u32>(s + kSndNext);
        if (next != 0) store<u32>(next + kSndPrev, load<u32>(s + kSndPrev));
        call(ctx, cfn::Sound_Free445390, {m, s});
        return;
    }
    const u32 kept = load<u8>(s + 98);
    if (kept != 0) {
        ret(ctx, kept);
        return;
    }
    if (load<u8>(s + 97) != 0) call(ctx, hfn::Emitter_Stop, {load<u32>(s + 4), 0});
    const u32 prev = load<u32>(s + kSndPrev);
    if (prev != 0) store<u32>(prev + kSndNext, load<u32>(s + kSndNext));
    else if (load<u8>(s + 96) != 0) store<u32>(m + kMgrPlaying3d, load<u32>(s + kSndNext));
    else store<u32>(m + kMgrPlaying2d, load<u32>(s + kSndNext));
    const u32 next = load<u32>(s + kSndNext);
    if (next != 0) store<u32>(next + kSndPrev, load<u32>(s + kSndPrev));
    else if (load<u8>(s + 96) != 0) store<u32>(m + kMgrTail3d, load<u32>(s + kSndPrev));
    store<u8>(s + 97, 0);
    store<u8>(s + 99, 0);
    call(ctx, cfn::Sound_Free445390, {m, s, 0});
}

// FsOp* FsOp_Begin5627D4(path, u32 a, s64 size, FsOptions options) (name?):
// a new 24-byte FsOp (FsOp_Init kind 6) started on path; when the start
// fails it is destroyed and freed and 0 returned. The 24-byte options come by
// value in r6-r8 and go on in r6-r8 / r7-r9 (64-bit).
DECOMP(005627D4, FsOp_Begin5627D4) {
    const u64 path = ctx.gpr[3], a = ctx.gpr[4], size = ctx.gpr[5];
    const u64 o0 = ctx.gpr[6], o1 = ctx.gpr[7], o2 = ctx.gpr[8];
    const u32 op = static_cast<u32>(call(ctx, cfn::Mem_New, {24}));
    if (op == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, cfn::FsOp_Init, {op, 0, kFsOpKindOpen, o0, o1, o2});
    if (call(ctx, cfn::FsOp_Start571A00, {op, path, a, size, o0, o1, o2}) != 0) {
        ret(ctx, op);
        return;
    }
    call(ctx, cfn::FsOp_Destruct, {op});
    call(ctx, cfn::Mem_FreePool, {op, 0, 0});
    ret(ctx, 0u);
}

// void Obj_Prepare3629D8(Object* o) (name?): removes o from the object list
// of its kind (+8 desc, +84 kind; none for no desc or kind 5), shifting the
// later entries down; each of those is then re-placed at its own position
// (0x003628B4 into a stack vec3, then 0x003626E8(entry, &pos, 0)).
DECOMP(003629D8, Obj_Prepare3629D8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 desc = load<u32>(o + 8);
    if (desc == 0) return;
    const u32 kind = load<u32>(desc + 84);
    if (kind == kKindNone) return;
    const u32 list = kObjLists + kind * kObjListStride;
    const u32 n = load<u16>(list + 8);
    if (n == 0) return;
    u32 i = 0;
    while (load<u32>(load<u32>(list) + 4 * i) != o) {
        if (++i >= n) return;
    }
    u32 count = n;
    if (static_cast<s32>(i) != static_cast<s32>(n - 1) && static_cast<s32>(i & 0xFFFF) < static_cast<s32>(n - 1)) {
        u32 j = i & 0xFFFF;
        bool more;
        do {
            const u32 arr = load<u32>(list);
            count = load<u16>(list + 8);
            const u32 v = load<u32>(arr + 4 * j + 4);
            const u32 at = arr + 4 * j;
            j = (j + 1) & 0xFFFF;
            more = static_cast<s32>(j) < static_cast<s32>(count - 1);
            store<u32>(at, v);
        } while (more);
    }
    count -= 1;
    store<u16>(list + 8, static_cast<u16>(count));
    Stack frame(ctx, 16);  // vec3 at r1+112
    for (u32 off = i * 4; i < (count & 0xFFFF); off += 4, ++i) {
        call(ctx, cfn::Obj_Pos3628B4, {frame.addr(), load<u32>(load<u32>(list) + off)});
        call(ctx, cfn::Obj_Place3626E8, {load<u32>(load<u32>(list) + off), frame.addr(), 0});
        count = load<u16>(list + 8);
    }
    ctx.gpr[3] = count & 0xFFFF;  // r3 as the original leaves it
}

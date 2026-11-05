// World_HandleEvent callees, part 20k: two object constructors of the
// level spawn table, the timed callback pool reset and the overlay layer
// teardown.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjBaseConstruct = 0x0035695C;  // (Object*, u32 kind, Def*) (symbols.txt Obj_BaseConstruct35695C)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> const char* (symbols.txt)
constexpr u32 kStringHashNoCase = 0x0056A710;  // (const char*) -> u32 (symbols.txt)
constexpr u32 kFmtIntro = 0x00ADC768, kFmtIdle = 0x00ADC788, kFmtOutro = 0x00ADC7A8;  // "%s_Intro" / "%s_Idle" / "%s_Outro"
constexpr u32 kVtable359CDC = 0x00B1F7C8;
constexpr u32 kVtable35A258 = 0x00B1F800;
constexpr u32 kNodePush = 0x00446984;          // (u32 hash, u32 arg) -> node (symbols.txt Node_PushF65B7C)
constexpr u32 kNodeHash = 0xB106F474;
constexpr u32 kNodeArg = 0x010B809C;           // u32 given to Node_PushF65B7C
constexpr u32 kFindById = 0x004A5AE8;          // (WorldObj*, s32 id) -> WorldObj* (WorldObj_FindById4A5AE8)
constexpr u32 kHash35A258A = 0xA5108477, kHash35A258B = 0x73C8B956;
constexpr u32 kMemAlloc = 0x0054ED38;          // (size, 0, align) (symbols.txt Mem_Alloc)
constexpr u32 kMemFreeFrom = 0x0054EE6C;       // (p, pool) (symbols.txt Mem_FreeFrom)
constexpr u32 kObjClearField8 = 0x00565A88;    // (o) (symbols.txt Obj_ClearField8)
constexpr u32 kPoolCreate = 0x00565AA8;        // (pool, buf, size, 0) (symbols.txt Pool_Create)
constexpr u32 kCbPool = 0x00F65B58;            // u32 24-byte Pool*
constexpr u32 kCbBuf = 0x00F65B5C;             // u32 its buffer
constexpr u32 kPoolDestructPart = 0x0052EFE0;  // (part*) (symbols.txt Pool_DestructPart)
constexpr u32 kOverlay = 0x010AA1B0;           // u8 flags (0x20: live), parts +8 / +24 / +40 / +56, +82 / +84 u16, +1124 u32 Pair*
}  // namespace

// Object* Obj_Construct359CDC(Object* o, Def* d) (name?): base object, then
// vtable 0x00B1F7C8, +180 = 0, +184 bit 31 = d +145 bit 0, +4 |= 0x20,
// +164..+176 zeroed; with a name (d +36, else d +4): +168 / +172 / +176 the
// hashes of "<name>_Intro" / "_Idle" / "_Outro".
DECOMP(00359CDC, Obj_Construct359CDC) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    const u32 w = (load<u32>(o + 184) & 0x7FFFFFFFu) | ((load<u8>(d + 145) & 1u) << 31);
    store<u32>(o + 180, 0);
    store<u32>(o + 184, w);
    store<u32>(o, kVtable359CDC);
    store<u32>(o + 4, load<u32>(o + 4) | 0x20);
    for (u32 k = 0; k < 16; k += 4) store<u32>(o + 164 + k, 0);
    u32 name = d + 36;
    u32 c = load<u8>(name);
    if (c == 0) name = d + 4;
    if (name == 0 || (c = load<u8>(name)) == 0) {
        ret(ctx, c);
        return;
    }
    const u32 fmts[3] = {kFmtIntro, kFmtIdle, kFmtOutro};
    for (u32 k = 0; k < 3; ++k) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
        store<u32>(o + 168 + 4 * k, static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
    }
}

// Object* Obj_Construct35A258(Object* o, Def* d) (name?): base object, then
// vtable 0x00B1F800, +164..+184 zeroed, +200 a timed node
// (Node_PushF65B7C(0xB106F474, *0x010B809C)), +204 = 0, +4 |= 0x40; with an
// owner (+24): +164 / +168 / +172 the objects of ids d +160 / +164 / +168;
// +176..+184 = d +148..+156, +188 zeroed, +192 / +196 two hashes.
DECOMP(0035A258, Obj_Construct35A258) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable35A258);
    for (u32 k = 0; k < 24; k += 4) store<u32>(o + 164 + k, 0);
    const u32 node = static_cast<u32>(call(ctx, kNodePush, {kNodeHash, load<u32>(kNodeArg)}));
    store<u32>(o + 200, node);
    store<u32>(o + 204, 0);
    store<u32>(o + 4, load<u32>(o + 4) | 0x40);
    if (load<u32>(o + 24) != 0) {
        for (u32 k = 0; k < 3; ++k) {
            const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(d + 160 + 4 * k)}));
            store<u32>(o + 164 + 4 * k, r);
        }
    }
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 176 + k, load<u32>(d + 148 + k));
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 188 + k, 0);
    store<u32>(o + 192, kHash35A258A);
    store<u32>(o + 196, kHash35A258B);
    ret(ctx, o + 188);
}

// void Callbacks_Reset44D880(s32 kb) (name?): the timed callback pool
// (0x00F65B58, Obj_ClearField8 then freed) and its buffer (0x00F65B5C)
// freed; for kb > 0 a kb KB buffer (align 16) and a 24-byte pool over it
// (Pool_Create) are allocated (0 on failure).
DECOMP(0044D880, Callbacks_Reset44D880) {
    const s32 kb = arg<s32>(ctx, 0);
    const u32 pool = load<u32>(kCbPool);
    if (pool != 0) {
        call(ctx, kObjClearField8, {pool});
        call(ctx, kMemFreeFrom, {pool, 0});
        store<u32>(kCbPool, 0);
    }
    const u32 buf = load<u32>(kCbBuf);
    if (buf != 0) {
        call(ctx, kMemFreeFrom, {buf, 0});
        store<u32>(kCbBuf, 0);
    }
    u64 r3 = buf != 0 ? (kCbBuf & 0xFFFF0000u) : 0;
    if (kb > 0) {
        const u32 size = static_cast<u32>(kb) << 10;
        const u32 nb = static_cast<u32>(call(ctx, kMemAlloc, {size, 0, 16}));
        store<u32>(kCbBuf, nb);
        r3 = 0;
        if (nb != 0) {
            const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {24, 0, 16}));
            if (p != 0) {
                for (u32 k = 0; k < 24; k += 4) store<u32>(p + k, 0);
                call(ctx, kPoolCreate, {p, load<u32>(kCbBuf), size, 0});
            }
            store<u32>(kCbPool, p);
            r3 = kCbPool & 0xFFFF0000u;
        }
    }
    ret(ctx, r3);
}

// void Overlay_Free429090() (name?): when the overlay at 0x010AA1B0 is live
// (bit 0x20): its four pool parts destructed, the +1124 pair (two entries,
// each with a +4 block) freed; then the live bit and +82 / +84 cleared.
DECOMP(00429090, Overlay_Free429090) {
    if ((load<u8>(kOverlay) & 0x20) == 0) {
        ret(ctx, 0);
        return;
    }
    for (u32 i = 0; i < 3; ++i) call(ctx, kPoolDestructPart, {kOverlay + 8 + 16 * i});
    call(ctx, kPoolDestructPart, {kOverlay + 56});
    const u32 pair = load<u32>(kOverlay + 1124);
    if (pair != 0) {
        for (u32 k = 0; k < 8; k += 4) {
            const u32 e = load<u32>(pair + k);
            if (e == 0) continue;
            const u32 blk = load<u32>(e + 4);
            if (blk != 0) {
                call(ctx, kMemFreeFrom, {blk, 0});
                store<u32>(e + 4, 0);
            }
            call(ctx, kMemFreeFrom, {e, 0});
        }
        call(ctx, kMemFreeFrom, {pair, 0});
        store<u32>(kOverlay + 1124, 0);
    }
    store<u8>(kOverlay, static_cast<u8>(load<u8>(kOverlay) & ~0x20u));
    store<u16>(kOverlay + 82, 0);
    store<u16>(kOverlay + 84, 0);
    ret(ctx, 0x010B0000u);  // lis r3, 0x10B (left)
}

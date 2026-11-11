// Game code, medium sweep part 26: more of the world-object destructor
// family whose base part sets vtable 0x00B1F678 (see mid25_a.cpp), with the
// "set parts enabled" helpers they call first.
#include "mid26.h"

using namespace sly4;
using namespace sly4::mid26;

// void f(T* o, u8 on) (called with 0 by Obj_Dtor_B1F918_0037A93C): the
// component +24 gets vt[40](c, on, 1); each def part (o->+8: 28-byte
// records +180, u32 count +184, re-read after a call) with an object +12
// gets vt[88](p, on); the slot +376 [o->+256] and the four words +288 get
// vt[40](e, 0, 1); then, per slot of six, the words +304, +328, +352, +376,
// +408 (not checked: the original tests the address o + 408 instead),
// +432 and +456. With on == 0, World_ListRemove_0046B7A4(world, 3,
// 0x00F37AF8, o, 0) last. r3 = that call's, else what the last vt[40] left.
DECOMP(0037A58C, Obj_SetPartsEnabled_0037A58C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    Vt40(ctx, load<u32>(o + 24), on);
    const u32 d = load<u32>(o + 8);
    Notify88On(ctx, d + 184, d + 180, 28, 12, on);
    Vt40(ctx, load<u32>(o + 376 + 4 * load<u32>(o + 256)), 0);
    Vt40Each(ctx, o, {288}, 4, 0);
    u32 r = 0;
    for (u32 k = 0; k < 6; ++k) {
        const u32 b = o + 4 * k;
        Vt40Each(ctx, b, {304, 328, 352, 376}, 1, 0);
        if (o + 408 != 0) {  // always true: the compiled test is on the slot's address
            const u32 e = load<u32>(b + 408);
            VCall(ctx, e, 40, {e, 0, 1});  // unchecked (quirk)
        }
        r = Vt40Each(ctx, b, {432, 456}, 1, 0);
    }
    if (on != 0) {
        ctx.gpr[3] = r;
        return;
    }
    Unlist26(ctx, o, 3, 0x00F37AF8u);
}

// T::~T(o) (Delete_0037AC20, 480 bytes): vtable 0x00B1F918, unlisted
// (8, 0x00F37AF0) and (3, 0x00F37AF8); Obj_SetPartsEnabled_0037A58C(o, 0);
// o->+232 freed; the four words +288 get vt[8]; per slot of six the words
// +408, +432 and +456 get vt[8] and are cleared; base tail.
DECOMP(0037A93C, Obj_Dtor_B1F918_0037A93C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F918u);
    Unlist26(ctx, o, 8, 0x00F37AF0u);
    Unlist26(ctx, o, 3, 0x00F37AF8u);
    call(ctx, 0x0037A58Cu, {o, 0});
    FreeSlot26(ctx, o + 232);
    Vt8Each(ctx, o, {288}, 4, false);
    Vt8Each(ctx, o, {408, 432, 456}, 6, true);
    BaseTail26(ctx, o);
}

// void f(T* o, u8 on) (called with 0 by Obj_Dtor_B1F9C8_00381AA8): the
// component +24 gets vt[40](c, on, 1); only with on == 0 do the rest get
// vt[40](e, 0, 1): per slot of twelve the words +248, +296 and +344; +392;
// +396; the nine words +212; the 32 words +400; +528.
// r3 = what the last vt[40] left (0 when its word was not set).
DECOMP(003817E0, Obj_SetPartsEnabled_003817E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    u32 r = Vt40(ctx, load<u32>(o + 24), on);
    if (on == 0) {
        Vt40Each(ctx, o, {248, 296, 344}, 12, 0);
        Vt40(ctx, load<u32>(o + 392), 0);
        Vt40(ctx, load<u32>(o + 396), 0);
        Vt40Each(ctx, o, {212}, 9, 0);
        r = Vt40Each(ctx, o, {400}, 32, 0);
        if (const u32 e = load<u32>(o + 528); e != 0) r = Vt40(ctx, e, 0);
    }
    ctx.gpr[3] = r;
}

// T::~T(o) (Delete_00381DF8): vtable 0x00B1F9C8; Obj_SetPartsEnabled_
// 003817E0(o, 0); vt[8] for the nine words +212, per slot of twelve the
// words +248 / +296 / +344, +392 and +396 (cleared), the 32 words +400 and
// +528 (cleared); base tail.
DECOMP(00381AA8, Obj_Dtor_B1F9C8_00381AA8) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F9C8u);
    call(ctx, 0x003817E0u, {o, 0});
    Vt8Each(ctx, o, {212}, 9, false);
    Vt8Each(ctx, o, {248, 296, 344}, 12, false);
    Vt8Slot(ctx, o + 392, true);
    Vt8Slot(ctx, o + 396, true);
    Vt8Each(ctx, o, {400}, 32, false);
    Vt8Slot(ctx, o + 528, true);
    BaseTail26(ctx, o);
}

namespace {
constexpr u32 kRefNodeRelease = 0x004432DCu;  // RefNode_Release(node)
constexpr u32 kEmitterStop = 0x00443424u;     // Emitter_Stop(handle, fade)
}  // namespace

// T::~T(o) (Delete_00384E9C): vtable 0x00B1FA08, unlisted (8, 0x00F37BE8),
// (6, 0x00F37BF0) and (4, 0x00F37BE0); o->+24->vt[40](.., 0, 1); the def's
// entries (o->+8, count +152) notify the objects of the 16-byte records
// o->+236 (+4) by vt[88](e, 0); +256 vt[8] (cleared); the words +276 for
// i < o->+540 (re-read after a call) vt[8] (cleared); the record objects
// whose def entry (36 bytes at def->+148, word +28) has bit 0x80000000 get
// vt[4] (count re-read after a call); o->+236 freed; vt[8] for the nine
// words +588, the 32 words +624, per slot of eight +752 / +784, the 32 words
// +816 and +944 (cleared); RefNode_Release(+952), RefNode_Release(+948),
// Emitter_Stop(+956, 1); base tail.
DECOMP(003848FC, Obj_Dtor_B1FA08_003848FC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1FA08u);
    Unlist26(ctx, o, 8, 0x00F37BE8u);
    Unlist26(ctx, o, 6, 0x00F37BF0u);
    Unlist26(ctx, o, 4, 0x00F37BE0u);
    Vt40(ctx, load<u32>(o + 24), 0);
    Notify88On(ctx, load<u32>(o + 8) + 152, o + 236, 16, 4, 0);
    Vt8Slot(ctx, o + 256, true);
    for (u32 i = 0, n = load<u32>(o + 540); i < n; ++i) {
        const u32 a = o + 276 + 4 * i;
        const u32 e = load<u32>(a);
        if (e == 0) continue;
        VCall(ctx, e, 8, {e});
        store<u32>(a, 0);
        n = load<u32>(o + 540);
    }
    const u32 d = load<u32>(o + 8);
    for (u32 i = 0, n = load<u32>(d + 152); i < n; ++i) {
        if ((load<u32>(load<u32>(d + 148) + 36 * i + 28) & 0x80000000u) == 0) continue;
        const u32 e = load<u32>(load<u32>(o + 236) + 16 * i + 4);
        if (e == 0) continue;
        VCall(ctx, e, 4, {e});
        n = load<u32>(d + 152);
    }
    FreeSlot26(ctx, o + 236);
    Vt8Each(ctx, o, {588}, 9, false);
    Vt8Each(ctx, o, {624}, 32, false);
    Vt8Each(ctx, o, {752, 784}, 8, false);
    Vt8Each(ctx, o, {816}, 32, false);
    Vt8Slot(ctx, o + 944, true);
    call(ctx, kRefNodeRelease, {load<u32>(o + 952)});
    call(ctx, kRefNodeRelease, {load<u32>(o + 948)});
    call(ctx, kEmitterStop, {load<u32>(o + 956), 1});
    BaseTail26(ctx, o);
}

namespace {
constexpr u32 kTmplRegistry26 = 0x01056F04u;  // template registry (as hevent17_g.cpp)
constexpr u32 kTmplVtBuild26 = 0x00B0DAACu;   // template base vtable

// The body shared by the two destructors of vtable 0x00B1C978: the 12
// buffers +104..+148 and +168 / +172 are freed (Mem_FreeFrom, cleared);
// then the base: vtable 0x00B0DAAC and SortedMap_Remove7330C0(registry,
// o + 4) (r3 = its).
void TmplDtorB1C978(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B1C978u);
    for (u32 off = 104; off <= 148; off += 4) FreeSlot26(ctx, o + off);
    FreeSlot26(ctx, o + 168);
    FreeSlot26(ctx, o + 172);
    store<u32>(o, kTmplVtBuild26);
    call(ctx, m26fn::SortedMap_Remove7330C0, {kTmplRegistry26, o + 4});
}
}  // namespace

// T::~T(o) (vtable 0x00B1C978 slot): see TmplDtorB1C978.
DECOMP(00764848, Tmpl_Dtor_B1C978_00764848) { TmplDtorB1C978(ctx, arg<u32>(ctx, 0)); }

// Deleting destructor (vtable 0x00B1C978 slot): nothing for a null o
// (r3 = 0); else TmplDtorB1C978 then SmallFree(o, 436) (r3 = its).
DECOMP(00764A60, Tmpl_Delete_B1C978_00764A60) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    TmplDtorB1C978(ctx, o);
    call(ctx, m26fn::SmallFree, {o, 436});
}

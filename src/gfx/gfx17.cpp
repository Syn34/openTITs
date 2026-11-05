// The renderer range's next recompiled functions (gfx17). Names carry the
// address where the role is unclear.
#include <atomic>
#include <bit>
#include <cmath>
#include <utility>

#include "fx.h"
#include "gfx17.h"
#include "../obj/obj.h"
#include "../obj/obj4.h"
#include "../obj/obj5.h"
#include "../obj/obj6.h"
#include "../obj/obj8.h"

using namespace sly4;

namespace {

namespace g10 = sly4::render::gfx10;
namespace g11 = sly4::render::gfx11;
namespace g16 = sly4::render::gfx16;
namespace g17 = sly4::render::gfx17;
using g11::lfs;
using g11::sgl;

// Copies the 3x3 part of a 4x4 f32 matrix into three 16-byte rows (lfs /
// stfs pairs, raw words); the rows' w words are left as they are.
void rows3x3(u32 dst, u32 m) {
    for (u32 r = 0; r < 3; ++r)
        for (u32 c = 0; c < 3; ++c) store<u32>(dst + 16 * r + 4 * c, load<u32>(m + 16 * r + 4 * c));
}

// ApeTListAllocator<AglDeferredDestructionObj>::GetAllocator(), inlined:
// the header's list, made (60 bytes, ApeTFreeList's inlined constructor,
// the original's store order) on first use. As in the original a failed
// allocation stores 0 and the caller goes on with it.
u32 defer_list(PPUContext& ctx) {
    const u32 hdr = load<u32>(g17::defer_state);
    u32 list = load<u32>(hdr);
    if (list != 0) return list;
    list = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g17::kFreeListSize, 0, 16}));
    if (list != 0) {
        const u32 count = load<u32>(hdr + 4);
        for (u32 off : {8u, 4u, 12u, 20u, 16u, 24u}) store<u32>(list + off, 0);
        const u16 flags = load<u16>(list + 54);
        for (u32 off : {32u, 28u, 36u, 40u, 44u, 48u}) store<u32>(list + off, 0);
        store<u16>(list + 52, 0);
        store<u16>(list + 54, flags & 0x7FFF);  // rlwimi 0,16,16: bit 0x8000 cleared
        store<u32>(list, g17::vt_defer_list);
        store<u32>(list + 56, g17::str_defer_list);
        call(ctx, g17::fn::FreeList_Init, {list, g17::kDeferEntrySize, count, 0, 0});
    }
    store<u32>(hdr, list);
    return list;
}

// The first entry of the list (its first node + 16), or 0.
u32 defer_first(PPUContext& ctx, u32 list) {
    const u32 node = static_cast<u32>(call(ctx, g17::fn::Obj_GetField4, {list}));
    return node != 0 ? node + g17::kFreeListNode : 0;
}

// Material param slot k = v.
void set_param17(PPUContext& ctx, u32 m, u32 k, u32 v) {
    store<u32>(static_cast<u32>(call(ctx, g17::fn::Material_ParamSlot, {m, k})), v);
}

// A colour word with its bytes 1 and 3 swapped (ARGB <-> ABGR; rlwinm /
// rlwimi 16,8,15 / 16,24,31).
constexpr u32 swap_rb(u32 c) { return (c & 0xFF00FF00u) | ((c << 16) & 0x00FF0000u) | ((c >> 16) & 0xFFu); }

// Appends the 4 colour bytes at src to the emitter curve c (u8 +6
// count, u32 colours from +20); the first one is also written as the
// second (gfx16.cpp's curve_push16).
void curve_push17(u32 c, u32 src) {
    const u32 n = load<u8>(c + 6);
    store<u8>(c + 6, static_cast<u8>(n + 1));
    const u32 dst = c + 20 + ((n << 2) & ~3u);
    for (u32 k = 0; k < 4; ++k) store<u8>(dst + k, load<u8>(src + k));
    const u32 m = load<u8>(c + 6);
    if (m == 1)
        for (u32 k = 0; k < 4; ++k) store<u8>(c + 20 + ((m << 2) & ~3u) + k, load<u8>(src + k));
}

// The shared tail of the instance IFF readers (obj/obj18.cpp
// CameraInstance_ReadChunks): the header's callback told before (0) and
// after (1) the chunks are read (r6 / r7 = its OPD and entry); then o is
// added to the parent (tag -1) or, without one, moved by the point offset
// and marked dirty (the inlined WorldObj_MarkDirty); then entered into
// the name map (marked unsorted; +8 bit 0x400). Returns 1.
void read_instance17(PPUContext& ctx, u32 holder, u32 r, u32 parent, u32 o, bool geom = false) {
    u32 hdr = load<u32>(holder);
    u32 cb = load<u32>(hdr + g17::kHdrCallback);
    if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(hdr + g17::kHdrCallbackArg), 0, cb, load<u32>(cb)});
    call(ctx, g17::fn::Iff_ReadChunks, {r, o});
    hdr = load<u32>(holder);
    cb = load<u32>(hdr + g17::kHdrCallback);
    if (cb != 0) call_ptr(ctx, cb, {o, load<u32>(hdr + g17::kHdrCallbackArg), 1, cb, load<u32>(cb)});
    if (parent != 0) {
        const u32 opd = load<u32>(load<u32>(parent) + g17::kVfnAddChild);
        call_ptr(ctx, opd, {parent, o, ~0ull, opd, load<u32>(opd)});
    } else {
        const u32 off = sly4::obj5::kPointOffset;
        const f64 x = g11::lfs(o + 64), y = g11::lfs(o + 68), z = g11::lfs(o + 72);
        const f64 nx = sgl(x + g11::lfs(off)), ny = sgl(y + g11::lfs(off + 4)), nz = sgl(z + g11::lfs(off + 8));  // fadds
        store<u32>(o + 64, g11::stfs_bits(nx));
        store<u32>(o + 68, g11::stfs_bits(ny));
        store<u32>(o + 72, g11::stfs_bits(nz));
        ctx.fpr[1] = nx;  // f1..f3 as the original leaves them
        ctx.fpr[2] = ny;
        ctx.fpr[3] = nz;
        sly4::obj4::MarkDirtyInline(ctx, o);
    }
    if (geom) {  // ApeGeomInstance: u8 +322 bit 0x20 cleared; mapped only with bits 9..12 of +320 == 3
        const u32 b = load<u8>(o + g17::kGeomFlagByte) & ~0x20u;  // rlwimi 0,26,26
        const u32 w = (load<u32>(o + g17::kGeomFlags) & ~0xFF00u) | (b << 8);  // rlwimi 8,16,23
        store<u8>(o + g17::kGeomFlagByte, static_cast<u8>(b));
        if (((w >> 9) & 0xF) != g17::kGeomMapped) return ret(ctx, 1u);  // rlwinm 23,28,31
    }
    const u32 key = load<u32>(o + sly4::obj::kObjNameHash);
    const u32 m = load<u32>(load<u32>(holder) + g17::kHdrMap);
    u32 cnt = load<u16>(m + 2);
    const u32 cap = load<u16>(m + 0);
    if (cnt == cap) {
        call(ctx, g17::fn::Array_Reserve, {m, (cap << 1) & 0xFFFEu});  // rlwinm 1,16,30
        cnt = load<u16>(m + 2);
    }
    const u32 e = load<u32>(m + 8);
    store<u32>(e + cnt * 8, key);
    store<u32>(e + load<u16>(m + 2) * 8 + 4, o);
    const u32 nc = load<u16>(m + 2) + 1u;
    const u32 peak = load<u16>(m + 4);
    const u32 flags = load<u16>(m + 6);
    store<u16>(m + 2, static_cast<u16>(nc));
    store<u16>(m + 4, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(nc & 0xFFFF) ? peak : nc));
    store<u16>(m + 6, static_cast<u16>(flags | 1));
    store<u32>(o + 8, load<u32>(o + 8) | g17::kObjInMap);
    ret(ctx, 1u);
}

}  // namespace

// u32 DeferredDestroy_Update_00512148()  [Render_Stop5124F4, WaitRenderIdle_005126E4] (name?)
// Runs the deferred destructions that are due (frame counter >= the
// entry's due frame, unsigned): destroy(obj) through its OPD, then the
// entry goes back to the list (vtable +16). Then ++frame counter, and
// returns 1 once the list is empty. The next node is read before the
// entry is destroyed.
DECOMP(00512148, DeferredDestroy_Update_00512148) {
    u32 cur = defer_first(ctx, defer_list(ctx));
    while (cur != 0) {
        const u32 next_node = load<u32>(cur - g17::kFreeListNode);
        const u32 next = next_node != 0 ? next_node + g17::kFreeListNode : 0;
        if (load<u32>(g17::defer_state + 4) >= load<u32>(cur + 4)) {
            const u32 destroy = load<u32>(cur + 8);
            call_ptr(ctx, destroy, {load<u32>(cur), destroy, load<u32>(destroy)});
            const u32 list = defer_list(ctx);
            const u32 opd = load<u32>(load<u32>(list) + g17::kFreeListFree);  // 0 list: called through, as the original
            call_ptr(ctx, opd, {list, cur, opd, load<u32>(opd)});
        }
        cur = next;
    }
    store<u32>(g17::defer_state + 4, load<u32>(g17::defer_state + 4) + 1);
    const u32 node = static_cast<u32>(call(ctx, g17::fn::Obj_GetField4, {defer_list(ctx)}));
    ret(ctx, node == 0 || node + g17::kFreeListNode == 0 ? 1u : 0u);
}

// void UvAnim_SetConstants_005D7438(Shader* s, Material* m)  [vtable 0x00B26178 slot 2] (name?)
// Fragment constants of the material's scrolling UV matrices
// (UvAnim_Matrices_005D70E0): 1 = out, 2..4 = a's 3x3 rows, 5..7 = b's.
// The rows' w words are whatever the stack held (the original never
// writes them), so the frame sits where the original's does. r3 is the
// last FProgram_SetConstant's; f1 / f2 are left as the original leaves
// them (b's [2][2] / [2][1]).
DECOMP(005D7438, UvAnim_SetConstants_005D7438) {
    const u32 m = arg<u32>(ctx, 1);
    Stack frame(ctx, g17::kUvAnimFrame);
    const u32 sp = frame.addr();
    const u32 out = sp + g17::kUvAnimOut, rows = sp + g17::kUvAnimRows, a = sp + g17::kUvAnimA, b = sp + g17::kUvAnimB;
    call(ctx, g17::fn::UvAnim_Matrices_005D70E0, {m, a, b, out});
    call(ctx, g17::fn::FProgram_SetConstant, {1, out});
    rows3x3(rows, a);
    call(ctx, g17::fn::FProgram_SetConstant, {2, rows});
    call(ctx, g17::fn::FProgram_SetConstant, {3, rows + 16});
    call(ctx, g17::fn::FProgram_SetConstant, {4, rows + 32});
    rows3x3(rows, b);
    call(ctx, g17::fn::FProgram_SetConstant, {5, rows});
    call(ctx, g17::fn::FProgram_SetConstant, {6, rows + 16});
    call(ctx, g17::fn::FProgram_SetConstant, {7, rows + 32});
    ctx.fpr[1] = g10::lfs_value(load<u32>(b + 40));
    ctx.fpr[2] = g10::lfs_value(load<u32>(b + 36));
}

// void EmitterJob_Run_0056F050(const JobHeader* h, records, u32 count, u32** busy)  [EmitterJobs_Submit_00528130] (name?)
// Under the job mutex: a descriptor from the queue ({queue, handle}),
// WorkQueue_Fn56EEF0 builds the emitter job in it, then it is submitted.
// r3 is the unlock's.
DECOMP(0056F050, EmitterJob_Run_0056F050) {
    const u32 h = arg<u32>(ctx, 0), records = arg<u32>(ctx, 1), count = arg<u32>(ctx, 2), busy = arg<u32>(ctx, 3);
    call(ctx, imp::sys_lwmutex_lock, {g17::job_mutex, g17::kJobLockTimeout});
    Stack frame(ctx, 16);
    const u32 ref = frame.addr();  // {queue, handle}
    store<u32>(ref, g17::job_queue);
    {
        const u32 q = load<u32>(ref);
        const u32 opd = load<u32>(load<u32>(q) + g17::kQueueAcquire);
        store<u32>(ref + 4, static_cast<u32>(call_ptr(ctx, opd, {q, opd, load<u32>(opd)})));
    }
    call(ctx, g17::fn::WorkQueue_Fn56EEF0, {ref, h, records, count, busy});
    {
        const u32 q = load<u32>(ref), handle = load<u32>(ref + 4);
        const u32 opd = load<u32>(load<u32>(q) + g17::kQueueSubmit);
        call_ptr(ctx, opd, {q, handle, opd, load<u32>(opd)});
    }
    call(ctx, imp::sys_lwmutex_unlock, {g17::job_mutex});
}

// void ThermalObject_SetParams_0054CE80(Shader* s, Material* m)  [vtable 0x00B24190 slot 2, PostProc_ThermalObject] (name?)
// The material's params p[0..12] (+124; 0 for all without them) into
// fragment constants (fx::set_fconst, the original's inlined
// FProgram_SetConstant): 1 = p[0..3], 2 = {p[4], p[5], v.z, v.w}, 3 =
// {p[6], p[7], p[8], v.w}, 4 = p[9..12], v = 0x01164BC0. Raw words (lfs /
// stfs pairs). r3 / f1 / f2 are left as the original leaves them.
DECOMP(0054CE80, ThermalObject_SetParams_0054CE80) {
    const u32 m = arg<u32>(ctx, 1);
    const u32 p = load<u32>(m + g17::kMatParams);
    u32 v[13];
    for (u32 k = 0; k < 13; ++k) v[k] = load<u32>(p != 0 ? p + 4 * k : g17::kThermalZero);
    const u32 z = load<u32>(g17::vec4_164BC0 + 8), w = load<u32>(g17::vec4_164BC0 + 12);
    fx::set_fconst(1, v[0], v[1], v[2], v[3]);
    fx::set_fconst(2, v[4], v[5], z, w);
    fx::set_fconst(3, v[6], v[7], v[8], w);
    fx::set_fconst(4, v[9], v[10], v[11], v[12]);
    // r3: the last record's new dirty mask when it is used, else {p[9], p[10]}.
    const u32 fp = load<u32>(fx::g::state + 28);
    ctx.gpr[3] = load<u32>(fp + 592 + 16 * 4) != 0 ? load<u64>(fp + 16) : (u64{v[9]} << 32) | v[10];
    ctx.fpr[1] = g10::lfs_value(v[12]);  // f1 / f2 as the original leaves them
    ctx.fpr[2] = g10::lfs_value(v[11]);
}

// bool SlyTerrainSnow_LoadParams(ShaderType*, Material* m, const Desc* d)  [vtable 0x00B25F28 slot 3, the "SlyTerrainSnow" type's LoadParams] (name?)
// Nothing (1) with config +152 bit 0x80 or desc version (u8 d+84) 0;
// 0 when the texture named d+16 isn't loaded. Otherwise it is added and
// its slot 4 called with (t, 1, 1, 1); then the params, each from the
// desc when its version is high enough (else the default): 3 / 4 the
// colours d+100 (v > 3, else white) / d+104 (v > 4, else 0xFF661919)
// with red and blue swapped, 5..9 d+108.. (v > k), 13 / 14 d+140 / +144,
// 10 colour d+128 (v > 10, else white) swapped, 18 = any of its low three
// bytes set, 11 / 12 d+132 / +136, 15 / 16 = 0. Floats are raw words
// (lfs / stfs pairs).
DECOMP(005CF5B0, SlyTerrainSnow_LoadParams) {
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if ((field<u8>(sly4::g::config, 152) & 0x80) != 0) return ret(ctx, 1u);  // rlwinm 25,31,31
    if (static_cast<s32>(ref<u8>(d + 84)) <= 0) return ret(ctx, 1u);
    const u32 t = static_cast<u32>(call(ctx, g17::fn::Tex_FindLoaded, {load<u32>(d + 16)}));
    if (t == 0) return ret(ctx, 0u);
    call(ctx, g17::fn::Material_AddTexture_004FF008, {m, t});
    fx::vcall(ctx, t, 16, {t, 1, 1, 1});
    auto ver = [&]() { return static_cast<s32>(ref<u8>(d + 84)); };  // re-read each time, as the original
    auto cst = [](u32 k) { return load<u32>(g17::kSnowConsts + 4 * k); };
    {
        const s32 v = ver();
        const u32 a = v > 3 ? load<u32>(d + 100) : g17::kSnowColourA;
        const u32 b = v > 4 ? load<u32>(d + 104) : g17::kSnowColourB;
        set_param17(ctx, m, 3, swap_rb(a));
        set_param17(ctx, m, 4, swap_rb(b));
    }
    const u32 c0 = cst(0);
    set_param17(ctx, m, 5, ver() > 5 ? load<u32>(d + 108) : c0);
    set_param17(ctx, m, 6, ver() > 6 ? load<u32>(d + 112) : c0);
    set_param17(ctx, m, 7, ver() > 7 ? load<u32>(d + 116) : cst(1));
    set_param17(ctx, m, 8, ver() > 8 ? load<u32>(d + 120) : cst(2));
    set_param17(ctx, m, 9, ver() > 9 ? load<u32>(d + 124) : cst(3));
    set_param17(ctx, m, 13, ver() > 13 ? load<u32>(d + 140) : c0);
    set_param17(ctx, m, 14, ver() > 14 ? load<u32>(d + 144) : c0);
    {
        const u32 c = ver() > 10 ? load<u32>(d + 128) : g17::kSnowColourA;
        set_param17(ctx, m, 10, swap_rb(c));
        set_param17(ctx, m, 18, (c & 0x00FFFFFFu) != 0 ? 1u : 0u);
    }
    set_param17(ctx, m, 11, ver() > 11 ? load<u32>(d + 132) : cst(4));
    set_param17(ctx, m, 12, ver() > 12 ? load<u32>(d + 136) : cst(5));
    set_param17(ctx, m, 15, 0);
    set_param17(ctx, m, 16, 0);
    ret(ctx, 1u);
}

// void View_EndDraw_00522140(Owner* o, const Opd* done)  [Sync_00548550] (name?)
// The end of a view's draw (V = o->+672): V's 3x4 matrix +816 saved to
// +1200 (raw words), the debug prims retired and drawn, a 12-byte record
// {22 << 24 | (r4 & 0xFF0000), r4 & 0xFFFF, 0} appended to the stream
// (render::g::state_cache, wrapping to its start; r4 is what
// Debug_Draw_0056AEFC leaves); V's first callback list (+520) run for the
// entries whose mode matches the current View's u8 +48 (each run resets
// the two state caches); the layers' records reset ({-1, 0} at +8, from
// layer 1 when skip_layer0 is set) and each layer ended (vtable +12); the
// caches reset, the current View's end callback (+660) and V's second
// list (+588); then the display bound (no clear) with its default rects,
// the current surface cleared, ++view_counter, and done(o) when given.
DECOMP(00522140, View_EndDraw_00522140) {
    namespace rg = sly4::render::g;
    const u32 o = arg<u32>(ctx, 0), done = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 672);
    for (u32 r = 0; r < 4; ++r)
        for (u32 c = 0; c < 3; ++c)
            store<u32>(v + g17::kViewSavedMatrix + 16 * r + 4 * c, load<u32>(v + g17::kViewMatrix + 16 * r + 4 * c));
    ctx.fpr[1] = g10::lfs_value(load<u32>(v + g17::kViewMatrix + 52));  // f1 / f2 as the copy leaves them
    ctx.fpr[2] = g10::lfs_value(load<u32>(v + g17::kViewMatrix + 56));
    call(ctx, g17::fn::Debug_RetirePrims, {});
    call(ctx, g17::fn::Debug_Draw_0056AEFC, {});
    {
        const u32 x = static_cast<u32>(ctx.gpr[4]);  // leftover r4 of Debug_Draw_0056AEFC
        const u32 w = load<u32>(rg::state_cache);
        u32 p = load<u32>(w + 16);
        if (p + 15 > load<u32>(w + 4)) p = load<u32>(w);  // cmplw
        p = (p + 3) & ~3u;
        store<u32>(w + 16, p);
        store<u32>(p, (g17::kStreamEndViewCmd << 24) | (x & 0x00FF0000u));  // rlwimi 24,0,7; 0,16,31
        store<u32>(p + 8, 0);
        store<u32>(p + 4, x & 0xFFFFu);  // rlwimi 16,0,15
        store<u32>(w + 16, load<u32>(w + 16) + 12);
    }
    auto run_list = [&](u32 list) {
        u32 n = load<u8>(list + g17::kViewCallbackCount);
        if (static_cast<s32>(n) <= 0) return;
        for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(n); i = (i + 1) & 0xFF) {
            const u32 e = list + (i << 4);
            const u32 opd = load<u32>(e);
            if (opd == 0) continue;
            const u32 cur = load<u32>(load<u32>(sly4::render::gfx15::g::current_surface) + 672);
            if (static_cast<s32>(load<u8>(cur + g17::kViewMode)) != static_cast<s32>(load<u32>(e + 8))) continue;  // cmpw
            call_ptr(ctx, opd, {load<u32>(e + 4), load<u32>(opd)});
            store<u64>(rg::state_cache_a, ~u64{0});
            store<u64>(rg::state_cache_b, ~u64{0});
            n = load<u8>(list + g17::kViewCallbackCount);
        }
    };
    run_list(v + g17::kViewCallbacksA);
    {
        const u32 first = load<u8>(g17::skip_layer0) != 0 ? 1u : 0u;
        for (u32 k = first; k < load<u32>(rg::layers); ++k) {
            const u32 layer = load<u32>(rg::layers + 4 + 4 * k);
            const u32 q = load<u32>(layer + g17::kLayerReset);
            store<u32>(q, 0xFFFFFFFFu);
            const u32 q2 = load<u32>(layer + g17::kLayerReset);
            store<u32>(q2 + 4, 0);
            store<u32>(layer + g17::kLayerReset, q2 + 8);
        }
        const u32 first2 = load<u8>(g17::skip_layer0) != 0 ? 1u : 0u;  // re-read after the reset loop, as the original
        for (u32 k = first2; k < load<u32>(rg::layers); ++k) {
            const u32 layer = load<u32>(rg::layers + 4 + 4 * k);
            fx::vcall(ctx, layer, g17::kLayerEnd, {layer});
        }
    }
    store<u64>(rg::state_cache_a, ~u64{0});
    store<u64>(rg::state_cache_b, ~u64{0});
    {
        const u32 cur = load<u32>(load<u32>(sly4::render::gfx15::g::current_surface) + 672);
        const u32 opd = load<u32>(cur + g17::kViewEndCallback);
        call_ptr(ctx, opd, {cur, opd, load<u32>(opd)});
    }
    run_list(v + g17::kViewCallbacksB);
    call(ctx, g17::fn::Surface_Bind_0053B668, {load<u32>(sly4::render::gfx15::g::display), 0});
    call(ctx, g17::fn::Draw_SetViewport_00532450, {load<u32>(sly4::render::gfx15::g::display), 0, 0});
    store<u32>(sly4::render::gfx15::g::current_surface, 0);
    store<u32>(g17::view_counter, load<u32>(g17::view_counter) + 1);
    if (done != 0) call_ptr(ctx, done, {o, load<u32>(done)});
    else ctx.gpr[3] = sly4::render::gfx15::g::current_surface;  // as the original leaves it
}

// void Render_DrawLayers_005170B8(Owner* o)  [Call_0051A6A0 when o->u8 +48] (name?)
// The view's layer draw: when the current View's surface (+512) is the
// display, the screen targets bound and its rects (+96 / +104) set; the
// layers 0, 1, 2, 10, 11 drawn (vtable +16); the draw queue built on
// first use (DrawQueue_Begin_00518610's inline copy), waited on (300 us
// sys_timer_usleep steps while +128 is set) and run; layers 3, 4, 5, 6,
// 9, 12; the stream (render::g::state_cache) run through Draw_Pass with
// a stack state {0, 0, -, -, f32[16] from 0x01164C10} (at the original's
// address, so its unwritten +8..+15 read the same bytes); layer 13; then
// o's vtable +28 (o, -1).
DECOMP(005170B8, Render_DrawLayers_005170B8) {
    namespace rg = sly4::render::g;
    const u32 o = arg<u32>(ctx, 0);
    const u32 view = load<u32>(load<u32>(sly4::render::gfx15::g::current_surface) + 672);
    if (load<u32>(view + 512) == load<u32>(sly4::render::gfx15::g::display)) {
        call(ctx, g17::fn::Display_BindScreenTargets, {});
        call(ctx, g17::fn::Draw_SetViewport_00532450, {load<u32>(view + 512), view + 96, view + 104});
    }
    auto draw = [&](u32 k) {
        const u32 layer = load<u32>(rg::layers + 4 + 4 * k);
        fx::vcall(ctx, layer, g17::kLayerDraw, {layer});
    };
    for (u32 k : {0u, 1u, 2u, 10u, 11u}) draw(k);
    const u32 q = g16::g::record_list;
    if (load<u8>(g16::g::record_list_ready) == 0) {
        store<u8>(q, 1);
        call(ctx, g17::fn::PtrVector_Construct, {q + 4});
        store<u16>(q + 10, static_cast<u16>(g16::g::kRecordListCapacity));
        store<u32>(q + 32, 0);
        std::atomic_ref<u32>(ref<u32>(q + 128)).store(0);  // lwarx / stwcx.
        std::atomic_ref<u32>(ref<u32>(q + 256)).store(0);
        store<u8>(g16::g::record_list_ready, 1);
    }
    while (load<u32>(q + 128) != 0) syscall(ctx, g17::kSysTimerUsleep, {g17::kQueueWaitUsec});
    call(ctx, g17::fn::DrawQueue_Run_00516CD0, {q});
    for (u32 k : {3u, 4u, 5u, 6u, 9u, 12u}) draw(k);
    {
        Stack st(ctx, g17::kPassStateFrame);
        const u32 state = st.addr();
        const u32 stream = load<u32>(rg::state_cache);
        store<u32>(state, 0);
        store<u32>(state + 4, 0);
        for (u32 k = 0; k < 16; ++k) store<u32>(state + 16 + 4 * k, load<u32>(g17::pass_matrix + 4 * k));  // lfs / stfs: raw
        ctx.fpr[1] = g10::lfs_value(load<u32>(g17::pass_matrix + 56));  // left over from the copy
        ctx.fpr[2] = g10::lfs_value(load<u32>(g17::pass_matrix + 60));
        call(ctx, g17::fn::Draw_Pass_005159E8, {stream, state});
    }
    draw(13);
    fx::vcall(ctx, o, g17::kOwnerFinish, {o, 0xFFFFFFFFu});
}

// bool Iff_HandleEmitCurve2_005295A0(IffReader* r, AglEmitProperties* p)  [IFF chunk handler, OPD 0x00F3B688] (name?)
// Iff_HandleEmitCurve_005291F8 with a leading u8 (unnamed, read and
// unused) and f32 "shadeVariance": the same new 84-byte curve at p->+112
// (references moved; a failed SmallAlloc leaves 0 there and the code
// writes through it, as the original), +12 / +16 = the lifetime / fade
// (or 1e8), u16 +8 = (s64)(shadeVariance * 256) truncated (fctidz), then
// the "RGBA" colours (read 4 bytes after the u8, at an odd stack address)
// or two white ones. Returns 1.
DECOMP(005295A0, Iff_HandleEmitCurve2_005295A0) {
    constexpr u32 K = g17::kEmitCurve2Consts;
    const u32 r = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    Stack frame(ctx, 32);
    const u32 fade_in = frame.addr(), fade_out = fade_in + 4, shade = fade_in + 8, ver = fade_in + 12, rgba = fade_in + 13;
    call(ctx, g16::fn::Iff_ReadU8, {r, ver, 0});
    call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, fade_in, g17::str_alpha_fade_in2});
    call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, fade_out, g17::str_alpha_fade_out2});
    call(ctx, g16::fn::FileReader_ReadF32Swapped, {r, shade, g17::str_shade_variance});
    const u32 c = static_cast<u32>(call(ctx, g16::fn::SmallAlloc, {g16::kEmitCurveSize}));
    if (c != 0) {
        store<u16>(c + 4, 1);
        store<u8>(c + 6, 0);
        store<u32>(c, g16::vt_emit_curve);
        store<u16>(c + 8, 0);
        store<u32>(c + 12, load<u32>(K + 4));
        store<u32>(c + 16, load<u32>(K + 4));
    }
    if (const u32 old = load<u32>(p + 112); old != c) {
        store<u32>(p + 112, c);
        if (c != 0) store<u16>(c + 4, static_cast<u16>(load<u16>(c + 4) + 1));
        if (old != 0) {
            const u16 refs = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, refs);
            if (static_cast<s16>(refs) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 cur = load<u32>(p + 112);
        store<u16>(cur + 4, static_cast<u16>(load<u16>(cur + 4) - 1));
    }
    const f64 eps = lfs(K), big = lfs(K + 4);
    {
        const f64 in = lfs(fade_in);
        const u32 cur = load<u32>(p + 112);
        const f64 life = lfs(p + 4);
        store<u32>(cur + 12, g11::stfs_bits(in > eps ? sgl(life / in) : big));  // fcmpu, fdivs
    }
    {
        const f64 out = lfs(fade_out);
        const u32 cur = load<u32>(p + 112);
        const f64 life = lfs(p + 4);
        store<u32>(cur + 16, g11::stfs_bits(out > eps ? sgl(life / out) : big));
    }
    {
        const f64 sv = sgl(lfs(shade) * lfs(K + 8));  // fmuls
        store<u16>(load<u32>(p + 112) + 8, static_cast<u16>(rt::fctid(sv, true)));  // fctidz, low halfword
        ctx.fpr[1] = std::bit_cast<f64>(rt::fctid(sv, true));  // f1 as the original leaves it
    }
    const u32 n = static_cast<u32>(call(ctx, g16::fn::ChunkReader_Remaining, {r})) >> 2;  // rlwinm 30,2,31
    if (n == 0) {
        curve_push17(load<u32>(p + 112), sly4::render::gfx15::g::white);
        curve_push17(load<u32>(p + 112), sly4::render::gfx15::g::white);
        return ret(ctx, 1u);
    }
    for (u32 k = 0; k < n; ++k) {  // cmplw
        call(ctx, g16::fn::FileReader_ReadU32, {r, rgba, sly4::render::gfx16::str_rgba});
        curve_push17(load<u32>(p + 112), rgba);
    }
    ret(ctx, 1u);
}

// bool EmitterInstance_ReadChunks_005A4918(IffReader* r, Obj* parent)  [IFF handler, OPD 0x00F3C928] (name?)
// A new emitter instance from its free list (made on first use: 352-byte
// items), constructed as kind 10 with +340 = 0 and the
// ApeEmitterInstance vtable (o = 0 when the list is out), then the
// shared reader tail (read_instance17). Returns 1.
DECOMP(005A4918, EmitterInstance_ReadChunks_005A4918) {
    const u32 r = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    const u32 alloc = sly4::obj6::ListAllocator(ctx, g17::emitter_insts, sly4::obj5::kEmitterInstListVtbl,
                                                sly4::obj5::kStrEmitterInstList, sly4::obj5::kEmitterInstanceSize);
    const u32 no = static_cast<u32>(sly4::obj4::Vcall0(ctx, alloc, sly4::obj8::kVfnListAlloc));
    u32 o = 0;
    if (no != 0) {
        call(ctx, g17::fn::WorldObj_Construct, {no, g17::kKindEmitter});
        store<u32>(no + sly4::obj5::kEmitter340, 0);
        store<u32>(no, sly4::obj5::kVtblEmitterInstance);
        o = no;
    }
    read_instance17(ctx, g17::emitter_insts, r, parent, o);
}

// bool SplineInstance_ReadChunks_005A83A8(IffReader* r, Obj* parent)  [IFF handler, OPD 0x00F3C9E8] (name?)
// Nothing (1) when the reader's u8 +12 has bit 0x04 (reuse). Otherwise a
// new spline instance from its free list (288-byte items; obj8's
// kSplineInstList) through Kind13_Construct (o = 0 when the list is
// out), then the shared reader tail. Returns 1.
DECOMP(005A83A8, SplineInstance_ReadChunks_005A83A8) {
    const u32 r = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    if ((load<u8>(r + g17::kIrFlags) & 0x04) != 0) return ret(ctx, 1u);  // rlwinm 0,29,29
    const u32 alloc = sly4::obj6::ListAllocator(ctx, g17::spline_insts, sly4::obj8::kSplineInstListVtbl,
                                                sly4::obj8::kStrSplineInstList, sly4::obj8::kSplineInstanceSize);
    const u32 no = static_cast<u32>(sly4::obj4::Vcall0(ctx, alloc, sly4::obj8::kVfnListAlloc));
    u32 o = 0;
    if (no != 0) {
        call(ctx, g17::fn::Kind13_Construct, {no});
        o = no;
    }
    read_instance17(ctx, g17::spline_insts, r, parent, o);
}

// bool GeomInstance_ReadChunks_005A1614(IffReader* r, Obj* parent)  [IFF handler, OPD 0x00F3C878] (name?)
// As EmitterInstance_ReadChunks_005A4918 for a geometry instance (kind 8,
// the ApeGeomInstance vtable, 352-byte items): after the parent / dirty
// step its u8 +322 loses bit 0x20, and it enters the name map only when
// bits 9..12 of +320 are then 3. Returns 1.
DECOMP(005A1614, GeomInstance_ReadChunks_005A1614) {
    const u32 r = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    const u32 alloc = sly4::obj6::ListAllocator(ctx, g17::geom_insts, g17::vt_geom_inst_list, g17::str_geom_inst_list,
                                                g17::kGeomInstanceSize);
    const u32 no = static_cast<u32>(sly4::obj4::Vcall0(ctx, alloc, sly4::obj8::kVfnListAlloc));
    u32 o = 0;
    if (no != 0) {
        call(ctx, g17::fn::WorldObj_Construct, {no, g17::kKindGeom});
        store<u32>(no, g17::vt_geom_instance);
        o = no;
    }
    read_instance17(ctx, g17::geom_insts, r, parent, o, true);
}

// bool LightInstance_ReadChunks_005A358C(IffReader* r, Obj* parent)  [IFF handler, OPD 0x00F3C8C8] (name?)
// As EmitterInstance_ReadChunks_005A4918 for a light instance (kind 9,
// the ApeLightInstance vtable, 400-byte items) with its defaults (+352
// f32[3] from 0x01164B30 as raw words, +368 3, +372 1, +376 / +388 1.0,
// u8 +380 127, u16 +382 0xFFFF, +384 0). Returns 1.
DECOMP(005A358C, LightInstance_ReadChunks_005A358C) {
    const u32 r = arg<u32>(ctx, 0), parent = arg<u32>(ctx, 1);
    const u32 alloc = sly4::obj6::ListAllocator(ctx, g17::light_insts, sly4::obj5::kLightInstListVtbl,
                                                sly4::obj5::kStrLightInstList, sly4::obj5::kLightInstanceSize);
    const u32 no = static_cast<u32>(sly4::obj4::Vcall0(ctx, alloc, sly4::obj8::kVfnListAlloc));
    u32 o = 0;
    if (no != 0) {
        call(ctx, g17::fn::WorldObj_Construct, {no, g17::kKindLight});
        store<u32>(no + 368, 3);
        store<u32>(no, g17::vt_light_instance);
        store<u16>(no + 382, 0xFFFF);
        store<u32>(no + 352, load<u32>(g17::light_default_vec));  // lfs / stfs: raw
        store<u32>(no + 356, load<u32>(g17::light_default_vec + 4));
        store<u32>(no + 372, 1);
        store<u32>(no + 360, load<u32>(g17::light_default_vec + 8));
        store<u32>(no + 376, 0x3F800000u);  // 1.0f
        store<u32>(no + 384, 0);
        store<u8>(no + 380, 127);
        store<u32>(no + 388, 0x3F800000u);
        ctx.fpr[1] = g10::lfs_value(load<u32>(g17::light_default_vec + 8));  // as the copy leaves it
        o = no;
    }
    read_instance17(ctx, g17::light_insts, r, parent, o);
}

// bool SlyHeightFog_LoadParams(ShaderType*, Material* m, Desc* d)  [vtable 0x00B25C58 slot 3, the "SlyHeightFog" type's LoadParams] (name?)
// The texture named d+4 (slot 4 with (t, 1, 1, 1), then added); the one
// named d+8 with its u16 +116 / +118 / +120 cleared, +128 bit 1 set, slot
// 4 (t, 3, 3, 3), added, param 17 = its u8 +73. Then the params, each from
// the desc when its version (u8 d+84) is high enough, else a default (K =
// 0x005C7DCC): 0 / 1 d+88 / +92, 2 d+96 * pi/180 (written back to the
// desc first), 3 d+100 (v > 3, else 0), 4 d+104 (v > 4, else 1), 5 d+108
// (v > 5, else 0), 6 / 7 d+112 / +116, 8 d+120 * pi/180 (written back),
// 9 d+124 (v > 9), 10 d+128 (v > 10, else 1), 11 d+132 (v > 11, else 0),
// 12..16 d+136.. (v > k, else 1, 1, 0.5, 5, 0.2). Raw words otherwise
// (lfs / stfs pairs). Returns 1.
DECOMP(005C7DE8, SlyHeightFog_LoadParams) {
    constexpr u32 K = g17::kHeightFogConsts;
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (const u32 t = static_cast<u32>(call(ctx, g17::fn::Tex_FindLoaded, {load<u32>(d + 4)})); t != 0) {
        fx::vcall(ctx, t, 16, {t, 1, 1, 1});
        call(ctx, g17::fn::Material_AddTexture_004FF008, {m, t});
    }
    if (const u32 t = static_cast<u32>(call(ctx, g17::fn::Tex_FindLoaded, {load<u32>(d + 8)})); t != 0) {
        const u16 f = load<u16>(t + 128);
        store<u16>(t + 116, 0);
        store<u16>(t + 118, 0);
        store<u16>(t + 120, 0);
        store<u16>(t + 128, static_cast<u16>(f | 1));
        fx::vcall(ctx, t, 16, {t, 3, 3, 3});
        call(ctx, g17::fn::Material_AddTexture_004FF008, {m, t});
        set_param17(ctx, m, 17, load<u8>(t + 73));
    }
    auto ver = [&]() { return static_cast<s32>(ref<u8>(d + 84)); };  // re-read where the original does
    auto cst = [](u32 k) { return load<u32>(K + 4 * k); };
    // d+off *= pi / 180 (fmuls), stored back to the desc; the param reads it again.
    auto to_radians = [&](u32 off) {
        const f64 v = sgl(lfs(d + off) * lfs(K + 4));
        store<u32>(d + off, g11::stfs_bits(v));
        ctx.fpr[1] = v;  // f1 as the original leaves it
        return load<u32>(d + off);
    };
    const s32 v0 = ver();
    const u32 p4 = v0 > 4 ? load<u32>(d + 104) : cst(0);
    const u32 p10 = v0 > 10 ? load<u32>(d + 128) : cst(0);
    set_param17(ctx, m, 0, load<u32>(d + 88));
    set_param17(ctx, m, 1, load<u32>(d + 92));
    set_param17(ctx, m, 2, to_radians(96));
    set_param17(ctx, m, 3, ver() > 3 ? load<u32>(d + 100) : cst(2));
    set_param17(ctx, m, 4, p4);
    set_param17(ctx, m, 5, ver() > 5 ? load<u32>(d + 108) : cst(2));
    set_param17(ctx, m, 6, load<u32>(d + 112));
    set_param17(ctx, m, 7, load<u32>(d + 116));
    set_param17(ctx, m, 8, to_radians(120));
    set_param17(ctx, m, 9, ver() > 9 ? load<u32>(d + 124) : cst(2));
    set_param17(ctx, m, 10, p10);
    set_param17(ctx, m, 11, ver() > 11 ? load<u32>(d + 132) : cst(2));
    set_param17(ctx, m, 12, ver() > 12 ? load<u32>(d + 136) : cst(0));
    set_param17(ctx, m, 13, ver() > 13 ? load<u32>(d + 140) : cst(0));
    set_param17(ctx, m, 14, ver() > 14 ? load<u32>(d + 144) : cst(3));
    set_param17(ctx, m, 15, ver() > 15 ? load<u32>(d + 148) : cst(4));
    set_param17(ctx, m, 16, ver() > 16 ? load<u32>(d + 152) : cst(5));
    ret(ctx, 1u);
}

// bool Iff_HandleEmitSpline_0052996C(IffReader* r, AglEmitProperties* p)  [IFF chunk handler, OPD 0x00F3B690] (name?)
// The emitter's spline: a u8 (unnamed, unused), u32 "isLoop" and
// "numPoints" (both 0 when absent); a new 32-byte spline shape (vtable
// 0x00B232C0, refs 1, its SplineArray reset) becomes p->+136 (references
// moved as Iff_HandleEmitCurve_005291F8's; a failed allocation leaves 0
// there and the code goes on through it, as the original); its array set
// up for numPoints (closed when isLoop == 1), then per point the vec3s
// "position", "control1", "control2" (raw words). A closed array gets
// point 0 copied after the last one; with more than one point, from the
// last down, control2 = control1 - position and, but for point 0,
// control1 = the previous point's control2 - position (fsubs); then
// point 0's control1 and the last point's control2 = 0x01164B40. Then
// SplineArray_Finish. Returns 1.
DECOMP(0052996C, Iff_HandleEmitSpline_0052996C) {
    const u32 r = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    Stack frame(ctx, 48);
    const u32 is_loop = frame.addr(), num = is_loop + 4, pos = is_loop + 8, c1 = is_loop + 20, c2 = is_loop + 32,
              ver = is_loop + 44;
    store<u64>(is_loop, 0);  // std: isLoop / numPoints default 0
    call(ctx, g16::fn::Iff_ReadU8, {r, ver, 0});
    call(ctx, g17::fn::Iff_ReadU32, {r, is_loop, g17::str_is_loop});
    call(ctx, g17::fn::Iff_ReadU32, {r, num, g17::str_num_points});
    const u32 c = static_cast<u32>(call(ctx, sly4::fn::Mem_Alloc, {g17::kEmitSplineSize, 0, 16}));
    if (c != 0) {
        store<u16>(c + 4, 1);
        store<u32>(c, g17::vt_emit_spline);
        call(ctx, g17::fn::SplineArray_Reset, {c + g17::kSplineArray});
    }
    if (const u32 old = load<u32>(p + g17::kEmitSpline); old != c) {
        store<u32>(p + g17::kEmitSpline, c);
        if (c != 0) store<u16>(c + 4, static_cast<u16>(load<u16>(c + 4) + 1));
        if (old != 0) {
            const u16 refs = static_cast<u16>(load<u16>(old + 4) - 1);
            store<u16>(old + 4, refs);
            if (static_cast<s16>(refs) <= 0) fx::vcall(ctx, old, 4, {old});
        }
    }
    {
        const u32 cur = load<u32>(p + g17::kEmitSpline);
        store<u16>(cur + 4, static_cast<u16>(load<u16>(cur + 4) - 1));
    }
    const u32 arr = load<u32>(p + g17::kEmitSpline) + g17::kSplineArray;
    call(ctx, g17::fn::SplineArray_Init, {arr, load<u32>(num), load<u32>(is_loop) == 1 ? 1u : 0u, 0});
    call(ctx, g16::fn::ChunkReader_Remaining, {r});  // result unused
    constexpr u32 P = g17::kSplinePointSize;
    for (u32 k = 0; k < load<u32>(num); ++k) {  // cmplw
        call(ctx, g17::fn::FileReader_ReadVec3Swapped, {r, pos, g17::str_position});
        call(ctx, g17::fn::FileReader_ReadVec3Swapped, {r, c1, g17::str_control1});
        call(ctx, g17::fn::FileReader_ReadVec3Swapped, {r, c2, g17::str_control2});
        const u32 pt = load<u32>(arr + 4) + P * k;
        for (u32 j = 0; j < 3; ++j) store<u32>(pt + 4 * j, load<u32>(pos + 4 * j));  // lfs / stfs: raw
        for (u32 j = 0; j < 3; ++j) store<u32>(pt + 16 + 4 * j, load<u32>(c1 + 4 * j));
        for (u32 j = 0; j < 3; ++j) store<u32>(pt + 32 + 4 * j, load<u32>(c2 + 4 * j));
    }
    u32 w = load<u32>(arr);
    if ((w & 0x80000000u) != 0) {  // closed: point 0 again after the last (rlwinm 1,31,31)
        const u32 pts = load<u32>(arr + 4), dst = pts + ((load<u32>(num) << 6) & ~63u);
        for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u}) store<u32>(dst + off, load<u32>(pts + off));
        w = load<u32>(arr);
    }
    const u32 n = w & 0x7FFF;  // rlwinm 0,17,31
    if (n > 1) {
        for (s32 i = static_cast<s32>(n - 1); i >= 0; --i) {
            const u32 pt = load<u32>(arr + 4) + P * static_cast<u32>(i);
            const f64 x = lfs(pt), y = lfs(pt + 4), z = lfs(pt + 8);
            store<u32>(pt + 32, g11::stfs_bits(sgl(lfs(pt + 16) - x)));
            store<u32>(pt + 36, g11::stfs_bits(sgl(lfs(pt + 20) - y)));
            store<u32>(pt + 40, g11::stfs_bits(sgl(lfs(pt + 24) - z)));
            if (i > 0) {
                const u32 prev = pt - P;
                const f64 a = sgl(lfs(prev + 32) - x), b = sgl(lfs(prev + 36) - y), cz = sgl(lfs(prev + 40) - z);
                store<u32>(pt + 16, g11::stfs_bits(a));
                store<u32>(pt + 20, g11::stfs_bits(b));
                store<u32>(pt + 24, g11::stfs_bits(cz));
            }
        }
        const u32 pts = load<u32>(arr + 4);
        constexpr u32 V = g17::vec3_zero_164B40;
        for (u32 j = 0; j < 3; ++j) store<u32>(pts + 16 + 4 * j, load<u32>(V + 4 * j));
        const u32 last = pts + (((load<u32>(arr) & 0x7FFF) - 1) << 6);
        for (u32 j = 0; j < 3; ++j) store<u32>(last + 32 + 4 * j, load<u32>(V + 4 * j));
    }
    call(ctx, g17::fn::SplineArray_Finish, {arr});
    ret(ctx, 1u);
}

// void Gcm_DumpLocks_0052F91C(s32 swap_label)  [f_00531A40, f_00532A60, f_00535D28, f_0053BF70, Display_WaitVSync_0053C4A0] (name?)
// The "RSX Potential Locks" debug dump (printf): the current gcm
// context, the ring buffer context and its begin / end / current, the
// linear double-buffer context, the command buffer's two buffers
// (address, offset), the PPU swap label (the argument, the full
// register), the frame flip label and the 21 layer labels (each read
// through its pointer, sign-extended), then the closing rule.
DECOMP(0052F91C, Gcm_DumpLocks_0052F91C) {
    const u64 swap = ctx.gpr[3];
    auto label = [](u32 p) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(load<u32>(p))))); };  // extsw
    call(ctx, g17::fn::printf, {g17::str_locks_title});
    call(ctx, g17::fn::printf, {g17::str_cur_ctx, load<u32>(fx::g::state)});
    call(ctx, g17::fn::printf, {g17::str_ring_ctx, load<u32>(g17::ring_ctx)});
    call(ctx, g17::fn::printf, {g17::str_begin, load<u32>(load<u32>(g17::ring_ctx))});
    call(ctx, g17::fn::printf, {g17::str_end, load<u32>(load<u32>(g17::ring_ctx) + 4)});
    call(ctx, g17::fn::printf, {g17::str_curr, load<u32>(load<u32>(g17::ring_ctx) + 8)});
    call(ctx, g17::fn::printf, {g17::str_linear_ctx, g17::linear_ctx});
    call(ctx, g17::fn::printf, {g17::str_begin, load<u32>(g17::linear_ctx)});
    call(ctx, g17::fn::printf, {g17::str_end, load<u32>(g17::linear_ctx + 4)});
    call(ctx, g17::fn::printf, {g17::str_curr, load<u32>(g17::linear_ctx + 8)});
    {
        const u32 cb = load<u32>(g17::command_buffer);
        const u32 off = load<u32>(cb + 276);
        call(ctx, g17::fn::printf, {g17::str_linear0, load<u32>(cb + 268), off, off});  // r6 = r5 as the original
    }
    {
        const u32 cb = load<u32>(g17::command_buffer);
        const u32 off = load<u32>(cb + 280);
        call(ctx, g17::fn::printf, {g17::str_linear1, load<u32>(cb + 272), off, off});
    }
    call(ctx, g17::fn::printf, {g17::str_swap_label, swap});
    call(ctx, g17::fn::printf, {g17::str_flip_label, label(g17::flip_label)});
    for (u32 k = 0; k < g17::kLayerLabelCount; ++k)
        call(ctx, g17::fn::printf, {g17::str_layer_labels + g17::kLayerLabelStrStride * k, label(g17::layer_labels + 4 * k)});
    call(ctx, g17::fn::printf, {g17::str_locks_end});
}

// bool SlyCharacter2_LoadParams(ShaderType*, Material* m, const Desc* d)  [vtable 0x00B259A0 slot 3, the "SlyCharacter2" type's LoadParams] (name?)
// Nothing (1) for desc version (u8 d+84) 0. Otherwise m->+20's slot 4
// with (3, 3, 1); the textures named d+16 (else the default of kind 0)
// and d+12 (else kind 5) added, each then slot 4 (t, 3, 3, 1). Params: 0
// = d+88 (v > 0, else 1.0), 1 = (d+92 when v > 1, else 9.75; 9.75 also
// when it is <= 0) * pi/180 (fmuls), then the fixed rest: 2 / 8 / 10 /
// 17 / 19 / 23 = 1.0, 5 / 14 = -0.1, 21 = 0.1, 22 = 2.0, 28..31 = 0.5,
// the others 0. Returns 1.
DECOMP(005BF608, SlyCharacter2_LoadParams) {
    constexpr u32 K = g17::kSlyCharacter2Consts;
    const u32 m = arg<u32>(ctx, 1), d = arg<u32>(ctx, 2);
    if (static_cast<s32>(ref<u8>(d + 84)) <= 0) return ret(ctx, 1u);
    {
        const u32 o = load<u32>(m + 20);
        fx::vcall(ctx, o, 16, {o, 3, 3, 1});
    }
    for (const auto& [name_off, kind] : {std::pair<u32, u32>{16, 0}, std::pair<u32, u32>{12, 5}}) {
        u32 t = static_cast<u32>(call(ctx, g17::fn::Tex_FindLoaded, {load<u32>(d + name_off)}));
        if (t == 0) t = static_cast<u32>(call(ctx, g17::fn::Tex_GetDefault, {kind}));
        call(ctx, g17::fn::Material_AddTexture_004FF008, {m, t});
        fx::vcall(ctx, t, 16, {t, 3, 3, 1});
    }
    const u32 one = load<u32>(K);
    set_param17(ctx, m, 0, static_cast<s32>(ref<u8>(d + 84)) > 0 ? load<u32>(d + 88) : one);
    {
        const s32 v = static_cast<s32>(ref<u8>(d + 84));
        f64 a = lfs(v > 1 ? d + 92 : K + 4);
        if (a < lfs(K + 8) || a == lfs(K + 8)) a = lfs(K + 4);  // fcmpu, cror lt|eq: NaN kept
        const f64 r = sgl(a * lfs(K + 12));  // fmuls
        ctx.fpr[1] = a;  // f1 / f2 as the original leaves them
        ctx.fpr[2] = lfs(K + 12);
        set_param17(ctx, m, 1, g11::stfs_bits(r));
    }
    // The fixed params 2..31 (raw words).
    const u32 fixed[30] = {one, 0, 0, g17::kMinusTenth, 0, 0, one, 0, one, 0, 0, 0, g17::kMinusTenth, 0, 0,
                           one, 0, one, 0, g17::kTenth, g17::kTwo, one, 0, 0, 0, 0, g17::kHalf, g17::kHalf, g17::kHalf, g17::kHalf};
    for (u32 k = 0; k < 30; ++k) set_param17(ctx, m, 2 + k, fixed[k]);
    ret(ctx, 1u);
}

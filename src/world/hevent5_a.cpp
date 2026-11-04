// World_HandleEvent callees, part 5: small helpers of the part-4 functions
// (language, strings, the backdrop prim, shutdown, sysutil polling, Player
// slots and the object state helpers).
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

namespace sly4 {
namespace hevent5a {
namespace afn {
constexpr u32 System_GetLanguage = 0x0056BE30;  // () -> s32
constexpr u32 System_SetLanguage = 0x0056BF84;  // (s32 language) (name?)
constexpr u32 Ui_RefreshText = 0x004D573C;      // (Element*): per top-level element (name?)
constexpr u32 Mem_FreeFrom = 0x0054EE6C;        // (p, pool)
constexpr u32 Shader_LoadGroup = 0x0050F0F0;    // (s32 group) -> material
constexpr u32 Prim_Build52374C = 0x0052374C;    // (material, a, b, c, VertexFormat*) -> object (name?)
constexpr u32 Sysutil_CheckCallback = 0x0056C184;
constexpr u32 memmove = 0x0071C3F0;
constexpr u32 Obj_Prepare3629D8 = 0x003629D8;   // (o): before state 4 (name?)
constexpr u32 Obj_SetState362D28 = 0x00362D28;  // decompiled below
constexpr u32 Anim_Play = 0x004A5C90;           // (AnimCtl*, anim, u8; f32) -> bit 0 = started
constexpr u32 Anim_SetPhase4A5DA8 = 0x004A5DA8; // (AnimCtl*, -, 1; f32 t) (name?)
constexpr u32 Anim_Find34CC34 = 0x0034CC34;     // (AnimCtl*, 0) -> current track or 0 (name?)
constexpr u32 Track_Time353780 = 0x00353780;    // (track) -> f32 (name?)
constexpr u32 Track_Length353748 = 0x00353748;  // (track) -> f32 (name?)
constexpr u32 Obj_Set4E3168 = 0x004E3168;       // (const char*, u32) (name?)
constexpr u32 Obj_Attach4E2DB0 = 0x004E2DB0;    // (u32, u32, 1, 0) (name?)
}  // namespace afn

constexpr u32 kUiRoots = 0x00F66868;        // u32 first top-level Element, linked by +156
constexpr u32 kStrings = 0x00F6687C;        // u32 StringTable* (0 = none)
constexpr u32 kVertexFormats = 0x010DC740;  // two 128-byte vertex formats
constexpr u32 kMainThreadId = 0x010F0E20 + 152;  // u64, in the g::mem_stats block
constexpr u32 kStringTableVtable = 0x00B23070;
constexpr u32 kModeRb = 0x00AF0E30;         // "rb"
constexpr u32 kEmptyString = 0x00ADCAA8;    // ""
constexpr u32 kStateAnimByState = 0x00F64F88;  // u32[] per state, for sub_004CF988 (name?)
constexpr u32 kZeroOne362D1C = 0x00362D1C;  // f32 0.0, 1.0
constexpr u32 kStringHeaderSize = 36;
}  // namespace hevent5a
}  // namespace sly4

using namespace sly4::hevent5a;

// void Prim_Unlock523A98(Prim*): empty in this build.
DECOMP(00523A98, Prim_Unlock523A98) {}

// s32 Lang_GetSystem(): System_GetLanguage().
DECOMP(005DBC60, Lang_GetSystem) {
    call(ctx, afn::System_GetLanguage);
}

// void Lang_Set5DBC84(s32 language): System_SetLanguage(language).
DECOMP(005DBC84, Lang_Set5DBC84) {
    call(ctx, afn::System_SetLanguage, {arg<u32>(ctx, 0)});
}

// void Game_Exit458B1C(Game* g): Game_Shutdown(g), sub_004A8AC4(), _exit(0).
DECOMP(00458B1C, Game_Exit458B1C) {
    call(ctx, fn::Game_Shutdown, {arg<u32>(ctx, 0)});
    call(ctx, fn::sub_004A8AC4);
    call(ctx, fn::_exit, {0});
}

// Vertex* Prim_Lock523A5C(Prim* p): b = *p + 24; b +8 bits 1..3 = 2 (locked
// for writing?), returns b +0 (the vertex buffer).
DECOMP(00523A5C, Prim_Lock523A5C) {
    const u32 b = load<u32>(arg<u32>(ctx, 0)) + 24;
    const u32 f = load<u32>(b + 8);
    const u32 g = load<u32>(b + 4);
    store<u32>(b + 8, (f & ~0xEu) | 4u);
    ret(ctx, load<u32>(b + 0));
    ctx.gpr[4] = g & 1;  // as the original leaves them
    ctx.gpr[5] = (f & ~0xEu) | 4u;
    ctx.gpr[6] = g;
}

// void Thread_SetMainId(u64 id): stores id (0 = the calling thread's id) at
// 0x010F0EB8.
DECOMP(005DBC18, Thread_SetMainId) {
    u64 id = ctx.gpr[3];
    if (id == 0) {
        Stack frame(ctx, 16);
        store<u64>(frame.addr(), 0);
        call(ctx, imp::sys_ppu_thread_get_id, {frame.addr()});
        id = load<u64>(frame.addr());
    }
    store<u64>(kMainThreadId, id);
    ctx.gpr[3] = id;  // as the original leaves it
}

// void Ui_SetStrings(StringTable* t): makes t the current string table and
// refreshes every top-level element's text.
DECOMP(004D580C, Ui_SetStrings) {
    u32 e = load<u32>(kUiRoots);
    store<u32>(kStrings, arg<u32>(ctx, 0));
    for (; e != 0; e = load<u32>(e + 156)) call(ctx, afn::Ui_RefreshText, {e});
}

// void Prim_Destroy5239E8(Prim* p): deletes *p (vfunc +8) and clears it, then
// frees p (pool 0) when p is non-null.
DECOMP(005239E8, Prim_Destroy5239E8) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 o = load<u32>(p);
    if (o != 0) {
        const u32 opd = load<u32>(load<u32>(o) + 8);
        ctx.gpr[4] = opd;
        ctx.gpr[5] = load<u32>(opd);
        call_ptr(ctx, opd, {o});
        store<u32>(p, 0);
    }
    if (p != 0) call(ctx, afn::Mem_FreeFrom, {p, 0});
}

// Prim* Prim_Create523AE0(Material* m, a, b, c): with m = 0 the material is
// Shader_LoadGroup(3); builds the object with Prim_Build52374C(m, a, b, c,
// vertex formats) and wraps it in a 4-byte block (Mem_Alloc(4, 0, 16)).
// Returns the block, 0 if the allocation failed (the object is then leaked).
DECOMP(00523AE0, Prim_Create523AE0) {
    u32 m = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3);
    if (m == 0) m = static_cast<u32>(call(ctx, afn::Shader_LoadGroup, {3}));
    const u32 o = static_cast<u32>(call(ctx, afn::Prim_Build52374C, {m, a, b, c, kVertexFormats}));
    const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {4, 0, 16}));
    if (p != 0) store<u32>(p, o);
    ret(ctx, p);
}

// void Player_Set3632C8(Object* o, Object* value, u32 which): with value set
// and which below (o +8) +152, the slot (o +328)[which] {+12 Object*} gets
// vfunc +188(slot, value, 0) and then vfunc +148(slot).
DECOMP(003632C8, Player_Set3632C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = arg<u32>(ctx, 1);
    const u32 which = arg<u32>(ctx, 2);
    const u32 def = load<u32>(o + 8);
    if (v == 0) return;
    if (which >= load<u32>(def + 152)) return;
    const u32 e = load<u32>(o + 328) + (which << 4);
    const u32 s = load<u32>(e + 12);
    if (s == 0) {
        ret(ctx, 0);  // r3 as the original leaves it
        return;
    }
    vcall(ctx, s, 188, {s, v, 0});
    const u32 s2 = load<u32>(e + 12);
    vcall(ctx, s2, 148, {s2});
}

// bool Game_PollSystemState(Game*): pumps sysutil callbacks and sets
// g::main_state from g::sysutil_flags: exit requested -> kMainQuit (returns 0);
// menu open, PSN offline or no pad -> kMainPaused; resume -> kMainResumed
// (returns 1); otherwise returns main_state != kMainQuit.
DECOMP(0045C6AC, Game_PollSystemState) {
    call(ctx, afn::Sysutil_CheckCallback, {arg<u32>(ctx, 0)});
    if (load<u32>(g::sysutil_flags) & kSysutilExitRequested) {
        store<u32>(g::main_state, kMainQuit);
        ret(ctx, 0);
        return;
    }
    if ((load<u32>(g::sysutil_flags) & kSysutilMenuOpen) || (load<u32>(g::sysutil_flags) & kSysutilNpOffline) ||
        (load<u32>(g::sysutil_flags) & kSysutilPadLost)) {
        store<u32>(g::main_state, kMainPaused);
        ret(ctx, 1);
        return;
    }
    const u32 flags = load<u32>(g::sysutil_flags);
    const u32 st = load<u32>(g::main_state);
    if (flags & kSysutilResume) {
        store<u32>(g::main_state, kMainResumed);
        ret(ctx, 1);
        return;
    }
    ret(ctx, st != kMainQuit ? 1 : 0);
}

// void Obj_Reset3636EC(Object* o): +396 |= 0x40000000, +160 = (+160 & 1) | 2;
// o +24 gets vfunc +40(+24, 1, 1); Obj_SetState362D28(o, +160 >> 1, 0); then
// for each slot i below (o +8) +152, slot (o +328)[i] {+4 Object*, +8 u32}
// with an object: vfunc +88(obj, +8 != 0), and when the def's entry
// ((o +8) +148)[i] (32 bytes) has +20 set: Obj_Set4E3168("", obj) and
// Obj_Attach4E2DB0(entry +20, obj, 1, 0).
DECOMP(003636EC, Obj_Reset3636EC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 f396 = load<u32>(o + 396) | 0x40000000u;
    u32 f160 = 2u | (load<u32>(o + 160) & 1u);
    const u32 def = load<u32>(o + 8);
    const u32 c = load<u32>(o + 24);
    store<u32>(o + 396, f396);
    store<u32>(o + 160, f160);
    if (c != 0) {
        vcall(ctx, c, 40, {c, 1, 1});
        f160 = load<u32>(o + 160);
    }
    call(ctx, afn::Obj_SetState362D28, {o, f160 >> 1, 0});
    for (u32 i = 0; i < load<u32>(def + 152); ++i) {
        const u32 e = load<u32>(o + 328) + i * 16;
        const u32 obj = load<u32>(e + 4);
        if (obj == 0) continue;
        const u32 opd = load<u32>(load<u32>(obj) + 88);
        ctx.gpr[6] = opd;  // the original leaves the OPD in r6 and its entry in r5
        ctx.gpr[5] = load<u32>(opd);
        call_ptr(ctx, opd, {obj, load<u32>(e + 8) != 0 ? 1u : 0u});
        if (load<u32>(load<u32>(def + 148) + i * 32 + 20) == 0) continue;
        call(ctx, afn::Obj_Set4E3168, {kEmptyString, load<u32>(e + 4)});
        call(ctx, afn::Obj_Attach4E2DB0, {load<u32>(load<u32>(def + 148) + i * 32 + 20), load<u32>(e + 4), 1, 0});
    }
    ret(ctx, load<u32>(def + 152));  // r3 as the original leaves it
}

// void StringTable_Load(StringTable* t, const char* path): t = {vtable,
// 0}; opens path ("rb"), reads the 36-byte header, and when that worked
// allocates the file's size (Mem_Alloc(size, 0, 16), unchecked), copies the
// header there, reads the rest after it and turns the header's offsets at
// +24, +28 and +32 into pointers. The file is closed either way.
DECOMP(004F8450, StringTable_Load) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 path = arg<u32>(ctx, 1);
    store<u32>(t + 0, kStringTableVtable);
    store<u32>(t + 4, 0);
    const u32 f = static_cast<u32>(call(ctx, fn::File_Open, {path, kModeRb}));
    if (f == 0) return;
    Stack frame(ctx, 48);
    const u32 hdr = frame.addr();
    const u32 size = static_cast<u32>(vcall(ctx, f, 24, {f}));
    if (static_cast<s32>(vcall(ctx, f, 12, {f, hdr, kStringHeaderSize, 0})) == static_cast<s32>(kStringHeaderSize)) {
        const u32 p = static_cast<u32>(call(ctx, fn::Mem_Alloc, {size, 0, 16}));
        store<u32>(t + 4, p);
        call(ctx, afn::memmove, {p, hdr, kStringHeaderSize});
        vcall(ctx, f, 12, {f, load<u32>(t + 4) + kStringHeaderSize, size - kStringHeaderSize, 0});
        const u32 b = load<u32>(t + 4);
        store<u32>(b + 24, load<u32>(b + 24) + b);
        store<u32>(b + 28, load<u32>(b + 28) + b);
        store<u32>(b + 32, load<u32>(b + 32) + b);
    }
    vcall(ctx, f, 8, {f});
}

// void Obj_SetState362D28(Object* o, s32 state, u8 flag): plays the state's
// anim ((o +296)[state]; state 3 and 4 prefer +316 / +320) on o +28 with
// Anim_Play(ctl, anim, flag; +392), keeping the start bit in +396 bit 24.
// When +396 bit 24 was set and state is 1 (bit 27) or 4 (bit 26) with the
// current track running (track +12 bit 27 clear), it carries the phase over:
// t = 1 - time / length (when length > 0) is given to Anim_SetPhase4A5DA8
// after a successful start. State 4 calls Obj_Prepare3629D8 first. With o +20,
// +396 bit 23 = sub_004CF988(o +20, table[state]) & 1.
DECOMP(00362D28, Obj_SetState362D28) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 state = arg<s32>(ctx, 1);
    const u32 flag = arg<u32>(ctx, 2) & 0xFF;
    const f32 zero = load<f32>(kZeroOne362D1C);
    f32 phase = zero;
    const u32 f396 = load<u32>(o + 396);
    if ((f396 & 0x01000000u) && load<u32>(o + 28) != 0 &&
        ((state == 1 && (f396 & 0x08000000u)) || (state == 4 && (f396 & 0x04000000u)))) {
        const u32 tr = static_cast<u32>(call(ctx, afn::Anim_Find34CC34, {load<u32>(o + 28), 0}));
        if (tr != 0 && !(load<u32>(tr + 12) & 0x08000000u)) {
            const f64 time = callf(ctx, afn::Track_Time353780, {tr}, {});
            const f64 len = callf(ctx, afn::Track_Length353748, {tr}, {});
            if (len > phase) {
                const f32 q = static_cast<f32>(time / len);
                phase = static_cast<f32>(static_cast<f64>(load<f32>(kZeroOne362D1C + 4)) - q);
            }
        }
    }
    if (state == 4) call(ctx, afn::Obj_Prepare3629D8, {o});
    const u32 so = static_cast<u32>(state) << 2;
    u32 anim = load<u32>(o + so + 296);
    if (state == 3 && load<u32>(o + 316) != 0) anim = load<u32>(o + 316);
    else if (state == 4 && load<u32>(o + 320) != 0) anim = load<u32>(o + 320);
    callf(ctx, afn::Anim_Play, {load<u32>(o + 28), anim, flag}, {load<f32>(o + 392)});
    const u32 r = static_cast<u32>(ctx.gpr[3]);
    const u32 n396 = (load<u32>(o + 396) & ~0x01000000u) | ((r & 1u) << 24);
    store<u32>(o + 396, n396);
    if ((r & 1u) && phase != zero) callf(ctx, afn::Anim_SetPhase4A5DA8, {load<u32>(o + 28), n396, 1}, {phase});
    const u32 c = load<u32>(o + 20);
    if (c != 0) {
        const u32 r2 = static_cast<u32>(call(ctx, hfn::sub_004CF988, {c, load<u32>(kStateAnimByState + so)}));
        store<u32>(o + 396, (load<u32>(o + 396) & ~0x00800000u) | ((r2 & 1u) << 23));
    } else {
        ret(ctx, 0);  // r3 as the original leaves it
    }
}

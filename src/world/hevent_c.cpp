// World_HandleEvent callees, 27..33 instructions: handle lookups, message
// dialogs, small loops.
#include "hevent.h"
#include "misc.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
namespace cfn {
constexpr u32 SortedMap_Find = 0x0076611C;   // (map, const u32* key) -> value slot
constexpr u32 sub_00362F1C = 0x00362F1C;     // (Object*, u8, u8) (name?)
constexpr u32 sub_00362B40 = 0x00362B40;     // (Object*, u8, u8) (name?)
constexpr u32 sub_003998BC = 0x003998BC;     // (Object*)
constexpr u32 sub_005319CC = 0x005319CC;     // () -> bool
constexpr u32 cellMsgDialogOpenErrorCode = 0x00ABDA84;  // import stub
constexpr u32 cellMsgDialogClose = 0x00ABDA24;          // import stub (f32 delay ms)
constexpr u32 Sysutil_CheckCallback = 0x0056C184;
constexpr u32 Registry_Find = 0x00766C44;    // (map, const u32* key) -> value slot (name?)
constexpr u32 sub_00371BA8 = 0x00371BA8;     // (Object*, u32)
constexpr u32 sub_005A5EA0 = 0x005A5EA0;     // (Object*, u32, u32)
constexpr u32 sub_005988C8 = 0x005988C8;     // (Object*, f32)
constexpr u32 sub_0035BD08 = 0x0035BD08;     // (Object*)
constexpr u32 Table_Find = 0x00799474;       // (map, const u32* key) -> value slot (name?)
constexpr u32 sub_004CF988 = 0x004CF988;     // (Object*, u32)
constexpr u32 sub_0052F8A0 = 0x0052F8A0;     // (f32) (name?)
constexpr u32 sub_005369E8 = 0x005369E8;     // (f32) (name?)
constexpr u32 sub_00536958 = 0x00536958;     // (bool) (name?)
}  // namespace cfn

constexpr u32 kFlags1163F80 = 0x01163F80;     // u8 x3 (+0 set by Ui_Release4D22B0) (name?)
constexpr u32 kMsgDialogState = 0x010BB850;   // u32: 0 none, 3 open, else the result (name?)
constexpr u32 kMsgDialogCallback = 0x00F39D98;  // OPD
constexpr u32 kMsgDialogCloseDelay = 0x004688C4;  // f32 0.0
constexpr u32 kRegistry10686A8 = 0x010686A8;  // map: key -> Object* (vfunc +12) (name?)
constexpr u32 kBrightnessConsts = 0x0046C8B0; // f32 1.0, 0.4, 0.8, 1.2; f64 1.2f, 0.8f
constexpr u32 kGammaConsts = 0x0046C824;      // f32 0.09, 0.01
constexpr u32 kWorldSettings = 4144;          // World +: settings block (+4 f32, +28 f32, +0 flags) (name?)
constexpr u32 kTag2237D4 = 0x2237D4;

// Object* Handle_ResolveMask: the handle map's object for `handle` while
// (obj->+4 & mask) is set, else 0. Leaves r4 = the masked flags.
void HandleResolveMask(PPUContext& ctx, u32 mask) {
    const u32 h = arg<u32>(ctx, 0);
    ctx.gpr[4] = h;
    if (h == 0) return ret(ctx, 0u);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, cfn::SortedMap_Find, {kHandleMap, frame.addr(), h})));
    if (obj == 0) return ret(ctx, 0u);
    const u32 m = load<u32>(obj + 4) & mask;
    ctx.gpr[4] = m;
    ret(ctx, m != 0 ? obj : 0u);
}
}  // namespace

// Handle lookups that test one bit of the object's flags word (+4).
DECOMP(003568E0, Handle_ResolveBit1) { HandleResolveMask(ctx, 0x1); }
DECOMP(0035B908, Handle_ResolveBit10000) { HandleResolveMask(ctx, 0x10000); }
DECOMP(00365B28, Handle_ResolveBit20000) { HandleResolveMask(ctx, 0x20000); }
DECOMP(00365BA4, Handle_ResolveBit400) { HandleResolveMask(ctx, 0x400); }
DECOMP(00365EAC, Handle_ResolveBit40) { HandleResolveMask(ctx, 0x40); }

// void Obj_Dispatch362(Object* o, u8 a, u8 alt, u8 b): unless o is 0 or its
// +396 has bit 0x02000000, sub_00362F1C(o, a, b) when alt, else sub_00362B40(o, a, b).
DECOMP(0036325C, Obj_Dispatch362) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFF, alt = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3) & 0xFF;
    ctx.gpr[8] = o;
    ctx.gpr[6] = a;
    ctx.gpr[4] = b;
    ctx.gpr[7] = arg<u32>(ctx, 3);
    if (o == 0) return;
    const u32 bit = (load<u32>(o + 396) >> 25) & 1;
    ctx.gpr[7] = bit;
    if (bit != 0) return;
    call(ctx, (alt & 0xFF) != 0 ? cfn::sub_00362F1C : cfn::sub_00362B40, {o, a, b, a, 0, o});
}

// void Obj_ForEach48(Object* o): sub_003998BC(o->+48[i]) for i < u16 o->+56.
DECOMP(003A0924, Obj_ForEach48) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n = 0;
    for (u32 i = 0;; ++i) {
        n = load<u16>(o + 56);
        if (i >= n) break;
        call(ctx, cfn::sub_003998BC, {load<u32>(load<u32>(o + 48) + i * 4)});
    }
    ret(ctx, n);
}

// bool Flags1163F81_Idle(): 1 when the bytes 0x01163F81 and 0x01163F82 are both
// 0; otherwise 0, and if sub_005319CC() byte 0x01163F80 = 0 (name?)
DECOMP(00465F98, Flags1163F81_Idle) {
    if (ref<u8>(kFlags1163F80 + 1) == 0 && ref<u8>(kFlags1163F80 + 2) == 0) return ret(ctx, 1u);
    if (call(ctx, cfn::sub_005319CC) != 0) ref<u8>(kFlags1163F80) = 0;
    ret(ctx, 0u);
}

// bool MsgDialog_OpenErrorCode(u32 code): opens the system error dialog unless
// one is up (state 0x010BB850 != 0); state = 3 on success.
DECOMP(0048CE2C, MsgDialog_OpenErrorCode) {
    const u32 code = arg<u32>(ctx, 0);
    const u32 st = load<u32>(kMsgDialogState);
    ctx.gpr[4] = st;
    if (st != 0) return ret(ctx, 0u);
    if (call(ctx, cfn::cellMsgDialogOpenErrorCode, {code, kMsgDialogCallback, 0, 0}) != 0) return ret(ctx, 0u);
    store<u32>(kMsgDialogState, 3);
    ctx.gpr[4] = 0x010C0000u;
    ctx.gpr[5] = 3;
    ret(ctx, 1u);
}

// bool Registry_Notify(u32 key): the registry 0x010686A8's object for key gets
// vfunc +12(obj, 1); 0 when there is none (name?)
DECOMP(002E9BE4, Registry_Notify) {
    const u32 key = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, cfn::Registry_Find, {kRegistry10686A8, frame.addr(), 0x01070000u})));
    if (obj == 0) return ret(ctx, 0u);
    vcall(ctx, obj, 12, {obj, 1});
    ret(ctx, 1u);
}

// void Obj_SetTarget168(Object* o, u8 force, u32 v): unless +360 has bit
// 0x08000000, when +360 bit 31 or force, and the state (+160 >> 1) is 0:
// o->+168 = v and sub_00371BA8(o, v). Returns with r3 = v otherwise.
DECOMP(0037202C, Obj_SetTarget168) {
    const u32 o = arg<u32>(ctx, 0), force = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    const u32 w = load<u32>(o + 360), s = load<u32>(o + 160);
    ctx.gpr[3] = v;
    ctx.gpr[5] = o;
    ctx.gpr[6] = s;
    const u32 b27 = (w >> 27) & 1;
    ctx.gpr[8] = b27;
    ctx.gpr[7] = w;
    if (b27 != 0) return;
    const u32 b31 = w >> 31;
    ctx.gpr[7] = b31;
    if (b31 == 0) {
        ctx.gpr[4] = force & 0xFF;
        if ((force & 0xFF) == 0) return;
    }
    ctx.gpr[4] = s >> 1;
    if ((s >> 1) != 0) return;
    store<u32>(o + 168, v);
    call(ctx, cfn::sub_00371BA8, {o, v});
}

// void Obj_Apply5A5EA0(Object* o, ?, u32 p, f32 x): sub_005A5EA0(o->+8, p, p)
// when both are set, then sub_005988C8(o->+0, x).
DECOMP(004F303C, Obj_Apply5A5EA0) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 2);
    const f64 x = farg(ctx, 0);
    if (p != 0) {
        const u32 c = load<u32>(o + 8);
        ctx.gpr[4] = c;
        if (c != 0) call(ctx, cfn::sub_005A5EA0, {c, p, p});
    }
    callf(ctx, cfn::sub_005988C8, {load<u32>(o)}, {x});
}

// void HandleMap_ForEach35BD08(): sub_0035BD08(obj) for every object in the handle map.
DECOMP(0035BFB0, HandleMap_ForEach35BD08) {
    for (u32 i = 0; static_cast<s32>(i) < static_cast<s32>(load<u16>(kHandleMap + 2)); ++i)
        call(ctx, cfn::sub_0035BD08, {load<u32>(load<u32>(kHandleMap + 8) + i * 8 + 4)});
}

// u16 Table560_Count(Object* o, u32 key): the u16 count (+8) of o->+560's
// vector for key, 0 without one (looked up twice).
DECOMP(004ED0CC, Table560_Count) {
    const u32 map = arg<u32>(ctx, 0) + 560;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 1));
    u32 n = 0;
    if (load<u32>(static_cast<u32>(call(ctx, cfn::Table_Find, {map, frame.addr()}))) != 0)
        n = load<u16>(load<u32>(static_cast<u32>(call(ctx, cfn::Table_Find, {map, frame.addr()}))) + 8);
    ret(ctx, n);
}

// u32 Table560_At(Object* o, u32 key, s32 i): element i of o->+560's vector
// for key, 0 when missing or out of range. Leaves r4 = the vector (or its data).
DECOMP(004ED140, Table560_At) {
    const u32 map = arg<u32>(ctx, 0) + 560;
    const s32 i = static_cast<s32>(arg<u32>(ctx, 2));
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), arg<u32>(ctx, 1));
    const u32 v = load<u32>(static_cast<u32>(call(ctx, cfn::Table_Find, {map, frame.addr()})));
    ctx.gpr[4] = v;
    if (v == 0 || i < 0) return ret(ctx, 0u);
    const u32 n = load<u16>(v + 8);
    ctx.gpr[5] = n;
    if (i >= static_cast<s32>(n)) return ret(ctx, 0u);
    const u32 data = load<u32>(v);
    ctx.gpr[4] = data;
    ret(ctx, load<u32>(data + static_cast<u32>(i) * 4));
}

// void Obj_SetState3(Object* o): state (+160 bits 1..) = 3, vfunc +20(o, 1),
// then sub_004CF988(o->+20, 0x2237D4) when set. r3 = 0 otherwise.
DECOMP(0036CC28, Obj_SetState3) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = (load<u32>(o + 160) & 1u) | (3u << 1);
    const u32 opd = load<u32>(load<u32>(o) + 20);
    store<u32>(o + 160, s);
    call_ptr(ctx, opd, {o, 1, opd, load<u32>(opd)});
    const u32 p = load<u32>(o + 20);
    if (p == 0) return ret(ctx, 0u);
    call(ctx, cfn::sub_004CF988, {p, kTag2237D4});
}

// void World_ApplyBrightness(): sub_0052F8A0(clamp((1 - settings f4) * 0.4 + 0.8, 0.8, 1.2))
// from the current world's settings block (name?)
DECOMP(0046C8D0, World_ApplyBrightness) {
    const u32 c = kBrightnessConsts;
    const u32 p = load<u32>(kCurrentWorld) + kWorldSettings;
    const f32 c0 = load<f32>(c), c1 = load<f32>(c + 4), c2 = load<f32>(c + 8), c3 = load<f32>(c + 12);
    const f64 hi = load<f64>(c + 16), lo = load<f64>(c + 24);
    f32 x = static_cast<f32>(static_cast<f64>(c0) - static_cast<f64>(load<f32>(p + 4)));
    x = static_cast<f32>(std::fma(static_cast<f64>(x), static_cast<f64>(c1), static_cast<f64>(c2)));
    const f32 d4 = static_cast<f32>(static_cast<f64>(c3) - static_cast<f64>(x));
    x = static_cast<f32>(rt::fsel(d4, hi, x));  // fsel f1, f4, f1, f5: d4 >= 0 ? x : hi
    const f32 d3 = static_cast<f32>(static_cast<f64>(x) - static_cast<f64>(c2));
    x = static_cast<f32>(rt::fsel(d3, lo, x));
    ctx.fpr[2] = lo;
    ctx.fpr[3] = d3;
    ctx.fpr[4] = d4;
    ctx.fpr[5] = hi;
    callf(ctx, cfn::sub_0052F8A0, {c, p}, {x});
}

// void Obj_RemoveDead3700(Object* o): removes (swap with last) every entry of
// the Object* array at +3700 (count +20) whose +172 lacks bit 0x40000.
// Leaves r4..r10 as the original's loop.
DECOMP(0001A7C0, Obj_RemoveDead3700) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 arr = o + 3700;
    s32 n = static_cast<s32>(load<u32>(o + 20));
    s32 i = 0;
    ctx.gpr[6] = static_cast<u64>(static_cast<s64>(n));
    ctx.gpr[9] = 0;
    if (n <= 0) return;
    u32 p = arr;
    u32 e = 0, bit = 0;
    do {
        e = p;
        bit = (load<u32>(load<u32>(e) + 172) >> 18) & 1;
        if (bit == 0) {
            --n;
            --i;
            p -= 4;
            store<u32>(o + 20, static_cast<u32>(n));
            store<u32>(e, load<u32>(arr + static_cast<u32>(n) * 4));
            n = static_cast<s32>(load<u32>(o + 20));
        }
        ++i;
        p += 4;
    } while (i < n);
    ctx.gpr[4] = p;
    ctx.gpr[5] = ctx.gpr[6] = static_cast<u64>(static_cast<s64>(n));
    ctx.gpr[7] = arr;
    ctx.gpr[8] = e;
    ctx.gpr[9] = static_cast<u64>(static_cast<s64>(i));
    ctx.gpr[10] = bit;
}

// s32 MsgDialog_Poll(): 0 with no dialog; runs the sysutil callbacks; 3 while
// it is still open; otherwise closes it and returns (and clears) the state.
DECOMP(004688C8, MsgDialog_Poll) {
    if (load<u32>(kMsgDialogState) == 0) return ret(ctx, 0u);
    call(ctx, cfn::Sysutil_CheckCallback);
    if (load<u32>(kMsgDialogState) == 3) return ret(ctx, 3u);
    callf(ctx, cfn::cellMsgDialogClose, {kMsgDialogCloseDelay}, {static_cast<f64>(load<f32>(kMsgDialogCloseDelay))});
    const s32 st = static_cast<s32>(load<u32>(kMsgDialogState));
    store<u32>(kMsgDialogState, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = static_cast<u32>(st);
    ret(ctx, st);
}

// void World_ApplyGamma(World* w): with config byte +35 == 1 (3D?),
// sub_005369E8(settings f28 * 0.09 + 0.01) and sub_00536958(settings +0 bit 0x04000000) (name?)
DECOMP(0046C830, World_ApplyGamma) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 b = ref<u8>(g::config + 35);
    ctx.gpr[4] = b;
    if (b != 1) return;
    const u32 p = w + kWorldSettings;
    const f32 c0 = load<f32>(kGammaConsts), c1 = load<f32>(kGammaConsts + 4);
    const f32 f = load<f32>(p + 28);
    ctx.fpr[2] = c1;
    ctx.fpr[3] = f;
    callf(ctx, cfn::sub_005369E8, {p, kGammaConsts},
          {static_cast<f32>(std::fma(static_cast<f64>(f), static_cast<f64>(c0), static_cast<f64>(c1)))});
    call(ctx, cfn::sub_00536958, {(load<u32>(p) >> 26) & 1});
}

// Object* Player_Get(): the object of the handle at 0x00F64D18, or else of the
// one at 0x00F64D10, while its flags (+4) have bit 0x200; 0 otherwise (name?)
// The first lookup passes the caller's r5 through, the second the handle.
DECOMP(00365C24, Player_Get) {
    constexpr u32 kHandleA = 0x00F64D18, kHandleB = 0x00F64D10, kLive = 0x200;
    Stack frame(ctx, 16);
    const u32 a = load<u32>(kHandleA);
    store<u32>(frame.addr() + 4, a);
    if (a != 0) {
        const u32 obj = load<u32>(static_cast<u32>(call(ctx, cfn::SortedMap_Find, {kHandleMap, frame.addr() + 4, ctx.gpr[5]})));
        if (obj != 0) {
            const u32 m = load<u32>(obj + 4) & kLive;
            ctx.gpr[4] = m;
            if (m != 0) return ret(ctx, obj);
        }
    }
    const u32 b = load<u32>(kHandleB);
    if (b == 0) return ret(ctx, 0u);
    store<u32>(frame.addr(), b);
    const u32 obj = load<u32>(static_cast<u32>(call(ctx, cfn::SortedMap_Find, {kHandleMap, frame.addr(), b})));
    if (obj == 0) return ret(ctx, 0u);
    const u32 m = load<u32>(obj + 4) & kLive;
    ctx.gpr[4] = m;
    ret(ctx, m != 0 ? obj : 0u);
}

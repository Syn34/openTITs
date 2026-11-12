// Game code, large-function sweep part 29: the add side of the 16-byte
// {owner, object, handle, flags} table at *0x00F65858 (the counterpart of
// PairTable_Remove_0042C7DC).
#include "big29.h"
#include "big30.h"

namespace sly4::obj {

namespace {
constexpr u32 kTable = 0x00F65858;   // PairEntry* (16 bytes each)
constexpr u32 kCount = 0x010AA690;   // u32 entries in use
constexpr u32 kProfileHash = 0x00014120;
constexpr u32 Save_Profile = 0x000C9FC8;     // (u32) -> profile or 0
constexpr u32 Obj_SceneEnd = 0x000DD02C;     // (Obj*, u8 drop, u8 keep)
constexpr u32 Obj_SetFlag968 = 0x00124370;   // (Obj*, bool)
constexpr u32 Obj_Detach_1243D0 = 0x001243D0;       // (Obj*)
constexpr u32 Player_SetSave = 0x00124D58;   // (Player*, Def*, u8 quick)
constexpr u32 Anim_Stop = 0x00254814;        // (Animator*, key)

struct PairEntry {
    be_u32 owner;   // +0
    be_u32 obj;     // +4: PairObj*
    be_u32 handle;  // +8
    be_u32 flags;   // +12
};
static_assert(sizeof(PairEntry) == 16);
// The object's control component: u16 +40 flags.
struct PairComp {
    pad<40> _0;
    be_u16 flags;  // +40
};
struct PairObj {
    pad<164> _0;
    gp<PairComp> comp;  // +164
    be_u32 key;         // +168
    be_u32 f172;        // +172
    pad<2> _176;
    be_u16 pairs;       // +178
    pad<8> _180;
    be_u32 f188;        // +188
    be_u32 f192;        // +192
    pad<4> _196;
    be_u32 f200;        // +200
};
static_assert(offsetof(PairObj, pairs) == 178 && offsetof(PairObj, f200) == 200);
struct PairProfile {
    pad<20> _0;
    be_u32 flags;  // +20
};

// The 0/1 of bit `sh` of v (rlwinm + cntlzw / xori).
inline u32 Bit(u32 v, u32 sh) { return (v >> sh) & 1; }
PairEntry& Entry(u32 base, u32 i) { return gm<PairEntry>(base + 16 * i); }
}  // namespace

// void f(owner a, Obj* o, u32 handle, bool top, bool r7, bool b28, bool
// b29, bool r10; stack arguments 8..11: bool s8, bool s9, bool s10, bool
// s11; top / b28 / b29 as their low bit) (called by f_00432AB4, Cmd_CameraSplit43807C, Cmd_Fire_004387DC,
// f_00439FD0). Nothing without a and o. The key is handle (or o->+168).
// With s11 and an entry whose object's key (its +8, else the object's
// +168) matches, every entry with that object gets +0 = a and that is
// all but the end calls; else a new entry {a, o, handle, flags} is
// appended (flags +12: bit 31 top, 30 o->+172 bit 1, 29 b29, 28 b28, 27 /
// 25 bits 12 / 10 of o->+164's u16 +40, 26 o->+172 bit 3; the rest kept),
// o->+178 counts it; without s9, o's +192 bit 3 and +188 bit 5 the profile
// 0x14120 may be applied (Player_SetSave(o, p, 0) when its +20 bit 3 and
// o's +188 bit 0 are set); o->vfn[96](o, 1); o->+164's +40 gets 0x1000
// set and 0x400 cleared; with r10 o->vfn[60]() is stopped (Anim_Stop(it,
// 0)) or, without one, f_000DC314(o, 0, 0); with o's +192 bit 3
// Obj_Detach_1243D0(o) (and Obj_SetFlag968(o, 1) with s10); without s8, r7 or
// o's +200 bit 8 gives o->vfn[176](o, 1). Ends with Obj_SceneEnd(o, 1, 0)
// and o->vfn[180](o, 1). No caller reads r3 (r3.sh).
void PairTable_Add_0042C328(u32 a, PairObj* o, u32 handle, u32 top, u8 f7, u32 b28, u32 b29, u8 f10, u8 s8, u8 s9, u8 s10,
                            u8 s11) {
    if (a == 0 || o == nullptr) return;
    auto finish = [&] {
        call(Obj_SceneEnd, o, 1u, 0u);
        vcall(o, 180, 1u);
    };
    be_u32& count = global<be_u32>(kCount);
    const u32 key = handle != 0 ? handle : o->key.get();
    if (key != 0) {
        u32 n = count;
        for (u32 i = 0; n > i && n != 0; ++i) {
            u32 base = global<be_u32>(kTable);
            const u32 eo = Entry(base, i).obj;
            if (eo == 0) continue;
            u32 k = Entry(base, i).handle;
            if (k == 0) k = gm<PairObj>(eo).key;
            if (k != key) continue;
            if (s11 == 0) break;  // append
            // Every entry of that object gets the owner (the loop restarts
            // at 0 with the table / count as last read).
            for (u32 j = 0; n > j; ++j) {
                PairEntry& e2 = Entry(base, j);
                if (e2.obj == eo) {
                    e2.owner = a;
                    n = count;
                    base = global<be_u32>(kTable);
                }
            }
            return finish();
        }
    }
    const u32 cnt = count;
    PairEntry& e = Entry(global<be_u32>(kTable), cnt);
    count = cnt + 1;
    u32 w = e.flags;
    e.owner = a;
    w = (w & ~0x20000000u) | ((b29 & 1) << 29);
    e.obj = guest(o);
    e.handle = handle;
    w = (w & ~0x10000000u) | ((b28 & 1) << 28);
    e.flags = w;
    w = (w & ~0x40000000u) | (Bit(o->f172, 1) << 30);
    e.flags = w;
    u32 c = o->comp.ea;
    w = (w & ~0x08000000u) | ((c != 0 ? Bit(gm<PairComp>(c).flags, 12) : 0) << 27);
    e.flags = w;
    w = (w & ~0x04000000u) | (Bit(o->f172, 3) << 26);
    e.flags = w;
    c = o->comp.ea;
    w = (w & ~0x02000000u) | ((c != 0 ? Bit(gm<PairComp>(c).flags, 10) : 0) << 25);
    w = (w & 0x7FFFFFFFu) | ((top & 1) << 31);
    e.flags = w;
    o->pairs = static_cast<u16>(o->pairs + 1);
    if (s9 == 0 && (o->f192 & 0x8u) != 0 && (o->f188 & 0x20u) != 0) {
        const u32 p = call<u32>(Save_Profile, kProfileHash);
        if (p != 0 && (gm<PairProfile>(p).flags & 0x8u) != 0 && (o->f188 & 0x1u) != 0) call(Player_SetSave, o, p, 0u);
    }
    vcall(host<u8>(e.obj), 96, 1u);
    if (const u32 k = gm<PairObj>(e.obj).comp.ea; k != 0) {
        PairComp& pc = gm<PairComp>(k);
        pc.flags = static_cast<u16>(pc.flags | 0x1000);
        PairComp& pc2 = gm<PairComp>(gm<PairObj>(e.obj).comp.ea);
        pc2.flags = static_cast<u16>(pc2.flags & ~0x0400);
    }
    if (f10 != 0) {
        const u32 an = vcall<u32>(host<u8>(e.obj), 60);
        if (an == 0)
            Obj_StopVoiceTree_000DC314(host<VoiceObj>(e.obj), 0u, 0);
        else
            call(Anim_Stop, an, 0u);
    }
    if (const u32 q = e.obj; q != 0 && (gm<PairObj>(q).f192 & 0x8u) != 0) {
        call(Obj_Detach_1243D0, q);
        if (s10 != 0) call(Obj_SetFlag968, q, 1u);
    }
    if (s8 == 0) {
        bool on = f7 != 0;
        if (!on) {
            const u32 q = e.obj;
            on = q != 0 && (gm<PairObj>(q).f200 & 0x100u) != 0;
        }
        if (on) vcall(host<u8>(e.obj), 176, 1u);
    }
    finish();
}

}  // namespace sly4::obj

NATIVE(0042C328, sly4::obj::PairTable_Add_0042C328)

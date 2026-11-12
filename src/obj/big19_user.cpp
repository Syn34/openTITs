// Game code, large-function sweep part 19: moving a user (player) block to
// a new level object (User_Travel484BE0).
#include "big19.h"

namespace sly4::obj {

namespace {
constexpr u32 Save_Profile = 0x000C9FC8;          // (Object*) -> profile or 0
constexpr u32 Obj_Notify24 = 0x000D949C;          // (Object*) -> object or 0
constexpr u32 Obj_LayerBelow = 0x000FCBD0;        // (list, Object*) -> next or 0
constexpr u32 Target_SetRef0FEEBC = 0x000FEEBC;   // (o, player)
constexpr u32 Obj_SetRef1088E4 = 0x001088E4;      // (o, player)
constexpr u32 Obj_SetRefs108A34 = 0x00108A34;     // (o, player)
constexpr u32 Obj_SetRef10DD60 = 0x0010DD60;      // (o, player)
constexpr u32 Obj_SetRef11135C = 0x0011135C;      // (o, player)
constexpr u32 Profile_Item121B40 = 0x00121B40;    // (profile, k) -> u32 hash or 0
constexpr u32 Obj_SlotIndex121B68 = 0x00121B68;   // (profile, u32 hash) -> s32 or -1
constexpr u32 Obj_Release12BF70 = 0x0012BF70;     // (Object*)
constexpr u32 Coach_FindSpline12C968 = 0x0012C968;  // (Object*, r4, u32 0x10000; f1)
constexpr u32 Obj_DropLayer2169C0 = 0x002169C0;   // (Object*)
constexpr u32 Obj_UpdateFlags2336 = 0x0029D838;   // (Object*, 0)
constexpr u32 Entry_IsNew2E9B78 = 0x002E9B78;     // (u32 hash)
constexpr u32 Registry_Notify = 0x002E9BE4;       // (u32 hash)
constexpr u32 HashedName_Set = 0x0036960C;        // (u32 user, const char*)
constexpr u32 Callbacks_Bind44BD74 = 0x0044BD74;  // ()
constexpr u32 User0_RefreshTarget = 0x0047BFD4;   // (World*, u32 user)
constexpr u32 World_SetCameraTarget = 0x00484858; // (World*, UserBlock*)
constexpr u32 Nop = 0x0054EC18;                   // (vec)
constexpr u32 PtrVector_Clear = 0x0054EC1C;       // (vec)
constexpr u32 SmallFree = 0x00552420;             // (p, u32 size)
constexpr u32 String_Format = 0x005534D0;         // (fmt, ...) -> const char*
constexpr u32 String_Copy = 0x00553684;           // (dst, src, u32 n)
constexpr u32 Vector_Add733964 = 0x00733964;      // (vec, const 192-byte record*)
constexpr u32 Singleton_Get = 0x007D3B80;         // () -> singleton (vtable +64)

constexpr u32 kUsers = 0x1280F0;     // World +0x1280F0: Object*[9], one per user
constexpr u32 kW128164 = 0x128164;   // World: an object whose +5572 bit 0x04000000 asks for item hashes
constexpr u32 kLayers = 0x1281E0;    // World: the layer list (u8 +345 index, +364 Object*[])
constexpr u32 kK = 0x00484BCC;       // .text f32 {1.0, 0.0, -1.0, 3.0}
constexpr u32 kIdentity = 0x01164BD0;  // identity matrix (rows of 16 bytes)
constexpr u32 kDescVec3 = 0x01164B30;
constexpr u32 kDescVt = 0x00B12260;    // the spawn descriptor vtable
constexpr u32 kFmtControllable = 0x00AE91A8;  // "Player%02d_Controllable"

constexpr u32 PtrVector_Construct = 0x005516E0;    // (vec)

// The user block: u8 +0 team, u8 +2 user, +8 player, +12 level.
struct UserBlock {
    u8 team;            // +0
    pad<1> _1;
    u8 user;            // +2
    pad<5> _3;
    be_u32 player;      // +8
    be_u32 level;       // +12
};
// A player object.
struct UtPlayer {
    pad<96> _0;
    Mtx44 m;            // +96
    pad<12> _160;
    u8 team;            // +172
    pad<5> _173;
    be_u16 refs;        // +178
    pad<4> _180;
    be_u64 f184;        // +184
    be_u32 f192;        // +192
    pad<4> _196;
    be_u32 f200;        // +200
    pad<12> _204;
    be_u32 owner;       // +216
    pad<32> _220;
    be_u32 vec;         // +252: PtrVec* of attached objects
    pad<88> _256;
    be_u32 w344;        // +344
    be_u32 w348;        // +348
    pad<28> _352;
    be_u32 w380;        // +380
    be_u32 w384;        // +384
    pad<188> _388;
    Mtx44 spot;         // +576: the saved spot (rows x / y / z)
    u8 sb[4];           // +640
    be_u32 s68;         // +644
    be_u32 s72;         // +648
};
static_assert(offsetof(UtPlayer, team) == 172 && offsetof(UtPlayer, refs) == 178 && offsetof(UtPlayer, owner) == 216);
static_assert(offsetof(UtPlayer, vec) == 252 && offsetof(UtPlayer, w344) == 344 && offsetof(UtPlayer, w380) == 380);
static_assert(offsetof(UtPlayer, spot) == 576 && offsetof(UtPlayer, s72) == 648);
// The spawn descriptor (192 bytes on the stack).
struct UtDesc {
    be_u32 vt;          // +0
    be_u32 w4, w8, w12; // +4 .. +12
    be_u32 vec;         // +16: the record vector
    pad<12> _20;
    Mtx44 m;            // +32
    Vec4 v96;           // +96
    be_u32 w112;        // +112
    be_u32 w116;        // +116
    be_u32 prof;        // +120
    be_u32 w124;        // +124
    be_u32 w128;        // +128
    be_u32 flags;       // +132
    be_u32 w136;        // +136
    u8 name[52];        // +140
};
static_assert(sizeof(UtDesc) == 192);
// The original's locals (sp offsets - 192).
struct UtLocals {
    UtDesc desc;        // sp+192
    PtrVec<u8> v;       // sp+384
};

f64 K(u32 off) { return lf(global<be_f32>(kK + off)); }
UtPlayer& Pl(u32 ea) { return gm<UtPlayer>(ea); }

// The layer walk: each object o of World +kLayers (from its +364 slot
// [u8 +345], then Obj_LayerBelow) asks vfunc 8 for its type (re-asked for
// each case tried); a case whose vfunc 56 owner is `old` (`zeroToo`: or 0
// for type 11) is pointed at the user's new object (ub +8).
struct Case { s32 type; u32 fn; };
void WalkLayers(u32 w, UserBlock* ub, u32 old, const Case* cs, u32 n, bool zeroToo11, bool type3) {
    const u32 lst = gm<be_u32>(w + kLayers);
    const u32 b = gm<u8>(lst + 345);
    u32 o = b != 0 ? gm<be_u32>(lst + 4 * b + 364).get() : 0u;
    while (o != 0) {
        bool done = false;
        for (u32 k = 0; k < n && !done; ++k) {
            if (vcall<s32>(&gm<u8>(o), 8) != cs[k].type) continue;
            done = true;
            const u32 own = vcall<u32>(&gm<u8>(o), 56);
            const u32 u = ub->player;
            if (own == old || (zeroToo11 && cs[k].type == 11 && own == 0)) call(cs[k].fn, o, u);
        }
        if (!done && type3 && vcall<s32>(&gm<u8>(o), 8) == 3) {
            if (vcall<u32>(&gm<u8>(o), 56) == old) {
                const u32 h = gm<be_u32>(o + 84);
                const u32 u = ub->player;
                if (h != 0) vcall(&gm<u8>(h), 40, 0u);
                gm<be_u32>(o + 84) = u;
                if (u != 0) Pl(u).refs = static_cast<u16>(Pl(u).refs + 1);
            }
        }
        o = call<u32>(Obj_LayerBelow, gm<be_u32>(w + kLayers).get(), o);
    }
}

// P's attachment vector (+252), made on demand (20 bytes, u16 +6 = 4).
u32 AttachVec(u32 P) {
    u32 vec = Pl(P).vec;
    if (vec == 0) {
        vec = call<u32>(fn::SmallAlloc, 20u);
        Pl(P).vec = vec;
        if (vec != 0) {
            call(PtrVector_Construct, vec);
            gm<be_u16>(vec + 6) = 4;
            vec = Pl(P).vec;
        }
    }
    return vec;
}
}  // namespace

// void f(World* w, UserBlock* ub, Object* level, const Mat* m, u8 a5, u8 a6,
// u8 a7, u8 a8, u8 a9 (stack)) (name?): ub {u8 +0 team, u8 +2 user, +8
// player, +12 level}. Nothing when the player's owner (+216 +52) already is
// `level` (0: level = ub +12), or without a level profile. The old
// player P's flags are noted (+192 bits 8 / 0x100 / 2, +200 bit 0x400);
// without a6 its attached objects (vector +252, made on demand) with bit
// 0x00800000 are dropped and detached (vfunc 288(o, P)); with a6 they are
// all detached and kept in a stack vector (the loop spins while the first
// is 0, as the original). Item hashes are exchanged between P's owner and
// the profile when World +0x128164 asks (Profile_Item121B40 / Registry_
// Notify / Entry_IsNew / the singleton's vfunc 64, then Callbacks_Bind44BD74).
// P's saved spot (+576: matrix, 4 bytes, word, f32) is kept when it has
// bit 8; P gets vfunc 88(P, 0, 1, 1, 1) and vfunc 40(P, 0 / 1). The user's
// slot object (World +0x1280F0[ub +2]) is reused when it stands for the
// profile (+216 +52), else the profile spawns one (vfunc 28 with the
// "Player%02d_Controllable" descriptor at m / P +96 / identity, team in
// flag bits 21..28, its 192-byte records added). The new object becomes ub
// +8 and the slot (u16 +178 bumped, vfunc 44, bit 0 of the u64 +184);
// with bit 8 it is released (a8) or put on a coach spline, takes the saved
// spot, and its notify / vfunc 376 / +384 objects get vfunc 44(x, 1). Its
// owner's name is hashed for the user (and the target refreshed with a9).
// Then (a5) the camera target or the layer objects are pointed at it, a6's
// kept objects re-attached (vfunc 284), a7 clamps its +348 to min(+344 *
// the old ratio, +344) and 0, and the slot still holding P takes it.
// No caller reads r3.
void User_Travel484BE0(u32 w, UserBlock* ub, u32 level, u32 matArg, u8 a5, u8 a6, u8 a7, u8 a8, u8 a9) {
    GuestFrame frame(400);
    UtLocals& L = *frame.as<UtLocals>(192);
    UtDesc& desc = L.desc;
    PtrVec<u8>& V = L.v;

    u32 P = ub->player;
    if (level == 0) level = ub->level;
    if (P != 0) {
        const u32 o = Pl(P).owner;
        if ((o != 0 ? gm<be_u32>(o + 52).get() : 0u) == level) return;
    }
    if (level == 0) return;
    const u32 prof = call<u32>(Save_Profile, level);
    if (prof == 0) return;
    P = ub->player;
    const bool pf8 = P != 0 && (Pl(P).f192 & 0x8u) != 0;
    const u32 own8 = pf8 ? P : 0;
    const bool f400 = P != 0 && (Pl(P).f200 & 0x400u) != 0;
    const bool pf100 = P != 0 && (Pl(P).f192 & 0x100u) != 0;
    const u32 coach = f400 ? gm<be_u32>(Pl(P).owner + 240).get() : 4u;
    const bool pf2 = P != 0 && (Pl(P).f192 & 0x2u) != 0;
    u32 ratio = global<be_u32>(kK);  // f32 bits: P +348 / +344 when +344 > 0, else 1.0
    if (pf2) {
        const u32 w344 = Pl(P).w344, w348 = Pl(P).w348;
        if (LfsW(w344) > K(4)) ratio = std::bit_cast<u32>(static_cast<f32>(LfsW(w348) / LfsW(w344)));
    }

    if (a6 == 0 && P != 0) {
        const u32 vec = AttachVec(P);
        for (u32 i = 0; i < gm<PtrVec<u8>>(vec).count; ++i) {
            const u32 o = gm<be_u32>(gm<be_u32>(vec) + 4 * i);
            if (o != 0 && (gm<be_u32>(o + 192) & 0x00800000u) != 0) {
                call(Obj_DropLayer2169C0, o);
                vcall(&gm<u8>(o), 288, P);
            }
        }
    }
    call(PtrVector_Construct, &V);
    gm<be_u16>(guest(&V) + 6) = 4;
    if (a6 != 0) {
        const u32 vec = AttachVec(P);  // unchecked P (as the original)
        while (gm<PtrVec<u8>>(vec).count != 0) {
            const u32 o = gm<be_u32>(gm<be_u32>(vec));
            if (o == 0) continue;  // spins forever (as the original)
            vcall(&gm<u8>(o), 288, P);
            PtrVecPush(V, o);
        }
    }

    // The spawn matrix (prof is never 0 here: the original's test is dead).
    const Mtx44& m = gm<Mtx44>(matArg != 0 ? matArg : (P != 0 ? P + 96 : kIdentity));
    Mtx44 mw;
    CopyMtxXYZ(mw, m);
    const u32 team = P != 0 ? Pl(P).team : ub->team;

    const u32 x = gm<be_u32>(w + kW128164);
    if (x != 0 && (gm<be_u32>(x + 5572) & 0x04000000u) != 0) {
        u32 src = 0;
        if (own8 != 0) {
            const u32 q = Pl(own8).owner;
            if (q != 0 && (gm<be_u32>(q + 20) & 0x8u) != 0) src = q;
        }
        const u32 dst = (gm<be_u32>(prof + 20) & 0x8u) != 0 ? prof : 0u;
        auto item = [](u32 p, u32 k) { return call<u64>(Profile_Item121B40, p, k); };
        auto notifyAll = [&](u32 p) {
            for (u32 k = 0; k < 4; ++k) call(Registry_Notify, item(p, k));
        };
        auto single = [&]() { vcall(&gm<u8>(call<u32>(Singleton_Get)), 64); };
        if (dst != 0 && src != 0) {
            for (u32 k = 0; k < 4; ++k) {
                if (item(src, k) == 0) continue;
                const u32 h = static_cast<u32>(item(src, k));
                if (call<s32>(Obj_SlotIndex121B68, dst, h) == -1) call(Entry_IsNew2E9B78, item(src, k));
            }
            single();
            notifyAll(dst);
        } else if (dst != 0) {
            notifyAll(dst);
        } else if (src != 0) {
            for (u32 k = 0; k < 4; ++k) call(Entry_IsNew2E9B78, item(src, k));
            single();
        }
        call(Callbacks_Bind44BD74);
    }

    // P's saved spot (identity, zeros without bit 8).
    Mtx44 st;
    CopyMtxXYZ(st, global<Mtx44>(kIdentity));
    u8 sb[4] = {0, 0, 0, 0};
    u32 s68 = 0, s72 = 0;
    bool saved = false;
    if (P != 0 && (Pl(P).f192 & 0x8u) != 0) {
        CopyMtxXYZ(st, Pl(P).spot);
        for (u32 k = 0; k < 4; ++k) sb[k] = Pl(P).sb[k];
        s68 = Pl(P).s68;
        s72 = Pl(P).s72;
        saved = true;
    }
    if (P != 0) {
        vcall(&Pl(P), 88, 0u, 1u, 1u, 1u);
        vcall(&Pl(P), 40, 0u);
        vcall(&Pl(P), 40, 1u);
    }

    const u32 user = ub->user;
    const u32 q = gm<be_u32>(w + kUsers + 4 * user);
    bool reuse = false;
    if (q != 0) {
        const u32 o = Pl(q).owner;
        reuse = (o != 0 ? gm<be_u32>(o + 52).get() : 0u) == gm<be_u32>(prof + 52);
    }
    u32 obj = q;
    if (!reuse) {
        // The spawn descriptor (as the original's frame).
        const u32 name = call<u32>(String_Format, kFmtControllable, user + 1);
        desc.vec = 0u;
        desc.vt = kDescVt;
        CopyMtxXYZ(desc.m, mw);
        const u32 old = desc.flags;  // read before it is written (stack garbage)
        CopyXYZ(desc.v96, global<Vec3>(kDescVec3));
        desc.w112 = 0u;
        desc.w116 = 0u;
        desc.prof = prof;
        desc.w124 = gm<be_u32>(prof + 36).get();
        desc.w128 = gm<be_u32>(prof + 40).get();
        u32 fl = (old | 0x80000000u) & ~0x40000000u;
        fl = (fl & ~0x20000000u) | (((static_cast<u32>(gm<u8>(prof + 32)) >> 2) & 1u) << 29);
        desc.w136 = 0u;
        fl |= 0x1FE00000u;
        desc.flags = fl;
        desc.w4 = 0u;
        desc.w8 = 0u;
        desc.w12 = 0u;
        desc.name[0] = 0;
        if (name != 0) call(String_Copy, &desc.name[0], name, 40u);
        u32 recs = gm<be_u32>(prof + 136);
        if (recs != 0 && gm<be_u16>(recs + 8) != 0) {
            for (u32 k = 0, off = 0;; ++k, off += 192) {
                const u32 n = recs != 0 ? gm<be_u16>(recs + 8).get() : 0u;
                if (k >= n) break;
                const u32 rec = gm<be_u32>(recs) + off;
                u32 vp = desc.vec;
                if (vp == 0) {
                    vp = call<u32>(fn::SmallAlloc, 20u);
                    desc.vec = vp;
                    if (vp != 0) {
                        call(PtrVector_Construct, vp);
                        gm<be_u16>(vp + 6) = 192;
                        vp = desc.vec;
                    }
                }
                call(Vector_Add733964, vp, rec);
                recs = gm<be_u32>(prof + 136);
            }
        }
        desc.flags = (desc.flags & ~0x1FE00000u) | ((team << 21) & 0x1FE00000u);
        obj = vcall<u32>(&gm<u8>(prof), 28, &desc);
        const u32 vp = desc.vec;
        desc.vt = kDescVt;  // the descriptor's destructor
        if (vp != 0) {
            call(PtrVector_Clear, vp);
            call(Nop, vp);
            call(SmallFree, desc.vec.get(), 20u);
        }
    }

    if (obj != 0) {
        ub->player = obj;
        gm<be_u32>(w + kUsers + 4 * ub->user) = obj;
        const u32 n = ub->player;
        Pl(n).refs = static_cast<u16>(Pl(n).refs + 1);
        vcall(&Pl(n), 44);
        const u32 p2 = ub->player;
        gm<be_u64>(p2 + 184) = gm<be_u64>(p2 + 184) | 1u;  // before the null test (as the original)
        if (p2 != 0 && (Pl(p2).f192 & 0x8u) != 0) {
            if (a8 != 0) call(Obj_Release12BF70, p2);
            else call(Coach_FindSpline12C968, p2, kK, 0x10000u, K(12));
            if (saved) {
                UtPlayer& d2 = Pl(p2);
                CopyMtxXYZ(d2.spot, st);
                for (u32 k = 0; k < 4; ++k) d2.sb[k] = sb[k];
                d2.s68 = s68;
                d2.s72 = s72;
            }
            if (const u32 nt = call<u32>(Obj_Notify24, p2); nt != 0) {
                vcall(&gm<u8>(nt), 44, 1u);
                call(Obj_UpdateFlags2336, nt, 0u);
            }
            if (const u32 a = vcall<u32>(&Pl(p2), 376); a != 0) vcall(&gm<u8>(a), 44, 1u);
            const u32 b = Pl(p2).w384;
            Pl(p2).w380 = b;
            if (b != 0) vcall(&gm<u8>(b), 44, 1u);
        }
        const u32 owner = Pl(ub->player).owner;
        const u32 u = ub->user;
        const u32 nm = vcall<u32>(&gm<u8>(owner), 8);
        if (nm != 0 && gm<u8>(nm) != 0) {
            call(HashedName_Set, u, nm);
            if (a9 != 0) call(User0_RefreshTarget, w, u);
        }
    }

    // Point the camera / the layer objects at the new player.
    const u32 p3 = ub->player;
    if (p3 != 0 && Pl(p3).team == 0) {
        const bool g8 = pf8 && (Pl(p3).f192 & 0x8u) != 0;
        const bool g400 = f400 && (Pl(p3).f200 & 0x400u) != 0;
        const bool g100 = pf100 && (Pl(p3).f192 & 0x100u) != 0;
        static constexpr Case kA[] = {{1, Obj_SetRef1088E4}, {11, Target_SetRef0FEEBC}, {2, Obj_SetRefs108A34}};
        static constexpr Case kC[] = {{7, Obj_SetRef10DD60}, {9, Obj_SetRef11135C}, {1, Obj_SetRef1088E4},
                                      {11, Target_SetRef0FEEBC}, {2, Obj_SetRefs108A34}};
        if (a5 == 0) {
            call(World_SetCameraTarget, w, ub);
        } else if (g8) {
            WalkLayers(w, ub, P, kA, 3, true, false);
        } else if (!g400) {
            if (!g100) call(World_SetCameraTarget, w, ub);
            else WalkLayers(w, ub, P, kA, 3, false, true);
        } else if (coach != gm<be_u32>(Pl(p3).owner + 240)) {
            call(World_SetCameraTarget, w, ub);
        } else {
            WalkLayers(w, ub, P, kC, 5, false, false);
        }
    }

    if (P != 0) {
        if (a6 != 0) {
            for (u32 i = 0; i < V.count; ++i) {
                const u32 o = gm<be_u32>(V.data.ea + 4 * i);
                if (o == 0) continue;
                if (const u32 u = ub->player; u != 0) vcall(&gm<u8>(o), 284, u);
            }
        }
        if (a7 != 0) {
            const u32 u = ub->player;
            const u32 r22 = (u != 0 && (Pl(u).f192 & 0x2u) != 0) ? u : 0u;
            if (pf2 && r22 != 0) {
                const u32 w344 = Pl(r22).w344;
                const f64 f1 = LfsW(w344);
                const f32 prod = static_cast<f32>(LfsW(ratio) * f1);
                const f64 f2 = prod;
                const bool lt = f2 < f1;
                const f64 mn = lt ? f2 : f1;
                if (mn > K(4)) Pl(r22).w348 = lt ? std::bit_cast<u32>(prod) : w344;
                else Pl(r22).w348 = global<be_u32>(kK + 4).get();
            }
        }
        be_u32& slot = gm<be_u32>(w + kUsers + 4 * ub->user);
        if (P == slot) slot = ub->player.get();
    }
    call(PtrVector_Clear, &V);
    call(Nop, &V);
}

}  // namespace sly4::obj

NATIVE(00484BE0, sly4::obj::User_Travel484BE0)

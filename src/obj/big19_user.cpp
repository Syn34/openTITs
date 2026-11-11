// Game code, large-function sweep part 19: moving a user (player) block to
// a new level object (User_Travel484BE0).
#include "big19.h"

using namespace sly4;
using namespace sly4::big19;

namespace {
namespace ut {
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

// Matrix word k (3 per row, rows 16 bytes apart).
constexpr u32 MatOff(u32 k) { return (k / 3) * 16 + (k % 3) * 4; }
inline f64 F(u32 w) { return std::bit_cast<f64>(LfsBits(w)); }

// The layer walk: each object o of World +kLayers (from its +364 slot
// [u8 +345], then Obj_LayerBelow) asks vfunc 8 for its type (re-asked for
// each case tried); a case whose vfunc 56 owner is `old` (`zeroToo`: or 0
// for type 11) is pointed at the user's new object (ub +8).
struct Case { s32 type; u32 fn; };
void WalkLayers(PPUContext& ctx, u32 w, u32 ub, u32 old, const Case* cs, u32 n, bool zeroToo11, bool type3) {
    const u32 lst = load<u32>(w + kLayers);
    const u32 b = load<u8>(lst + 345);
    u32 o = b != 0 ? load<u32>(lst + 4 * b + 364) : 0;
    while (o != 0) {
        bool done = false;
        for (u32 k = 0; k < n && !done; ++k) {
            if (static_cast<s32>(VCall(ctx, o, 8, {o})) != cs[k].type) continue;
            done = true;
            const u32 own = static_cast<u32>(VCall(ctx, o, 56, {o}));
            const u32 u = load<u32>(ub + 8);
            if (own == old || (zeroToo11 && cs[k].type == 11 && own == 0)) call(ctx, cs[k].fn, {o, u});
        }
        if (!done && type3 && static_cast<s32>(VCall(ctx, o, 8, {o})) == 3) {
            if (static_cast<u32>(VCall(ctx, o, 56, {o})) == old) {
                const u32 h = load<u32>(o + 84);
                const u32 u = load<u32>(ub + 8);
                if (h != 0) VCall(ctx, h, 40, {h, 0});
                store<u32>(o + 84, u);
                if (u != 0) store<u16>(u + 178, static_cast<u16>(load<u16>(u + 178) + 1));
            }
        }
        o = static_cast<u32>(call(ctx, Obj_LayerBelow, {load<u32>(w + kLayers), o}));
    }
}
}  // namespace ut
}  // namespace

// u32 f(World* w, UserBlock* ub, Object* level, const Mat* m, u8 a5, u8 a6,
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
// Returns the last call's r3 (Nop) or, early, what the original's r3 holds.
DECOMP(00484BE0, User_Travel484BE0) {
    using namespace ut;
    using namespace b19fn;
    const u32 w = arg<u32>(ctx, 0), ub = arg<u32>(ctx, 1);
    u32 level = arg<u32>(ctx, 2);
    const u32 matArg = arg<u32>(ctx, 3);
    const u32 a5 = arg<u32>(ctx, 4) & 0xFF, a6 = arg<u32>(ctx, 5) & 0xFF;
    const u32 a7 = arg<u32>(ctx, 6) & 0xFF, a8 = arg<u32>(ctx, 7) & 0xFF;
    const u32 a9 = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116) & 0xFF;  // the 9th argument (stack)
    Stack frame(ctx, 400);
    const u32 sp = frame.addr();
    const u32 desc = sp + 192, V = sp + 384;

    u32 P = load<u32>(ub + 8);
    u32 r3 = P;
    if (level == 0) level = load<u32>(ub + 12);
    if (P != 0) {
        const u32 o = load<u32>(P + 216);
        r3 = o != 0 ? load<u32>(o + 52) : 0;
        if (r3 == level) { ret(ctx, r3); return; }
    }
    if (level == 0) { ret(ctx, r3); return; }
    const u32 prof = static_cast<u32>(call(ctx, Save_Profile, {level}));
    if (prof == 0) { ret(ctx, 0); return; }
    P = load<u32>(ub + 8);
    const bool pf8 = P != 0 && (load<u32>(P + 192) & 0x8u) != 0;
    const u32 own8 = pf8 ? P : 0;
    const bool f400 = P != 0 && (load<u32>(P + 200) & 0x400u) != 0;
    const bool pf100 = P != 0 && (load<u32>(P + 192) & 0x100u) != 0;
    const u32 coach = f400 ? load<u32>(load<u32>(P + 216) + 240) : 4;
    const bool pf2 = P != 0 && (load<u32>(P + 192) & 0x2u) != 0;
    u32 ratio = load<u32>(kK);  // f32 bits: P +348 / +344 when +344 > 0, else 1.0
    if (pf2) {
        const u32 w344 = load<u32>(P + 344), w348 = load<u32>(P + 348);
        ctx.fpr[1] = F(w348);
        if (F(w344) > F(load<u32>(kK + 4))) ratio = std::bit_cast<u32>(static_cast<f32>(F(w348) / F(w344)));
    }

    if (a6 == 0 && P != 0) {
        u32 vec = load<u32>(P + 252);
        if (vec == 0) {
            vec = static_cast<u32>(call(ctx, SmallAlloc, {20}));
            store<u32>(P + 252, vec);
            if (vec != 0) {
                call(ctx, PtrVector_Construct, {vec});
                store<u16>(vec + 6, 4);
                vec = load<u32>(P + 252);
            }
        }
        for (u32 i = 0; i < load<u16>(vec + 8); ++i) {
            const u32 o = load<u32>(load<u32>(vec) + 4 * i);
            if (o != 0 && (load<u32>(o + 192) & 0x00800000u) != 0) {
                call(ctx, Obj_DropLayer2169C0, {o});
                VCall(ctx, o, 288, {o, P});
            }
        }
    }
    call(ctx, PtrVector_Construct, {V});
    store<u16>(V + 6, 4);
    if (a6 != 0) {
        u32 vec = load<u32>(P + 252);  // unchecked P (as the original)
        if (vec == 0) {
            vec = static_cast<u32>(call(ctx, SmallAlloc, {20}));
            store<u32>(P + 252, vec);
            if (vec != 0) {
                call(ctx, PtrVector_Construct, {vec});
                store<u16>(vec + 6, 4);
                vec = load<u32>(P + 252);
            }
        }
        while (load<u16>(vec + 8) != 0) {
            const u32 o = load<u32>(load<u32>(vec));
            if (o == 0) continue;  // spins forever (as the original)
            VCall(ctx, o, 288, {o, P});
            PtrVectorPush(ctx, V, o);
        }
    }

    // The spawn matrix (prof is never 0 here: the original's test is dead).
    const u32 m = matArg != 0 ? matArg : (P != 0 ? P + 96 : kIdentity);
    u32 mw[12];
    for (u32 k = 0; k < 12; ++k) mw[k] = load<u32>(m + MatOff(k));
    ctx.fpr[1] = F(mw[4]);
    ctx.fpr[2] = F(mw[5]);
    ctx.fpr[3] = F(mw[6]);
    ctx.fpr[4] = F(mw[3]);
    const u32 team = P != 0 ? load<u8>(P + 172) : load<u8>(ub + 0);

    const u32 x = load<u32>(w + kW128164);
    if (x != 0 && (load<u32>(x + 5572) & 0x04000000u) != 0) {
        u32 src = 0;
        if (own8 != 0) {
            const u32 q = load<u32>(own8 + 216);
            if (q != 0 && (load<u32>(q + 20) & 0x8u) != 0) src = q;
        }
        const u32 dst = (load<u32>(prof + 20) & 0x8u) != 0 ? prof : 0;
        auto notifyAll = [&](u32 p) {
            for (u32 k = 0; k < 4; ++k) call(ctx, Registry_Notify, {call(ctx, Profile_Item121B40, {p, k})});
        };
        auto single = [&]() {
            const u32 s = static_cast<u32>(call(ctx, Singleton_Get, {}));
            VCall(ctx, s, 64, {s});
        };
        if (dst != 0 && src != 0) {
            for (u32 k = 0; k < 4; ++k) {
                if (call(ctx, Profile_Item121B40, {src, k}) == 0) continue;
                const u32 h = static_cast<u32>(call(ctx, Profile_Item121B40, {src, k}));
                if (static_cast<s32>(call(ctx, Obj_SlotIndex121B68, {dst, h})) == -1)
                    call(ctx, Entry_IsNew2E9B78, {call(ctx, Profile_Item121B40, {src, k})});
            }
            single();
            notifyAll(dst);
        } else if (dst != 0) {
            notifyAll(dst);
        } else if (src != 0) {
            for (u32 k = 0; k < 4; ++k) call(ctx, Entry_IsNew2E9B78, {call(ctx, Profile_Item121B40, {src, k})});
            single();
        }
        call(ctx, Callbacks_Bind44BD74, {});
    }

    // P's saved spot (identity, zeros without bit 8).
    u32 st[12];
    for (u32 k = 0; k < 12; ++k) st[k] = load<u32>(kIdentity + MatOff(k));
    u8 sb[4] = {0, 0, 0, 0};
    u32 s68 = 0, s72 = 0;
    bool saved = false;
    if (P != 0 && (load<u32>(P + 192) & 0x8u) != 0) {
        const u32 s = P + 576;
        for (u32 k = 0; k < 12; ++k) {
            st[k] = load<u32>(s + MatOff(k));
            ctx.fpr[1 + k] = F(st[k]);
        }
        for (u32 k = 0; k < 4; ++k) sb[k] = load<u8>(s + 64 + k);
        s68 = load<u32>(s + 68);
        s72 = load<u32>(s + 72);
        saved = true;
    }
    if (P != 0) {
        VCall(ctx, P, 88, {P, 0, 1, 1, 1});
        VCall(ctx, P, 40, {P, 0});
        VCall(ctx, P, 40, {P, 1});
    }

    const u32 user = load<u8>(ub + 2);
    const u32 q = load<u32>(w + kUsers + 4 * user);
    bool reuse = false;
    if (q != 0) {
        const u32 o = load<u32>(q + 216);
        reuse = (o != 0 ? load<u32>(o + 52) : 0) == load<u32>(prof + 52);
    }
    u32 obj = q;
    if (!reuse) {
        // The spawn descriptor (stack +192, as the original's frame).
        const u32 name = static_cast<u32>(call(ctx, String_Format, {kFmtControllable, user + 1}));
        store<u32>(desc + 16, 0);
        store<u32>(desc + 0, kDescVt);
        for (u32 k = 0; k < 12; ++k) store<u32>(desc + 32 + MatOff(k), mw[k]);
        const u32 old = load<u32>(desc + 132);  // read before it is written (stack garbage)
        for (u32 k = 0; k < 3; ++k) store<u32>(desc + 96 + 4 * k, load<u32>(kDescVec3 + 4 * k));
        store<u32>(desc + 112, 0);
        store<u32>(desc + 116, 0);
        store<u32>(desc + 120, prof);
        store<u32>(desc + 124, load<u32>(prof + 36));
        const u32 w40 = load<u32>(prof + 40);
        store<u32>(desc + 128, w40);
        u32 fl = (old | 0x80000000u) & ~0x40000000u;
        store<u32>(desc + 132, fl);
        fl = (fl & ~0x20000000u) | (((static_cast<u32>(load<u8>(prof + 32)) >> 2) & 1u) << 29);
        store<u32>(desc + 136, 0);
        fl |= 0x1FE00000u;
        store<u32>(desc + 132, fl);
        store<u32>(desc + 4, 0);
        store<u32>(desc + 8, 0);
        store<u32>(desc + 12, 0);
        store<u8>(desc + 140, 0);
        ctx.fpr[1] = F(w40);
        ctx.fpr[2] = F(load<u32>(kDescVec3 + 4));
        ctx.fpr[3] = F(mw[2]);
        ctx.fpr[4] = F(mw[5]);
        ctx.fpr[5] = F(mw[6]);
        if (name != 0) call(ctx, String_Copy, {desc + 140, name, 40});
        u32 recs = load<u32>(prof + 136);
        if (recs != 0 && load<u16>(recs + 8) != 0) {
            for (u32 k = 0, off = 0;; ++k, off += 192) {
                const u32 n = recs != 0 ? load<u16>(recs + 8) : 0;
                if (k >= n) break;
                const u32 rec = load<u32>(recs) + off;
                u32 vp = load<u32>(desc + 16);
                if (vp == 0) {
                    vp = static_cast<u32>(call(ctx, SmallAlloc, {20}));
                    store<u32>(desc + 16, vp);
                    if (vp != 0) {
                        call(ctx, PtrVector_Construct, {vp});
                        store<u16>(vp + 6, 192);
                        vp = load<u32>(desc + 16);
                    }
                }
                call(ctx, Vector_Add733964, {vp, rec});
                recs = load<u32>(prof + 136);
            }
        }
        store<u32>(desc + 132, (load<u32>(desc + 132) & ~0x1FE00000u) | ((team << 21) & 0x1FE00000u));
        obj = static_cast<u32>(VCall(ctx, prof, 28, {prof, desc}));
        const u32 vp = load<u32>(desc + 16);
        store<u32>(desc + 0, kDescVt);  // the descriptor's destructor
        if (vp != 0) {
            call(ctx, PtrVector_Clear, {vp});
            call(ctx, Nop, {vp});
            call(ctx, SmallFree, {load<u32>(desc + 16), 20});
        }
    }

    if (obj != 0) {
        store<u32>(ub + 8, obj);
        store<u32>(w + kUsers + 4 * load<u8>(ub + 2), obj);
        const u32 n = load<u32>(ub + 8);
        store<u16>(n + 178, static_cast<u16>(load<u16>(n + 178) + 1));
        VCall(ctx, n, 44, {n});
        const u32 p2 = load<u32>(ub + 8);
        store<u64>(p2 + 184, load<u64>(p2 + 184) | 1u);  // before the null test (as the original)
        if (p2 != 0 && (load<u32>(p2 + 192) & 0x8u) != 0) {
            if (a8 != 0) {
                call(ctx, Obj_Release12BF70, {p2});
            } else {
                ctx.fpr[1] = F(load<u32>(kK + 12));
                call(ctx, Coach_FindSpline12C968, {p2, kK, 0x10000});
            }
            if (saved) {
                const u32 s = p2 + 576;
                store<u8>(s + 64, sb[0]);
                for (s32 k = 11; k >= 0; --k) store<u32>(s + MatOff(static_cast<u32>(k)), st[k]);
                store<u8>(s + 65, sb[1]);
                store<u8>(s + 66, sb[2]);
                store<u8>(s + 67, sb[3]);
                store<u32>(s + 68, s68);
                store<u32>(s + 72, s72);
            }
            const u32 nt = static_cast<u32>(call(ctx, Obj_Notify24, {p2}));
            if (nt != 0) {
                VCall(ctx, nt, 44, {nt, 1});
                call(ctx, Obj_UpdateFlags2336, {nt, 0});
            }
            const u32 a = static_cast<u32>(VCall(ctx, p2, 376, {p2}));
            if (a != 0) VCall(ctx, a, 44, {a, 1});
            const u32 b = load<u32>(p2 + 384);
            store<u32>(p2 + 380, b);
            if (b != 0) VCall(ctx, b, 44, {b, 1});
        }
        const u32 owner = load<u32>(load<u32>(ub + 8) + 216);
        const u32 u = load<u8>(ub + 2);
        const u32 nm = static_cast<u32>(VCall(ctx, owner, 8, {owner}));
        if (nm != 0 && load<u8>(nm) != 0) {
            call(ctx, HashedName_Set, {u, nm});
            if (a9 != 0) call(ctx, User0_RefreshTarget, {w, u});
        }
    }

    // Point the camera / the layer objects at the new player.
    const u32 p3 = load<u32>(ub + 8);
    if (p3 != 0 && load<u8>(p3 + 172) == 0) {
        const bool g8 = pf8 && (load<u32>(p3 + 192) & 0x8u) != 0;
        const bool g400 = f400 && (load<u32>(p3 + 200) & 0x400u) != 0;
        const bool g100 = pf100 && (load<u32>(p3 + 192) & 0x100u) != 0;
        static constexpr Case kA[] = {{1, Obj_SetRef1088E4}, {11, Target_SetRef0FEEBC}, {2, Obj_SetRefs108A34}};
        static constexpr Case kC[] = {{7, Obj_SetRef10DD60}, {9, Obj_SetRef11135C}, {1, Obj_SetRef1088E4},
                                      {11, Target_SetRef0FEEBC}, {2, Obj_SetRefs108A34}};
        if (a5 == 0) {
            call(ctx, World_SetCameraTarget, {w, ub});
        } else if (g8) {
            WalkLayers(ctx, w, ub, P, kA, 3, true, false);
        } else if (!g400) {
            if (!g100) call(ctx, World_SetCameraTarget, {w, ub});
            else WalkLayers(ctx, w, ub, P, kA, 3, false, true);
        } else if (coach != load<u32>(load<u32>(p3 + 216) + 240)) {
            call(ctx, World_SetCameraTarget, {w, ub});
        } else {
            WalkLayers(ctx, w, ub, P, kC, 5, false, false);
        }
    }

    if (P != 0) {
        if (a6 != 0) {
            for (u32 i = 0; i < load<u16>(V + 8); ++i) {
                const u32 o = load<u32>(load<u32>(V) + 4 * i);
                if (o == 0) continue;
                const u32 u = load<u32>(ub + 8);
                if (u != 0) VCall(ctx, o, 284, {o, u});
            }
        }
        if (a7 != 0) {
            const u32 u = load<u32>(ub + 8);
            const u32 r22 = (u != 0 && (load<u32>(u + 192) & 0x2u) != 0) ? u : 0;
            if (pf2 && r22 != 0) {
                const u32 w344 = load<u32>(r22 + 344);
                const f64 f1 = F(w344);
                const f32 prod = static_cast<f32>(F(ratio) * f1);
                const f64 f2 = prod;
                const bool lt = f2 < f1;
                const f64 mn = lt ? f2 : f1;
                ctx.fpr[2] = f2;
                if (mn > F(load<u32>(kK + 4))) {
                    store<u32>(r22 + 348, lt ? std::bit_cast<u32>(prod) : w344);
                    ctx.fpr[1] = mn;
                } else {
                    store<u32>(r22 + 348, load<u32>(kK + 4));
                    ctx.fpr[1] = f1;
                }
            }
        }
        const u32 slot = w + kUsers + 4 * load<u8>(ub + 2);
        if (P == load<u32>(slot)) store<u32>(slot, load<u32>(ub + 8));
    }
    call(ctx, PtrVector_Clear, {V});
    ret(ctx, call(ctx, Nop, {V}));
}

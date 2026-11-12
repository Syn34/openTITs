// Game code, large-function sweep part 27: the animation controller's
// script step (picks the controller's next state).
#include "big27.h"
#include "native/vec.h"

namespace sly4::obj {

namespace {
constexpr u32 kPool30E = 0x0030E1B0;  // .text pool: f32 0.0, pi / 180 (+4); vperm masks (+16, +32)
constexpr u32 kAnimOff = 0x01068C50;  // +72 u8: scripts off
constexpr u32 kFxName = 0xE9A7A1FBu;  // effect spawned by command 2
constexpr u32 IdentityMat = 0x01164BD0;
constexpr u32 GetU8 = 0x004A08C0;                   // (const u8*) -> *p
constexpr u32 Obj_Check = 0x001F465C;               // (obj) -> bool
constexpr u32 Anim_SetTarget = 0x0030DBF0;          // (o, x; f32 t, u8 force)
constexpr u32 AnimCtl_Play_0030BE48 = 0x0030BE48;   // (anim player, anim; f32, f32, const f32* params, const Mat44* at, u8) (name?)
constexpr u32 Effect_Spawn = 0x004A5FC8;            // (u32 name, owner, value, u8 start) -> Effect*
constexpr u32 Matrix_RotationY = 0x00553E18;        // (Mat34*; f1 angle)
constexpr u32 String_Assign = 0x00555F80;           // (String*, const char*)
constexpr u32 String_Length = 0x00555FA4;           // (const char*) -> u32

// A script command: vfn[12] its kind; +4 its argument, +8 / +12, +16..
// the rows a kind-2 command copies, +80 the frame mode, +84 / +88, u8 +100.
struct AnimCmd {
    pad<4> _0;
    be_u32 arg;        // +4
    be_f32 f8;         // +8
    be_f32 f12;        // +12
    be_u32 rows[14];   // +16 .. +68
    pad<4> _72;
    pad<4> _76;
    be_s32 mode;       // +80
    be_f32 f84;        // +84
    be_f32 f88;        // +88
    pad<8> _92;
    u8 b100;           // +100
};
static_assert(offsetof(AnimCmd, mode) == 80 && offsetof(AnimCmd, b100) == 100);
struct AnimScript {
    pad<36> _0;
    be_u32 cmds;    // +36: AnimCmd*[]
    pad<4> _40;
    be_u16 count;   // +44
};
// The anim player: +60 / +80 busy counts, +92 / +96 the current anim, +112.. rows.
struct AnimPlayer {
    pad<60> _0;
    be_u16 h60;        // +60
    pad<18> _62;
    be_u16 h80;        // +80
    pad<10> _82;
    be_u32 cur;        // +92
    be_u32 cur2;       // +96
    pad<12> _100;
    be_u32 rows[14];   // +112 .. +164 (as the command's +16 .. +68)
};
static_assert(offsetof(AnimPlayer, cur) == 92 && offsetof(AnimPlayer, rows) == 112);
// The state blocks c->+704 (+64 u8 wait, +68 the anim waited for, +72 the
// next state's value) and c->+708 (+80).
struct AnimNext {
    pad<64> _0;
    u8 wait;      // +64
    pad<3> _65;
    be_u32 h68;   // +68
    be_u32 w72;   // +72
    pad<4> _76;
    be_u32 w80;   // +80
};
// The text block c->+716: a char vector (+64 data, u16 +68 capacity, +72
// size, +74 high mark), +84, u8 +89, +92 a String.
struct AnimText {
    pad<64> _0;
    be_u32 data;   // +64
    be_u16 cap;    // +68
    pad<2> _70;
    be_u16 size;   // +72
    be_u16 mark;   // +74
    pad<8> _76;
    be_u32 w84;    // +84
    pad<1> _88;
    u8 b89;        // +89
    pad<2> _90;
    pad<4> str;    // +92
};
static_assert(offsetof(AnimText, size) == 72 && offsetof(AnimText, str) == 92);
struct AnimCtl {
    pad<216> _0;
    be_u32 owner;        // +216: its +344 the anim list (PtrVec, entries' +32 the hash)
    pad<336> _220;
    u8 b556;             // +556 (GetU8)
    pad<103> _557;
    be_u32 player;       // +660: AnimPlayer*
    pad<1> _664;
    u8 b665;             // +665
    pad<10> _666;
    u8 b676;             // +676
    pad<1> _677;
    u8 b678;             // +678
    pad<13> _679;
    be_u32 script;       // +692: AnimScript*
    be_s32 idx;          // +696
    pad<1> _700;
    u8 skip;             // +701
    pad<2> _702;
    be_u32 next;         // +704: AnimNext*
    be_u32 next2;        // +708: AnimNext*
    pad<4> _712;
    be_u32 text;         // +716: AnimText*
    pad<56> _720;
    be_u32 x776[3];      // +776
};
static_assert(offsetof(AnimCtl, b556) == 556 && offsetof(AnimCtl, player) == 660 && offsetof(AnimCtl, script) == 692);
static_assert(offsetof(AnimCtl, next) == 704 && offsetof(AnimCtl, text) == 716 && offsetof(AnimCtl, x776) == 776);
// The original's locals (sp offsets - 112).
struct AnimLocals {
    Vec4 off1;   // sp+112
    Vec4 off2;   // sp+128
    Mtx44 M;     // sp+144: the anim's frame
    Mtx44 R;     // sp+208
};
static_assert(offsetof(AnimLocals, M) == 32 && offsetof(AnimLocals, R) == 96);

AnimCmd* Cmd(const AnimCtl* c, s32 idx) {
    return host<AnimCmd>(gm<be_u32>(gm<AnimScript>(c->script).cmds + 4 * static_cast<u32>(idx)));
}
}  // namespace

// s32 f(Ctl* c) (called by 0030EBF0, AnimTask_Update3129E8), bracketed by
// c->vfn[32](c, 0) calls. 4 when GetU8(c+556) is set. Else 7 when the first
// set c->+776[i] (i < 3) passes Obj_Check and u8 c+665 is set. Else, with
// a script (c->+692: +36 command pointers, u16 +44 count), the result
// still -1 and kAnimOff+72 clear: from the index c->+696 (first, when u8
// c+701 is set, skipped forward to the next command of kind 7, wrapping to
// 0; the byte cleared) commands run until one ends the step or ten have
// run (then c->+704->+72 = 1.0, result 0); the index (wrapping at the
// count) goes back to c->+696. Command kinds (cmd->vfn[12]()):
// 7  Anim_SetTarget(c, cmd->+4, -, 1; cmd->+8); continues.
// 6  c->+704->+72 = cmd->+4; result 0, ends.
// 5  as 6, first c->+704 +64 = 1 / +68 = cmd->+4 when c->+660 (the anim
//    player) is busy (+92 set, u16 +60 > cmd->+4 or u16 +80 set).
// 3  with a player: the anim named cmd->+4 in c->+216's list (+344) plays
//    (0030BE48) at the frame of Game->+4216 (when its +192 bit 3, else at
//    0 + 96, quirk) moved by cmd->+80: 1 an offset (s * +84, 0, s * +88),
//    s = cmd->+8, through the frame; 2 rotated by +84 degrees about y
//    and offset by (0 * +88, 0 * +88, s * +88) in the rotated frame;
//    others as they are; continues.
// 2  with a player: busy -> c->+704 +64 = 1, +68 = 0, the index stays,
//    result 0, ends; else a set cmd->+4 goes to player +92 / +96, cmd's
//    +16.. rows to +112.., and the effect kFxName spawns; continues.
// 1  c->+708->+80 = cmd->+4; result 1, ends.
// 0  when u8 c+676 or c+678: cmd's string (+4) is appended to c->+716's
//    char vector (+64 data, u16 +68 capacity, +72 size, +74 high mark;
//    the size first cleared) and assigned to its +92 (+84 = 0, u8 +89 = 0);
//    result 3, ends.
// A result still -1 at the end becomes 0.
s32 Anim_PickState_0030E1E0(AnimCtl* c) {
    vcall(c, 32, 0u);
    s32 res = -1;
    if (call<u32>(GetU8, &c->b556) != 0) {
        res = 4;
    } else {
        for (u32 i = 0; i < 3; ++i) {
            const u32 x = c->x776[i];
            if (x == 0) continue;
            if (call<u32>(Obj_Check, x) != 0 && c->b665 != 0) res = 7;
            break;
        }
        const u32 scr = c->script;
        if (scr != 0 && gm<AnimScript>(scr).count != 0 && res == -1 && global<u8>(kAnimOff + 72) == 0) {
            GuestFrame frame(416);
            AnimLocals& L = *frame.as<AnimLocals>(112);
            s32 idx = c->idx;
            if (c->skip != 0) {
                u32 sc = scr;
                for (;;) {
                    if (idx == 0) break;
                    AnimCmd* cmd = host<AnimCmd>(gm<be_u32>(gm<AnimScript>(sc).cmds + 4 * static_cast<u32>(idx)));
                    if (vcall<s32>(cmd, 12) == 7) break;
                    sc = c->script;
                    ++idx;
                    if (!(idx < static_cast<s32>(gm<AnimScript>(sc).count))) {
                        idx = 0;
                        break;
                    }
                }
                c->skip = 0;
            }
            u32 steps = 0;
            bool done = false;
            do {
                AnimCmd* cmd = Cmd(c, idx);
                const s32 t = vcall<s32>(cmd, 12);
                if (t == 7) {
                    call(Anim_SetTarget, c, cmd->arg.get(), lf(cmd->f8), 1u);
                } else if (t == 6 || t == 5) {
                    if (const u32 p = c->player; t == 5 && p != 0) {
                        const u32 h = cmd->arg;
                        const AnimPlayer& pl = gm<AnimPlayer>(p);
                        if (pl.cur != 0u || pl.h60 > h || pl.h80 != 0) {
                            AnimNext& q = gm<AnimNext>(c->next);
                            q.wait = 1;
                            q.h68 = h;
                        }
                    }
                    gm<AnimNext>(c->next).w72 = cmd->arg.get();
                    res = 0;
                    done = true;
                } else if (t == 3) {
                    const u32 p = c->player;
                    if (p != 0) {
                        const PtrVec<u8>& vec = gm<PtrVec<u8>>(c->owner + 344);
                        const u32 h = cmd->arg;
                        const u32 n = vec.count;
                        u32 e = 0;
                        for (u32 k = 0; k < n; ++k) {
                            const u32 data = vec.data.ea;
                            if (gm<be_u32>(gm<be_u32>(data + 4 * k) + 32) == h) {
                                e = gm<be_u32>(data + 4 * k);
                                break;
                            }
                        }
                        if (e != 0) {
                            const u32 g5 = gm<be_u32>(global<be_u32>(kWorld) + 4216);
                            const u32 src = (g5 != 0 && (gm<be_u32>(g5 + 192) & 8) != 0) ? g5 : 0u;
                            Mtx44& M = L.M;
                            CopyMtxXYZ(M, gm<Mtx44>(src + 96));  // a null source reads address 96 (quirk)
                            const s32 mode = cmd->mode;
                            if (mode == 2) {
                                const f32 f31 = static_cast<f32>(Muls(lf(global<be_f32>(kPool30E)), lf(cmd->f88)));
                                const f32 f30 = static_cast<f32>(Muls(lf(cmd->f8), lf(cmd->f88)));
                                Mtx44& R = L.R;
                                CopyMtxXYZ(R, global<Mtx44>(IdentityMat));
                                call(Matrix_RotationY, &R, Muls(lf(cmd->f84), lf(global<be_f32>(kPool30E + 4))));
                                const V4 m0 = Lvx(&M.r[0]), m1 = Lvx(&M.r[1]), m2 = Lvx(&M.r[2]), m3 = Lvx(&M.r[3]);
                                V4 r[4];
                                for (u32 k = 0; k < 4; ++k) {
                                    const V4 x = Lvx(&R.r[k]);
                                    r[k] = Row3N(m0, m1, m2, x.e[0], x.e[1], x.e[2]);
                                }
                                r[3] = Add4N(m3, r[3]);
                                for (u32 k = 0; k < 4; ++k) Stvx(&R.r[k], r[k]);
                                // (M3.x, M3.y, M3.z, R'3.w) by the two vperms, plus the offset.
                                const V4 base{{m3.e[0], m3.e[1], m3.e[2], r[3].e[3]}};
                                Stvx(&L.off2, Add4N(base, Row3N(r[0], r[1], r[2], f31, f31, f30)));
                                for (u32 k = 0; k < 3; ++k) CopyXYZ(M.r[k], R.r[k]);
                                CopyXYZ(R.r[3], L.off2);
                                CopyXYZ(M.r[3], L.off2);
                            } else if (mode == 1) {
                                const f64 sc = lf(cmd->f8);
                                const f32 f3 = static_cast<f32>(Muls(sc, lf(cmd->f84)));
                                const f32 f2 = static_cast<f32>(Muls(sc, lf(cmd->f88)));
                                const f32 f1 = static_cast<f32>(Muls(lf(global<be_f32>(kPool30E)), lf(global<be_f32>(kPool30E))));
                                const V4 m0 = Lvx(&M.r[0]), m1 = Lvx(&M.r[1]), m2 = Lvx(&M.r[2]), m3 = Lvx(&M.r[3]);
                                Stvx(&L.off1, Add4N(m3, Row3N(m0, m1, m2, f3, f1, f2)));
                                CopyXYZ(M.r[3], L.off1);
                            }
                            call(AnimCtl_Play_0030BE48, p, e, lf(cmd->f8), lf(cmd->f12), &cmd->rows[0], &M, static_cast<u32>(cmd->b100));
                        }
                    }
                } else if (t == 2) {
                    if (const u32 p = c->player; p != 0) {
                        AnimPlayer& pl = gm<AnimPlayer>(p);
                        if (pl.cur != 0u || pl.h60 != 0 || pl.h80 != 0) {
                            AnimNext& q = gm<AnimNext>(c->next);
                            done = true;
                            res = 0;
                            --idx;  // the same command again next time
                            q.wait = 1;
                            q.h68 = 0u;
                        } else if (const u32 h = cmd->arg; h != 0) {
                            pl.cur = h;
                            pl.cur2 = h;
                            static constexpr u32 kRows[12] = {0, 1, 2, 4, 5, 6, 8, 9, 10, 11, 12, 13};  // +16.. words (+28 / +44 skipped)
                            for (u32 k : kRows) pl.rows[k] = cmd->rows[k].get();
                            call(Effect_Spawn, kFxName, 0u, 0u, 1u);
                        }
                    }
                } else if (t == 1) {
                    AnimNext& q = gm<AnimNext>(c->next2);
                    res = 1;
                    q.w80 = cmd->arg.get();
                    done = true;
                } else if (t == 0) {
                    if (c->b676 != 0 || c->b678 != 0) {
                        AnimText& s = gm<AnimText>(c->text);
                        const u32 str = guest(&cmd->arg);
                        res = 3;
                        s.size = 0;
                        const u32 len = call<u32>(String_Length, str);
                        u32 r4 = s.size, r3 = r4;  // the size as the original keeps it in registers
                        for (u32 k = 0; k < len; ++k) {
                            if (const u32 cap = s.cap; (r3 & 0xFFFF) == cap) {
                                call(fn::PtrVector_Reserve, guest(&s) + 64, (cap << 1) & 0xFFFFFFFEu);
                                r4 = s.size;
                            }
                            gm<u8>(s.data + (r4 & 0xFFFF)) = gm<u8>(str + k);
                            r4 = s.size + 1u;
                            u32 mx = s.mark;
                            s.size = static_cast<u16>(r4);
                            r3 = r4 & 0xFFFF;
                            if (!(static_cast<s32>(mx) > static_cast<s32>(r3))) mx = r3 = r4;
                            s.mark = static_cast<u16>(mx);
                        }
                        call(String_Assign, &s.str, str);
                        s.w84 = 0u;
                        s.b89 = 0;
                        done = true;
                    }
                }
                ++idx;
                if (!(idx < static_cast<s32>(gm<AnimScript>(c->script).count))) idx = 0;
                if (++steps >= 10) {
                    AnimNext& q = gm<AnimNext>(c->next);
                    res = 0;
                    done = true;
                    q.w72 = 0x3F800000u;  // 1.0
                }
            } while (!done);
            c->idx = idx;
        }
        if (res == -1) res = 0;
    }
    vcall(c, 32, 0u);
    return res;
}

}  // namespace sly4::obj

NATIVE(0030E1E0, sly4::obj::Anim_PickState_0030E1E0)

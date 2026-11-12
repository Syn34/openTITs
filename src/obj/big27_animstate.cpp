// Game code, large-function sweep part 27: the animation controller's
// script step (picks the controller's next state).
#include "../sly4.h"
#include "big27.h"

using namespace sly4;
using namespace sly4::big27;

namespace {
constexpr u32 kPool30E = 0x0030E1B0;  // .text pool: f32 0.0, pi / 180 (+4); vperm masks (+16, +32)
constexpr u32 kAnimOff = 0x01068C50;  // +72 u8: scripts off
constexpr u32 kFxName = 0xE9A7A1FBu;  // effect spawned by command 2
constexpr u32 kRow[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
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
DECOMP(0030E1E0, Anim_PickState_0030E1E0) {
    const u32 c = arg<u32>(ctx, 0);
    VCall(ctx, c, 32, {c, 0});
    s32 res = -1;
    if (static_cast<u32>(call(ctx, b27fn::GetU8, {c + 556})) != 0) {
        res = 4;
    } else {
        for (u32 i = 0; i < 3; ++i) {
            const u32 x = load<u32>(c + 776 + 4 * i);
            if (x == 0) continue;
            if (static_cast<u32>(call(ctx, b27fn::Obj_Check, {x})) != 0 && load<u8>(c + 665) != 0) res = 7;
            break;
        }
        const u32 scr = load<u32>(c + 692);
        if (scr != 0 && load<u16>(scr + 44) != 0 && res == -1 && load<u8>(kAnimOff + 72) == 0) {
            Stack frame(ctx, 416);
            const u32 sp = frame.addr();
            const u32 M = sp + 144;  // the anim's frame
            s32 idx = load<s32>(c + 696);
            if (load<u8>(c + 701) != 0) {
                u32 sc = scr;
                for (;;) {
                    if (idx == 0) break;
                    const u32 cmd = load<u32>(load<u32>(sc + 36) + 4 * static_cast<u32>(idx));
                    if (static_cast<s32>(VCall(ctx, cmd, 12, {cmd})) == 7) break;
                    sc = load<u32>(c + 692);
                    ++idx;
                    if (!(idx < static_cast<s32>(load<u16>(sc + 44)))) {
                        idx = 0;
                        break;
                    }
                }
                store<u8>(c + 701, 0);
            }
            u32 steps = 0;
            bool done = false;
            do {
                const u32 cmd = load<u32>(load<u32>(load<u32>(c + 692) + 36) + 4 * static_cast<u32>(idx));
                const s32 t = static_cast<s32>(VCall(ctx, cmd, 12, {cmd}));
                if (t == 7) {
                    // r5 is what the command's vfn[12] left (unread).
                    callf(ctx, b27fn::Anim_SetTarget, {c, load<u32>(cmd + 4), ctx.gpr[5], 1}, {Lf(cmd + 8)});
                } else if (t == 6 || t == 5) {
                    if (const u32 p = load<u32>(c + 660); t == 5 && p != 0) {
                        const u32 h = load<u32>(cmd + 4);
                        if (load<u32>(p + 92) != 0 || load<u16>(p + 60) > h || load<u16>(p + 80) != 0) {
                            const u32 q = load<u32>(c + 704);
                            store<u8>(q + 64, 1);
                            store<u32>(q + 68, h);
                        }
                    }
                    store<u32>(load<u32>(c + 704) + 72, load<u32>(cmd + 4));
                    res = 0;
                    done = true;
                } else if (t == 3) {
                    const u32 p = load<u32>(c + 660);
                    if (p != 0) {
                        const u32 vec = load<u32>(c + 216) + 344;
                        const u32 h = load<u32>(cmd + 4);
                        const u32 n = load<u16>(vec + 8);
                        u32 e = 0;
                        for (u32 k = 0; k < n; ++k) {
                            const u32 data = load<u32>(vec);
                            if (load<u32>(load<u32>(data + 4 * k) + 32) == h) {
                                e = load<u32>(data + 4 * k);
                                break;
                            }
                        }
                        if (e != 0) {
                            const u32 g5 = load<u32>(load<u32>(b27fn::Game) + 4216);
                            const u32 src = (g5 != 0 && (load<u32>(g5 + 192) & 8) != 0) ? g5 : 0;
                            for (u32 k = 0; k < 12; ++k) store<u32>(M + kRow[k], load<u32>(src + 96 + kRow[k]));
                            const s32 mode = load<s32>(cmd + 80);
                            // r5 / r6: what the original leaves there (unread).
                            u32 r5 = kPool30E, r6 = g5 != 0 ? (load<u32>(g5 + 192) & 8) : vec;
                            if (mode == 2) {
                                const f32 f31 = static_cast<f32>(Muls(Lf(kPool30E), Lf(cmd + 88)));
                                const f32 f30 = static_cast<f32>(Muls(Lf(cmd + 8), Lf(cmd + 88)));
                                const u32 R = sp + 208;
                                for (u32 k = 0; k < 12; ++k) store<u32>(R + kRow[k], load<u32>(b27fn::IdentityMat + kRow[k]));
                                callf(ctx, b27fn::Matrix_RotationY, {R}, {Muls(Lf(cmd + 84), Lf(kPool30E + 4))});
                                const V4 m0 = Lvx(M), m1 = Lvx(M + 16), m2 = Lvx(M + 32), m3 = Lvx(M + 48);
                                V4 r[4];
                                for (u32 k = 0; k < 4; ++k) {
                                    const V4 x = Lvx(R + 16 * k);
                                    r[k] = Row3N26(m0, m1, m2, x.e[0], x.e[1], x.e[2]);
                                }
                                r[3] = Add4N26(m3, r[3]);
                                for (u32 k = 0; k < 4; ++k) Stvx(R + 16 * k, r[k]);
                                // (M3.x, M3.y, M3.z, R'3.w) by the two vperms, plus the offset.
                                const V4 base{{m3.e[0], m3.e[1], m3.e[2], r[3].e[3]}};
                                Stvx(sp + 128, Add4N26(base, Row3N26(r[0], r[1], r[2], f31, f31, f30)));
                                for (u32 k = 0; k < 9; ++k) store<u32>(M + kRow[k], load<u32>(R + kRow[k]));
                                for (u32 k = 0; k < 3; ++k) store<u32>(R + 48 + 4 * k, load<u32>(sp + 128 + 4 * k));
                                for (u32 k = 0; k < 3; ++k) store<u32>(M + 48 + 4 * k, load<u32>(sp + 128 + 4 * k));
                                r5 = 200;
                                r6 = 16;
                            } else if (mode == 1) {
                                const f64 s = Lf(cmd + 8);
                                const f32 f3 = static_cast<f32>(Muls(s, Lf(cmd + 84)));
                                const f32 f2 = static_cast<f32>(Muls(s, Lf(cmd + 88)));
                                const f32 f1 = static_cast<f32>(Muls(Lf(kPool30E), Lf(kPool30E)));
                                const V4 m0 = Lvx(M), m1 = Lvx(M + 16), m2 = Lvx(M + 32), m3 = Lvx(M + 48);
                                Stvx(sp + 112, Add4N26(m3, Row3N26(m0, m1, m2, f3, f1, f2)));
                                for (u32 k = 0; k < 3; ++k) store<u32>(M + 48 + 4 * k, load<u32>(sp + 112 + 4 * k));
                                r5 = 160;
                                r6 = 176;
                            }
                            callf(ctx, b27fn::AnimCtl_Play_0030BE48, {p, e, r5, r6, cmd + 16, M, load<u8>(cmd + 100)},
                                  {Lf(cmd + 8), Lf(cmd + 12)});
                        }
                    }
                } else if (t == 2) {
                    if (const u32 p = load<u32>(c + 660); p != 0) {
                        if (load<u32>(p + 92) != 0 || load<u16>(p + 60) != 0 || load<u16>(p + 80) != 0) {
                            const u32 q = load<u32>(c + 704);
                            done = true;
                            res = 0;
                            --idx;  // the same command again next time
                            store<u8>(q + 64, 1);
                            store<u32>(q + 68, 0);
                        } else if (const u32 h = load<u32>(cmd + 4); h != 0) {
                            store<u32>(p + 92, h);
                            store<u32>(p + 96, h);
                            static constexpr u32 kSrc[12] = {16, 20, 24, 32, 36, 40, 48, 52, 56, 60, 64, 68};
                            static constexpr u32 kDst[12] = {112, 116, 120, 128, 132, 136, 144, 148, 152, 156, 160, 164};
                            for (u32 k = 0; k < 12; ++k) store<u32>(p + kDst[k], load<u32>(cmd + kSrc[k]));
                            SetF1(ctx, cmd + 68);  // leftover f1 (unread)
                            call(ctx, b27fn::Effect_Spawn, {kFxName, 0, 0, 1});
                        }
                    }
                } else if (t == 1) {
                    const u32 q = load<u32>(c + 708);
                    res = 1;
                    store<u32>(q + 80, load<u32>(cmd + 4));
                    done = true;
                } else if (t == 0) {
                    if (load<u8>(c + 676) != 0 || load<u8>(c + 678) != 0) {
                        const u32 s = load<u32>(c + 716);
                        const u32 str = cmd + 4;
                        res = 3;
                        store<u16>(s + 72, 0);
                        const u32 len = static_cast<u32>(call(ctx, b27fn::String_Length, {str}));
                        u32 r4 = load<u16>(s + 72), r3 = r4;  // the size as the original keeps it in registers
                        for (u32 k = 0; k < len; ++k) {
                            if (const u32 cap = load<u16>(s + 68); (r3 & 0xFFFF) == cap) {
                                call(ctx, b27fn::PtrVector_Reserve, {s + 64, (cap << 1) & 0xFFFFFFFEu});
                                r4 = load<u16>(s + 72);
                            }
                            store<u8>(load<u32>(s + 64) + (r4 & 0xFFFF), load<u8>(str + k));
                            r4 = load<u16>(s + 72) + 1;
                            u32 mx = load<u16>(s + 74);
                            store<u16>(s + 72, static_cast<u16>(r4));
                            r3 = r4 & 0xFFFF;
                            if (!(static_cast<s32>(mx) > static_cast<s32>(r3))) mx = r3 = r4;
                            store<u16>(s + 74, static_cast<u16>(mx));
                        }
                        call(ctx, b27fn::String_Assign, {s + 92, str});
                        store<u32>(s + 84, 0);
                        store<u8>(s + 89, 0);
                        done = true;
                    }
                }
                ++idx;
                if (!(idx < static_cast<s32>(load<u16>(load<u32>(c + 692) + 44)))) idx = 0;
                if (++steps >= 10) {
                    const u32 q = load<u32>(c + 704);
                    res = 0;
                    done = true;
                    store<u32>(q + 72, 0x3F800000u);  // 1.0
                }
            } while (!done);
            store<u32>(c + 696, static_cast<u32>(idx));
        }
        if (res == -1) res = 0;
    }
    VCall(ctx, c, 32, {c, 0});
    ret(ctx, static_cast<u32>(res));
}

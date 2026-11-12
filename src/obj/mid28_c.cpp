// Game code, medium sweep part 28: AnimScript commands that switch a
// target's component (enable / visible).
#include "mid28.h"

using namespace sly4;
using namespace sly4::mid28;

namespace {
using namespace m28fn;
constexpr u32 kFindComp = 0x004BB4B4u;   // Effect_FindComponent(e, hash)
constexpr u32 kFindById = 0x000D6A28u;   // Obj_FindById(id)
constexpr u32 kFindByHash = 0x000DB290u; // Obj_FindByHash_000DB290(o, hash)
constexpr u32 kFindAfter = 0x000D92B0u;  // List_FindAfter_000D92B0(o, key, after)
constexpr u32 kHashStr = 0x0056A6E0u;    // Hash_ToString(h)
constexpr u32 kFormat = 0x005534D0u;     // String_Format(fmt, ...)
constexpr u32 kW28 = 0x00F65CA0u;        // the world pointer

inline u32 Rotl28c(u32 v, u32 n) { return (v << n) | (v >> (32 - n)); }

// The command's "FromTarget" component path: the kind-5 component c->+12
// of the effect x (Effect_FindComponent, vt[32]() == 5) whose record +28
// is a 0x02235C33 target gives its +280.
u32 CompTarget28(PPUContext& ctx, u32 c, u32 x, u32 keyOff = 12) {
    const u32 key = load<u32>(c + keyOff);
    if (key == 0) return 0;
    const u32 e = static_cast<u32>(call(ctx, kFindComp, {x, key}));
    if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return 0;
    const u32 q = load<u32>(e + 28);
    if (q == 0) return 0;
    ctx.gpr[3] = 0x02235C33u;  // the compare constant is built in r3
    return load<u32>(q + 276) == 0x02235C33u ? load<u32>(q + 280) : 0;
}

// The target t, else Obj_FindById(c->+16) when set (r3 = 0 when neither).
u32 ById28(PPUContext& ctx, u32 c, u32 t, u32 idOff = 16) {
    if (t != 0) return t;
    const u32 id = load<u32>(c + idOff);
    if (id == 0) {
        ctx.gpr[3] = 0;
        return 0;
    }
    return static_cast<u32>(call(ctx, kFindById, {id}));
}

// The target narrowed to its child c->+20 (Obj_FindByHash_000DB290) when
// that is set; 0 when none.
u32 Narrow28(PPUContext& ctx, u32 c, u32 t, u32 hOff = 20) {
    const u32 h = load<u32>(c + hOff);
    return h != 0 ? static_cast<u32>(call(ctx, kFindByHash, {t, h})) : t;
}
// CompTarget28, else Obj_FindById(c->+16), the variant that formats the
// key of each failed step with Hash_ToString (results unused): c->+12
// once when the component path fails, c->+16 once when the id is 0 and
// twice when Obj_FindById fails. 0 when none (r3 = the last Hash_ToString).
u32 TargetDbg28(PPUContext& ctx, u32 c, u32 x) {
    u32 t = 0;
    if (const u32 key = load<u32>(c + 12); key != 0) {
        const u32 e = static_cast<u32>(call(ctx, kFindComp, {x, key}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 5) {
            if (const u32 q = load<u32>(e + 28); q != 0 && load<u32>(q + 276) == 0x02235C33u) t = load<u32>(q + 280);
        }
        if (t == 0) call(ctx, kHashStr, {load<u32>(c + 12)});
    }
    if (t == 0) {
        const u32 id = load<u32>(c + 16);
        if (id != 0) t = static_cast<u32>(call(ctx, kFindById, {id}));
        if (t == 0) {
            if (id != 0) call(ctx, kHashStr, {load<u32>(c + 16)});
            call(ctx, kHashStr, {load<u32>(c + 16)});
        }
    }
    return t;
}
// The holder / target pair of the record commands, fields from c + b:
// holder = the player slot word *0x00F65CA0 + (s32) c->+b * 0x20C80 +
// 4216 when c->+b >= 0 and it is set, else CompTarget28 with key c->+(b +
// 4), else Obj_FindById(c->+(b + 8)); n = Obj_Notify24(holder); target =
// CompTarget28 with key c->+(b + 12), else Obj_FindById(c->+(b + 16)). A
// missing side formats its two keys with Hash_ToString unless u8
// c->+quiet is set (r3 as the original leaves it) and returns false.
bool HolderTarget28(PPUContext& ctx, u32 c, u32 x, u32 b, u32 quiet, u32& n, u32& t) {
    auto miss = [&](u32 k1, u32 k2) {
        const u32 q = load<u8>(c + quiet);
        ctx.gpr[3] = q;
        if (q != 0) return;
        if (const u32 a = load<u32>(c + k1); a != 0) call(ctx, kHashStr, {a});
        else ctx.gpr[3] = 0;
        if (const u32 v = load<u32>(c + k2); v != 0) call(ctx, kHashStr, {v});
    };
    const s32 slot = static_cast<s32>(load<u32>(c + b));
    u32 h = 0;
    if (slot >= 0) h = load<u32>(load<u32>(kW28) + static_cast<u32>(slot) * 0x00020C80u + 4208 + 8);
    if (h == 0) h = ById28(ctx, c, CompTarget28(ctx, c, x, b + 4), b + 8);
    n = 0;
    if (h != 0) n = static_cast<u32>(call(ctx, 0x000D949Cu, {h}));  // Obj_Notify24
    if (n == 0) {
        miss(b + 4, b + 8);
        return false;
    }
    t = ById28(ctx, c, CompTarget28(ctx, c, x, b + 12), b + 16);
    if (t == 0) {
        miss(b + 12, b + 16);
        return false;
    }
    return true;
}
}  // namespace

// u32 f(Cmd* c, Effect* x) (AnimScriptCommandEnableComponent): the target
// (CompTarget28, else Obj_FindById(c->+16)), narrowed by c->+20, finds
// its component c->+24 (List_FindAfter_000D92B0); with u8 c->+28 == 0 it
// gets vt[44](comp, 0), else vt[44](comp, c->+28) when its u8 +16 is
// clear. Not found: String_Format of the failure message with the
// target's vt[76]() name and Hash_ToString(c->+24). r3 as the original
// leaves it.
DECOMP(00072974, AnimCmd_EnableComponent_00072974) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 t = ById28(ctx, c, CompTarget28(ctx, c, x));
    if (t == 0) return;
    t = Narrow28(ctx, c, t);
    if (t == 0) return;
    const u32 comp = static_cast<u32>(call(ctx, kFindAfter, {t, load<u32>(c + 24), 0}));
    if (comp == 0) {
        const u32 name = static_cast<u32>(VCall(ctx, t, 76, {t}));
        const u32 hs = static_cast<u32>(call(ctx, kHashStr, {load<u32>(c + 24)}));
        call(ctx, kFormat, {0x00AC4A34u, name, hs});
        return;
    }
    const u32 b = load<u8>(c + 28);
    if (load<u8>(comp + 16) == 0 && b != 0) {
        VCall(ctx, comp, 44, {comp, b});
    } else if (b == 0) {
        VCall(ctx, comp, 44, {comp, 0});
    }
}

// u32 f(Cmd* c, Effect* x) (AnimScriptCommandSetComponentVisible): the
// target (c->+24 != -1: the player slot word at *0x00F65CA0 + c->+24 *
// 0x20C80 + 4216; else CompTarget28), else
// Obj_FindById(c->+16), narrowed by c->+20, finds its component c->+28
// and calls its vt[52](comp, u8 c->+32, 1). Not found: String_Format of
// the failure message with Hash_ToString(c->+28), the def name (+216 ->
// +56) and the target's vt[76]() name. r3 as the original leaves it.
DECOMP(00088494, AnimCmd_SetComponentVisible_00088494) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const s32 slot = static_cast<s32>(load<u32>(c + 24));
    u32 t;
    if (slot != -1) t = load<u32>(load<u32>(kW28) + static_cast<u32>(slot) * 0x00020C80u + 4208 + 8);
    else t = CompTarget28(ctx, c, x);
    t = ById28(ctx, c, t);
    if (t == 0) return;
    t = Narrow28(ctx, c, t);
    if (t == 0) return;
    const u32 comp = static_cast<u32>(call(ctx, kFindAfter, {t, load<u32>(c + 28), 0}));
    if (comp == 0) {
        const u32 hs = static_cast<u32>(call(ctx, kHashStr, {load<u32>(c + 28)}));
        const u32 def = load<u32>(t + 216) + 56;
        const u32 name = static_cast<u32>(VCall(ctx, t, 76, {t}));
        call(ctx, kFormat, {0x00AC50DCu, hs, def, name});
        return;
    }
    const u32 opd = load<u32>(load<u32>(comp) + 52);
    // r6 = the entry and r7 = the OPD, as the original call sequence leaves them.
    call_ptr(ctx, opd, {comp, load<u8>(c + 32), 1, load<u32>(opd), opd});
}

// u32 f(Cmd* c, Effect* x): with a name at c->+12 the object named so
// (Obj_FindName_0034654C(*(world +0x1281D4), name)) leaves (u8 c->+92 ==
// 0: Hk_RemoveEntity92FFC8(&res, its +8 entity, it) when in) or joins
// (Hk_AddEntity92F2A8(world physics (+0x1281D4)->+4, it, 1) when out) the
// physics world. Else the target (CompTarget28 with key c->+80, else
// Obj_FindById(c->+80)), narrowed by c->+84, gets vt[92](t, u8 c->+92 ==
// 0, (s32) c->+88). r3 as the original leaves it.
DECOMP(000725E0, AnimCmd_SetPhysics_000725E0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (load<u8>(c + 12) != 0) {
        const u32 mgr = load<u32>(load<u32>(kW28) + 0x001281D4u);
        const u32 phys = load<u32>(mgr + 4);
        const u32 r = static_cast<u32>(call(ctx, 0x0034654Cu, {mgr, c + 12}));  // Obj_FindName_0034654C
        if (r == 0) return;
        const u32 b = load<u8>(c + 92);
        if (b == 0) {
            if (const u32 ent = load<u32>(r + 8); ent != 0) {
                Stack frame(ctx, 32);  // the result at 112(r1)
                call(ctx, 0x0092FFC8u, {frame.addr(), ent, r});  // Hk_RemoveEntity92FFC8
            }
            return;
        }
        if (load<u32>(r + 8) == 0 && phys != 0) call(ctx, 0x0092F2A8u, {phys, r, 1});  // Hk_AddEntity92F2A8
        return;
    }
    u32 t = ById28(ctx, c, CompTarget28(ctx, c, x, 80), 80);
    if (t == 0) return;
    t = Narrow28(ctx, c, t, 84);
    if (t == 0) return;
    const u32 opd = load<u32>(load<u32>(t) + 92);
    const u32 on = load<u8>(c + 92) == 0;
    const u64 v = static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(c + 88))));
    // r6 = the entry and r7 = the OPD, as the original call sequence leaves them.
    call_ptr(ctx, opd, {t, on, v, load<u32>(opd), opd});
}

// u32 f(Cmd* c, Effect* x): the target (CompTarget28, else
// Obj_FindById(c->+16); each miss also formats its key with
// Hash_ToString, result unused) picks its entry c->+20
// (List_FindAfter_000D92B0, or 0x000D1288(t, 15, 0) when 0) and copies
// the fields selected by the mask c->+24: bit 0x1 the top bit of +32 from
// u8 c->+28 (and bit 30 from u8 c->+96 at the end), 0x2 bits 0x3FC00000
// from c->+32, 0x4..0x20 the words +36..+48, 0x40 / 0x80 the vectors +64
// / +80. r3 / f1 as the original leaves them.
DECOMP(00088ABC, AnimCmd_SetDisplay_00088ABC) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 t = TargetDbg28(ctx, c, x);
    if (t == 0) return;
    if (const u32 h = load<u32>(c + 20); h != 0) t = static_cast<u32>(call(ctx, kFindAfter, {t, h, 0}));
    else t = static_cast<u32>(call(ctx, 0x000D1288u, {t, 15, 0}));
    if (t == 0) {
        call(ctx, kHashStr, {load<u32>(c + 20)});
        return;
    }
    u32 f = load<u32>(c + 24);
    if ((f & 1u) != 0) {
        store<u32>(t + 32, (load<u32>(t + 32) & 0x7FFFFFFFu) | ((load<u8>(c + 28) & 1u) << 31));
        f = load<u32>(c + 24);
    }
    if ((f & 2u) != 0) {
        const u32 w = load<u32>(c + 32);
        store<u32>(t + 32, (load<u32>(t + 32) & ~0x3FC00000u) | (Rotl28c(w, 22) & 0x3FC00000u));
        f = load<u32>(c + 24);
    }
    for (u32 k = 0; k < 4; ++k)
        if ((f & (4u << k)) != 0) {
            ctx.fpr[1] = Lf(c + 36 + 4 * k);
            store<u32>(t + 36 + 4 * k, load<u32>(c + 36 + 4 * k));
        }
    for (u32 k = 0; k < 2; ++k)
        if ((f & (0x40u << k)) != 0) {
            const u32 s = c + 64 + 16 * k, d = t + 64 + 16 * k;
            const u32 a = load<u32>(s), b = load<u32>(s + 4), z = load<u32>(s + 8);
            ctx.fpr[1] = Lf(s);
            store<u32>(d, a);
            store<u32>(d + 4, b);
            store<u32>(d + 8, z);
        }
    if ((f & 1u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 w = (load<u32>(t + 32) & ~0x40000000u) | ((load<u8>(c + 96) & 1u) << 30);
    store<u32>(t + 32, w);
    ctx.gpr[3] = w;
}

// u32 f(Cmd* c, Effect* x, Ctx* s): the target (CompTarget28, else
// Obj_FindById(c->+16); a miss formats the key with Hash_ToString and
// returns, quirk: with neither key nor id it goes on with a null target)
// gets the value c->+32, blended toward c->+28 by s->+32 / range when
// s->+32 < range (range = c->+24, or the kind-1 component c->+20's
// vt[20]() float, used when >= 1e-5): its animator vt[60]() gets
// Obj_AnimCall3541E0_00276E4C(a, -, c->+36; v), else the object itself
// Node_SetSignRec_000DBD7C(t, -, c->+36; v). r3 / f1 as the original
// leaves them.
DECOMP(0006C370, AnimCmd_BlendSign_0006C370) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 s = arg<u32>(ctx, 2);
    u32 t = CompTarget28(ctx, c, x);
    if (t == 0) {
        if (const u32 id = load<u32>(c + 16); id != 0) {
            t = static_cast<u32>(call(ctx, kFindById, {id}));
            if (t == 0) {
                call(ctx, kHashStr, {load<u32>(c + 16)});
                return;
            }
        } else if (const u32 key = load<u32>(c + 12); key != 0) {
            call(ctx, kHashStr, {key});
            return;
        }
    }
    f64 range = Lf(c + 24);
    if (const u32 k2 = load<u32>(c + 20); k2 != 0) {
        const u32 e = static_cast<u32>(call(ctx, kFindComp, {x, k2}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 1) {
            VCall(ctx, e, 20, {e});
            range = ctx.fpr[1];
        }
    }
    ctx.fpr[1] = Lf(0x0006C368u);
    f64 v = Lf(c + 32);
    if (range > ctx.fpr[1] || range == ctx.fpr[1]) {
        ctx.fpr[1] = Lf(s + 32);
        if (ctx.fpr[1] < range) {
            const f64 q = Sgl28(ctx.fpr[1] / range);
            ctx.fpr[1] = q;
            const f64 lo = Lf(c + 28);
            v = Sgl28(std::fma(Sgl28(v - lo), q, lo));
        }
    }
    const u32 a = static_cast<u32>(VCall(ctx, t, 60, {t}));
    // r4 is unread by both callees (left as the vt[60] call leaves it).
    if (a != 0) callf(ctx, 0x00276E4Cu, {a, ctx.gpr[4], load<u32>(c + 36)}, {v});  // Obj_AnimCall3541E0_00276E4C
    else callf(ctx, 0x000DBD7Cu, {t, ctx.gpr[4], load<u32>(c + 36)}, {v});  // Node_SetSignRec_000DBD7C
}

// u32 f(Cmd* c, Effect* x): the target (TargetDbg28) must have flag 0x8
// at +192 (else Hash_ToString(c->+16)); then by the field hash c->+20:
// 0x76A67631 / 0x66B98D77 / 0x09AED71D set the settings record
// (CallK2_000D95C4(t)) +240 vector = c->+32, +256 = c->+48, u8 +260 =
// u8 c->+32; 0xC15C9D78, when u8 +724 has 0x80 but not 0x40, +184 bit
// 0x8000 is clear and 0x000D1288(t, 45, 0) is set, calls vt[28](t, 0, 6)
// and puts u8 c->+32 into the top bit of *(t->+20). r3 / f1 as the
// original leaves them.
DECOMP(0006FA40, AnimCmd_SetSetting_0006FA40) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 t = TargetDbg28(ctx, c, x);
    if (t == 0) return;
    if ((load<u32>(t + 192) & 8u) == 0) {
        call(ctx, kHashStr, {load<u32>(c + 16)});
        return;
    }
    const u32 sel = load<u32>(c + 20);
    if (sel == 0x76A67631u || sel == 0x66B98D77u || sel == 0x09AED71Du) {
        const u32 p = static_cast<u32>(call(ctx, 0x000D95C4u, {t}));  // CallK2_000D95C4
        if (p == 0) return;
        if (sel == 0x76A67631u) {
            const u32 a = load<u32>(c + 32), b = load<u32>(c + 36), z = load<u32>(c + 40);
            ctx.fpr[1] = Lf(c + 32);
            store<u32>(p + 240, a);
            store<u32>(p + 244, b);
            store<u32>(p + 248, z);
        } else if (sel == 0x66B98D77u) {
            ctx.fpr[1] = Lf(c + 48);
            store<u32>(p + 256, load<u32>(c + 48));
        } else {
            const u32 v = load<u8>(c + 32);
            store<u8>(p + 260, static_cast<u8>(v));
            ctx.gpr[3] = v;
        }
        return;
    }
    if (sel != 0xC15C9D78u) {
        ctx.gpr[3] = sel;
        return;
    }
    const u32 w20 = load<u32>(t + 20);
    const u32 b = load<u8>(t + 724);
    if ((b & 0x80u) == 0) {
        ctx.gpr[3] = b;
        return;
    }
    if ((b & 0x40u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if ((load<u32>(t + 188) & 0x8000u) != 0) {
        ctx.gpr[3] = 0x8000u;
        return;
    }
    if (call(ctx, 0x000D1288u, {t, 45, 0}) == 0) return;
    VCall(ctx, t, 28, {t, 0, 6});
    const u32 w = (load<u32>(w20) & 0x7FFFFFFFu) | ((load<u8>(c + 32) & 1u) << 31);
    store<u32>(w20, w);
    ctx.gpr[3] = w;
}

// u32 f(Cmd* c, Effect* x): the holder (c->+12 >= 0: the player slot
// word *0x00F65CA0 + c->+12 * 0x20C80 + 4216, when set; else
// CompTarget28 with key c->+16, else Obj_FindById(c->+20)) and its
// notifier Obj_Notify24(h); the record of the target (CompTarget28 with
// key c->+24, else Obj_FindById(c->+28)) is removed from it
// (Records_RemoveKey_0029F7A0(n, t)). A missing side formats its keys
// with Hash_ToString unless u8 c->+32 is set. r3 as the original leaves
// it.
DECOMP(00085024, AnimCmd_ForgetRecord_00085024) {
    u32 n, t;
    if (!HolderTarget28(ctx, arg<u32>(ctx, 0), arg<u32>(ctx, 1), 12, 32, n, t)) return;
    call(ctx, 0x0029F7A0u, {n, t});  // Records_RemoveKey_0029F7A0
}

// u32 f(Cmd* c, Effect* x): like AnimCmd_ForgetRecord_00085024 with the
// fields at +32..+52 (quiet byte +62), but a new record of the holder's
// notifier (Rec128_Alloc_0029F760(n), none: r3 = 0) takes the target
// (+8), the f32s c->+16 / +20 / +24 (+16 / +20 / +24), c->+52 (+40), 1
// (+36), the flag bits 0x10000000..0x00800000 of +120 from u8 c->+60,
// +56, +57, +58, +59, +61 (each stored as set), and the 4 bytes c->+12.
// r3 / f1 as the original leaves them.
DECOMP(00089678, AnimCmd_AddRecord_00089678) {
    const u32 c = arg<u32>(ctx, 0);
    u32 n, t;
    if (!HolderTarget28(ctx, c, arg<u32>(ctx, 1), 32, 62, n, t)) return;
    const u32 r = static_cast<u32>(call(ctx, 0x0029F760u, {n}));  // Rec128_Alloc_0029F760
    if (r == 0) return;
    const u32 v52 = load<u32>(c + 52);
    store<u32>(r + 16, load<u32>(c + 16));
    u32 w = load<u32>(r + 120);
    store<u32>(r + 20, load<u32>(c + 20));
    w = (w & ~0x10000000u) | ((load<u8>(c + 60) & 1u) << 28);
    store<u32>(r + 120, w);
    store<u32>(r + 8, t);
    w = (w & ~0x08000000u) | ((load<u8>(c + 56) & 1u) << 27);
    store<u32>(r + 120, w);
    store<u32>(r + 40, v52);
    w = (w & ~0x04000000u) | ((load<u8>(c + 57) & 1u) << 26);
    store<u32>(r + 36, 1);
    store<u32>(r + 120, w);
    ctx.fpr[1] = Lf(c + 24);
    store<u32>(r + 24, load<u32>(c + 24));
    w = (w & ~0x02000000u) | ((load<u8>(c + 58) & 1u) << 25);
    store<u32>(r + 120, w);
    w = (w & ~0x01000000u) | ((load<u8>(c + 59) & 1u) << 24);
    store<u32>(r + 120, w);
    w = (w & ~0x00800000u) | ((load<u8>(c + 61) & 1u) << 23);
    store<u32>(r + 120, w);
    for (u32 k = 0; k < 4; ++k) store<u8>(r + k, load<u8>(c + 12 + k));
    ctx.gpr[3] = load<u8>(c + 15);
}

// u32 f(Cmd* c, Effect* x): the target (CompTarget28, else
// Obj_FindById(c->+16)) with flag 0x8 at +192 hands its def's (+216)
// values to the effect's components: the kind-2 components c->+20 / +24
// / +28 get +20 = def +1464 / +1468 / +1472, the kind-3 components c->+32
// / +36 get +32..+40 = the def's vectors +1488 / +1504. r3 / f1 as the
// original leaves them.
DECOMP(000749C0, AnimCmd_ApplyDefValues_000749C0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 t = ById28(ctx, c, CompTarget28(ctx, c, x));
    if (t == 0) return;
    ctx.gpr[3] = load<u32>(t + 192) & 8u;
    if (ctx.gpr[3] == 0) return;
    const u32 d = load<u32>(t + 216);
    auto comp = [&](u32 off, u32 kind) -> u32 {
        const u32 key = load<u32>(c + off);
        if (off == 20) ctx.gpr[3] = key;  // only the first key is loaded into r3
        if (key == 0) return 0;
        const u32 e = static_cast<u32>(call(ctx, kFindComp, {x, key}));
        if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != kind) return 0;
        return e;
    };
    for (u32 k = 0; k < 3; ++k)
        if (const u32 e = comp(20 + 4 * k, 2); e != 0) {
            ctx.gpr[3] = load<u32>(d + 1464 + 4 * k);
            store<u32>(e + 20, static_cast<u32>(ctx.gpr[3]));
        }
    for (u32 k = 0; k < 2; ++k)
        if (const u32 e = comp(32 + 4 * k, 3); e != 0) {
            const u32 s = d + 1488 + 16 * k;
            ctx.gpr[3] = s;
            const u32 a = load<u32>(s), b = load<u32>(s + 4), z = load<u32>(s + 8);
            store<u32>(e + 32, a);
            store<u32>(e + 36, b);
            ctx.fpr[1] = Lf(s + 8);
            store<u32>(e + 40, z);
        }
}

// u32 f(Cmd* c, Effect* x): with a skeleton name c->+20 the target
// (CompTarget28, else Obj_FindById(c->+16)) has its animator +160 bound
// to the skeleton X: the effect's binding (Effect_Bind4BB7E0(x, name, 0)
// ->+208), else the global skeleton map (*0x00F8F12C)->+16 entry
// (HashMap_Find2). With matching bone-count fields ((+4 >> 10) & 63 of
// X and of the animator's +208) the animator gets vt[44](a, X) and each
// of the target's kind-16 entries (0x000D1288(t, 16, prev), +12 == 16)
// gets Comp_SetLook287318(e, 0). r3 as the original leaves it.
DECOMP(00096F40, AnimCmd_BindSkeleton_00096F40) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    if (load<u32>(c + 20) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 t = ById28(ctx, c, CompTarget28(ctx, c, x));
    if (t == 0) return;
    const u32 a = load<u32>(t + 160);
    if (a == 0) return;
    u32 sk;
    const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 20), 0}));  // Effect_Bind4BB7E0
    if (b != 0) {
        sk = load<u32>(b + 208);
    } else {
        Stack frame(ctx, 16);  // the key at 112(r1)
        store<u32>(frame.addr(), load<u32>(c + 20));
        const u32 r = static_cast<u32>(call(ctx, 0x007399A4u, {load<u32>(load<u32>(0x00F8F12Cu) + 16), frame.addr()}));  // HashMap_Find2
        sk = load<u32>(r);
        if (sk == 0) return;
    }
    const u32 ga = (load<u32>(load<u32>(a + 208) + 4) >> 10) & 63u;
    if (ga != ((load<u32>(sk + 4) >> 10) & 63u)) {
        ctx.gpr[3] = ga;
        return;
    }
    VCall(ctx, a, 44, {a, sk});
    for (u32 p = static_cast<u32>(call(ctx, 0x000D1288u, {t, 16, 0})); p != 0; p = static_cast<u32>(call(ctx, 0x000D1288u, {t, 16, p})))
        if (load<u32>(p + 12) == 16) call(ctx, 0x00287318u, {p, 0});  // Comp_SetLook287318
}

// u32 f(Cmd* c, Effect* x): the kind-5 component c->+12 of the effect
// (key not checked) and its record +28 give the time t = c->+20 (or the
// kind-1 component c->+16's vt[20]() float); the linked object
// (Obj_FindLinked_000D6AA0(rec, 0, 0)) sets its track c->+24 to t (u8
// c->+28 = relative): through its animator vt[60]() (f_00298EE4,
// Anim_SetTrackTime_00276A20, f_00299674), else Anim_SetTrackTime_000DC550
// on the object; without one the component's own animation (+36,
// Obj_FindComp34CC34 by c->+24) gets Obj_SetF32Via_00354150(t, scaled by
// Track_Length353748 when relative: length > 1e-5 ? length * t : 1).
// r3 / f1 as the original leaves them.
DECOMP(0006C800, AnimCmd_SetTrackTime_0006C800) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(call(ctx, kFindComp, {x, load<u32>(c + 12)}));
    if (e == 0) return;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return;
    const u32 rec = load<u32>(e + 28);
    if (rec == 0) return;
    f64 tm = Lf(c + 20);
    if (const u32 k2 = load<u32>(c + 16); k2 != 0) {
        const u32 e2 = static_cast<u32>(call(ctx, kFindComp, {x, k2}));
        if (e2 != 0 && static_cast<u32>(VCall(ctx, e2, 32, {e2})) == 1) {
            VCall(ctx, e2, 20, {e2});
            tm = ctx.fpr[1];
        }
    }
    const u32 o = static_cast<u32>(call(ctx, 0x000D6AA0u, {rec, 0, 0}));  // Obj_FindLinked_000D6AA0
    if (o != 0) {
        const u32 a = static_cast<u32>(VCall(ctx, o, 60, {o}));
        // r4 is unread by both Anim_SetTrackTime callees (left as is).
        if (a == 0) {
            callf(ctx, 0x000DC550u, {o, ctx.gpr[4], load<u8>(c + 28), load<u32>(c + 24)}, {tm});  // Anim_SetTrackTime_000DC550
            return;
        }
        call(ctx, 0x00298EE4u, {a});
        callf(ctx, 0x00276A20u, {a, ctx.gpr[4], load<u8>(c + 28), load<u32>(c + 24)}, {tm});  // Anim_SetTrackTime_00276A20
        call(ctx, 0x00299674u, {a});
        return;
    }
    const u32 an = load<u32>(e + 36);
    if (an == 0) return;
    const u32 tr = static_cast<u32>(call(ctx, 0x0034CC34u, {an, load<u32>(c + 24)}));  // Obj_FindComp34CC34
    if (tr == 0) return;
    if (load<u8>(c + 28) != 0) {
        call(ctx, 0x00353748u, {tr});  // Track_Length353748
        const f64 len = ctx.fpr[1];
        tm = len > Lf(0x0006C7F8u) ? Sgl28(len * tm) : Lf(0x0006C7FCu);
    }
    callf(ctx, 0x00354150u, {tr}, {tm});  // Obj_SetF32Via_00354150
}

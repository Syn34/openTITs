// Game code, medium sweep part 29: more AnimScript commands (target
// overrides, save profiles, volumes, events, colours, attach, audio).
#include "mid29.h"

using namespace sly4;
using namespace sly4::mid29;

namespace {
using namespace m29fn;

// The kind-`kind` component c->+off's +20 (Effect_FindComponent, vt[32]);
// Hash_ToString(c->+off) when the key is set but that fails. 0 when
// none (r3 = 0 for an empty key, else as the steps leave it).
u32 KindComp29(PPUContext& ctx, u32 c, u32 x, u32 off, u32 kind) {
    const u32 k = load<u32>(c + off);
    if (k == 0) {
        ctx.gpr[3] = 0;
        return 0;
    }
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
    if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == kind) return load<u32>(e + 20);
    call(ctx, HashStr29, {load<u32>(c + off)});
    return 0;
}

// The kind-5 component path without the empty-key test: the target of
// Effect_FindComponent(x, key)'s record, else 0.
u32 CompNoKey29(PPUContext& ctx, u32 x, u32 key) {
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, key}));
    if (e == 0 || static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return 0;
    const u32 q = load<u32>(e + 28);
    if (q == 0) return 0;
    return RecTarget29(ctx, q);
}

// The id c->+idOff, replaced by the kind-12 component c->+keyOff's
// vt[28]() (called again for the value) when that is non-zero.
u32 IdOf29(PPUContext& ctx, u32 c, u32 x, u32 idOff, u32 keyOff) {
    u32 id = load<u32>(c + idOff);
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, load<u32>(c + keyOff)}));
    if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 12 && static_cast<u32>(VCall(ctx, e, 28, {e})) != 0)
        id = static_cast<u32>(VCall(ctx, e, 28, {e}));
    return id;
}

// The level of the audio parameter commands (r28, before its extsb): u8
// c->+27 without the kind-1/2 component c->+20; else from lo = u8 c->+28 ?
// (s8) c->+29 : (u8 c->+24 ? 0 : -128) raised to the kind-2 component's
// (s8) vt[24]() (capped at 127) or for kind 1 to lo + (s32)((f32)(hi -
// lo) * vt[20]()) capped at hi = u8 c->+30 ? (s8) c->+31 : 127.
s32 AudioLevel29(PPUContext& ctx, u32 c, u32 x) {
    const u32 k = load<u32>(c + 20);
    if (k == 0) return load<u8>(c + 27);
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
    if (e == 0) return load<u8>(c + 27);
    const s32 lo = load<u8>(c + 28) != 0 ? static_cast<s8>(load<u8>(c + 29)) : (load<u8>(c + 24) != 0 ? 0 : -128);
    const s32 hi = load<u8>(c + 30) != 0 ? static_cast<s8>(load<u8>(c + 31)) : 127;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) == 2) {
        const s32 v = static_cast<s8>(static_cast<u8>(VCall(ctx, e, 24, {e})));
        const s32 m = v < 127 ? v : 127;
        return lo < m ? m : lo;
    }
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) != 1) return load<u8>(c + 27);
    VCall(ctx, e, 20, {e});
    const f64 span = Sgl28(static_cast<f64>(static_cast<s64>(hi - lo)));
    const u64 t = rt::fctiw(Sgl28(ctx.fpr[1] * span), true);
    ctx.fpr[1] = std::bit_cast<f64>(t);  // leftover f1 (fctiwz)
    const s32 r = lo + static_cast<s32>(static_cast<u32>(t));
    const s32 m = r < hi ? r : hi;
    return lo < m ? m : lo;
}

inline u64 Sx32(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }
inline u64 Sx8(s32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s8>(v))); }
}  // namespace

// u32 f(Cmd* c, Effect* x): copies the override settings (u8 c->+16 and
// the 4 bytes c->+17..+20) into the subsystems of the target of the
// kind-5 component c->+12: +968 bit 0x40 / +969 with bit 8 of +192;
// enable byte / value at +433 / +434 (bit 0x800), +689 / +696 (0x200,
// +692 = 0 when disabled), +761 / +768 (0x400, +764), +1689 / +1696
// (0x100, +1692), the enable forced to 0 while f32 +348 <= *0x00081AFC;
// +1384 bit 0x80 / +1385 with bit 0x2000 of +200. r3 / f1 as the original
// leaves them.
DECOMP(00081B00, AnimCmd_SetTargetOverride_00081B00) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 k = load<u32>(c + 12);
    if (k == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
    if (e == 0) return;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return;
    const u32 q = load<u32>(e + 28);
    if (q == 0) return;  // r3 = the kind (5)
    if (load<u32>(q + 276) != kTarget29) {
        ctx.gpr[3] = kTarget29;
        return;
    }
    const u32 t = load<u32>(q + 280);
    if (t == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    auto copy4 = [&](u32 dst) {
        for (u32 i = 0; i < 4; ++i) store<u8>(dst + i, load<u8>(c + 17 + i));
    };
    if ((load<u32>(t + 192) & 8u) != 0) {
        store<u8>(t + 968, static_cast<u8>((load<u8>(t + 968) & ~0x40u) | ((load<u8>(c + 16) << 6) & 0x40u)));
        copy4(t + 969);
    }
    auto gate = [&](u32 en, u32 val, u32 word) {
        const f64 f = Lf(t + 348);
        const u32 b = f <= Lf(0x00081AFCu) ? 0 : load<u8>(c + 16);
        store<u8>(t + en, static_cast<u8>(b));
        copy4(t + val);
        if (word != 0 && load<u8>(t + en) == 0) store<u32>(t + word, 0);
        ctx.fpr[1] = f;
    };
    if ((load<u32>(t + 192) & 0x800u) != 0) gate(433, 434, 0);
    if ((load<u32>(t + 192) & 0x200u) != 0) gate(689, 696, 692);
    if ((load<u32>(t + 192) & 0x400u) != 0) gate(761, 768, 764);
    if ((load<u32>(t + 192) & 0x100u) != 0) gate(1689, 1696, 1692);
    if ((load<u32>(t + 200) & 0x2000u) != 0) {
        store<u8>(t + 1384, static_cast<u8>((load<u8>(t + 1384) & ~0x80u) | ((load<u8>(c + 16) << 7) & 0x80u)));
        copy4(t + 1385);
        ctx.gpr[3] = load<u8>(c + 20);
        return;
    }
    ctx.gpr[3] = t;
}

// u32 f(Cmd* c): applies the save profile c->+12 (Save_Profile0C9FC8)
// to the player (slot word *0x00F65CA0 + 4216). With bit 8 of +192:
// unless u8 c->+18 and the def (+216) +52 already matches the profile's
// +52, Player_SetSave124D58(p, prof, u8 c->+16 == 0); with u8 c->+16 and
// +17 also vt[28](p, 0, 0) and Body_SetMode(vt[64](), 1) when vt[64]() is
// set. Else with bit 0x800 / 0x2000 of +200 Player_ResetFromDef238270 /
// Player_SwitchDef_0024B1E8(p, prof, u8 c->+16 == 0). r3 as the original
// leaves it.
DECOMP(00097E50, AnimCmd_ApplySaveProfile_00097E50) {
    constexpr u32 kProfile = 0x000C9FC8u;  // Save_Profile0C9FC8(hash)
    const u32 c = arg<u32>(ctx, 0);
    const u32 k = load<u32>(c + 12);
    if (k == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 p = load<u32>(load<u32>(kWorld29) + 4208 + 8);
    if (p == 0) {
        ctx.gpr[3] = k;
        return;
    }
    if ((load<u32>(p + 192) & 8u) != 0) {
        const u32 s = static_cast<u32>(call(ctx, kProfile, {k}));
        if (s == 0) return;
        if (load<u8>(c + 18) != 0) {
            const u32 d = load<u32>(p + 216);
            const u32 cur = d != 0 ? load<u32>(d + 52) : 0;
            if (cur == load<u32>(s + 52)) return;  // r3 = the profile
        }
        call(ctx, 0x00124D58u, {p, s, load<u8>(c + 16) == 0 ? 1u : 0u});  // Player_SetSave124D58
        if (load<u8>(c + 16) == 0 || load<u8>(c + 17) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
        VCall(ctx, p, 28, {p, 0, 0});
        if (static_cast<u32>(VCall(ctx, p, 64, {p})) == 0) return;
        const u32 b = static_cast<u32>(VCall(ctx, p, 64, {p}));
        call(ctx, 0x00261328u, {b, 1});  // Body_SetMode
        return;
    }
    const u32 w = load<u32>(p + 200);
    u32 fn;
    if ((w & 0x800u) != 0) fn = 0x00238270u;        // Player_ResetFromDef238270
    else if ((w & 0x2000u) != 0) fn = 0x0024B1E8u;  // Player_SwitchDef_0024B1E8
    else {
        ctx.gpr[3] = k;
        return;
    }
    const u32 s = static_cast<u32>(call(ctx, kProfile, {k}));
    if (s == 0) return;
    call(ctx, fn, {p, s, load<u8>(c + 16) == 0 ? 1u : 0u});
}

// u32 f(Cmd* c, Effect* x): refreshes three volume objects of the volume
// manager *0x00F657FC (u8 c->+36 the argument, u8 c->+37 a clear first).
// A: the kind-21 component c->+16's +20, else the manager's entry c->+12
// (SortedMap_Find734750(mgr + 68) index into +12, 112-byte items) ->
// Obj_ClearLinks_0039BEFC, Volume_Refresh399758(v, b, 0, 0). B: the
// kind-19 component c->+24's, else the first +12 item whose map (+76)
// has c->+20 (index into +28, 336-byte items) -> Field_Clear88_184_0039BEDC,
// Volume_Item3992AC(v, b, 0, 0). C: the kind-20 component c->+32's, else
// the first +44 item (368 bytes) whose Item_Hash39FEBC is c->+28 ->
// f_0039D32C(v, b, 1, u8 c->+38). Each missing key is formatted with
// Hash_ToString. r3 as the original leaves it.
DECOMP(00073078, AnimCmd_RefreshVolumes_00073078) {
    constexpr u32 kVol = 0x00F657FCu;
    constexpr u32 kMapFind = 0x00734750u;  // SortedMap_Find734750(map, &key)
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the keys at 112(r1) / 116(r1)
    const u32 key = frame.addr();
    u32 v = KindComp29(ctx, c, x, 16, 21);
    if (const u32 k = load<u32>(c + 12); k != 0 && v == 0) {
        const u32 g = load<u32>(kVol);
        store<u32>(key + 4, k);
        const u32 p = static_cast<u32>(call(ctx, kMapFind, {g + 68, key + 4}));
        const s32 idx = static_cast<s16>(load<u16>(p));
        v = idx != -1 ? load<u32>(g + 12) + static_cast<u32>(idx) * 112 : 0;
        if (v == 0) call(ctx, HashStr29, {load<u32>(c + 12)});
    }
    if (v != 0) {
        if (load<u8>(c + 37) != 0) call(ctx, 0x0039BEFCu, {v});  // Obj_ClearLinks_0039BEFC
        call(ctx, 0x00399758u, {v, load<u8>(c + 36), 0, 0});     // Volume_Refresh399758
    }
    v = KindComp29(ctx, c, x, 24, 19);
    if (const u32 k = load<u32>(c + 20); k != 0 && v == 0) {
        const u32 g = load<u32>(kVol);
        for (u32 i = 0, off = 0; i < load<u16>(load<u32>(g + 4) + 2); ++i, off += 112) {
            const u32 r = load<u32>(g + 12) + off;
            if (r == 0) continue;
            store<u32>(key, k);
            const u32 p = static_cast<u32>(call(ctx, kMapFind, {r + 76, key}));
            const s32 idx = static_cast<s16>(load<u16>(p));
            if (idx != -1) {
                v = load<u32>(g + 28) + static_cast<u32>(idx) * 336;
                break;
            }
        }
        if (v == 0) call(ctx, HashStr29, {load<u32>(c + 20)});
    }
    if (v != 0) {
        if (load<u8>(c + 37) != 0) call(ctx, 0x0039BEDCu, {v});  // Field_Clear88_184_0039BEDC
        call(ctx, 0x003992ACu, {v, load<u8>(c + 36), 0, 0});     // Volume_Item3992AC
    }
    v = KindComp29(ctx, c, x, 32, 20);
    if (const u32 k = load<u32>(c + 28); k != 0 && v == 0) {
        const u32 g = load<u32>(kVol);
        for (u32 i = 0, off = 0; i < load<u16>(load<u32>(g + 36) + 2); ++i, off += 368) {
            const u32 it = load<u32>(g + 44) + off;
            if (it == 0) continue;
            if (static_cast<u32>(call(ctx, 0x0039FEBCu, {it})) == k) {  // Item_Hash39FEBC
                v = it;
                break;
            }
        }
        if (v == 0) call(ctx, HashStr29, {load<u32>(c + 28)});
    }
    if (v != 0) call(ctx, 0x0039D32Cu, {v, load<u8>(c + 36), 1, load<u8>(c + 38)});
}

// u32 f(Cmd* c, Effect* x): posts a world event (World_Query482D14(world,
// sign, &ev)) of kind c->+40 from a (the kind-5 component c->+12's target,
// else Obj_FindById of c->+16 or the kind-12 component c->+20's vt[28]())
// to b (likewise c->+24 / c->+28 / c->+32). The 24-byte event is zeroed
// and filled by kind: 28 Rec_Init28_00464EA0(&ev, c->+36, def +52, a +168)
// (0, 0 without a); 1 Event1_Init_00464ED4(&ev, b, def +52, a +168,
// parent (+336) +168, 0, 0); 16 EventInit_00465494 and 3 Event3_Init_00464F88
// (&ev, b, def +52, a +168, parent +168); 11 EventInit_0046528C and 4
// EventInit_0046502C (&ev, b, a +168, parent +168). sign = (s8) a->+172
// when bit 0 of a->+188 is set (0 for kind 28). r3 as the original leaves it.
DECOMP(00086000, AnimCmd_PostEvent_00086000) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 a = CompNoKey29(ctx, x, load<u32>(c + 12));
    if (a == 0) a = static_cast<u32>(call(ctx, FindById29, {IdOf29(ctx, c, x, 16, 20)}));
    u32 b = CompNoKey29(ctx, x, load<u32>(c + 24));
    if (b == 0) b = static_cast<u32>(call(ctx, FindById29, {IdOf29(ctx, c, x, 28, 32)}));
    ctx.gpr[3] = b;
    const u32 kind = load<u32>(c + 40);
    if (kind == 0) return;
    Stack frame(ctx, 32);  // the event at 112(r1)..135(r1)
    const u32 ev = frame.addr();
    for (u32 k = 0; k < 24; k += 4) store<u32>(ev + k, 0);
    u32 sign = 0;
    if (kind == 28) {
        if (a == 0) {
            call(ctx, 0x00464EA0u, {ev, load<u32>(c + 36), 0, 0});  // Rec_Init28_00464EA0
        } else {
            const u32 d = load<u32>(a + 216);
            const u32 d52 = d != 0 ? load<u32>(d + 52) : 0;
            call(ctx, 0x00464EA0u, {ev, load<u32>(c + 36), d52, load<u32>(a + 168)});
        }
    } else {
        if (a == 0) return;
        if ((load<u32>(a + 188) & 1u) != 0) sign = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(a + 172))));
        auto def52 = [&]() {
            const u32 d = load<u32>(a + 216);
            return d != 0 ? load<u32>(d + 52) : 0u;
        };
        auto par168 = [&]() {
            const u32 o = load<u32>(a + 336);
            return o != 0 ? load<u32>(o + 168) : 0u;
        };
        switch (kind) {
        case 16: {
            const u32 d52 = def52(), a168 = load<u32>(a + 168);
            call(ctx, 0x00465494u, {ev, b, d52, a168, par168()});  // EventInit_00465494
            break;
        }
        case 11: {
            const u32 a168 = load<u32>(a + 168);
            call(ctx, 0x0046528Cu, {ev, b, a168, par168()});  // EventInit_0046528C
            break;
        }
        case 4: {
            const u32 a168 = load<u32>(a + 168);
            call(ctx, 0x0046502Cu, {ev, b, a168, par168()});  // EventInit_0046502C
            break;
        }
        case 3: {
            const u32 d52 = def52(), a168 = load<u32>(a + 168);
            call(ctx, 0x00464F88u, {ev, b, d52, a168, par168()});  // Event3_Init_00464F88
            break;
        }
        case 1: {
            const u32 d52 = def52(), a168 = load<u32>(a + 168);
            call(ctx, 0x00464ED4u, {ev, b, d52, a168, par168(), 0, 0});  // Event1_Init_00464ED4
            break;
        }
        default:
            return;
        }
    }
    call(ctx, 0x00482D14u, {load<u32>(kWorld29), Sx32(sign), ev});  // World_Query482D14
}

// u32 f(Cmd* c, Effect* x, a3): sets the colour EffectRule_Colour76708(&rgba,
// c, x, a3) on the component c->+12 (vt[32]() asked afresh for each kind
// test): kind 10 its node's
// (Node_FindCached_004BFDDC) vt[56](node, rgba); kind 7 the material +24
// (else the live AglMaterial named +20) f32 +64..+76 = each byte / 255
// (*0x00076968); kind 5 with a type-9 record +352..+360 (rgb) then
// vt[36](); type 8 the material c->+16 of it (WorldObj_Material4A59B4) as
// kind 7, else the record's bytes +316..+319. r3 / f1 as the original
// leaves them.
DECOMP(00076970, AnimCmd_SetColour_00076970) {
    constexpr u32 kColour = 0x00076708u;  // EffectRule_Colour76708(out, c, x, a3)
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    const u32 a3 = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);  // the colours at 112(r1)..131(r1), one per path
    const u32 col = frame.addr();
    const f64 s = Lf(0x00076968u);
    auto colour = [&](u32 o) { call(ctx, kColour, {col + o, c, x, a3}); };
    auto sc = [&](u32 o, u32 i) { return Sgl28(static_cast<f64>(load<u8>(col + o + i)) * s); };
    auto four = [&](u32 m, u32 o) {
        colour(o);
        for (u32 i = 0; i < 4; ++i) store<u32>(m + 64 + 4 * i, Bits28(sc(o, i)));
        ctx.fpr[1] = sc(o, 3);
        ctx.gpr[3] = load<u8>(col + o + 2);
    };
    const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, load<u32>(c + 12)}));
    if (e == 0) return;
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) == 10) {
        const u32 n = static_cast<u32>(call(ctx, 0x004BFDDCu, {e}));  // Node_FindCached_004BFDDC
        if (n == 0) return;
        colour(0);
        const u32 w = load<u32>(col);
        const u32 opd = load<u32>(load<u32>(n) + 56);
        call_ptr(ctx, opd, {n, w, load<u32>(opd)});  // r5 = the entry
        return;
    }
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) == 7) {
        u32 m = load<u32>(e + 24);
        if (m == 0) {
            if (const u32 h = load<u32>(e + 20); h != 0) m = MatFind29(ctx, h);
            if (m == 0) {
                ctx.gpr[3] = 0;
                return;
            }
        }
        four(m, 4);
        return;
    }
    if (static_cast<u32>(VCall(ctx, e, 32, {e})) != 5) return;
    const u32 q = load<u32>(e + 28);
    if (q == 0) return;  // r3 = the kind (5)
    const u32 ty = (load<u32>(q + 4) >> 10) & 0x3Fu;
    if (ty == 9) {
        colour(8);
        store<u32>(q + 352, Bits28(sc(8, 0)));
        store<u32>(q + 356, Bits28(sc(8, 1)));
        store<u32>(q + 360, Bits28(sc(8, 2)));
        VCall(ctx, q, 36, {q});
        ctx.fpr[1] = sc(8, 2);
        return;
    }
    if (ty != 8) {
        ctx.gpr[3] = ty;
        return;
    }
    u32 m = 0;
    if (const u32 k3 = load<u32>(c + 16); k3 != 0) m = static_cast<u32>(call(ctx, 0x004A59B4u, {q, k3}));  // WorldObj_Material4A59B4
    if (m == 0) {
        colour(16);
        for (u32 i = 0; i < 4; ++i) store<u8>(q + 316 + i, load<u8>(col + 16 + i));
        ctx.gpr[3] = q + 316;
        return;
    }
    four(m, 12);
}

// u32 f(Cmd* c, Effect* x): attaches a (the kind-5 component c->+12's
// target, else Obj_FindById(c->+12)) to b (c->+16 likewise), detaching b
// from its old parent (+336, Obj_Detach0D72C0) unless that is a. When
// CallK2_000D9544(a) accepts b (Obj_CanAccept_002B91EC):
// f_002B9454(h, b, 0x01164B40, 0x0006D838, 1, f1 = *0x0006D838). Else
// Obj_Attach0D15C0(a, &desc) with a 228-byte attach descriptor: +128
// position c->+32.., +144 the rotation quaternion of the angles c->+48 /
// +52 / +56 (* *0x0006D83C, via Math_Cosf / Math_Sinf), +160 c->+20, +168
// c->+64, +176 flags (bit 31 a+172 bit 0x200 or u8 c->+116, 0x02000000 /
// 0x10000000 / 0x08000000 from u8 c->+117 / +118 / +121), +192 b, +224
// c->+112 when u8 c->+120; then vt[48](a, f1 = *0x0006D838) with u8
// c->+119 and the descriptor's +180 handle released (f_0054EC1C,
// f_0054EC18, SmallFree(h, 20)). r3 / f1 as the original leaves them.
DECOMP(0006D840, AnimCmd_AttachToTarget_0006D840) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    u32 a = CompTarget29(ctx, c, x, 12);
    if (a == 0) {
        a = static_cast<u32>(call(ctx, FindById29, {load<u32>(c + 12)}));
        if (a == 0) return;
    }
    u32 b = CompTarget29(ctx, c, x, 16);
    if (b == 0) {
        b = static_cast<u32>(call(ctx, FindById29, {load<u32>(c + 16)}));
        if (b == 0) return;
    }
    if (const u32 p = load<u32>(b + 336); p != 0) {
        if (a == p) {
            ctx.gpr[3] = p;
            return;
        }
        call(ctx, 0x000D72C0u, {p, b});  // Obj_Detach0D72C0
    }
    const u32 h = static_cast<u32>(call(ctx, 0x000D9544u, {a}));  // CallK2_000D9544
    if (h != 0 && static_cast<u32>(call(ctx, 0x002B91ECu, {h, b})) != 0) {  // Obj_CanAccept_002B91EC
        callf(ctx, 0x002B9454u, {h, b, 0x01164B40u, 0x0006D838u, 1}, {Lf(0x0006D838u)});
        return;
    }
    const u32 on = (load<u32>(a + 172) & 0x200u) != 0 ? 1u : load<u8>(c + 116);
    const u32 top = on != 0 ? 1u : 0u;
    Stack frame(ctx, 240);  // the descriptor at 112(r1)..339(r1)
    const u32 d = frame.addr();
    const u32 old = load<u32>(d + 176) & 0x00FFFFFFu;  // read before it is written (as the original)
    for (u32 i = 0; i < 3; ++i) store<u32>(d + 128 + 4 * i, load<u32>(0x01164B40u + 4 * i));
    for (u32 i = 0; i < 4; ++i) store<u32>(d + 144 + 4 * i, load<u32>(0x01164C80u + 4 * i));
    for (u32 off : {164u, 168u, 172u}) store<u32>(d + off, 0);
    store<u32>(d + 176, old);
    store<u32>(d + 180, 0);
    store<u8>(d + 0, 0);
    store<u8>(d + 64, 0);
    store<u32>(d + 196, 0);
    store<u32>(d + 200, 0xFFFFFFFFu);
    store<u32>(d + 204, 0);
    store<u32>(d + 208, 0);
    store<u32>(d + 212, 0xFFFFFFFFu);
    store<u32>(d + 216, 0);
    store<u32>(d + 220, 0);
    store<u32>(d + 224, 0);
    store<u32>(d + 192, b);
    store<u32>(d + 160, load<u32>(c + 20));
    for (u32 i = 0; i < 3; ++i) store<u32>(d + 128 + 4 * i, load<u32>(c + 32 + 4 * i));
    const f64 kk = Lf(0x0006D83Cu);
    const f64 A = Sgl28(Lf(c + 52) * kk);
    const f64 B = Sgl28(Lf(c + 56) * kk);
    const f64 C = Sgl28(Lf(c + 48) * kk);
    auto trig = [&](u32 fn, f64 v) {
        callf(ctx, fn, {}, {v});
        return ctx.fpr[1];
    };
    const f64 cA = trig(0x007344E0u, A), cB = trig(0x007344E0u, B), cC = trig(0x007344E0u, C);  // Math_Cosf
    const f64 sA = trig(0x007345F0u, A), sB = trig(0x007345F0u, B), sC = trig(0x007345F0u, C);  // Math_Sinf
    const f64 f2 = Sgl28(sA * sB), f3 = Sgl28(cA * cB), f4 = Sgl28(cB * sA), f5 = Sgl28(cA * sB);
    const f64 f6 = Sgl28(f2 * sC), f7 = Sgl28(f3 * sC), f8 = Sgl28(f4 * sC), f1 = Sgl28(f5 * sC);
    u32 fl = (load<u32>(d + 176) & 0x7FFFFFFFu) | (top << 31);
    fl = (fl & ~0x02000000u) | ((load<u8>(c + 117) & 1u) << 25);
    fl = (fl & ~0x10000000u) | ((load<u8>(c + 118) & 1u) << 28);
    fl = (fl & ~0x08000000u) | ((load<u8>(c + 121) & 1u) << 27);
    store<u32>(d + 168, load<u32>(c + 64));
    store<u32>(d + 176, fl);
    const f64 qw = Sgl28(std::fma(f3, cC, -f6));
    const f64 qx = Sgl28(std::fma(f2, cC, f7));
    const f64 qz = Sgl28(std::fma(f5, cC, -f8));
    const f64 qy = Sgl28(std::fma(f4, cC, f1));
    store<u32>(d + 156, Bits28(qw));
    store<u32>(d + 144, Bits28(qx));
    store<u32>(d + 152, Bits28(qz));
    store<u32>(d + 148, Bits28(qy));
    f64 last = qy;
    if (load<u8>(c + 120) != 0) {
        store<u32>(d + 224, load<u32>(c + 112));
        last = Lf(c + 112);
    }
    ctx.fpr[1] = last;
    call(ctx, 0x000D15C0u, {a, d});  // Obj_Attach0D15C0
    if (load<u8>(c + 119) != 0) {
        ctx.fpr[1] = Lf(0x0006D838u);
        VCall(ctx, a, 48, {a});
    } else {
        ctx.gpr[3] = 0;  // the flag byte tested
    }
    const u32 w = load<u32>(d + 180);
    if (w == 0) return;
    call(ctx, 0x0054EC1Cu, {w});
    call(ctx, 0x0054EC18u, {w});
    call(ctx, 0x00552420u, {load<u32>(d + 180), 20});  // SmallFree
}

// u32 f(Cmd* c, Effect* x): sets an audio parameter to AudioLevel29.
// With u8 c->+25 Call_6D5038_If_0044CEB0((s32) c->+16, level); else the
// emitter handle (*0x010AFEA4 with u8 c->+26, else the kind-13 component
// c->+12's +20, else 0) when Audio_IsEmitterValid gets
// Voice_SetParamByHandle_0044A78C(handle, (s32) c->+16, level). r3 / f1 as
// the original leaves them.
DECOMP(0009170C, AnimCmd_SetAudioParam_0009170C) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 x = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);  // the handle at 112(r1)
    const u32 hp = frame.addr();
    store<u32>(hp, 0);
    store<u16>(hp + 2, 0);
    if (load<u8>(c + 25) != 0) {
        const u32 id = load<u32>(c + 16);
        const s32 lv = AudioLevel29(ctx, c, x);
        call(ctx, 0x0044CEB0u, {Sx32(id), Sx8(lv)});  // Call_6D5038_If_0044CEB0
        return;
    }
    if (load<u8>(c + 26) != 0) {
        store<u32>(hp, load<u32>(0x010AFEA4u));
    } else if (const u32 k = load<u32>(c + 12); k != 0) {
        const u32 e = static_cast<u32>(call(ctx, FindComp29, {x, k}));
        if (e != 0 && static_cast<u32>(VCall(ctx, e, 32, {e})) == 13) store<u32>(hp, load<u32>(e + 20));
    }
    const u32 h = load<u32>(hp);
    if (static_cast<u32>(call(ctx, 0x004437CCu, {h})) == 0) return;  // Audio_IsEmitterValid
    const u32 id = load<u32>(c + 16);
    const s32 lv = AudioLevel29(ctx, c, x);
    call(ctx, 0x0044A78Cu, {h, Sx32(id), Sx8(lv)});  // Voice_SetParamByHandle_0044A78C
}

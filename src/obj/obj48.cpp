// Game code, part 48: the small-function sweep continued from the remaining
// 103 instruction functions (size order; obj47*.cpp did 101-103). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj48.h"

using namespace sly4;
using namespace sly4::obj48;

// ---------------------------------------------------------------- sweep: 103 instructions
// u32 f(Obj* o): c = Effect_FindComponent(o->+32, 0xBEE2EF10); unless c,
// c->vt[32](c) == 5 and c->+28 are all set, r3 = the first that fails. The
// anchor hash is 0x1D6616C5 / 0x21804DF8 / 0 for o->+532 = 0 / 1 / other;
// WorldObj_FindByHash(hash, 0) gives the anchor whose vec3 at +128 is used
// (else Hash_ToString(hash) for the miss message and the vec3 0x01164C00).
// Then o->+544..552 = the vec3 at c->+28 + 64, o->+560..568 = the anchor
// vec3, o->+576 = 0; r3 = c->+28 + 64.
DECOMP(003C1658, Effect_SetAnchorPose_003C1658) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = static_cast<u32>(call(ctx, kFindComp48, {load<u32>(o + 32), 0xBEE2EF10u}));
    if (c == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = static_cast<u32>(VcOE48(ctx, c, 32, {c}));
    if (k != 5) {
        ret(ctx, k);
        return;
    }
    if (load<u32>(c + 28) == 0) {
        ret(ctx, 0u);
        return;
    }
    auto anchor = [&]() -> u32 {
        const s32 s = static_cast<s32>(load<u32>(o + 532));
        return s == 0 ? 0x1D6616C5u : (s == 1 ? 0x21804DF8u : 0u);
    };
    constexpr u32 kBase = 0x01164BD0u;
    u32 v0 = load<u32>(kBase + 48), v1 = load<u32>(kBase + 52), v2 = load<u32>(kBase + 56);
    const u32 w = static_cast<u32>(call(ctx, 0x005801DCu, {anchor(), 0}));
    if (w != 0) {
        v0 = load<u32>(w + 80 + 48);
        v1 = load<u32>(w + 80 + 52);
        v2 = load<u32>(w + 80 + 56);
    } else {
        call(ctx, kHashStr48, {anchor()});  // r4 = o->+532 left over
    }
    const u32 p = load<u32>(c + 28) + 64;
    Cp48(o + 544, p);
    Cp48(o + 548, p + 4);
    store<u32>(o + 560, v0);
    Cp48(o + 552, p + 8);
    store<u32>(o + 564, v1);
    store<u32>(o + 568, v2);
    store<u32>(o + 576, 0);
    ctx.fpr[1] = Lfs48(p + 8);  // f1 = the last lfs
    ret(ctx, p);
}

// bool f(Snd* s): starts a sound. When the u16 s->+104 and +106 are both 0:
// bit 0 of s->+72 = (u8 s->+96 != 0); Audio_StartFocusSound(&h, s->+0 ?
// *s->+0 : 0, s + 16), s->+4 = h, u8 s->+102 = 1; then, when the voice
// table is live (u8 0x010AEC60), h's low half is non-zero, its high half
// indexes a 272-byte slot (count 0x010B0218, base 0x010B021C) whose +256
// carries the same serial (bits 15..30) and a clear bit 31, and
// Voice_IsPlaying(slot->+16): u8 s->+98 = (u8 s->+96 == 0), +97 = 1, r3 = 1.
// Otherwise (a preset sound in +104) Emitter_Start(s->+104) and, when that
// works, s->+4 = s->+104, +98 = +97 = 1, r3 = 1. r3 = 0 on every miss.
DECOMP(004451EC, Sound_TryStart_004451EC) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 h0 = load<u32>(s);
    const u8 b96 = load<u8>(s + 96);
    const u32 f72 = load<u32>(s + 72);
    if (load<u16>(s + 104) == 0 && load<u16>(s + 106) == 0) {
        store<u32>(s + 72, b96 != 0 ? (f72 | 1u) : (f72 & ~1u));
        const u32 a = h0 != 0 ? load<u32>(h0) : 0u;
        Stack fr(ctx, 16);  // sp+112: the handle out
        call(ctx, 0x00444610u, {fr.addr(), a, s + 16});
        store<u32>(s + 4, load<u32>(fr.addr()));
        store<u8>(s + 102, 1);
        if (load<u8>(0x010AEC60u) == 0) {
            ret(ctx, 0u);
            return;
        }
        const u32 h = load<u32>(s + 4);
        const u32 lo = h & 0xFFFFu, idx = h >> 16;
        if (lo == 0 || !(static_cast<s32>(idx) < static_cast<s32>(load<u32>(0x010B0218u)))) {
            ret(ctx, 0u);
            return;
        }
        const u32 e = load<u32>(0x010B021Cu) + idx * 272u;
        if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo || (load<u8>(e + 256) & 0x80u) != 0) {
            ret(ctx, 0u);
            return;
        }
        if (static_cast<u32>(call(ctx, 0x006D5E10u, {load<u32>(e + 16)})) == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u8>(s + 98, load<u8>(s + 96) == 0 ? 1 : 0);
    } else {
        if (static_cast<u32>(call(ctx, 0x0044448Cu, {load<u32>(s + 104)})) == 0) {
            ret(ctx, 0u);
            return;
        }
        store<u32>(s + 4, load<u32>(s + 104));
        store<u8>(s + 98, 1);
    }
    store<u8>(s + 97, 1);
    ret(ctx, 1u);
}

// void f(SoundMgr* m): constructor. Two arrays (+4: count 512, stride 16,
// data +12, allocator +16; +28: count 50, stride 16, data +36, allocator
// +40, element vtables 0x00F2E978 / 0x00F2FD30 at +24 / +48) whose storage
// comes from allocator->vt[0](allocator, count * (stride / 2), 0, 16) when
// the count is non-zero, then Array_Init each; SlotList_Init_00784C84(m +
// 52, 512), u8 +84 = 0 and the global 0x00F65BE8 = m (r3 = 0x00F60000 left).
DECOMP(004518D0, SoundMgr_Construct_004518D0) {
    const u32 m = arg<u32>(ctx, 0);
    store<u16>(m + 4, 512);
    store<u16>(m + 6, 0);
    store<u16>(m + 8, 0);
    store<u16>(m + 10, 16);
    store<u32>(m + 12, 0);
    u32 a = static_cast<u32>(call(ctx, 0x00550EE8u));
    store<u32>(m + 16, a);
    u32 n = load<u16>(m + 4);
    if (n != 0) {
        const u32 sz = n * ((load<u16>(m + 10) >> 1) & 0x7FFFu);
        store<u32>(m + 12, static_cast<u32>(VcEO48(ctx, load<u32>(a), 0, {a, sz, 0, 16})));
    }
    call(ctx, 0x00558F38u, {m + 4});
    store<u32>(m + 24, 0x00F2E978u);
    store<u16>(m + 34, 16);
    store<u32>(m + 20, 0);
    store<u16>(m + 28, 50);
    store<u16>(m + 30, 0);
    store<u16>(m + 32, 0);
    store<u32>(m + 36, 0);
    a = static_cast<u32>(call(ctx, 0x00550EE8u));
    store<u32>(m + 40, a);
    n = load<u16>(m + 28);
    if (n != 0) {
        const u32 sz = n * ((load<u16>(m + 34) >> 1) & 0x7FFFu);
        store<u32>(m + 36, static_cast<u32>(VcEO48(ctx, load<u32>(a), 0, {a, sz, 0, 16})));
    }
    call(ctx, 0x00558F38u, {m + 28});
    store<u32>(m + 48, 0x00F2FD30u);
    store<u32>(m + 44, 0);
    call(ctx, 0x00784C84u, {m + 52, 512});
    store<u8>(m + 84, 0);
    store<u32>(0x00F65BE8u, m);
    ret(ctx, 0x00F60000u);
}

// ---------------------------------------------------------------- sweep: 104 instructions
// u32 f(Obj* o): Obj_RegisterParam(o, p) for the 19 parameter descriptors
// 0x00F952AC, 0x00F952C4, 0x00F952E0 and 0x00F95320 + 24 * k (k = 0..15);
// r3 = the last result.
DECOMP(0003BE60, Params_Register19_0003BE60) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004F7128u, {o, 0x00F952ACu});
    call(ctx, 0x004F7128u, {o, 0x00F952C4u});
    u64 r = call(ctx, 0x004F7128u, {o, 0x00F952E0u});
    for (u32 k = 0; k < 16; ++k) r = call(ctx, 0x004F7128u, {o, 0x00F95320u + 24 * k});
    ctx.gpr[3] = r;
}

// u32 f(Cmd* c, Owner* x): resolves the target t = Effect_Bind4BB7E0(x,
// c->+16, 0), else Effect_Bind4BB7E0(x, c->+16, 1) (when +16 is set), else
// Obj_FindById(c->+12)->+160 (Hash_ToString(c->+12) on a miss). With a
// child name c->+20 the child is looked up with WorldObj_ForEach(t,
// 0x00F2FAF8, {name, 0}, 0) (Hash_ToString(name) and t->vt[12](t) when
// none). When the result is a kind-8 object ((+4 >> 10) & 63) and the
// list (*0x00F65CA0)->+4216 is non-empty and Obj_Notify26(list) works,
// r3 = Recs_AddKeyed_002ACAB8(that, obj, c->+24); r3 is otherwise what the
// last step left.
DECOMP(0006BF28, Effect_BindTarget_0006BF28) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    u32 r3 = load<u32>(c + 16);
    if (r3 != 0) {
        t = r3 = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 16), 0}));
        if (t == 0) t = r3 = static_cast<u32>(call(ctx, 0x004BB7E0u, {x, load<u32>(c + 16), 1}));
    }
    if (t == 0) {
        r3 = load<u32>(c + 12);
        if (r3 != 0) {
            r3 = static_cast<u32>(call(ctx, 0x000D6A28u, {r3}));
            if (r3 != 0) t = load<u32>(r3 + 160);
            else r3 = static_cast<u32>(call(ctx, kHashStr48, {load<u32>(c + 12)}));
        }
    }
    if (t == 0) {
        ret(ctx, r3);
        return;
    }
    u32 q = t;
    r3 = load<u32>(c + 20);
    if (r3 != 0) {
        Stack fr(ctx, 16);  // sp+112: {name, found}
        const u32 v = fr.addr();
        store<u32>(v, r3);
        store<u32>(v + 4, 0);
        r3 = static_cast<u32>(call(ctx, 0x0057F5D8u, {t, 0x00F2FAF8u, v, 0}));
        q = load<u32>(v + 4);
        if (q == 0) {
            call(ctx, kHashStr48, {load<u32>(c + 20)});
            r3 = static_cast<u32>(VcOE48(ctx, t, 12, {t}));
            q = load<u32>(v + 4);
        }
    }
    if (q == 0) {
        ret(ctx, r3);
        return;
    }
    r3 = (load<u32>(q + 4) >> 10) & 0x3Fu;
    if (r3 != 8) {
        ret(ctx, r3);
        return;
    }
    r3 = load<u32>(load<u32>(kGameRoot48) + 4208 + 8);
    if (r3 == 0) {
        ret(ctx, 0u);
        return;
    }
    r3 = static_cast<u32>(call(ctx, 0x000D94CCu, {r3}));
    if (r3 == 0) {
        ret(ctx, 0u);
        return;
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x002ACAB8u, {r3, q, load<u32>(c + 24)})));
}

// T* f(?, XmlCursor* x): new 28-byte record (base Obj_InitVt_B21C98, vtable
// 0x00B0F8B0): +12 = hash 0x7A30D7E3; +16 = hash 0x93116591, else 0xDD693D52
// (u8 +24 = 1 when only the second is present); +20 = s32 0xDDCC7674 (u8
// +24 = 0), else 0x1EB4C7D3 (u8 +24 = 1), default 9 (the readers return 0
// when the attribute is found).
DECOMP(0007A3EC, XmlNew_B0F8B0_0007A3EC) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 28, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m + 20, 9);
    store<u32>(m, 0x00B0F8B0u);
    store<u8>(m + 24, 0);
    call(ctx, kReadHash47, {x, 0x7A30D7E3u, m + 12, 0});
    if (RdHash47(ctx, x, 0x93116591u, m + 16) == 0) store<u8>(m + 24, 0);
    else if (RdHash47(ctx, x, 0xDD693D52u, m + 16) == 0) store<u8>(m + 24, 1);
    Stack fr(ctx, 16);  // sp+112: the s32
    const u32 v = fr.addr();
    store<u32>(v, 0xFFFFFFFFu);
    if (static_cast<u32>(call(ctx, kReadS32_48, {x, 0xDDCC7674u, v, Sx48(load<u32>(m + 20))})) == 0) {
        const u32 r = load<u32>(v);
        store<u8>(m + 24, 0);
        store<u32>(m + 20, r);
    } else if (static_cast<u32>(call(ctx, kReadS32_48, {x, 0x1EB4C7D3u, v, Sx48(load<u32>(m + 20))})) == 0) {
        const u32 r = load<u32>(v);
        store<u8>(m + 24, 1);
        store<u32>(m + 20, r);
    }
    ret(ctx, m);
}

// T* f(?, XmlCursor* x): new 68-byte record (base Obj_InitVt_B21C98, vtable
// 0x00B0FB30): char[32] +12 = string 0x00012B61 (default 0x00AC4DD4,
// copied when non-empty), hashes +48 / +52 / +56 = 0x88185806 / 0x7FAAD4A0
// / 0xFF457B34, f32 +60 = 0xFF478E71 (default -1), s32 +64 = 0x042930B2.
DECOMP(0007D3A0, XmlNew_B0FB30_0007D3A0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 68, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 48, 0);
    store<u32>(m + 52, 0);
    store<u32>(m + 56, 0);
    store<u32>(m + 64, 0);
    store<u32>(m + 60, 0xBF800000u);
    store<u32>(m, 0x00B0FB30u);
    store<u8>(m + 12, 0);
    {
        Stack fr(ctx, 16);  // sp+112: const char*
        store<u32>(fr.addr(), 0);
        call(ctx, 0x004FA2ACu, {x, 0x00012B61u, fr.addr(), 0x00AC4DD4u});
        const u32 s = load<u32>(fr.addr());
        if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553684u, {m + 12, s, 32});
    }
    RdHash47(ctx, x, 0x88185806u, m + 48);
    RdHash47(ctx, x, 0x7FAAD4A0u, m + 52);
    RdHash47(ctx, x, 0xFF457B34u, m + 56);
    RdF32_47(ctx, x, 0xFF478E71u, m + 60);
    RdS32_48(ctx, x, 0x042930B2u, m + 64);
    ret(ctx, m);
}

// u32 f(Cmd* c, Owner* x): scene start. Unless c->+32 and c->+36 are both
// 0 (r3 = 0): the scene a = c->+32, replaced by comp->vt[24](comp) when
// comp = Effect_FindComponent(x, c->+36) answers 2 to vt[32]; the same for
// b from the component 0xB2E59A9F; r3 = World_StartScene46C3B8(
// *0x00F65CA0, a, c->+40, 1, c + 12, b, x, 0, 0, 0).
DECOMP(00084ABC, Scene_StartFromComp_00084ABC) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 a = load<u32>(c + 32);
    if (a == 0 && load<u32>(c + 36) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = load<u32>(c + 36);
    if (k != 0) {
        const u32 p = static_cast<u32>(call(ctx, kFindComp48, {x, k}));
        if (p != 0 && static_cast<u32>(VcOE48(ctx, p, 32, {p})) == 2) a = static_cast<u32>(VcOE48(ctx, p, 24, {p}));
    }
    u32 b = 0;
    const u32 q = static_cast<u32>(call(ctx, kFindComp48, {x, 0xB2E59A9Fu}));
    if (q != 0 && static_cast<u32>(VcOE48(ctx, q, 32, {q})) == 2) b = static_cast<u32>(VcOE48(ctx, q, 24, {q}));
    const u32 g = load<u32>(kGameRoot48), d = load<u32>(c + 40);
    ret(ctx, static_cast<u32>(call(ctx, 0x0046C3B8u, {g, a, d, 1, c + 12, b, x, 0, 0, 0})));
}

// u32 f(Cmd* c, Owner* x): applies a model command to m = (*0x00F65CA0)->
// +0x1281F8 (r3 = c when there is none). u8 c->+24: Model_Reset31F968(m,
// 0, u8 c->+26 ? c->+20 : 0); else, with u8 +26: m->+52 =
// List_IndexOf_0031F8EC(m, c->+20), +56 = 0, f32 +60 = Table_GetF_0031F8AC(
// m, 0), bit 31 of +68 = (that == 0), bit 30 cleared. Then, with u8 c->+25,
// f32 m->+64 = c->+12, or comp->vt[20](comp) when comp =
// Effect_FindComponent(x, c->+16) answers 1 to vt[32].
DECOMP(0008BE18, Model_ApplyCmd_0008BE18) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 m = load<u32>(load<u32>(kGameRoot48) + 0x001281F8u);
    if (m == 0) {
        ret(ctx, c);
        return;
    }
    if (load<u8>(c + 24) != 0) {
        call(ctx, 0x0031F968u, {m, 0, load<u8>(c + 26) != 0 ? load<u32>(c + 20) : 0u});
    } else if (load<u8>(c + 26) != 0) {
        const u32 i = static_cast<u32>(call(ctx, 0x0031F8ECu, {m, load<u32>(c + 20)}));
        store<u32>(m + 52, i);
        store<u32>(m + 56, 0);
        call(ctx, 0x0031F8ACu, {m, 0});
        const f64 f = ctx.fpr[1];
        store<u32>(m + 60, W48(f));
        u32 w = load<u32>(m + 68) & 0x7FFFFFFFu;
        if (f == Lfs48(0x0008BE10u)) w |= 0x80000000u;  // the constant 0.0f before the function
        store<u32>(m + 68, w & ~0x40000000u);
    }
    if (load<u8>(c + 25) == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 r3 = load<u8>(c + 25);
    u32 v = load<u32>(c + 12);
    const u32 kd = load<u32>(c + 16);
    if (kd != 0) {
        r3 = static_cast<u32>(call(ctx, kFindComp48, {x, kd}));
        if (r3 != 0) {
            const u32 p = r3;
            r3 = static_cast<u32>(VcOE48(ctx, p, 32, {p}));
            if (r3 == 1) {
                r3 = static_cast<u32>(VcOE48(ctx, p, 20, {p}));
                v = W48(ctx.fpr[1]);
            }
        }
    }
    store<u32>(m + 64, v);
    ret(ctx, r3);
}

// T* f(?, XmlCursor* x): new 24-byte record (base Obj_InitVt_B21C98, vtable
// 0x00B10648): +12 = hash 0x8A93973F; when that is absent, the masks +16 /
// +20 collect 1 << s32 0x10EB6B29 of every child 0x00014042 / 0x03D3BE2D.
DECOMP(0008C090, XmlNew_B10648_0008C090) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 m = NewXmlObj47(ctx, 24, x);
    if (m == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + 12, 0);
    store<u32>(m + 16, 0);
    store<u32>(m, 0x00B10648u);
    store<u32>(m + 20, 0);
    if (static_cast<u32>(call(ctx, kReadHash47, {x, 0x8A93973Fu, m + 12, 0})) == 0) {
        Stack fr(ctx, 16);  // sp+112: the s32 (not initialised)
        const u32 v = fr.addr();
        const u32 tags[2] = {0x00014042u, 0x03D3BE2Du};
        for (u32 j = 0; j < 2; ++j) {
            for (u32 e = static_cast<u32>(call(ctx, 0x004F92BCu, {x, tags[j]})); e != 0;
                 e = static_cast<u32>(call(ctx, 0x004F8FC8u, {e, tags[j]}))) {
                call(ctx, kReadS32_48, {e, 0x10EB6B29u, v, 0});
                const u32 bit = Slw48(1, load<u32>(v));
                store<u32>(m + 16 + 4 * j, load<u32>(m + 16 + 4 * j) | bit);
            }
        }
    }
    ret(ctx, m);
}

// u32 f(Obj* o): phase step. e = o->+216 + 8 * o->+724 + 232 (the phase
// table entry). k = o->vt[32](o, 1): 0 -> World_StartScene46C3B8(
// *0x00F65CA0, e->+4, 0, 1, 0, 0, 0, 0, 0, 0) and o->vt[28](o, 1, 1);
// 1 -> unless the u16 (*0x00F65CA0)->+0x128214 is set (r3 = it):
// o->vt[28](o, 1, 2), ++o->+724 and Msg_Post(o, 0xAFEE1340, the hash
// 0x00F646EC[o->+724] (0x997AB20E past 2), 0). Other k: r3 = k.
DECOMP(000BDDA0, Obj_StepPhase_000BDDA0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 e = load<u32>(o + 216) + (load<u32>(o + 724) << 3) + 232;
    const u32 k = static_cast<u32>(VcOE48(ctx, o, 32, {o, 1}));
    if (k == 0) {
        const u32 g = load<u32>(kGameRoot48), a = load<u32>(e + 4);
        call(ctx, 0x0046C3B8u, {g, a, 0, 1, 0, 0, 0, 0, 0, 0});
        ret(ctx, static_cast<u32>(VcOE48(ctx, o, 28, {o, 1, 1})));
        return;
    }
    if (k != 1) {
        ret(ctx, k);
        return;
    }
    const u32 busy = load<u16>(load<u32>(kGameRoot48) + 0x00128214u);
    if (busy != 0) {
        ret(ctx, busy);
        return;
    }
    VcOE48(ctx, o, 28, {o, 1, 2});
    const u32 n = load<u32>(o + 724) + 1;
    store<u32>(o + 724, n);
    const u32 msg = n > 2 ? 0x997AB20Eu : load<u32>(0x00F646ECu + 4 * n);
    ret(ctx, static_cast<u32>(call(ctx, 0x00470660u, {o, 0xAFEE1340u, msg, 0})));
}

// u32 f(Obj* o): refresh. o->vt[68](o) and o->vt[64](o) each give a part
// p (when non-null p->vt[44](p, 1)). When o->+160 has a peer whose +136 is
// set, bit 0x20 of the u8 Peer_GetVec(peer)->+28 = bit 7 of the u8 +320
// of Obj_FindComponent(o, 32, 0) (0 without one). With an effect in
// o->+356: its +136 |= 0x100 and Effect_Release(o->+356, 0xD5E4206F).
// Then the u16 +40 of o->+164 (when set) |= 0x4000. r3 = what the last
// step left.
DECOMP(000E158C, Obj_Refresh_000E158C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 slot : {68u, 64u}) {
        const u32 p = static_cast<u32>(VcOE48(ctx, o, slot, {o}));
        if (p != 0) VcOE48(ctx, p, 44, {p, 1});
    }
    if (load<u32>(o + 160) != 0 && static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) != 0 &&
        load<u32>(static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) + 136) != 0) {
        u32 on = 0;
        const u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 32, 0}));
        if (c != 0) on = (load<u8>(c + 320) >> 7) & 1u;
        const u32 pr = static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)}));
        const u32 v = static_cast<u32>(call(ctx, 0x0057BE98u, {pr}));
        store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (on << 5)));
    }
    u32 r3 = load<u32>(o + 356);
    if (r3 != 0) {
        store<u32>(r3 + 136, load<u32>(r3 + 136) | 0x100u);
        r3 = static_cast<u32>(call(ctx, 0x004CF3C4u, {load<u32>(o + 356), 0xD5E4206Fu}));
    }
    const u32 q = load<u32>(o + 164);
    if (q != 0) {
        r3 = load<u16>(q + 40) | 0x4000u;
        store<u16>(q + 40, static_cast<u16>(r3));
    }
    ret(ctx, r3);
}

// u32 f(Obj* o, Target* t): attach. Unless t is null (r3 = o) or has no
// definition d = Target_GetDef0D951C(t) (r3 = 0): Call_HandleNotify_F64D08
// when bit 0 of o->+188 is set; the part p = o->vt[64](o), when present
// and bit 29 of d->+124 is set, gets p->vt[44](p, 0) and
// Obj_SetPhysicsActive(p, 0); o->vt[28](o, 0, 0), (o, 1, 6), (o, 2, 1);
// then t goes into o's slot table (o->+20)[u16 o->+6 bytes] and ++u16
// t->+178 (r3 = t + 178).
DECOMP(0012D0F0, Obj_AttachTarget_0012D0F0) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    if (t == 0) {
        ret(ctx, o);
        return;
    }
    const u32 d = static_cast<u32>(call(ctx, 0x000D951Cu, {t}));
    if (d == 0) {
        ret(ctx, 0u);
        return;
    }
    if ((load<u32>(o + 188) & 1u) != 0) call(ctx, 0x004902B8u, {1});
    const u32 p = static_cast<u32>(VcOE48(ctx, o, 64, {o}));
    if (p != 0 && (load<u32>(d + 124) & 0x20000000u) != 0) {
        VcOE48(ctx, p, 44, {p, 0});
        call(ctx, 0x00260C14u, {p, 0});
    }
    VcOE48(ctx, o, 28, {o, 0, 0});
    VcOE48(ctx, o, 28, {o, 1, 6});
    VcOE48(ctx, o, 28, {o, 2, 1});
    const u32 a = load<u32>(o + 20) + load<u16>(o + 6);
    const u16 h = load<u16>(t + 178);
    store<u32>(a, t);
    store<u16>(t + 178, static_cast<u16>(h + 1));
    ret(ctx, t + 178);
}

// u32 f(Obj* o; f32 dt): ramp. With s = o->+612, s->+52 and the config
// o->+396 all set (else r3 = o / 0 / s->+52): when s->+28 is null or one
// of its (s8 +32) 224-byte entries from s->+72 has +36 set, the ramp
// resets (o->+964 = 1, f32 +968 = 0, bit 31 of +1088 cleared, r3 = that
// word), first playing Sound_Play44C474(&tmp, o->+924, 0...) and
// Obj_TimedEffect_0023BCA0(o) when bit 31 was set. Otherwise o->+964 = 2
// and f32 +968 = clamp(+968 + dt, 0, cfg->+1012); bit 31 of +1088 (and r3)
// = (the result >= the limit); f1 = the limit.
DECOMP(0023BE10, Obj_RampStep_0023BE10) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 s = load<u32>(o + 612);
    if (s == 0) {
        ret(ctx, o);
        return;
    }
    if (load<u32>(s + 52) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 cfg = load<u32>(o + 396);
    if (cfg == 0) {
        ret(ctx, load<u32>(s + 52));
        return;
    }
    bool busy = false;
    const u32 l = load<u32>(s + 28);
    if (l == 0) {
        busy = true;
    } else {
        const u32 n = static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(l + 32))));
        if (n != 0) {
            u32 p = load<u32>(s + 72), i = 0;
            do {
                if (load<u32>(p + 36) != 0) {
                    busy = true;
                    break;
                }
                ++i;
                p += 224;
            } while (n > i);
        }
    }
    if (busy) {
        u32 w = load<u32>(o + 1088);
        if ((w & 0x80000000u) != 0) {
            Stack fr(ctx, 16);  // sp+144: the sound handle out
            call(ctx, 0x0044C474u, {fr.addr(), load<u32>(o + 924), 0, 0, 0, 0, 0, 0, 0, 0, 0});
            call(ctx, 0x0023BCA0u, {o});
            w = load<u32>(o + 1088);
        }
        store<u32>(o + 964, 1);
        store<u32>(o + 968, 0);
        store<u32>(o + 1088, w & 0x7FFFFFFFu);
        ret(ctx, w & 0x7FFFFFFFu);
        return;
    }
    store<u32>(o + 964, 2);
    const f64 lim = Lfs48(cfg + 944 + 68);
    const f64 f2 = F48(Lfs48(o + 968) + dt);
    const f64 m = f2 < lim ? f2 : lim;
    const f64 lo = Lfs48(0x0023BE0Cu);  // 0.0f before the function
    const f64 v = m > lo ? m : lo;
    const u32 w = load<u32>(o + 1088);
    store<u32>(o + 968, W48(v));
    const u32 r = v >= lim ? 1u : 0u;
    store<u32>(o + 1088, (w & 0x7FFFFFFFu) | (r << 31));
    ctx.fpr[1] = lim;
    ret(ctx, r);
}

// T* f(Doc* d, XmlCursor* x, u8 flag): the name attribute 0x00AD4C0C of x
// (XmlCursor_DocPath(d) and r3 = 0 without one) becomes a 24-byte
// SmallAlloc record: hash = String_HashNoCase(name) at +4, +8 = 0, +12 =
// 6, registered as {hash, rec} in the array 0x01056F04 (grown with
// Array_Reserve when full; +4 high-water mark, bit 0 of +6 = dirty), vtable
// 0x00B1432C, +20 = 0; u8 +16 = flag and +20 = hash 0x88B4A252. A failed
// allocation still writes flag to address 16 and reads into address 20
// (kept).
DECOMP(0027CA10, XmlNamed_New_0027CA10) {
    const u32 d = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u8 flag = arg<u8>(ctx, 2);
    const u32 name = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AD4C0Cu}));
    if (name == 0) {
        call(ctx, 0x004F8944u, {d});
        ret(ctx, 0u);
        return;
    }
    u32 r = 0;
    const u32 m = static_cast<u32>(call(ctx, 0x0055238Cu, {24}));
    if (m != 0) {
        store<u32>(m, 0x00B0DAACu);
        u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {name}));
        store<u32>(m + 8, 0);
        store<u8>(m + 16, 0);
        store<u32>(m + 4, h);
        store<u32>(m + 12, 6);
        constexpr u32 A = 0x01056F04u;
        const u32 cap = load<u16>(A);
        u32 cnt = load<u16>(A + 2);
        if (cnt == cap) {
            call(ctx, 0x00558F3Cu, {A, (cap << 1) & 0xFFFEu});
            h = load<u32>(m + 4);
            cnt = load<u16>(A + 2);
        }
        const u32 data = load<u32>(A + 8);
        store<u32>(data + ((cnt << 3) & 0x7FFF8u), h);
        store<u32>(data + (load<u16>(A + 2) << 3) + 4, m);
        const u32 n = load<u16>(A + 2) + 1;
        const u32 hw = load<u16>(A + 4);
        const u32 fl = load<u16>(A + 6);
        store<u16>(A + 2, static_cast<u16>(n));
        store<u16>(A + 4, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
        store<u16>(A + 6, static_cast<u16>(fl | 1u));
        store<u32>(m, 0x00B1432Cu);
        store<u32>(m + 20, 0);
        r = m;
    }
    store<u8>(r + 16, flag);
    RdHash47(ctx, x, 0x88B4A252u, r + 20);
    ret(ctx, r);
}

// bool f(Ctx* o, Obj* p, s32 i): can p use slot i. Looks up 0xCE33CC21 in
// the map (*0x00F65BE8)->+28 (result unused), then p->+60's model
// (+204) in the map 0x0106819C: without an entry that has both +8 and +12
// String_Format(0x00AD8B88, p->+60->vt[12](...)), r3 = 0. Otherwise the
// entry's u8 +0 picks an 860-byte record of o->+20 whose u8 +824 is the
// group g (255 = none, >= 12 is reported with String_Format(0x00B1D308,
// g, 12)); r3 = 1 when bit (u8 entry +1) of the u16 at (*0x00F65CA0) +
// 4056 * i + 180 + 2 * g is clear.
DECOMP(002E6B8C, Slot_CanUse_002E6B8C) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    Stack fr(ctx, 16);  // sp+112: model key, sp+116: hash key
    const u32 sp = fr.addr();
    const u32 g = load<u32>(kGameRoot48);
    store<u32>(sp + 4, 0xCE33CC21u);
    call(ctx, 0x0073B608u, {load<u32>(0x00F65BE8u) + 28, sp + 4});
    const u32 rec = g + i * 4056u;
    const u32 mdl = load<u32>(load<u32>(p + 60) + 204);
    u32 e = 0;
    if (mdl != 0) {
        store<u32>(sp, mdl);
        e = load<u32>(static_cast<u32>(call(ctx, 0x0076642Cu, {0x0106819Cu, sp})));
        if (e != 0 && load<u32>(e + 8) != 0 && load<u32>(e + 12) != 0) e = 0;
    }
    if (e == 0) {
        const u32 q = load<u32>(p + 60);
        const u32 nm = static_cast<u32>(VcOE48(ctx, q, 12, {q}));
        call(ctx, 0x005534D0u, {0x00AD8B88u, nm});
        ret(ctx, 0u);
        return;
    }
    const u32 r = load<u32>(o + 20) + load<u8>(e) * 860u;
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 grp = load<u8>(r + 824);
    if (grp == 255) {
        ret(ctx, 0u);
        return;
    }
    if (grp >= 12) {
        call(ctx, 0x005534D0u, {0x00B1D308u, grp, 12});
        ret(ctx, 0u);
        return;
    }
    const u32 bit = Slw48(1, load<u8>(e + 1));
    ret(ctx, (load<u16>(rec + ((grp << 1) & 0x1FEu) + 180) & bit) != 0 ? 0u : 1u);
}

// u32 f(Owner* o, XmlCursor* x, u16 kind): reads a key/value block into
// the next record b of o's pool (*o - 152: u16 +146 entries of 132 bytes
// from +152, each u16 +0 element count): u16 b->+0 = kind, +2 = 8, +4 = 8 +
// 4 * kind; for every child 0x00ADD588 of x with the attribute 0x00ADD590:
// key[i] (at b + +2) = String_HashNoCase(attr), val[i] (at b + +4) = f32
// 0x04DA9951 (default *0x0038CF94). r3 = 0 when a child lacks the
// attribute, else 1; f1 = the last value.
DECOMP(0038CF98, Pool_ReadKeyVals_0038CF98) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2);
    const u32 base = load<u32>(o) - 152;
    const u32 e = load<u16>(base + 146) * 132u;
    const u32 c = load<u16>(base + e + 152);
    const u32 b = base + e + (((c * 3u) * 4u + 23u) & ~15u) + 152;
    store<u16>(b, static_cast<u16>(kind));
    store<u16>(b + 2, 8);
    store<u16>(b + 4, static_cast<u16>(((kind << 2) & 0x3FFFCu) + 8));
    u32 n = static_cast<u32>(call(ctx, 0x004F936Cu, {x, 0x00ADD588u}));
    if (n == 0) {
        ret(ctx, 1u);
        return;
    }
    const f64 def = Lfs48(0x0038CF94u);
    Stack fr(ctx, 16);  // sp+112: the f32
    const u32 v = fr.addr();
    u32 i = 0;
    do {
        const u32 a = static_cast<u32>(call(ctx, 0x004F9628u, {n, 0x00ADD590u}));
        if (a == 0) {
            ret(ctx, 0u);
            return;
        }
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {a}));
        store<u32>(b + load<u16>(b + 2) + ((i << 2) & 0x3FFFCu), h);
        callf(ctx, 0x004F9B20u, {n, 0x04DA9951u, v}, {def});
        Cp48(b + load<u16>(b + 4) + (i << 2), v);
        ctx.fpr[1] = Lfs48(v);
        i = (i + 1) & 0xFFFFu;
        n = static_cast<u32>(call(ctx, 0x004F9078u, {n, 0x00ADD588u}));
    } while (n != 0);
    ret(ctx, 1u);
}

// u32 f(Cmd* c): run a queued command on the target c->+12: 0xFD81 flushes
// the list (*0x00F65BE8) (List_FlushPending_00454DBC with u8 c->+16, else
// 0x00454E88; r3 = its result). Otherwise the target o is looked up in
// the map (*0x00F65BE8)->+4 (Hash_ToString(c->+12) on a miss); with o:
// Pending_Flush_00453578(o, u8 c->+18) when u8 +17; u8 +16 ?
// (Obj_FlushPending_0045477C(o) when u8 +20, o->vt[24](o), and with u8
// +19 Obj_Tick4536C0(o; *0x0042E6B8) keeping the u8 (*0x00F65BE8)->+0) :
// o->vt[28](o, 0). Always ends with r3 = Flag_Set8_40000000(c).
DECOMP(0042E6C0, Cmd_RunOnTarget_0042E6C0) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 h = load<u32>(c + 12);
    constexpr u32 L = 0x00F65BE8u;
    if (h == 0xFD81u) {
        ret(ctx, static_cast<u32>(call(ctx, load<u8>(c + 16) != 0 ? 0x00454DBCu : 0x00454E88u, {load<u32>(L)})));
        return;
    }
    if (h != 0) {
        Stack fr(ctx, 16);  // sp+112: the key
        store<u32>(fr.addr(), h);
        const u32 o = load<u32>(static_cast<u32>(call(ctx, 0x00733F8Cu, {load<u32>(L) + 4, fr.addr()})));
        if (o == 0) call(ctx, kHashStr48, {load<u32>(c + 12)});
        if (o != 0) {
            if (load<u8>(c + 17) != 0) call(ctx, 0x00453578u, {o, load<u8>(c + 18)});
            if (load<u8>(c + 16) == 0) {
                VcOE48(ctx, o, 28, {o, 0});
            } else {
                if (load<u8>(c + 20) != 0) call(ctx, 0x0045477Cu, {o});
                VcOE48(ctx, o, 24, {o});
                if (load<u8>(c + 19) != 0) {
                    const u8 keep = load<u8>(load<u32>(L));
                    callf(ctx, 0x004536C0u, {o}, {Lfs48(0x0042E6B8u)});
                    store<u8>(load<u32>(L), keep);
                }
            }
        }
    }
    ret(ctx, static_cast<u32>(call(ctx, 0x0042CA60u, {c})));
}

// bool f(x): collects into a stack array (cap 32, stride 16, storage from
// the default allocator's vt[0], Array_Init; +16 = 0, +20 = 0x00F39AC8)
// with 0x004415D8(&arr, x), Mem_FreeFrom(p, 0) for every non-null pointer
// at element +4, then frees the storage (allocator->vt[8](allocator,
// data, cap * stride / 2)) and Array_Destruct; r3 = 1.
DECOMP(0044F6A8, Collect_FreeAll_0044F6A8) {
    const u32 x = arg<u32>(ctx, 0);
    Stack fr(ctx, 32);  // sp+112: the array (24 bytes)
    const u32 A = fr.addr();
    store<u16>(A, 32);
    store<u16>(A + 2, 0);
    store<u16>(A + 4, 0);
    store<u32>(A + 8, 0);
    store<u16>(A + 6, 16);
    const u32 al = static_cast<u32>(call(ctx, 0x00550EE8u));
    store<u32>(A + 12, al);
    const u32 cap = load<u16>(A);
    if (cap != 0) {
        const u32 sz = cap * ((load<u16>(A + 6) >> 1) & 0x7FFFu);
        store<u32>(A + 8, static_cast<u32>(VcEO48(ctx, load<u32>(al), 0, {al, sz, 0, 16})));
    }
    call(ctx, 0x00558F38u, {A});
    store<u32>(A + 20, 0x00F39AC8u);
    store<u32>(A + 16, 0);
    call(ctx, 0x004415D8u, {A, x});
    const s32 n = static_cast<s32>(load<u16>(A + 2));
    for (s32 i = 0; i < n; ++i) {
        const u32 p = load<u32>(load<u32>(A + 8) + 8 * static_cast<u32>(i) + 4);
        if (p != 0) call(ctx, 0x0054EE6Cu, {p, 0});
    }
    const u32 sz = load<u16>(A) * ((load<u16>(A + 6) >> 1) & 0x7FFFu);
    store<u16>(A + 2, 0);
    const u32 data = load<u32>(A + 8);
    if (data != 0) {
        const u32 a2 = load<u32>(A + 12);
        VcOE48(ctx, a2, 8, {a2, data, sz});
    }
    call(ctx, 0x0054EC94u, {A});
    ret(ctx, 1u);
}

// u32 f(Sys* s): reset. For both 11968-byte halves h: the 172-byte block
// at h + 176 gets u8 +4 = 255, +24 = 0, the word +4 = 0xFF000000 | (+4 &
// 0x3FFF), the vtable 0x00B219C0, zeros at +8..+28, the vec pairs
// 0x01164AFC (+32 / +40), the vec4 0x01164C90 (+48), the vec3 0x01164B70
// (+64), +11600 / +11604 = 0, and two 172-byte memsets at +11616 / +11788;
// then memset(s, 0, 24144) (r3 = s).
DECOMP(00788594, Sys_Reset_00788594) {
    const u32 s = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 2; ++i) {
        u32 b = s + 11968 * i + 176;
        const u32 w = load<u32>(b + 4);
        store<u8>(b + 4, 255);
        store<u32>(b + 24, 0);
        store<u32>(b + 4, (w & 0x00003FFFu) | 0xFF000000u);
        Cp48(b + 32, 0x01164AFCu);
        Cp48(b + 36, 0x01164AFCu + 4);
        Cp48(b + 40, 0x01164AFCu);
        store<u32>(b, 0x00B219C0u);
        Cp48(b + 44, 0x01164AFCu + 4);
        store<u32>(b + 8, 0);
        Cp48(b + 48, 0x01164C90u);
        store<u32>(b + 12, 0);
        Cp48(b + 52, 0x01164C90u + 4);
        store<u32>(b + 16, 0);
        store<u32>(b + 20, 0);
        Cp48(b + 56, 0x01164C90u + 8);
        store<u32>(b + 28, 0);
        Cp48(b + 60, 0x01164C90u + 12);
        Cp48(b + 64, 0x01164B70u);
        store<u32>(b + 11600, 0);
        Cp48(b + 68, 0x01164B70u + 4);
        store<u32>(b + 11604, 0);
        Cp48(b + 72, 0x01164B70u + 8);
        for (u32 j = 0; j < 2; ++j, b += 172) call(ctx, 0x00726E54u, {b + 11616, 0, 172});
    }
    ctx.fpr[1] = Lfs48(0x01164B70u + 8);  // f1 = the last lfs
    ret(ctx, static_cast<u32>(call(ctx, 0x00726E54u, {s, 0, 24144})));
}

// s32 f(SortedArray* s, const u32* key): binary search of the 28-byte
// elements (+8 data, u16 +2 count), re-sorting first with qsort(data, n,
// 28, s->+40) when bit 0 of u16 +6 (dirty) is set. With the default
// comparator 0x00F3A030 the u32 element +0 is compared with *key
// (unsigned); otherwise s->+40 is called on (element, &probe), probe being
// a stack element {*key, 0, 0, -FLT_MAX-ish 0xFF7FFFFF, 0, 0, 0}. r3 =
// the index found or -1.
DECOMP(0078987C, SortedArray_Find_0078987C) {
    const u32 s = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 f6 = load<u16>(s + 6);
    if ((f6 & 1u) != 0) {
        const u32 n = load<u16>(s + 2);
        store<u16>(s + 6, static_cast<u16>(f6 & 0xFFFEu));
        if (static_cast<s32>(n) > 0) call(ctx, 0x0071EC28u, {load<u32>(s + 8), n, 28, load<u32>(s + 40)});
    }
    s32 hi = static_cast<s32>(load<u16>(s + 2)) - 1, lo = 0;
    const u32 cmp = load<u32>(s + 40);
    const u32 data = load<u32>(s + 8);
    if (cmp == 0x00F3A030u) {
        while (hi >= lo) {
            const s32 mid = (hi + lo) >> 1;
            const u32 k = load<u32>(key);
            const u32 v = load<u32>(data + static_cast<u32>(mid) * 28u);
            if (v > k) hi = mid - 1;
            else if (v < k) lo = mid + 1;
            else {
                ret(ctx, mid);
                return;
            }
        }
        ret(ctx, -1);
        return;
    }
    Stack fr(ctx, 32);  // sp+112: the probe element
    const u32 pr = fr.addr();
    store<u32>(pr + 4, 0);
    store<u32>(pr + 8, 0);
    store<u32>(pr + 12, 0xFF7FFFFFu);
    store<u8>(pr + 16, 0);
    const u32 k = load<u32>(key);
    store<u32>(pr + 20, 0);
    store<u32>(pr + 24, 0);
    store<u32>(pr, k);
    while (hi >= lo) {
        const s32 mid = (hi + lo) >> 1;
        const u32 opd = load<u32>(s + 40);
        const s32 r = static_cast<s32>(static_cast<u32>(VcSeq48(ctx, opd, {data + static_cast<u32>(mid) * 28u, pr}, load<u32>(opd), opd)));
        if (r > 0) hi = mid - 1;
        else if (r < 0) lo = mid + 1;
        else {
            ret(ctx, mid);
            return;
        }
    }
    ret(ctx, -1);
}

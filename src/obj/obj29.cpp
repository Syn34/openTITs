// Game code, part 29: the continued sweep of small still-recompiled game
// functions (in size order, from 45 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where a known caller reads a volatile register the
// original leaves behind (regflow liveout), the value is set the same way,
// as the comments say.
#include "obj29.h"

using namespace sly4;
using namespace sly4::obj29;

// ---------------------------------------------------------------- sweep: 45 instructions
// void f(T* o, u8 b): o->vt[388](o), o->vt[28](o, 0, b), the u8
// o->+440[o->+444] = b, then o->vt[384](o)
DECOMP(00190054, Obj_SetIndexedByte_00190054) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    VCall(ctx, o, 388, {o});
    VCall(ctx, o, 28, {o, 0, b});
    store<u8>(o + load<u32>(o + 444) + 440, static_cast<u8>(b));
    VCall(ctx, o, 384, {o});
}

// void f(T* o, u8 b): when b, the children +372 and +368 (when set) get
// vt[8]() and are cleared; then 0x000D74C0(o, b)
DECOMP(001A60FC, Obj_ReleasePair_001A60FC) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) {
        for (u32 off : {372u, 368u}) {
            const u32 c = load<u32>(o + off);
            if (c == 0) continue;
            VCall(ctx, c, 8, {c});
            store<u32>(o + off, 0);
        }
    }
    ctx.gpr[3] = call(ctx, 0x000D74C0, {o, b});
}

// void f(T* a, U* b): 0x000E44C0(a, b), then b->+236 = a->+268, the seven
// words b->+208.. = a->+240.. (raw f32 copies) and the field 0xFFFE0000 of
// b->+20 = 4 (r3 = 4, f1 = the last word copied)
DECOMP(001AE3B4, Obj_CopyParams_001AE3B4) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    store<u32>(b + 236, load<u32>(a + 268));
    u32 w = 0;
    for (u32 k = 0; k < 7; ++k) {
        w = load<u32>(a + 240 + 4 * k);
        store<u32>(b + 208 + 4 * k, w);
    }
    store<u32>(b + 20, (load<u32>(b + 20) & 0x1FFFFu) | (4u << 17));
    ctx.gpr[3] = 4;
    ctx.fpr[1] = F32Bits28(w);
}

// T::~T() deleting (vtables 0x00B16B48 / +380 0x00B16D1C / +384
// 0x00B16D44): the PtrVectors +576 and +556, the base 0x001AE468(o), then
// SmallFree(o, 608)
DECOMP(001C345C, VtB16B48_Delete_001C345C) { DeleteB16B48_29(ctx, arg<u32>(ctx, 0)); }

// void f(T* o, u8 b): when b and the child +496 is set, child->vt[40](child,
// 0), then (reloaded) vt[40](child, 1) when bit 0x40000 of its +172 is set;
// then 0x000E4544(o, b)
DECOMP(001F6634, Obj_ToggleChild_001F6634) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0 && load<u32>(o + 496) != 0) {
        const u32 c = load<u32>(o + 496);
        VCall(ctx, c, 40, {c, 0});
        const u32 c2 = load<u32>(o + 496);
        if ((load<u32>(c2 + 172) & 0x40000u) != 0) VCall(ctx, c2, 40, {c2, 1});
    }
    ctx.gpr[3] = call(ctx, 0x000E4544, {o, b});
}

// The hit handler: when Obj_TestE0BCC(o, h) and h->+52 * the f32
// 0x01054D00->+72 > *0x002089A8, r = o->vt[352](o, -, h->+4, 0xA77EB5E9;
// that product) and bit 0x100 of the u16 h->+14 = (r == 0) (r3 =
// cntlzw(r); 0x01054D00 when not above)
DECOMP(002089B0, Obj_OnHitScaled_002089B0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (static_cast<u32>(call(ctx, 0x000E0BCC, {o, h})) == 0) return;
    const f32 v = Mul28(LfsAt(h + 52), LfsAt(0x01054D00 + 72));
    ctx.fpr[1] = v;
    if (!(v > LfsAt(0x002089A8))) {
        ctx.gpr[3] = 0x01054D00;
        return;
    }
    const u32 r = static_cast<u32>(VCallOpdR4_28(ctx, o, 352, load<u32>(h + 4), 0xA77EB5E9u));
    const u32 z = static_cast<u32>(std::countl_zero(r));
    store<u16>(h + 14, static_cast<u16>((load<u16>(h + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// void f(Color* out, T* o, s32 i): the color o->+64[i] (48-byte records;
// four copies of *0x00264E90 for i = -1), or o->+16 when the color equals
// the 16 bytes at 0x01164C90. Quirk kept: the original compares the stack
// slot it is about to write the color to before writing it (a strict
// aliasing reorder), so it really compares what the slot held before.
DECOMP(00264E98, Color_Pick_00264E98) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = arg<s32>(ctx, 2);
    u32 c[4];
    const u32 k = load<u32>(0x00264E90);
    for (u32& w : c) w = k;
    if (i != -1) {
        const u32 p = load<u32>(o + 64) + static_cast<u32>(i) * 48u;
        for (u32 j = 0; j < 4; ++j) c[j] = load<u32>(p + 4 * j);
    }
    const u32 slot = static_cast<u32>(ctx.gpr[1]) - 16;  // 48(r1) in the original's 64-byte frame
    const u64 s0 = load<u64>(slot), s1 = load<u64>(slot + 8);
    for (u32 j = 0; j < 4; ++j) store<u32>(slot + 4 * j, c[j]);
    if (s0 == load<u64>(0x01164C90) && s1 == load<u64>(0x01164C98))
        for (u32 j = 0; j < 4; ++j) c[j] = load<u32>(o + 16 + 4 * j);
    for (u32 j = 0; j < 4; ++j) store<u32>(out + 4 * j, c[j]);
    ctx.fpr[1] = F32Bits28(c[0]);
}

// void f(T* o, ?, u8 scaled, u32 name; f32 t): the track Anim_Find34CC34(
// o->+36, name) (when o->+36 is set) gets Obj_SetF32Via_00354150(track; t),
// with t = Track_Length353748(track) * t (when above *0x00276A18, else
// *0x00276A1C) when scaled
DECOMP(00276A20, Anim_SetTrackTime_00276A20) {
    const u32 o = arg<u32>(ctx, 0), scaled = arg<u32>(ctx, 2), name = arg<u32>(ctx, 3);
    f64 t = farg(ctx, 0);
    const u32 a = load<u32>(o + 36);
    if (a == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 tr = static_cast<u32>(call(ctx, 0x0034CC34, {a, name}));
    if (tr == 0) return;
    if ((scaled & 0xFFu) != 0) {
        call(ctx, 0x00353748, {tr});
        const f64 len = ctx.fpr[1];
        t = len > LfsAt(0x00276A18) ? static_cast<f64>(Mul28(len, t)) : LfsAt(0x00276A1C);
    }
    callf(ctx, 0x00354150, {tr}, {t});
}

// void f(T* o): for each bit i < 31 set in o->+24, the 2-bit field i of the
// words at o->+68 becomes 1 when its low bit is set, else 3; then o->+24 = 0
DECOMP(0027C290, Obj_PromoteBits_0027C290) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 31; ++i) {
        if ((load<u32>(o + 24) & (1u << i)) == 0) continue;
        const u32 ea = o + 68 + ((i >> 2) & ~3u);
        const u32 sh = (i << 1) & 30u;
        const u32 v = (load<u32>(ea) & (1u << sh)) == 0 ? 3u : 1u;
        store<u32>(ea, (load<u32>(ea) & ~(3u << sh)) | (v << sh));
    }
    store<u32>(o + 24, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o, u32 v): for each of the o->+28 records (220 bytes at o->+24)
// with a material +12, Material_ParamSlot(m, 2) = (f32)v (r3 = the count;
// f1 = the conversion temporary)
DECOMP(00287260, Mesh_SetParam2_00287260) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    u32 n = load<u32>(o + 28);
    if (n == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f32 f = static_cast<f32>(static_cast<f64>(v));
    ctx.fpr[1] = std::bit_cast<f64>(static_cast<u64>(v));
    for (u32 i = 0; i < n; ++i) {
        const u32 m = load<u32>(load<u32>(o + 24) + i * 220 + 12);
        if (m == 0) continue;
        const u32 slot = static_cast<u32>(call(ctx, o29fn::Material_ParamSlot, {m, 2}));
        store<f32>(slot, f);
        n = load<u32>(o + 28);
    }
    ctx.gpr[3] = n;
}

// bool f(T* o, U* p): p is set, p's id (p->+216->+52, 0 without) is in the
// u32 list o->+44 (u16 count +52; an empty list matches) and the s32 o->+64
// < the u16 o->+32
DECOMP(002B91EC, Obj_CanAccept_002B91EC) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    bool found = true;
    if (p != 0) {
        const u32 n = load<u16>(o + 52);
        if (n != 0) {
            found = false;
            const u32 list = load<u32>(o + 44);
            for (u32 i = 0; i < n; ++i) {
                const u32 e = load<u32>(list + 4 * i);
                const u32 q = load<u32>(p + 216);
                if (e == (q != 0 ? load<u32>(q + 52) : 0u)) {
                    found = true;
                    break;
                }
            }
        }
    }
    ctx.gpr[3] = (p != 0 && found && static_cast<s32>(load<u32>(o + 64)) < static_cast<s32>(load<u16>(o + 32))) ? 1 : 0;
}

// void f(T* a, u32 id, u8 b, u8 c): Loader_Start2DEA50(a, the world entry
// whose +2560 is id (from the list kWorldPtr->* + 4208 + 80, u16 count +88;
// 0 for none or id 0), b, c)
DECOMP(002E56F4, Loader_StartById_002E56F4) {
    const u32 a = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    const u32 b = arg<u32>(ctx, 2) & 0xFFu, c = arg<u32>(ctx, 3) & 0xFFu;
    u32 found = 0;
    if (id != 0) {
        const u32 w = load<u32>(kWorldPtr) + 4208;
        const u32 n = load<u16>(w + 88);
        for (u32 i = 0; i < n; ++i) {
            const u32 e = load<u32>(load<u32>(w + 80) + 4 * i);
            if (load<u32>(e + 2560) == id) {
                found = e;
                break;
            }
        }
    }
    ctx.gpr[3] = call(ctx, 0x002DEA50, {a, found, b, c});
}

// T::~T() (vtable 0x00B1DD80, then 0x00B1DCA8): the member +84
// (0x004A0E08), the owned 140-byte effect +40 (Effect_Destruct, SmallFree,
// cleared), the PtrVector +44, then the base 0x00464AD0(o)
DECOMP(00312BD4, VtB1DD80_Dtor_00312BD4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1DD80);
    call(ctx, o29fn::VtB21B58_DtorTrivial, {o + 84});
    const u32 e = load<u32>(o + 40);
    store<u32>(o, 0x00B1DCA8);
    if (e != 0) {
        call(ctx, 0x004C8E80, {e});  // Effect_Destruct
        call(ctx, o19fn::SmallFree, {e, 140});
        store<u32>(o + 40, 0);
    }
    VecDtor(ctx, o + 44);
    ctx.gpr[3] = call(ctx, 0x00464AD0, {o});
}

// T* f(?, XmlCursor* c, U* owner): new 2576-byte object (base
// 0x00769BD8(p, owner), vtable 0x00B1E7B0) whose +2572 is 58 when the hash
// 0x63C971C0 reads 0xFEE2E46A, else 0
DECOMP(0032ED6C, NewXmlObj_0032ED6C) {
    const u32 c = arg<u32>(ctx, 1), owner = arg<u32>(ctx, 2);
    u32 k = 0;
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), 0);
        call(ctx, o29fn::XmlCursor_ReadHash, {c, 0x63C971C0, frame.addr(), 0});
        if (load<u32>(frame.addr()) == 0xFEE2E46Au) k = 58;
    }
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {2576}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x00769BD8, {p, owner});
    store<u32>(p + 2572, k);
    store<u32>(p, 0x00B1E7B0);
    ctx.gpr[3] = p;
}

// void f(): under the lwmutex 0x0106BFC8, frees the object 0x0106BFE0
// (Obj_ClearField8 first) and the block 0x00F64CA0, clearing both
DECOMP(003550BC, Globals_Free_003550BC) {
    constexpr u32 kMutex = 0x0106BFC8;
    call(ctx, 0x00ABF0E4, {kMutex, 100000});  // sys_lwmutex_lock
    const u32 q = load<u32>(0x0106BFE0);
    if (q != 0) {
        call(ctx, 0x00565A88, {q});  // Obj_ClearField8
        call(ctx, o19fn::Mem_FreeFrom, {q, 0});
        store<u32>(0x0106BFE0, 0);
    }
    const u32 g = load<u32>(0x00F64CA0);
    if (g != 0) {
        call(ctx, o19fn::Mem_FreeFrom, {g, 0});
        store<u32>(0x00F64CA0, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00ABF104, {kMutex});  // sys_lwmutex_unlock
}

// void f(T* a, U* b): bit 0x10 of the u8 b->+12->+244->+305 (when set) =
// bit 0 of the node table byte at node(a->+4) + *0x0106CF70 + 4 * the byte
// at node(a->+2) + *0x0106CF7E (nodes: 128-byte records at kNodeTable->+24;
// 0 for -1; the second address is 0 when node(a->+2)'s u8 +2 is 0 -- quirk
// kept: a null node is read at 2)
DECOMP(00395348, Node_CopyFlag_00395348) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 q = load<u32>(load<u32>(b + 12) + 244);
    ctx.gpr[3] = 0;
    if (q == 0) return;
    const u32 tbl = load<u32>(0x00F65038);
    const s32 i4 = static_cast<s16>(load<u16>(a + 4));
    const u32 n4 = i4 != -1 ? load<u32>(tbl + 24) + static_cast<u32>(i4) * 128u : 0u;
    const s32 i2 = static_cast<s16>(load<u16>(a + 2));
    const u32 n2 = i2 != -1 ? load<u32>(tbl + 24) + static_cast<u32>(i2) * 128u : 0u;
    const u32 e = load<u8>(n2 + 2) != 0 ? n2 + load<u16>(0x0106CF7E) : 0u;
    const u32 off = load<u16>(0x0106CF70) + (static_cast<u32>(load<u8>(e)) << 2);
    const u32 bit = load<u8>(n4 + off) & 1u;
    store<u8>(q + 305, static_cast<u8>((load<u8>(q + 305) & ~0x10u) | (bit << 4)));
}

// T::~T() deleting (vtable 0x00B20338): the nine 36-byte members from +132
// to +420 (0x004A0E08, last first), the PtrVector +112, the base
// 0x00490B34(o), then SmallFree(o, 480)
DECOMP(003B06C8, VtB20338_Delete_003B06C8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B20338);
    for (u32 k = 0; k < 9; ++k) call(ctx, o29fn::VtB21B58_DtorTrivial, {o + 420 - 36 * k});
    VecDtor(ctx, o + 112);
    call(ctx, 0x00490B34, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 480});
}

// bool Load(IffReader* r): the 65-byte table 0x010BE708 cleared once (flag
// byte 0x010BE700), a u8 read into a stack local and dropped, then
// table[1..64] = 64 FileReader_ReadByte(r, &table[1 + i], 0). Returns 1.
DECOMP(0048C0A0, Table_Load64_0048C0A0) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(0x010BE700) == 0) {
        call(ctx, o29fn::Memset, {0x010BE708, 0, 65});
        store<u8>(0x010BE700, 1);
    }
    {
        Stack frame(ctx, 16);
        call(ctx, o29fn::Iff_ReadU8, {r, frame.addr(), 0});
    }
    for (u32 i = 0; i < 64; ++i) call(ctx, o29fn::FileReader_ReadByte, {r, 0x010BE709 + i, 0});
    ctx.gpr[3] = 1;
}

// T::~T() deleting (vtable 0x00B138E8): the PtrVector +180, the base
// (0x00B12260, owned vector +16), then SmallFree(o, 208)
DECOMP(007470CC, VtB138E8_Delete_007470CC) { DeleteVecVecPtr29(ctx, 0x00B138E8, 180, o19fn::SmallFree, 208); }

// T::~T() of the 0x00B12890 class: see DtorB12890_29
DECOMP(007476B0, VtB12890_Dtor_007476B0) { DtorB12890_29(ctx); }

// The same body at a second address (another class with the same layout)
DECOMP(00747CD4, VtB12890_Dtor_00747CD4) { DtorB12890_29(ctx); }

// void f(Spring* s, const f32* target, ?, const f32* k; f32 dt): one RK4
// step of the damped spring x'' = (target - x) * k[0] - v * k[1] on s->+0
// (x) / +4 (v), with the constants at 0x00748F08 (written as the
// original's single-precision sequence)
DECOMP(00748F18, Spring_StepRK4_00748F18) {
    const u32 s = arg<u32>(ctx, 0), pa = arg<u32>(ctx, 1), pb = arg<u32>(ctx, 3);
    const f64 dt = farg(ctx, 0);
    constexpr u32 K = 0x00748F08;
    const f64 f2 = LfsAt(pa), k0 = LfsAt(K), f4 = LfsAt(s + 4), k1 = LfsAt(K + 4);
    const f64 f6 = LfsAt(pb), f7 = LfsAt(pb + 4), k2 = LfsAt(K + 8), k3 = LfsAt(K + 12);
    f64 f8 = Madd28(f4, k0, f2);
    const f64 f3a = Madd28(k0, k0, f4);
    f64 f5 = Mul28(dt, k1);
    f8 = Mul28(f8, f6);
    f64 f9 = Madd28(f3a, f5, f2);
    f9 = Mul28(f9, f6);
    f8 = Nmsub29(f3a, f7, -f8);
    f9 = -f9;
    const f64 f10a = Madd28(f8, f5, f4);
    f64 f12 = Madd28(f10a, f5, f2);
    f9 = Nmsub29(f10a, f7, f9);
    f12 = Mul28(f12, f6);
    f5 = Madd28(f9, f5, f4);
    f12 = -f12;
    f64 g2 = Madd28(f5, dt, f2);
    const f64 f10 = Add28(f10a, f5);
    f5 = Nmsub29(f5, f7, f12);
    g2 = Mul28(g2, f6);
    f64 f3 = Madd28(f10, k2, f3a);
    const f64 g6 = Madd28(f5, dt, f4);
    g2 = -g2;
    f5 = Add28(f9, f5);
    f3 = Add28(f3, g6);
    g2 = Nmsub29(g6, f7, g2);
    f5 = Madd28(f5, k2, f8);
    f3 = static_cast<f32>(f3 / k3);
    g2 = Add28(f5, g2);
    g2 = static_cast<f32>(g2 / k3);
    store<f32>(s + 4, Madd28(dt, g2, f4));
    const f32 x = Madd28(dt, f3, LfsAt(pa));
    store<f32>(s, x);
    ctx.fpr[1] = x;
}

// T::~T() deleting (vtable 0x00B15008): the PtrVector +180, the base
// (0x00B12260, owned vector +16), then SmallFree(o, 208)
DECOMP(00750F94, VtB15008_Delete_00750F94) { DeleteVecVecPtr29(ctx, 0x00B15008, 180, o19fn::SmallFree, 208); }

// T::~T() deleting (vtable 0x00B1A758): the PtrVector +188, the base
// (0x00B12260, owned vector +16), then Mem_FreeFrom(o, 0)
DECOMP(0075B730, VtB1A758_Delete_0075B730) { DeleteVecVecPtr29(ctx, 0x00B1A758, 188, o19fn::Mem_FreeFrom, 0); }

// void f(T* o, u32 rgb): o->vt[56](o, (rgb << 8) | the alpha byte of the
// color o->vt[52] returns (into a stack local))
DECOMP(00795AB4, Obj_SetRgbKeepAlpha_00795AB4) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    u32 alpha;
    {
        Stack frame(ctx, 16);
        VCall(ctx, o, 52, {frame.addr(), o});
        alpha = load<u8>(frame.addr() + 3);
    }
    // r6 / r7 = the vtable and ((alpha << 24) | rgb) >> 8, as the original
    // leaves them
    const u32 w = ((v & 0x00FFFFFFu) << 8) | alpha;
    const u32 r7 = ((alpha << 24) | (v & 0x00FFFFFFu)) >> 8;
    const u32 vt = load<u32>(o);
    const u32 opd = load<u32>(vt + 56);
    call_ptr(ctx, opd, {o, w, opd, vt, r7});
}

// ---------------------------------------------------------------- sweep: 46 instructions
// void f(Vec3* out, T* o): out = o->+256->vt[80](out, it) when o->+256 is
// set and is not the world's current object (kWorldPtr->* + 4208 + 8 with
// flag 0x2 of +192); else the vector *0x00F557F0 + 48 (raw copy; f1 = z)
DECOMP(000137C4, Obj_GetFocusPos_000137C4) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 256);
    if (c != 0) {
        const u32 s = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
        const u32 cur = (s != 0 && (load<u32>(s + 192) & 0x2u) != 0) ? s : 0u;
        if (c != cur) {
            VCall(ctx, c, 80, {out, c});
            return;
        }
    }
    CopyVec3F28(ctx, out, load<u32>(0x00F557F0) + 48);
}

// void f(T* o, u8 b): when o->vt[68](o) is set, (again) its vt[44](it, b
// == 0); then 0x00124598(o, b)
DECOMP(0001CB78, Obj_SetSubFlag_0001CB78) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (static_cast<u32>(VCall(ctx, o, 68, {o})) != 0) {
        const u32 q = static_cast<u32>(VCall(ctx, o, 68, {o}));
        VCall(ctx, q, 44, {q, b == 0 ? 1u : 0u});
    }
    ctx.gpr[3] = call(ctx, 0x00124598, {o, b});
}

// u8 f(T* o, U* scope): v = 32 when the u8 o->+16 is set, else 0; the
// object Effect_Bind4BB7E0(scope, o->+12, 0) and (when the u8 o->+17 is
// set) its children (+272, then the siblings +268) of kind 10 ((+4 >> 10)
// & 0x3F) get the u8 +208->+112->+69 = v. Returns v.
DECOMP(00099A44, Obj_SetKind10Flag_00099A44) {
    const u32 o = arg<u32>(ctx, 0), scope = arg<u32>(ctx, 1);
    u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {scope, load<u32>(o + 12), 0}));
    const u32 v = load<u8>(o + 16) != 0 ? 32u : 0u;
    bool first = true;
    while (e != 0) {
        if (((load<u32>(e + 4) >> 10) & 0x3Fu) == 10) {
            const u32 p = load<u32>(e + 208);
            if (p != 0) {
                const u32 q = load<u32>(p + 112);
                if (q != 0) store<u8>(q + 69, static_cast<u8>(v));
            }
        }
        if (load<u8>(o + 17) == 0) break;
        e = load<u32>(e + (first ? 272u : 268u));
        first = false;
    }
    ctx.gpr[3] = v;
}

// void Cmd::Run(): CmdList_Push_0042C138(a new 52-byte command
// 0x0042F3BC(p, +12; the f32s +16..+40, then (stack) the s32 +44 and the
// u8s +48, +49, +50), or 0 when the allocation fails)
DECOMP(0009DAE8, Cmd_PushNew_0009DAE8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {52}));
    if (p != 0) {
        const u64 s = Sx(load<u32>(o + 44));
        const u32 b0 = load<u8>(o + 48), b1 = load<u8>(o + 49), b2 = load<u8>(o + 50);
        // the floats take the GPR slots 3..9, so the integers after them go in
        // the parameter save area (the original also leaves them in r5..r8)
        callf(ctx, 0x0042F3BC, {p, load<u32>(o + 12), s, b0, b1, b2, ctx.gpr[9], ctx.gpr[10], 0, s, b0, b1, b2},
              {LfsAt(o + 16), LfsAt(o + 20), LfsAt(o + 24), LfsAt(o + 28), LfsAt(o + 32), LfsAt(o + 36), LfsAt(o + 40)});
    }
    ctx.gpr[3] = call(ctx, 0x0042C138, {p});
}

// T* f(?, XmlCursor* c): new 16-byte command (AnimCmd_ConstructBase,
// vtable 0x00B11648) whose s32 +12 is read from 0x368328E5 (-1 becomes 0)
DECOMP(000A2D5C, NewXmlS32_000A2D5C) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::Mem_Alloc, {16, 0, 16}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, o28fn::AnimCmd_ConstructBase, {p});
    store<u32>(p + 12, 0);
    store<u32>(p, 0x00B11648);
    u32 v;
    {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), 0xFFFFFFFFu);
        call(ctx, 0x004F99BC, {c, 0x368328E5, frame.addr(), 0xFFFFFFFFu});  // XmlCursor_ReadS32
        v = load<u32>(frame.addr());
    }
    store<u32>(p + 12, v == 0xFFFFFFFFu ? 0u : v);
    ctx.gpr[3] = p;
}

// T* f(u32 id, const Vec3* pos): the nearest object (squared distance
// under *0x000D6C04) of the list 0x01013B34 (u16 count +8) whose
// +216->+52 is id, or 0 (f1 = the last distance computed)
DECOMP(000D6C08, ObjList_FindNearest_000D6C08) {
    const u32 id = arg<u32>(ctx, 0), pos = arg<u32>(ctx, 1);
    f64 best_d = LfsAt(0x000D6C04);
    const u32 list = load<u32>(0x01013B34);
    const u32 n = load<u16>(0x01013B3C);
    u32 best = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(list + 4 * i);
        const u32 q = load<u32>(e + 216);
        if (q == 0 || load<u32>(q + 52) != id) continue;
        const u32 v = e + 144;
        const f32 dy = Sub28(LfsAt(v + 4), LfsAt(pos + 4));
        const f32 dx = Sub28(LfsAt(v), LfsAt(pos));
        const f32 dz = Sub28(LfsAt(v + 8), LfsAt(pos + 8));
        const f32 d = Madd28(dz, dz, Madd28(dx, dx, Mul28(dy, dy)));
        ctx.fpr[1] = d;
        if (best_d > d) {
            best_d = d;
            best = e;
        }
    }
    ctx.gpr[3] = best;
}

// void f(T* o, U* a, u8 b): 0x000D8C18(o, o->+160, a, b), then the same on
// each child (+192 of the 240-byte records of o->+340, u16 count +8)
DECOMP(000D8F08, Obj_ApplyTree_000D8F08) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu;
    call(ctx, 0x000D8C18, {o, load<u32>(o + 160), a, b});
    for (u32 i = 0;; ++i) {
        const u32 c = load<u32>(o + 340);
        const u32 n = c != 0 ? load<u16>(c + 8) : 0u;
        if (i >= n) {
            ctx.gpr[3] = c;
            return;
        }
        call(ctx, 0x000D8F08, {load<u32>(load<u32>(c) + i * 240 + 192), a, b});
    }
}

// T* f(T* o, u32 hash): the first child (+192 of the 240-byte records of
// o->+340, u16 count +8) whose +168 is hash, searching each subtree before
// the next child; 0 for none
DECOMP(000DB290, Obj_FindByHash_000DB290) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    for (u32 i = 0;; ++i) {
        const u32 c = load<u32>(o + 340);
        const u32 n = c != 0 ? load<u16>(c + 8) : 0u;
        if (i >= n) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 r = load<u32>(load<u32>(c) + i * 240 + 192);
        if (load<u32>(r + 168) == h) {
            ctx.gpr[3] = r;
            return;
        }
        if (static_cast<u32>(call(ctx, 0x000DB290, {r, h})) != 0) return;
    }
}

// T::~T() (vtables 0x00B12CD8 / +380 0x00B12E78 / +384 0x00B12EA0): the
// skeleton +548 (removed from its Havok world +8 first, when set, then
// AnimSkel_Release7D4EA8), RefNode_Release(+560), then the base set
// 0x00B12550 / 0x00B1200C / 0x00B12528 and the base destructor 0x000D7024
DECOMP(000EB0F8, VtB12CD8_Dtor_000EB0F8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 548);
    store<u32>(o, 0x00B12CD8);
    store<u32>(o + 380, 0x00B12E78);
    store<u32>(o + 384, 0x00B12EA0);
    if (s != 0) {
        u32 r = s;
        const u32 w = load<u32>(s + 8);
        if (w != 0) {
            Stack frame(ctx, 16);
            call(ctx, 0x0092FFC8, {frame.addr(), w, s});  // Hk_RemoveEntity92FFC8
            r = load<u32>(o + 548);
        }
        call(ctx, o26fn::AnimSkel_Release, {r});
    }
    call(ctx, o24fn::RefNode_Release, {load<u32>(o + 560)});
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B12550);
    ctx.gpr[3] = call(ctx, o29fn::Obj_BaseDtor_0D7024, {o});
}

// void f(T* o, const Mat4x3* m): the 3x3 rows of m (+0, +16, +32) and the
// row +48 into o->+448.. (rows of 16 bytes, raw f32 copies), then
// o->+440->vt[84](it, &o->+448) when set (f1 = the last word copied)
DECOMP(000EDD74, Obj_SetMatrix_000EDD74) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) {
            w = load<u32>(m + 16 * r + 4 * k);
            store<u32>(o + 448 + 16 * r + 4 * k, w);
        }
    ctx.fpr[1] = F32Bits28(w);
    const u32 c = load<u32>(o + 440);
    if (c != 0) VCall(ctx, c, 84, {c, o + 448});
}

// void f(T* o): for each of the eight 16-byte slots at o->+788 until one
// with a clear +0: the effect +8 (when set) gets Effect_Release(e,
// 0x10CBB), the flag 0x200 of its +136 and is cleared; then +0 = 0 (r3 =
// the last effect, or 0)
DECOMP(001304E0, Obj_ReleaseEffects_001304E0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = 0;
    for (u32 i = 0; i < 8; ++i) {
        const u32 a = o + 788 + 16 * i;
        if (load<u32>(a) == 0) {
            r = 0;
            break;
        }
        const u32 b = a + 8;
        r = load<u32>(b);
        if (r != 0) {
            call(ctx, 0x004CF3C4, {r, 0x10CBB});  // Effect_Release
            r = load<u32>(b);
            store<u32>(r + 136, load<u32>(r + 136) | 0x200u);
            store<u32>(b, 0);
        }
        store<u32>(a, 0);
    }
    ctx.gpr[3] = r;
}

// void f(T* o): unless o->+396 is already 16, sets it; then, when the
// world's player (kWorldPtr->* + 0x1281F4) has the flag 0x20 of +28, the
// trophy 0xB2308356 (Trophy_Award0B27B4) unless its u8 +261 is set, which
// is cleared; and the u8 kWorldPtr->* + 424 = 0 (r3 = kWorldPtr->* + 80)
DECOMP(00175CF0, Player_SetState16_00175CF0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 396) == 16) return;
    store<u32>(o + 396, 16);
    u32 w = load<u32>(kWorldPtr);
    const u32 e = load<u32>(w + 0x1281F4);
    if (e != 0 && (load<u32>(e + 28) & 0x20u) != 0) {
        if (load<u8>(e + 261) == 0) {
            call(ctx, 0x000B27B4, {0xB2308356u});  // Trophy_Award0B27B4
            w = load<u32>(kWorldPtr);
        }
        store<u8>(load<u32>(w + 0x1281F4) + 261, 0);
        w = load<u32>(kWorldPtr);
    }
    store<u8>(w + 80 + 344, 0);
    ctx.gpr[3] = w + 80;
}

// The hit handler: HitApplyK29 on o->+216's f32 +232 against *0x00192E3C,
// constant 0xA77EB5E9
DECOMP(00192E40, Obj_OnHitScaled_00192E40) { HitApplyK29(ctx, 232, 0x00192E3C, 0xA77EB5E9u); }

// void f(T* o, u8 a, u8 b): each 96-byte record of o->+388 (u16 count +392)
// gets record->+80->vt[52](it, a, b) (r3 = the count)
DECOMP(001BD2B0, Obj_ForEachChild_001BD2B0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    u32 n = load<u16>(o + 392);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(o + 388) + 96 * i + 80);
        VCall(ctx, e, 52, {e, a, b});
        n = load<u16>(o + 392);
    }
    ctx.gpr[3] = n;
}

// Secondary-base deleting destructor thunks of 0x001C345C (this - 384 and
// this - 380)
DECOMP(001C3510, VtB16B48_DeleteAdj384_001C3510) { DeleteB16B48_29(ctx, arg<u32>(ctx, 0) - 384); }
DECOMP(001C35C8, VtB16B48_DeleteAdj380_001C35C8) { DeleteB16B48_29(ctx, arg<u32>(ctx, 0) - 380); }

// void f(T* o, u8 b): o->+160->vt[40](it, b, 0) when set; then the def
// Target_GetDef0D951C(o) (when set) gets vt[44](def, b) and
// 0x002B81FC(def, b)
DECOMP(001EB814, Obj_SetActive_001EB814) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 160);
    if (c != 0) VCall(ctx, c, 40, {c, b, 0});
    const u32 d = static_cast<u32>(call(ctx, 0x000D951C, {o}));
    if (d == 0) return;
    VCall(ctx, d, 44, {d, b});
    ctx.gpr[3] = call(ctx, 0x002B81FC, {d, b});
}

// void f(T* o): each 8-byte entry of o->+640 (u16 count +648): its object
// +4 (when set) gets vt[40](it, 0), then 0x001ED85C(o, entry->+0); then
// the count = 0 (r3 = the count)
DECOMP(001ED9D4, Obj_FlushEntries_001ED9D4) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 648);
        if (i >= n) {
            store<u16>(o + 648, 0);
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(o + 640) + 8 * i;
        const u32 c = load<u32>(e + 4);
        if (c != 0) VCall(ctx, c, 40, {c, 0});
        call(ctx, 0x001ED85C, {o, load<u32>(e)});
    }
}

// void f(T* o; f32 a, f32 b): see ApplySlot29
DECOMP(00279414, Obj_ApplySlot_00279414) { ApplySlot29(ctx); }

// void f(T* o, U* n, u8 flag): when o->+1244 changes to n: the old one (when
// set) gets vt[40](old, 0); then bit 31 of o->+1268 = flag & 1 (0 for a
// null n) and n's u16 +178 is incremented
DECOMP(00296758, Obj_SetTarget_00296758) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 cur = load<u32>(o + 1244);
    ctx.gpr[3] = cur;
    if (cur == n) return;
    if (cur != 0) VCall(ctx, cur, 40, {cur, 0});
    store<u32>(o + 1244, n);
    if (n == 0) {
        const u32 w = load<u32>(o + 1268) & 0x7FFFFFFFu;
        store<u32>(o + 1268, w);
        ctx.gpr[3] = w;
        return;
    }
    store<u32>(o + 1268, (load<u32>(o + 1268) & 0x7FFFFFFFu) | (flag << 31));
    store<u16>(n + 178, static_cast<u16>(load<u16>(n + 178) + 1));
    ctx.gpr[3] = n + 178;
}

// void f(T* o): Coach_SetMode2A8AE0(o, 0, 0, 0), o->+2456 = 0, then each of
// the u16 o->+2468 entries (12 bytes at o->+2460) gets entry->+0->vt[8]()
// and the count = 0
DECOMP(002AB80C, Coach_Reset_002AB80C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x002A8AE0, {o, 0, 0, 0});
    const u32 n = load<u16>(o + 2468);
    store<u32>(o + 2456, 0);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = load<u32>(load<u32>(o + 2460) + 12 * i);
        VCall(ctx, e, 8, {e});
    }
    store<u16>(o + 2468, 0);
    ctx.gpr[3] = 0;
}

// void f(T* o; f32 a, f32 b): ApplySlot29 (a second copy)
DECOMP(002C5078, Obj_ApplySlot_002C5078) { ApplySlot29(ctx); }

// bool f(T* o, const Rec* src): copies the 68 bytes of src into the
// record of o->+428 (76 bytes, u16 count +436) with the same +68 and sets
// its u8 +73; false when there is none (r3 = 64 / 0)
DECOMP(002D98DC, Rec_Update_002D98DC) {
    const u32 o = arg<u32>(ctx, 0), src = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 436);
    const u32 p = load<u32>(o + 428);
    ctx.gpr[3] = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 e = p + 76 * i;
        if (load<u32>(e + 68) != load<u32>(src + 68)) continue;
        for (u32 k = 0; k < 64; k += 8) {
            const u32 a = load<u32>(src + k), b = load<u32>(src + k + 4);
            store<u32>(e + k, a);
            store<u32>(e + k + 4, b);
        }
        store<u32>(e + 64, load<u32>(src + 64));
        store<u8>(e + 73, 1);
        ctx.gpr[3] = 64;
        return;
    }
}

// bool f(T* o): false when an entry of the list o->+36 (u16 count +44) with
// the u8 +52 set has an effect +48->+52 for which Effect_Check4CF620(e, 0)
// is 0
DECOMP(002FCC8C, Obj_EffectsDone_002FCC8C) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < load<u16>(o + 44); ++i) {
        const u32 e = load<u32>(load<u32>(o + 36) + 4 * i);
        if (load<u8>(e + 52) == 0) continue;
        const u32 q = load<u32>(e + 48);
        if (q == 0) continue;
        const u32 fx = load<u32>(q + 52);
        if (fx == 0) continue;
        if (static_cast<u32>(call(ctx, 0x004CF620, {fx, 0})) == 0) {
            ctx.gpr[3] = 0;
            return;
        }
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, a): 0x00325CE4(o, a); when the u8 o->+18 is 4, o->vt[28](o, 0)
// and each 32-byte entry of o->+2572 (u16 count +2580) gets +20 =
// Obj_FindById(+16)
DECOMP(0032F65C, Obj_ResolveIds_0032F65C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00325CE4, {o, arg<u32>(ctx, 1)});
    const u32 k = load<u8>(o + 18);
    ctx.gpr[3] = k;
    if (k != 4) return;
    VCall(ctx, o, 28, {o, 0});
    u32 i = 0;
    for (;; ++i) {
        const u32 n = load<u16>(o + 2580);
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(o + 2572) + 32 * i;
        store<u32>(e + 20, static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(e + 16)})));
    }
}

// T::~T() deleting, pooled (vtable 0x00B1F338, then 0x00B13974): the
// child +8 gets vt[8]() when the u8 +25 is set (the first vtable is only
// stored then), +8 = 0; then the pool (*(r13 - 28332))->+44 gets vt[12](
// pool, o, the u16 +4)
DECOMP(00347A4C, VtB1F338_DeletePooled_00347A4C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 8);
    if (c != 0 && load<u8>(o + 25) != 0) {
        const u32 vt = load<u32>(c);  // read before the store (c may be o)
        store<u32>(o, 0x00B1F338);
        VCallVt(ctx, vt, 8, {c});
    }
    store<u32>(o + 8, 0);
    store<u32>(o, 0x00B13974);
    const u32 pool = Pool28332(ctx);
    VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// bool f(T* o, a, b): false after o->+8->vt[20](it, a, b) when the u8
// o->vt[16] returns (into a stack local) is set and o->+8 is; else true
DECOMP(00347DE8, Obj_ForwardIf_00347DE8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    u32 on;
    {
        Stack frame(ctx, 16);
        VCall(ctx, o, 16, {frame.addr(), o});
        on = load<u8>(frame.addr());
    }
    const u32 c = on != 0 ? load<u32>(o + 8) : 0u;
    if (c == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    VCall(ctx, c, 20, {c, a, b});
    ctx.gpr[3] = 0;
}

// void f(T* o, u32 hash, u8 v): the u8 +256 = v of each 320-byte record
// of o->+108 (u16 count +8) matched by hash: 0x001F3381 matches all, the
// first two also match the table 0x00ADC0B8[i], any its own +0
DECOMP(0035327C, Obj_SetByteByHash_0035327C) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    for (u32 i = 0;; ++i) {
        const u32 c = load<u32>(o + 108);
        const u32 n = c != 0 ? load<u16>(c + 8) : 0u;
        if (i >= n) return;
        const bool m = h == 0x001F3381u || (i < 2 && h == load<u32>(0x00ADC0B8 + 4 * i)) ||
                       h == load<u32>(load<u32>(c) + 320 * i);
        if (!m) continue;
        const u32 e = load<u32>(c) + 320 * i;
        if (e != 0) store<u8>(e + 256, static_cast<u8>(v));
    }
}

// void f(T* o, u8 on): when it changes bit 0x80000 of o->+532 (to on & 1);
// for the states 2 / 3 (o->+160 >> 1) Speaker_SayId4A5C90(o->+28, the
// line +184 (on) or +188 (off), 0; *0x0038474C) first (r3 = o->+188 when
// unchanged)
DECOMP(00384750, Obj_SetTalking_00384750) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    const u32 b = on & 0xFFu;
    if (b == ((load<u32>(o + 532) >> 19) & 1u)) {
        ctx.gpr[3] = load<u32>(o + 188);
        return;
    }
    const u32 k = load<u32>(o + 160) >> 1;
    if (k == 3 || k == 2)
        callf(ctx, 0x004A5C90, {load<u32>(o + 28), b == 0 ? load<u32>(o + 188) : load<u32>(o + 184), 0}, {LfsAt(0x0038474C)});
    const u32 w = (load<u32>(o + 532) & ~0x80000u) | ((on & 1u) << 19);
    store<u32>(o + 532, w);
    ctx.gpr[3] = w;
}

// void f(Pool* o, u32 handle): when the handle (low 16 bits a serial, high
// 16 the index below o->+24) names a live 16-byte entry of o->+28 (serial
// = +12 >> 15) without the flag 0x80 of its u8 +12: Track_Release38DC0C(
// entry->+8) and HandlePool_Free_00773C8C(&out, o, handle) (r3 as left)
DECOMP(00398DAC, TrackPool_Free_00398DAC) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 lo = h & 0xFFFFu;
    ctx.gpr[3] = lo;
    if (lo == 0 || !(static_cast<s32>(h >> 16) < static_cast<s32>(load<u32>(o + 24)))) return;
    const u32 e = load<u32>(o + 28) + ((h >> 12) & 0xFFFF0u);
    ctx.gpr[3] = e;
    if (((load<u32>(e + 12) >> 15) & 0xFFFFu) != lo || (load<u8>(e + 12) & 0x80u) != 0) return;
    call(ctx, 0x0038DC0C, {load<u32>(e + 8)});
    Stack frame(ctx, 16);
    call(ctx, 0x00773C8C, {frame.addr(), o, h});
}

// void f(T* o, u8 on): bit 31 of o->+356 = on & 1, then each set entry of
// the list o->+0 (u16 count +8) gets vt[40](it, on, 1) (r3 = the count)
DECOMP(0039CF9C, List_SetOn_0039CF9C) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    store<u32>(o + 356, (load<u32>(o + 356) & 0x7FFFFFFFu) | (on << 31));
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 8);
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 c = load<u32>(load<u32>(o) + 4 * i);
        if (c != 0) VCall(ctx, c, 40, {c, on & 0xFFu, 1});
    }
}

// void f(u32 hash, u8 a, u8 b): 0x0042BD94(the index of hash in the list
// 0x010AA694 (u16 count +8; entries with bit 0x20000000 of +24 skipped;
// -1 for none, 0 for hash 0), a, b)
DECOMP(0042BE98, Cmd_ByHash_0042BE98) {
    const u32 h = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    u32 idx = 0;
    if (h != 0) {
        idx = 0xFFFFFFFFu;
        const u32 data = load<u32>(0x010AA694);
        const u32 n = load<u16>(0x010AA69C);
        for (u32 i = 0; i < n; ++i) {
            const u32 e = load<u32>(data + 4 * i);
            if ((load<u32>(e + 24) & 0x20000000u) == 0 && load<u32>(e) == h) {
                idx = i;
                break;
            }
        }
    }
    ctx.gpr[3] = call(ctx, 0x0042BD94, {idx, a, b});
}

// void Cmd::Run(): the target +12 (or, with a filter +16, the object
// WorldObj_ForEach(+12, 0x00F390A8, {filter, &found}, 0) finds) gets bit
// 0x100 of +164 = bit 31 of +20; then Flag_Set8_40000000(this)
DECOMP(004383E0, Cmd_SetObjFlag_004383E0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 t = load<u32>(o + 12);
    const u32 f = load<u32>(o + 16);
    if (f != 0) {
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), f);
        store<u32>(frame.addr() + 4, 0);
        call(ctx, 0x0057F5D8, {t, 0x00F390A8, frame.addr(), 0});  // WorldObj_ForEach
        const u32 found = load<u32>(frame.addr() + 4);
        if (found != 0) t = found;
    }
    if (t != 0) store<u32>(t + 164, (load<u32>(t + 164) & ~0x100u) | ((load<u32>(o + 20) >> 31) << 8));
    ctx.gpr[3] = call(ctx, o24fn::Flag_Set8_40000000, {o});
}

// void T::Init(): Obj_RegisterParam for the five 24-byte params from
// 0x010AB310, 0x0043BC58(o), then o->+8 gets +44 = 0x00AE36EC and the
// flag 0x80 of its u8 +40, and o->+88 = 0x00F39B40
DECOMP(0043C070, Obj_InitParams_0043C070) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 5; ++k) call(ctx, 0x004F7128, {o, 0x010AB310u + 24 * k});  // Obj_RegisterParam
    call(ctx, 0x0043BC58, {o});
    const u32 p = load<u32>(o + 8);
    const u32 b = load<u8>(p + 40);
    store<u32>(p + 44, 0x00AE36EC);
    store<u8>(p + 40, static_cast<u8>(b | 0x80u));
    store<u32>(o + 88, 0x00F39B40);
    ctx.gpr[3] = p;
}

// void f(?, a, u8 on): the UI elements Ui_Find(0x00AE8AD8, a) and
// Ui_Find(0x00AE8AE8, a) (when found) get vt[88](it, on)
DECOMP(004704A8, Ui_ShowPair_004704A8) {
    const u32 a = arg<u32>(ctx, 1), on = arg<u32>(ctx, 2) & 0xFFu;
    for (u32 name : {0x00AE8AD8u, 0x00AE8AE8u}) {
        const u32 u = static_cast<u32>(call(ctx, 0x004D5544, {name, a}));  // Ui_Find
        if (u != 0) VCall(ctx, u, 88, {u, on});
    }
}

// T* f(?, XmlCursor* c): new 24-byte record (vtable 0x00B21CD8, u16 +4 =
// 1) reading the hashes 0x0024728B into +8 (default 0) and 0x00014B01 into
// +20 (default 0)
DECOMP(004BEAB4, NewXmlHash2_004BEAB4) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B21CD8);
    store<u32>(p + 20, 0);
    call(ctx, o29fn::XmlCursor_ReadHash, {c, 0x0024728B, p + 8, 0});
    call(ctx, o29fn::XmlCursor_ReadHash, {c, 0x00014B01, p + 20, load<u32>(p + 20)});
    ctx.gpr[3] = p;
}

// void f(T* o, a): o->vt[84](o, Matrix_FromRows(&m, the rows o->+96, +112,
// +128 (copied to the stack, 16 bytes apart), a))
DECOMP(00734EC8, Obj_SetRowsMatrix_00734EC8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    Stack frame(ctx, 112);
    const u32 rows = frame.addr(), m = frame.addr() + 48;
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) store<u32>(rows + 16 * r + 4 * k, load<u32>(o + 96 + 16 * r + 4 * k));
    ctx.fpr[1] = LfsAt(o + 136);
    ctx.fpr[2] = LfsAt(o + 132);
    call(ctx, 0x00553B4C, {m, rows, a});  // Matrix_FromRows
    VCall(ctx, o, 84, {o, m});
}

// void f(T* d, const Mat4x3* m; f32 a, f32 b): the 4 rows of m into d (raw
// copies), +64 = a, +68 = b, the u8s +72 / +88 and the words +76, +80, +84,
// +92 cleared, then the 4 bytes at 0x01164CA8 into +96
DECOMP(00735E60, Obj_InitFromMatrix_00735E60) {
    const u32 d = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) {
            if (r == 3 && k == 2) break;
            store<u32>(d + 16 * r + 4 * k, load<u32>(m + 16 * r + 4 * k));
        }
    const u32 z = load<u32>(m + 56);
    store<f32>(d + 64, static_cast<f32>(farg(ctx, 0)));
    store<u32>(d + 56, z);
    store<f32>(d + 68, static_cast<f32>(farg(ctx, 1)));
    store<u8>(d + 72, 0);
    store<u32>(d + 76, 0);
    store<u32>(d + 80, 0);
    store<u32>(d + 84, 0);
    store<u8>(d + 88, 0);
    store<u32>(d + 92, 0);
    u32 b = 0;
    for (u32 k = 0; k < 4; ++k) {
        b = load<u8>(0x01164CA8 + k);
        store<u8>(d + 96 + k, static_cast<u8>(b));
    }
    ctx.gpr[3] = b;
    ctx.fpr[3] = F32Bits28(z);
}

// Deleting ~T() with PtrVectors +40 and +20 whose base (vtable 0x00B0DAAC)
// is removed from the sorted map 0x01056F04 by its key +4; free_fn size
static void DeleteKeyed29(PPUContext& ctx, u32 vt, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, vt);
    VecDtor(ctx, o + 40);
    VecDtor(ctx, o + 20);
    store<u32>(o, 0x00B0DAAC);
    call(ctx, 0x007330C0, {0x01056F04, o + 4});  // SortedMap_Remove7330C0
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, size});
}

// T::~T() deleting (vtable 0x00B1C8C8; 96 bytes): see DeleteKeyed29
DECOMP(007646E4, VtB1C8C8_Delete_007646E4) { DeleteKeyed29(ctx, 0x00B1C8C8, 96); }

// T::~T() deleting (vtable 0x00B1CE68; 64 bytes): see DeleteKeyed29
DECOMP(0076543C, VtB1CE68_Delete_0076543C) { DeleteKeyed29(ctx, 0x00B1CE68, 64); }

// void f(Vec* v, s32 i): removes the 32-byte element i of v (+0 data, u16
// +8 count) by moving the last one into it (+0..+8 raw, +16, +20 and the
// six flag bits 0xFC000000 of +24 one at a time, as the original stores
// them); r3 = the last flag word read, f1 = the last f32
DECOMP(0076AE8C, Vec32_RemoveSwap_0076AE8C) {
    const u32 v = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 n = load<u16>(v + 8) - 1u;
    store<u16>(v + 8, static_cast<u16>(n));
    if (static_cast<s32>(i) == static_cast<s32>(n & 0xFFFFu)) return;
    const u32 data = load<u32>(v);
    const u32 d = data + i * 32, s = data + ((n & 0xFFFFu) << 5);
    store<u32>(d, load<u32>(s));
    store<u32>(d + 4, load<u32>(s + 4));
    const u32 z = load<u32>(s + 8);
    store<u32>(d + 8, z);
    store<u32>(d + 16, load<u32>(s + 16));
    store<u32>(d + 20, load<u32>(s + 20));
    u32 w = (load<u32>(d + 24) & 0x7FFFFFFFu) | (load<u32>(s + 24) & 0x80000000u);
    store<u32>(d + 24, w);
    u32 sw = 0;
    for (u32 bit = 0x40000000u; bit >= 0x04000000u; bit >>= 1) {
        sw = load<u32>(s + 24);
        w = (w & ~bit) | (sw & bit);
        store<u32>(d + 24, w);
    }
    ctx.gpr[3] = sw;
    ctx.fpr[1] = F32Bits28(z);
}

// void f(T* ctx, Node* n): marks n (+272 = 1) and, unless already marked,
// its children +232, +236, +240 (recursively), then the siblings +244
DECOMP(0077CFE4, Node_MarkTree_0077CFE4) {
    const u32 c = arg<u32>(ctx, 0);
    u32 n = arg<u32>(ctx, 1);
    for (;;) {
        u32 r = load<u32>(n + 272);
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
        store<u32>(n + 272, 1);
        for (u32 off : {232u, 236u, 240u}) {
            const u32 k = load<u32>(n + off);
            r = k;
            if (k != 0) r = static_cast<u32>(call(ctx, 0x0077CFE4, {c, k}));
        }
        n = load<u32>(n + 244);
        if (n == 0) {
            ctx.gpr[3] = r;
            return;
        }
    }
}

// ---------------------------------------------------------------- sweep: 47 instructions
// void f(?, a, s32 k, u8 alt): 0x0048F90C(a, 0xCB186A06, alt ? 0x39EE3E3A
// : 0x795BCF46, 0; v) with v by k: 0 -> *0x0001B49C, 1 -> *0x00F9CCA4,
// 2 -> *0x00F9CCF0, else *0x0001B498
DECOMP(0001B4A0, Obj_PlayByLevel_0001B4A0) {
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 3);
    const s32 k = arg<s32>(ctx, 2);
    u32 ea = 0x0001B498;
    if (k == 0) ea = 0x0001B49C;
    else if (k == 1) ea = 0x00F9CCA4;
    else if (k == 2) ea = 0x00F9CCF0;
    const u32 h = (b & 0xFFu) != 0 ? 0x39EE3E3Au : 0x795BCF46u;
    callf(ctx, 0x0048F90C, {a, 0xCB186A06u, h, 0}, {LfsAt(ea)});
}

// void f(Vec3* out, T* o): out = the position of the current target
// (o->+412 + 20 * o->+1796, its object +4, that one's +4, the vector +128;
// the inner +4 is not null-checked) when the u8
// o->+1811 is clear or o->+1052 is not set and the target has an object;
// else Obj_GetFocusPos_000137C4(out, o)
DECOMP(0004E660, Obj_GetTargetPos_0004E660) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 t = o + load<u32>(o + 1796) * 20u + 412;
    const bool focus = load<u8>(o + 1811) != 0 && load<u32>(o + 1052) != 0;
    if (focus || t == 0 || load<u32>(t + 4) == 0) {
        ctx.gpr[3] = call(ctx, 0x000137C4, {out, o});
        return;
    }
    const u32 v = load<u32>(load<u32>(t + 4) + 4) + 128;
    CopyVec3F28(ctx, out, v);
    ctx.gpr[3] = v;
}

// void Cmd::Run(): with the camera 0x00F66FD0, when the u8 +32 and the
// hash +12 are set, by the camera mode +92: 0 -> Obj_Setup9535_00090D00(
// this, cam, hash), 1 -> 0x00090C00(this, cam, hash); then, when the u8
// +33 is set, the camera's +32.. = the vector +16 (raw)
DECOMP(00090DB0, Cmd_CameraSetup_00090DB0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F66FD0);
    if (g == 0) return;
    const u32 h = load<u32>(o + 12);
    if (load<u8>(o + 32) != 0 && h != 0) {
        const s32 k = static_cast<s32>(load<u32>(g + 92));
        if (k == 0) call(ctx, 0x00090D00, {o, g, h});
        else if (k == 1) call(ctx, 0x00090C00, {o, g, h});
    }
    const u32 b = load<u8>(o + 33);
    ctx.gpr[3] = b;
    if (b == 0) return;
    for (u32 k = 0; k < 3; ++k) store<u32>(g + 32 + 4 * k, load<u32>(o + 16 + 4 * k));
    ctx.fpr[1] = LfsAt(o + 24);
}

// T* f(?, XmlCursor* c): new 24-byte record (vtable 0x00B11B38, u16 +4 =
// 1, +20 / u16 +22 / +8 cleared) whose +8 is String_HashNoCase of the
// attribute named 0x00AC5A5C (when present)
DECOMP(000A77F8, NewXmlAttrHash_000A77F8) {
    const u32 c = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o19fn::SmallAlloc, {24}));
    if (p == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u16>(p + 4, 1);
    store<u32>(p + 20, 0);
    store<u32>(p, 0x00B11B38);
    store<u16>(p + 22, 0);
    store<u32>(p + 8, 0);
    store<u32>(p + 20, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {c, 0x00AC5A5C}));  // XmlCursor_FindAttrByName
    if (a != 0) store<u32>(p + 8, static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {a})));
    ctx.gpr[3] = p;
}

// void f(T* o, u8 b): when b, o->+536 = 0 and the seven 16-byte emitter
// slots at +428 are stopped (Emitter_Stop(h, 1)) and cleared (+0, +4);
// then 0x000E4544(o, b)
static void StopEmitters7_29(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) {
        store<u32>(o + 536, 0);
        for (u32 i = 0; i < 7; ++i) {
            const u32 s = o + 428 + 16 * i;
            call(ctx, o25fn::Emitter_Stop, {load<u32>(s), 1});
            store<u32>(s, 0);
            store<u32>(s + 4, 0);
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E4544, {o, b});
}

// StopEmitters7_29 at three addresses (classes sharing the layout)
DECOMP(001AE6AC, Obj_StopEmitters_001AE6AC) { StopEmitters7_29(ctx); }
DECOMP(001BF794, Obj_StopEmitters_001BF794) { StopEmitters7_29(ctx); }
DECOMP(001D20D0, Obj_StopEmitters_001D20D0) { StopEmitters7_29(ctx); }

// void f(T* a, U* b): as 0x001AE3B4 (0x000E44C0(a, b), b->+236 = a->+268,
// b->+208.. = a->+240.. (7 words), the field 0xFFFE0000 of b->+20 = 4),
// then b->+240 = a->+272 (r3 = 4, f1 = that word)
DECOMP(001C3308, Obj_CopyParams_001C3308) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {a, b});
    store<u32>(b + 236, load<u32>(a + 268));
    for (u32 k = 0; k < 7; ++k) store<u32>(b + 208 + 4 * k, load<u32>(a + 240 + 4 * k));
    store<u32>(b + 20, (load<u32>(b + 20) & 0x1FFFFu) | (4u << 17));
    const u32 w = load<u32>(a + 272);
    store<u32>(b + 240, w);
    ctx.gpr[3] = 4;
    ctx.fpr[1] = F32Bits28(w);
}

// void f(T* o, u8 b): when b, the skeletons +624 and +628 (when set) are
// removed from their Havok world +8 (Body_Remove931F00(world, s), when
// set) and released (AnimSkel_Release7D4EA8); then 0x000E4544(o, b)
DECOMP(001DEBA4, Obj_ReleaseBodies_001DEBA4) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) {
        for (u32 off : {624u, 628u}) {
            const u32 s = load<u32>(o + off);
            if (s == 0) continue;
            u32 r = s;
            const u32 w = load<u32>(s + 8);
            if (w != 0) {
                call(ctx, 0x00931F00, {w, s});  // Body_Remove931F00
                r = load<u32>(o + off);
            }
            call(ctx, o26fn::AnimSkel_Release, {r});
        }
    }
    ctx.gpr[3] = call(ctx, 0x000E4544, {o, b});
}

// bool f(Area* o, u32 id): Area_Enter(o, id); when it succeeds and the
// f32 +456 > *0x0021B730, the bit of id's index in the list +344 (u16
// count +352; none when absent) is set in +448
DECOMP(0021B738, Area_EnterMark_0021B738) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    if (static_cast<u32>(call(ctx, 0x0020E7A4, {o, id})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const f64 v = LfsAt(o + 456);
    ctx.fpr[1] = v;
    if (v > LfsAt(0x0021B730)) {
        const s32 n = load<u16>(o + 352);
        const u32 list = load<u32>(o + 344);
        s32 idx = -1;
        for (s32 i = 0; i < n; ++i)
            if (load<u32>(list + 4 * static_cast<u32>(i)) == id) {
                idx = i;
                break;
            }
        const u32 sh = static_cast<u32>(idx) & 63u;
        store<u32>(o + 448, load<u32>(o + 448) | (sh < 32 ? 1u << sh : 0u));  // slw
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, U* except): when the counter o->+428 drops to 0, each object
// of the list o->+344 (u16 count +352) other than except whose +216 has
// the flag 0x08 of its u8 +32 gets Obj_SceneEnd0DD02C(it, 0, 0) (r3 = the
// count)
DECOMP(002257B4, Obj_EndScenes_002257B4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 428) - 1u;
    store<u32>(o + 428, c);
    if (c != 0) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 352);
        if (i >= n) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(load<u32>(o + 344) + 4 * i);
        if (e == x || (load<u8>(load<u32>(e + 216) + 32) & 0x08u) == 0) continue;
        call(ctx, 0x000DD02C, {e, 0, 0});
    }
}

// void f(T* o): pops every skeleton of the stack o->+144 (depth (+40 >> 7)
// & 0xF, entries +148..): Body_Remove931F00(s->+8, s), AnimSkel_Release
// (twice when its s16 +6 is positive), then the next one moves to +144
DECOMP(00275030, Obj_PopBodies_00275030) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    while (((load<u32>(o + 40) >> 7) & 0xFu) != 0) {
        const u32 s = load<u32>(o + 144);
        call(ctx, 0x00931F00, {load<u32>(s + 8), s});  // Body_Remove931F00
        call(ctx, o26fn::AnimSkel_Release, {load<u32>(o + 144)});
        const u32 s2 = load<u32>(o + 144);
        if (static_cast<s16>(load<u16>(s2 + 6)) > 0) call(ctx, o26fn::AnimSkel_Release, {s2});
        const u32 w = load<u32>(o + 40);
        const u32 d = ((w >> 7) & 0xFu) - 1u;
        store<u32>(o + 40, (w & ~0x780u) | ((d << 7) & 0x780u));
        const u32 p = o + 144 + ((d << 2) & 0x3Cu);
        store<u32>(o + 144, load<u32>(p));
        store<u32>(p, 0);
    }
    ctx.gpr[3] = 0;
}

// T::~T() deleting (vtable 0x00B1CD38): the child +36 gets vt[40](it, 0)
// and (reloaded) vt[40](it, 1) and is cleared when bit 0x40000 of its +172
// is set; then Mem_FreeFrom(o, 0)
DECOMP(002B4308, VtB1CD38_Delete_002B4308) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 36);
    store<u32>(o, 0x00B1CD38);
    if (c != 0 && (load<u32>(c + 172) & 0x40000u) != 0) {
        VCall(ctx, c, 40, {c, 0});
        const u32 c2 = load<u32>(o + 36);
        VCall(ctx, c2, 40, {c2, 1});
        store<u32>(o + 36, 0);
    }
    ctx.gpr[3] = call(ctx, o19fn::Mem_FreeFrom, {o, 0});
}

// T::~T() deleting (vtable 0x00B1D500, then 0x00B1D4D8): the child +16
// gets vt[40](it, 0), the blocks +24 and +32 are freed (Mem_FreeFrom) and
// cleared, then SmallFree(o, 208)
DECOMP(002ECCB8, VtB1D500_Delete_002ECCB8) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 c = load<u32>(o + 16);
    store<u32>(o, 0x00B1D500);
    if (c != 0) VCall(ctx, c, 40, {c, 0});
    call(ctx, o19fn::Mem_FreeFrom, {load<u32>(o + 24), 0});
    const u32 b = load<u32>(o + 32);
    store<u32>(o + 24, 0);
    call(ctx, o19fn::Mem_FreeFrom, {b, 0});
    store<u32>(o + 32, 0);
    store<u32>(o, 0x00B1D4D8);
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 208});
}

// void f(T* o, u8 b): each object of the chain o->+2576 (next +2564) gets
// vt[28](it, b); then Recs_Reset325A30(o, b)
DECOMP(003274D0, Obj_ResetChain_003274D0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) VCall(ctx, c, 28, {c, b});
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, b});
}

// void f(T* o, u8 b): +2620 / +2624 = 0; the target +2616 (when set): when
// Obj_Notify24(o->+12->+8) returns r, 0x0029F510(r, target->+168); then
// target->vt[40](it, 0) and +2616 = 0; then Recs_Reset325A30(o, b)
DECOMP(00332330, Obj_DropTarget_00332330) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 t = load<u32>(o + 2616);
    store<u32>(o + 2620, 0);
    store<u32>(o + 2624, 0);
    if (t != 0) {
        const u32 q = load<u32>(load<u32>(o + 12) + 8);
        if (q != 0) {
            const u32 r = static_cast<u32>(call(ctx, 0x000D949C, {q, t}));  // Obj_Notify24
            if (r != 0) call(ctx, 0x0029F510, {r, load<u32>(load<u32>(o + 2616) + 168)});
        }
        const u32 t2 = load<u32>(o + 2616);
        VCall(ctx, t2, 40, {t2, 0});
        store<u32>(o + 2616, 0);
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, b});
}

// s32 f(T* o, s32 k): when the u8 o->vt[16] returns (into a stack local)
// is set, k >= 1 and o->+8 is set, o->+8->vt[20](it, k, 1) minus that
// object's +268 (read before the call); else 0
DECOMP(00347BFC, Obj_ForwardCount_00347BFC) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = arg<s32>(ctx, 1);
    u32 on;
    {
        Stack frame(ctx, 16);
        VCall(ctx, o, 16, {frame.addr(), o});
        on = load<u8>(frame.addr());
    }
    const u32 c = (on != 0 && k >= 1) ? load<u32>(o + 8) : 0u;
    if (c == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 base = load<u32>(c + 268);
    const u32 r = static_cast<u32>(VCall(ctx, c, 20, {c, static_cast<u32>(k), 1}));
    ret<s32>(ctx, static_cast<s32>(r - base));
}

// void f(T* o; f32 t): the first time (u8 +536), Effect_SetHash4CF988(
// +532, 0xB0520D85); then 0x003D26B8 / 0x003D2514 / 0x003CB968(+400) /
// 0x003D2020 (each with t) and World_AddTimedCallback(world, 0,
// 0x00F38458, o, 0)
DECOMP(003D280C, Obj_Update_003D280C) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 t = farg(ctx, 0);
    if (load<u8>(o + 536) == 0) {
        store<u8>(o + 536, 1);
        call(ctx, o27fn::Effect_SetHash, {load<u32>(o + 532), 0xB0520D85u});
    }
    callf(ctx, 0x003D26B8, {o}, {t});
    callf(ctx, 0x003D2514, {o}, {t});
    callf(ctx, 0x003CB968, {o + 400}, {t});
    callf(ctx, 0x003D2020, {o}, {t});
    ctx.gpr[3] = call(ctx, 0x0046B5F8, {load<u32>(kWorldPtr), 0, 0x00F38458, o, 0});
}

// bool f(T* o, const Event* e): for an event of kind 2 (+0), when both
// children +64 / +68 have bit 0x40000 of +172 and the u8 +176 is clear:
// 0x003DB840(o; e->+4), 0x003DB698(o), 0x003DB140(o; e->+4). Returns 1.
DECOMP(003DC860, Obj_OnEvent2_003DC860) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    if (static_cast<s32>(load<u32>(e)) == 2) {
        const u32 a = load<u32>(o + 64);
        const u32 b = a != 0 && (load<u32>(a + 172) & 0x40000u) != 0 ? load<u32>(o + 68) : 0u;
        if (b != 0 && (load<u32>(b + 172) & 0x40000u) != 0 && load<u8>(o + 176) == 0) {
            const f64 t = LfsAt(e + 4);
            callf(ctx, 0x003DB840, {o}, {t});
            call(ctx, 0x003DB698, {o});
            callf(ctx, 0x003DB140, {o}, {t});
        }
    }
    ctx.gpr[3] = 1;
}

// T::T(a, Owner* b, c, d): SubMachine_Construct(this, a, c, b->+0, d),
// vtable 0x00B20C30, +56 = b, +60 / +68 / +96 / +100 / +112 = 0, bit 31 of
// +64 and the u8s +65 / +66 cleared, the PtrVector +72 (u16 +78 = 4), then
// the flag 0x8000 of +28 (r3 = that word)
DECOMP(0041F1FC, SubMachine_Ctor_0041F1FC) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4);
    call(ctx, 0x0047C5D8, {o, a, c, load<u32>(b), d});  // SubMachine_Construct
    const u32 w = load<u32>(o + 64) & 0x7FFFFFFFu;
    store<u32>(o + 56, b);
    store<u32>(o + 60, 0);
    store<u32>(o, 0x00B20C30);
    store<u32>(o + 64, w);
    store<u8>(o + 65, 0);
    store<u8>(o + 66, 0);
    store<u32>(o + 68, 0);
    call(ctx, 0x005516E0, {o + 72});  // PtrVector_Construct
    store<u32>(o + 96, 0);
    store<u16>(o + 78, 4);
    store<u32>(o + 100, 0);
    store<u32>(o + 112, 0);
    const u32 f = load<u32>(o + 28) | 0x8000u;
    store<u32>(o + 28, f);
    ctx.gpr[3] = f;
}

// void f(List* o): each set entry of o->+4 (u16 count +12) gets vt[4]();
// then the count = 0 and +24 = (+24 & 0x7FFFFFFF) | 0x20000000
DECOMP(0042BF50, List_ReleaseAll_0042BF50) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    for (u32 i = 0; i < load<u16>(o + 12); ++i) {
        const u32 c = load<u32>(load<u32>(o + 4) + 4 * i);
        if (c != 0) VCall(ctx, c, 4, {c});
    }
    store<u16>(o + 12, 0);
    const u32 w = (load<u32>(o + 24) & 0x7FFFFFFFu) | 0x20000000u;
    store<u32>(o + 24, w);
    ctx.gpr[3] = w;
}

// bool Cmd::Done(): false when the player's (kWorldPtr->* + 0x1281E0)
// current layer (+364[u8 +345], 0 without) or one below it
// (Obj_LayerBelow) is the target +16; else +16 = 0 and true
DECOMP(0042D670, Cmd_LayerGone_0042D670) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 0x1281E0);
    if (p != 0) {
        const u32 b = load<u8>(p + 345);
        u32 c = b != 0 ? load<u32>(p + 4 * b + 364) : 0u;
        while (c != 0) {
            if (c == load<u32>(o + 16)) {
                ctx.gpr[3] = 0;
                return;
            }
            c = static_cast<u32>(call(ctx, 0x000FCBD0, {p, c}));  // Obj_LayerBelow
        }
    }
    store<u32>(o + 16, 0);
    ctx.gpr[3] = 1;
}

// bool Cmd::Run(): false when Anim_TrackSet164_000D8760(+12, +24, 1)
// fails; with bit 0x08000000 of +40, +12->vt[176](it, bit 0x20000000 of
// +40) and bit 0x2000 of +12->+172 cleared. True without a target +12.
DECOMP(004325C0, Cmd_SetTrack_004325C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    if (a == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000D8760, {a, load<u32>(o + 24), 1})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 w = load<u32>(o + 40);
    if ((w & 0x08000000u) != 0) {
        const u32 a2 = load<u32>(o + 12);
        VCall(ctx, a2, 176, {a2, (w & 0x20000000u) != 0 ? 1u : 0u});
        const u32 a3 = load<u32>(o + 12);
        store<u32>(a3 + 172, load<u32>(a3 + 172) & ~0x2000u);
    }
    ctx.gpr[3] = 1;
}

// Cmd::Cmd(a, b, c, d, e, f, g): the base (Obj_InitVt_B20CC8(this, 1)),
// vtable 0x00B212E0, +12 = a, +16 = b and the bits 31..27 of +20 = bit 0 of
// c, d, e, f, g (r3 = that word)
DECOMP(00438D14, Cmd_Ctor_00438D14) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 c = arg<u32>(ctx, 3), d = arg<u32>(ctx, 4), e = arg<u32>(ctx, 5);
    const u32 f = arg<u32>(ctx, 6), g = arg<u32>(ctx, 7);
    call(ctx, o27fn::Obj_InitVt_B20CC8, {o, 1});
    u32 w = load<u32>(o + 20) & 0x07FFFFFFu;
    store<u32>(o + 12, a);
    w |= ((c & 1u) << 31) | ((d & 1u) << 30);
    store<u32>(o, 0x00B212E0);
    w |= (e & 1u) << 29;
    store<u32>(o + 16, b);
    w |= ((f & 1u) << 28) | ((g & 1u) << 27);
    store<u32>(o + 20, w);
    ctx.gpr[3] = w;
}

// bool f(u32 handle): with voice enabled (u8 0x010AEC60), the handle
// (serial low 16 / index high 16 below 0x010B0200->+24) names a live
// 272-byte entry of 0x010B0200->+28 (serial = +256 >> 15, flag 0x80 of
// the u8 +256 clear) whose voice +16 is playing (Voice_IsPlaying)
DECOMP(00443368, Voice_HandlePlaying_00443368) {
    const u32 h = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (load<u8>(0x010AEC60) == 0) return;
    const u32 lo = h & 0xFFFFu, hi = h >> 16;
    constexpr u32 kPool = 0x010B0200;
    if (lo == 0 || !(static_cast<s32>(hi) < static_cast<s32>(load<u32>(kPool + 24)))) return;
    const u32 e = load<u32>(kPool + 28) + hi * 272u;
    if (((load<u32>(e + 256) >> 15) & 0xFFFFu) != lo || (load<u8>(e + 256) & 0x80u) != 0) return;
    const u32 r = static_cast<u32>(call(ctx, 0x006D5E10, {load<u32>(e + 16)}));  // Voice_IsPlaying
    ctx.gpr[3] = r != 0 ? 1 : 0;
}

// void f(World* w): the player w->+0x1281E0 (when set) gets vt[20](it;
// *0x00484528) and Obj_RestoreFrom544(it); then w->+0x128584 (when set)
// gets vt[28]()
DECOMP(00484530, World_RestorePlayer_00484530) {
    const u32 w = arg<u32>(ctx, 0);
    const u32 p = load<u32>(w + 0x1281E0);
    ctx.gpr[3] = p;
    if (p == 0) return;
    ctx.fpr[1] = LfsAt(0x00484528);
    VCall(ctx, p, 20, {p});
    call(ctx, 0x000FCC3C, {load<u32>(w + 0x1281E0)});  // Obj_RestoreFrom544
    const u32 q = load<u32>(w + 0x128584);
    ctx.gpr[3] = 0x128584;  // the offset constant, as the original leaves it
    if (q != 0) VCall(ctx, q, 28, {q});
}

// bool Load(XmlCursor* c): into the settings 0x010C2C28 (when set): the
// bool 0x00AF0144 (+757), the f32 0x00AF022C (+760, default *0x004EE58C)
// and the color 0x00AF023C (+764, default 0x01164CA0); then +756 = 1
DECOMP(004EE590, Settings_LoadA_004EE590) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x004FA504, {c, 0x00AF0144, g + 757, 0});  // XmlCursor_ReadBoolByName
    callf(ctx, 0x004FA5C4, {c, 0x00AF022C, g + 760}, {LfsAt(0x004EE58C)});  // XmlCursor_ReadF32ByName
    call(ctx, 0x004FA684, {c, 0x00AF023C, g + 764, 0x01164CA0});  // XmlCursor_ReadColorByName
    store<u8>(g + 756, 1);
    ctx.gpr[3] = 1;
}

// bool Load(XmlCursor* c): into the settings 0x010C2C28 (when set): the
// bool 0x00AF0144 (+817), the s32 0x00AF0268 (its low byte into +818) and
// the f32 0x00AF022C (+820, default *0x004EEA78); then +816 = 1
DECOMP(004EEA80, Settings_LoadB_004EEA80) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x010C2C28);
    if (g == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    call(ctx, 0x004FA504, {c, 0x00AF0144, g + 817, 0});  // XmlCursor_ReadBoolByName
    {
        Stack frame(ctx, 16);
        call(ctx, 0x004FA564, {c, 0x00AF0268, frame.addr(), 0});  // XmlCursor_ReadS32ByName
        store<u8>(g + 818, static_cast<u8>(load<u32>(frame.addr())));
    }
    callf(ctx, 0x004FA5C4, {c, 0x00AF022C, g + 820}, {LfsAt(0x004EEA78)});  // XmlCursor_ReadF32ByName
    store<u8>(g + 816, 1);
    ctx.gpr[3] = 1;
}

// void f(Rng* r, f32* out; f32 lo, f32 hi): two steps of the inlined
// random generator (+0 u64 last value, +8 seed; see RandStep28, only the
// final state stored) give a = lo + (hi - lo) * v0 * k, b = lo + (hi - lo)
// * v1 * k (k = *0x00735468); out = (a, b) scaled to the length
// *0x0073546C
DECOMP(00735470, Rng_RandomDir2_00735470) {
    const u32 r = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const f64 lo = farg(ctx, 0), hi = farg(ctx, 1);
    const u64 old = load<u64>(r);
    const u32 s = load<u32>(r + 8);
    const u32 x1 = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    const u32 x2 = (std::rotl(x1, 20) + x1) ^ 0x87654321u;
    const f64 d = Sub28(hi, lo);
    const f64 k = LfsAt(0x00735468);
    const f32 v0 = static_cast<f32>(static_cast<f64>(static_cast<s64>(old)));
    const f32 v1 = static_cast<f32>(static_cast<f64>(x1 & 0xFFFFu));
    store<u32>(r + 8, std::rotl(x2, 16));
    store<u64>(r, x2 & 0xFFFFu);
    const f32 a = Madd28(d, Mul28(v0, k), lo);
    const f32 b = Madd28(d, Mul28(v1, k), lo);
    const f32 len = static_cast<f32>(std::sqrt(static_cast<f64>(Madd28(a, a, Mul28(b, b)))));
    const f32 sc = static_cast<f32>(LfsAt(0x0073546C) / len);
    store<f32>(out, Mul28(a, sc));
    const f32 y = Mul28(b, sc);
    store<f32>(out + 4, y);
    ctx.gpr[3] = out;
    ctx.fpr[1] = y;
}

// bool f(T* o, ?, b; f32 t): o->vt[16], vt[20] and vt[24], each (o, -, b;
// t) (r4 = the OPD, as the original leaves it). Returns 1.
DECOMP(007378C4, Obj_Apply3_007378C4) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0);
    for (u32 slot : {16u, 20u, 24u}) {
        const u32 opd = load<u32>(load<u32>(o) + slot);
        ctx.fpr[1] = t;
        call_ptr(ctx, opd, {o, opd, b, load<u32>(opd)});
    }
    ctx.gpr[3] = 1;
}

// void f(Quat* q, const Vec3* axis; f32 angle): the rotation by angle
// around axis (normalized to *0x0074BEEC): q = (axis * sin(a), cos(a)) with
// a = angle * *0x0074BEE8
DECOMP(0074BEF0, Quat_FromAxisAngle_0074BEF0) {
    const u32 q = arg<u32>(ctx, 0), ax = arg<u32>(ctx, 1);
    const f64 y = LfsAt(ax + 4), x = LfsAt(ax), z = LfsAt(ax + 8);
    constexpr u32 K = 0x0074BEE8;
    const f32 half = Mul28(farg(ctx, 0), LfsAt(K));
    const f32 n2 = Madd28(z, z, Madd28(x, x, Mul28(y, y)));
    const f32 inv = static_cast<f32>(LfsAt(K + 4) / static_cast<f64>(static_cast<f32>(std::sqrt(static_cast<f64>(n2)))));
    f32 qx = Mul28(x, inv), qy = Mul28(y, inv), qz = Mul28(z, inv);
    const f64 s = callf(ctx, 0x007345F0, {}, {half});  // Math_Sinf
    qx = Mul28(s, qx);
    qy = Mul28(s, qy);
    qz = Mul28(s, qz);
    const f64 c = callf(ctx, 0x007344E0, {}, {half});  // Math_Cosf
    store<f32>(q, qx);
    store<f32>(q + 4, qy);
    store<f32>(q + 8, qz);
    store<f32>(q + 12, static_cast<f32>(c));
}

// void T::Init(a): +128.. = the vector 0x01164B40, +144.. = the 4 words at
// 0x01164C80 (raw), +160..+172 / +180 / +196 / +204 / +208 / +216..+224
// and the u8s +0 / +64 cleared, the bits 0xFF000000 of +176 cleared, +192
// = a, +200 / +212 = -1
DECOMP(00755708, Obj_Init_00755708) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    store<u32>(o + 128, load<u32>(0x01164B40));
    store<u32>(o + 132, load<u32>(0x01164B44));
    const u32 z = load<u32>(0x01164B48);
    const u32 w = load<u32>(o + 176) & 0x00FFFFFFu;
    store<u32>(o + 136, z);
    u32 last = 0;
    for (u32 k = 0; k < 4; ++k) {
        last = load<u32>(0x01164C80 + 4 * k);
        store<u32>(o + 144 + 4 * k, last);
    }
    for (u32 off : {160u, 164u, 168u, 172u}) store<u32>(o + off, 0);
    store<u32>(o + 176, w);
    store<u32>(o + 180, 0);
    store<u8>(o, 0);
    store<u8>(o + 64, 0);
    store<u32>(o + 192, a);
    store<u32>(o + 196, 0);
    store<u32>(o + 200, 0xFFFFFFFFu);
    store<u32>(o + 204, 0);
    store<u32>(o + 208, 0);
    store<u32>(o + 212, 0xFFFFFFFFu);
    store<u32>(o + 216, 0);
    store<u32>(o + 220, 0);
    store<u32>(o + 224, 0);
    ctx.fpr[1] = F32Bits28(last);
}

// void f(a, b, Ref* n): the component Effect_FindComponent(a, b) of kind 5
// (vt[32]) gets its +28 replaced by n (the old one gets vt[8]()), n's u16
// +4 incremented
DECOMP(00767CB8, Effect_SetRef_00767CB8) {
    const u32 n = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {arg<u32>(ctx, 0), arg<u32>(ctx, 1)}));
    if (c == 0) return;
    if (static_cast<s32>(VCall(ctx, c, 32, {c})) != 5) return;
    const u32 old = load<u32>(c + 28);
    ctx.gpr[3] = 0;
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(c + 28, n);
    if (n == 0) return;
    store<u16>(n + 4, static_cast<u16>(load<u16>(n + 4) + 1));
    ctx.gpr[3] = n + 4;
}

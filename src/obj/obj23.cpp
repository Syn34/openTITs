// Game code, part 23: the continued sweep of small still-recompiled game
// functions (in size order, from 17 instructions up). Most have no known
// caller context yet, so their names describe what they do and carry the
// address. Each was checked against the original in the PPC simulator.
//
// Register notes: where the original leaves a value in r3..r9 or f1..f5
// that a caller could read, the value is set the same way, as the comments
// say ("r4 left ...").
#include "obj23.h"

using namespace sly4;
using namespace sly4::obj23;

namespace {
constexpr u32 kUnlockSkip = 0x010BB849;     // u8: entries 0, 1 and 4 always open (name?)
constexpr u32 kSaveTaskState = 0x00F9D1A8;  // u32 shared by the save threads: 2 = ok (name?)
}  // namespace

// ---------------------------------------------------------------- sweep: 17 instructions
// bool f(?, s32 i, u32 x, u32 thr): entries 0, 1 and 4 are open while the
// byte at 0x010BB849 is set; otherwise x >= thr. r4 left as that byte when
// it was read.
DECOMP(0046C7E0, Unlock_IsOpen_0046C7E0) {
    const s32 i = arg<s32>(ctx, 1);
    if (i == 0 || i == 1 || i == 4) {
        const u32 b = load<u8>(kUnlockSkip);
        ctx.gpr[4] = b;
        if (b != 0) {
            ret(ctx, 1u);
            return;
        }
    }
    ret(ctx, arg<u32>(ctx, 2) < arg<u32>(ctx, 3) ? 0u : 1u);
}
// void Binding_Reserve(Binding* b, u32 n): b->+72 = n (the capacity), b->+68
// = Mem_Alloc(n * 4, 0, 16) (the name array)
DECOMP(004D2BB8, Binding_Reserve) {
    const u32 b = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    store<u32>(b + 72, n);
    const u32 p = static_cast<u32>(call(ctx, o22fn::Mem_Alloc, {n << 2, 0, 16}));
    store<u32>(b + 68, p);
}
// Material* f(u32 hash): s = Shader_FindByHash(hash); 0 when there is none,
// else Material_Alloc() with +16 = s (Material_Create inlined)
DECOMP(004FF46C, Material_CreateByHash) {
    const u32 s = static_cast<u32>(call(ctx, o23fn::Shader_FindByHash, {ctx.gpr[3]}));
    if (s == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 m = static_cast<u32>(call(ctx, o23fn::Material_Alloc, {s}));
    store<u32>(m + 16, s);
}
// T* f(): the world's +4216 object when its +192 has bit 0x2, else 0 (r4
// left as the result, r5 as the bit when tested)
DECOMP(00734470, World_Get4216IfFlag2_00734470) {
    const u32 p = load<u32>(load<u32>(kWorldPtr) + 4208 + 8);
    u32 r = 0;
    if (p != 0) {
        const u32 f = load<u32>(p + 192) & 0x2u;
        ctx.gpr[5] = f;
        if (f != 0) r = p;
    }
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// f(Obj* o, u8 a, u8 b): o's virtual +48 (o, a, b)
DECOMP(007375B0, Obj_VCall48U8_007375B0) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 48, {o, arg<u32>(ctx, 1) & 0xFFu, arg<u32>(ctx, 2) & 0xFFu});
}
// f32 f(const Obj* o): the u8 o->+308->+12 indexed (stride 4) by
// (*(T**)0x00F94B84)->+64 picks k[1] (set) or k[0] (f32 k[2] at `k`);
// r3 left as 0 / k, r4 as the index * 4, r5 as k.
static void SelByFlagF94B84(PPUContext& ctx, u32 k) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F94B74 + 16) + 64) << 2;
    const u32 b = load<u8>(tbl + off);
    ctx.gpr[4] = off;
    ctx.gpr[5] = k;
    ctx.gpr[3] = b != 0 ? k : 0u;
    ctx.fpr[1] = LfsAt(b != 0 ? k + 4 : k);
}
// f32 f(const Obj* o): the flag picks 0x00738EFC or 0x00738EF8
DECOMP(00738F00, Obj_SelF32ByFlag_00738F00) { SelByFlagF94B84(ctx, 0x00738EF8); }
// f32 f(const Obj* o): the flag picks 0x00738F4C or 0x00738F48
DECOMP(00738F50, Obj_SelF32ByFlag_00738F50) { SelByFlagF94B84(ctx, 0x00738F48); }
// ~T() entered through the base at +380: p = this - 380, +380 = 0x00B1200C,
// +0 = 0x00B126B8, then the base destructor 0x000DF934(p) (r4 / r5 = the
// vtables, as left)
DECOMP(00744468, AdjDtor380_00744468) {
    const u32 o = arg<u32>(ctx, 0) - 380;
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o, 0x00B1200C, 0x00B126B8});
}
// void f(Vec3* out, const Obj* o): out = o->+656..+664 * *(f32*)0x01057358 *
// o->+192 (f1..f4 / r5 left)
DECOMP(0074C394, Obj_ScaledVec656_0074C394) {
    const u32 d = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    constexpr u32 kBase = 0x01057310;
    const f32 k = load<f32>(kBase + 72), s = load<f32>(o + 192);
    const f32 x = (load<f32>(o + 656) * k) * s, y = (load<f32>(o + 660) * k) * s, z = (load<f32>(o + 664) * k) * s;
    store<f32>(d, x);
    store<f32>(d + 4, y);
    store<f32>(d + 8, z);
    ctx.gpr[5] = kBase;
    ctx.fpr[1] = x;
    ctx.fpr[2] = y;
    ctx.fpr[3] = z;
    ctx.fpr[4] = s;
}
// void f(Obj* o, const Vec3* v): v == 0 clears bit 0x20000000 of o->+1268;
// otherwise sets it and copies v to o->+1248..+1256 (f1 left as v->z)
DECOMP(00755890, Obj_SetVec1248_00755890) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    if (v == 0) {
        const u32 w = load<u32>(o + 1268) & ~0x20000000u;
        store<u32>(o + 1268, w);
        ctx.gpr[5] = w;  // r5 left
        return;
    }
    const u32 w = load<u32>(o + 1268) | 0x20000000u;
    store<u32>(o + 1268, w);
    ctx.gpr[5] = w;  // r5 left
    CopyW(o + 1248, v, 0);
    CopyW(o + 1248, v, 4);
    ctx.fpr[1] = F32Bits(CopyW(o + 1248, v, 8));
}
// f(Obj* o): the member at o + 80's virtual +16 (o + 80)
DECOMP(0076380C, Member80_VCall16_0076380C) {
    const u32 m = arg<u32>(ctx, 0) + 80;
    VCall(ctx, m, 16, {m});
}
// f(Obj* o): the member at o + 80's virtual +16 (o + 80)
DECOMP(00763A6C, Member80_VCall16_00763A6C) {
    const u32 m = arg<u32>(ctx, 0) + 80;
    VCall(ctx, m, 16, {m});
}
// char* f(const Obj* o, char* dst): sprintf(dst, "0x%08x", o->+20); returns dst
DECOMP(00793DD4, Obj_FormatHex20_00793DD4) {
    const u32 o = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1);
    call(ctx, o23fn::String_Printf, {dst, 0x00B21D18, load<u32>(o + 20)});  // "0x%08x"
    ret(ctx, dst);
}

// ---------------------------------------------------------------- sweep: 18 instructions
// f32 f(const Obj* o): *(f32*)0x00048CD4 when bit 0x4000 of the word at
// o->+60->+188 is set or the top nibble's low 3 bits of o->+68->+416 are 1,
// else *(f32*)0x00048CD8 (r3 left as 0x00048CD4, r4 as the bit)
DECOMP(00048CE0, Obj_SelF32_00048CE0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(load<u32>(o + 60) + 188) & 0x4000u;
    ctx.gpr[4] = b;
    bool first = b != 0;
    if (!first) first = ((load<u32>(load<u32>(o + 68) + 416) >> 28) & 7u) == 1;
    ctx.gpr[3] = 0x00048CD4;
    ctx.fpr[1] = LfsAt(first ? 0x00048CD4 : 0x00048CD8);
}
// f32 f(const Obj* o): *(f32*)0x00057770 unless o->+308 is 9 and o->+4 is not
// 10..12, then *(f32*)0x0005776C (r3 left as 0x0005776C, r4 as +308)
DECOMP(00057778, Obj_SelF32_00057778) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 308);
    ctx.gpr[4] = s;
    bool second = true;
    if (s == 9) {
        const s32 k = load<s32>(o + 4);
        second = k == 10 || k == 11 || k == 12;
    }
    ctx.gpr[3] = 0x0005776C;
    ctx.fpr[1] = LfsAt(second ? 0x00057770 : 0x0005776C);
}
// f(Obj* o): sub_00488DF4(world, o + 12, o + 144, o + 152) (r7 = the world too)
DECOMP(00083560, World_Call488DF4_00083560) {
    const u32 o = arg<u32>(ctx, 0), w = load<u32>(kWorldPtr);
    call(ctx, 0x00488DF4, {w, o + 12, o + 144, o + 152, w});
}
// f(const Obj* o): sub_00464488(&rec[o->+12] + 176, o->+16, o->+20) with the
// 11968-byte records at 0x010B19E0 (r4 / r5 as the original passes them:
// the same address and the offset, both 64-bit)
DECOMP(00088E98, Rec_Call464488_00088E98) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 off = static_cast<u64>(load<u32>(o + 12)) * 11968u;
    const u64 a = 0x010B19E0ull + off + 176;
    callf(ctx, 0x00464488, {static_cast<u32>(a), a, off}, {LfsAt(o + 16), LfsAt(o + 20)});
}
// The save-data delete thread: cellSaveDataDelete2(-1), the save task state
// at 0x00F9D1A8 = 2, sys_ppu_thread_exit(0) (r4 / r5 as left)
DECOMP(000B0B84, SaveDelete_Thread_000B0B84) {
    call(ctx, o23imp::cellSaveDataDelete2, {0xFFFFFFFFu});
    store<u32>(kSaveTaskState, 2);
    call(ctx, o23imp::sys_ppu_thread_exit, {0, 0x00FA0000, 2});
}
// ~T(): +384 = 0x00B12528, +380 = 0x00B1200C, +0 = 0x00B126B8, then the base
// destructor 0x000DF934(this) (r4..r6 = the vtables, as left)
DECOMP(0016819C, Dtor_Vt3_0016819C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o, 0x00B12528, 0x00B1200C, 0x00B126B8});
}
// f(Obj* o, u8 b): bit 0x100 of o->+356->+136 = (b == 0) when there is a
// +356, then sub_000E7730(o, b) (r5 = b, r6 / r7 as left)
DECOMP(001AE980, Obj_SetFlag356_001AE980) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 356);
    if (c != 0) {
        const u32 z = static_cast<u32>(std::countl_zero(b));
        const u32 w = (load<u32>(c + 136) & ~0x100u) | ((z << 3) & 0x100u);
        store<u32>(c + 136, w);
        ctx.gpr[6] = z;
        ctx.gpr[7] = w;
    }
    call(ctx, 0x000E7730, {o, b, b});
}
// f32 f(const Obj* o): sub_00922918(o->+416 + 224) when there is a +416, else
// *(f32*)0x001FB170 (r3 left as that address)
DECOMP(001FB178, Obj_Get416F32_001FB178) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 416);
    if (p != 0) {
        call(ctx, 0x00922918, {p + 224});
        return;
    }
    ctx.gpr[3] = 0x001FB170;
    ctx.fpr[1] = LfsAt(0x001FB170);
}
// void f(Vec3* out, const Obj* o): out = o->+388 + 432 (raw words; r4..r8 /
// f1 left)
DECOMP(00206F6C, Obj_GetVec388_00206F6C) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 q = load<u32>(arg<u32>(ctx, 1) + 388) + 432;
    CopyW(d, q, 0);
    CopyW(d, q, 4);
    ctx.fpr[1] = F32Bits(CopyW(d, q, 8));
    ctx.gpr[4] = q + 8;
    ctx.gpr[5] = d + 4;
    ctx.gpr[6] = d + 8;
    ctx.gpr[7] = q + 4;
    ctx.gpr[8] = static_cast<u64>(q) + 8;
}
// void f(Vec3* out, const Obj* o): out = o->+144 + (o->+396 + 944)->+80
// (f1..f5 / r4 / r5 left)
DECOMP(0023C614, Obj_AddVec144_0023C614) {
    const u32 d = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 a = o + 144, q = load<u32>(o + 396) + 944;
    const f32 x = load<f32>(q + 80) + load<f32>(a), y = load<f32>(q + 84) + load<f32>(a + 4), z = load<f32>(q + 88) + load<f32>(a + 8);
    store<f32>(d, x);
    store<f32>(d + 4, y);
    store<f32>(d + 8, z);
    ctx.gpr[4] = q;
    ctx.gpr[5] = a;
    ctx.fpr[1] = x;
    ctx.fpr[2] = y;
    ctx.fpr[3] = z;
    ctx.fpr[4] = LfsAt(q + 88);
    ctx.fpr[5] = LfsAt(q + 84);
}
// f(?, Obj* p): p's virtual +48 (p) when p is set (r3 = 0 otherwise)
DECOMP(0024C784, Obj_VCall48IfSet_0024C784) {
    const u32 p = arg<u32>(ctx, 1);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 48, {p});
}
// f(Obj* o, f32 x): sub_000D85A4(o, x), then sub_0024C860(o, x)
DECOMP(0024D6B0, Obj_Call2_0024D6B0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    callf(ctx, 0x000D85A4, {o}, {x});
    callf(ctx, 0x0024C860, {o}, {x});
}
// f(Obj* o, u8 b): o->+16 = b, then o's virtual +48 (o, b, 1)
DECOMP(0025FA1C, Obj_SetByte16VCall48_0025FA1C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 16, static_cast<u8>(b));
    VCall(ctx, o, 48, {o, b, 1});
}
// f(Obj* o, u8 b): o->+16 = b, then o's virtual +48 (o, b, 1)
DECOMP(0028BBD0, Obj_SetByte16VCall48_0028BBD0) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    store<u8>(o + 16, static_cast<u8>(b));
    VCall(ctx, o, 48, {o, b, 1});
}
// bool f(const Obj* o, s32 key): one of the o->+64 144-byte entries at
// o->+24 has +48 == key (r5..r8 left as the loop leaves them)
DECOMP(002B940C, Obj_Has144Key_002B940C) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const s32 n = load<s32>(o + 64);
    ctx.gpr[5] = 0;
    ctx.gpr[6] = static_cast<u32>(n);
    u32 p = load<u32>(o + 24);
    for (s32 i = 0; i < n; ++i, p += 144) {
        const u32 k = load<u32>(p + 48);
        ctx.gpr[7] = static_cast<u32>(i);
        ctx.gpr[8] = k;
        if (k == key) {
            ret(ctx, 1u);
            return;
        }
        ctx.gpr[7] = static_cast<u32>(i + 1);
    }
    if (n <= 0) ctx.gpr[7] = 0;
    ret(ctx, 0u);
}
// f(Component* c): Anim_Set2549F8(c, 1); c->+368 = 0; the bits 0x7F800000
// of c->+384 = 6 << 23 (r4 / r5 left)
DECOMP(002C0D4C, Anim_Reset_002C0D4C) {
    const u32 c = arg<u32>(ctx, 0);
    call(ctx, 0x002549F8, {c, 1});
    const u32 w = (load<u32>(c + 384) & ~0x7F800000u) | (6u << 23);
    store<u32>(c + 368, 0);
    store<u32>(c + 384, w);
    ctx.gpr[3] = w;
    ctx.gpr[4] = 6;
    ctx.gpr[5] = 0;
}
// f(const Obj* o): o->+32's virtual +92 (p, 1, -1)
DECOMP(00314FB8, Member32_VCall92_00314FB8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    VCall(ctx, p, 92, {p, 1, ~0ull});
}
// void f(void** pp): Mem_FreeFrom(*pp, 0) and *pp = 0 when *pp is set
DECOMP(00321240, Ptr_FreeAndClear_00321240) {
    const u32 pp = arg<u32>(ctx, 0);
    const u32 p = load<u32>(pp);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0054EE6C, {p, 0});
    store<u32>(pp, 0);
    ret(ctx, 0u);
}
// void f(Obj* o, Node* n): appends n to the list at o->+2576 (linked by
// +2564); n->+2568 = o (r5 left 0, r6 as the old tail)
DECOMP(00321CF4, Obj_AppendNode_00321CF4) {
    const u32 o = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    u32 p = load<u32>(o + 2576);
    if (p == 0) {
        store<u32>(o + 2576, n);
    } else {
        u32 nx;
        while ((nx = load<u32>(p + 2564)) != 0) p = nx;
        ctx.gpr[6] = p;
        store<u32>(p + 2564, n);
    }
    ctx.gpr[5] = 0;
    store<u32>(n + 2568, o);
}
// f(Obj* p): p's virtual +12 (p) when p is set (r3 / r4 = 0 otherwise)
DECOMP(0035B3BC, Obj_VCall12IfSet_0035B3BC) {
    const u32 p = arg<u32>(ctx, 0);
    if (p == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    VCall(ctx, p, 12, {p});
}
// f(Obj* o): Music_SetNode449E48(o->+12, o->+16 (u8), 0), then o->+8 |= 0x40000000
DECOMP(00438CCC, Obj_Play_00438CCC) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00449E48, {load<u32>(o + 12), load<u8>(o + 16), 0});
    call(ctx, 0x0042CA60, {o});
}
// f(Obj* o): World_ShowHeadline(world, 0), then o->+8 |= 0x40000000
DECOMP(00439E04, Obj_Headline_00439E04) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00482B20, {load<u32>(kWorldPtr), 0, 0x00F60000});
    call(ctx, 0x0042CA60, {o});
}
// bool f(Reader* r): the words 0x010AC8E0, 0x00F65B34 and 0x00F65B38 = 0,
// Iff_ReadChunks(r, 0); returns 1
DECOMP(0044E454, Iff_ReadReset_0044E454) {
    store<u32>(0x010AC8E0, 0);
    store<u32>(0x00F65B34, 0);
    store<u32>(0x00F65B38, 0);
    call(ctx, 0x00569388, {ctx.gpr[3], 0, 0, 0x00F60000, 0x00F60000});
    ret(ctx, 1u);
}
// ~T(): vtable 0x00B219D0; SmallFree(+8, +24) when +8 is set (r4 = 0 / r6
// = the vtable as left)
static void DtorB219D0(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 8);
    store<u32>(o, 0x00B219D0);
    ctx.gpr[6] = 0x00B219D0;
    if (p == 0) {
        ctx.gpr[4] = 0;
        return;
    }
    call(ctx, obj19::o19fn::SmallFree, {p, load<u32>(o + 24), o, 0x00B219D0});
}
DECOMP(00464AD0, Dtor_B219D0_00464AD0) { DtorB219D0(ctx); }
DECOMP(00490B34, Dtor_B219D0_00490B34) { DtorB219D0(ctx); }
// f(u32 off, ...): p = *(T**)(off + 0x0012847C); target(p, 1) when p is set
// and has bit 31 of +164 (r4 left as 0x0012847C or the bit)
static void Call12847C(PPUContext& ctx, u32 target) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 0x0012847Cu);
    ctx.gpr[4] = 0x0012847C;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u32>(p + 164) >> 31;
    ctx.gpr[4] = b;
    if (b == 0) {
        ret(ctx, p);
        return;
    }
    call(ctx, target, {p, 1});
}
DECOMP(00489EAC, Slot_Call425E54_00489EAC) { Call12847C(ctx, 0x00425E54); }
DECOMP(00489FD4, Slot_Call426A90_00489FD4) { Call12847C(ctx, 0x00426A90); }
// f(Obj* o, Effect* e): Effect_Bind4BB7E0(e, o->+12, 0), then
// sub_004CC68C(o, that, o->+16 (u8))
DECOMP(004CC704, Obj_Bind_004CC704) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = static_cast<u32>(call(ctx, 0x004BB7E0, {arg<u32>(ctx, 1), load<u32>(o + 12), 0}));
    call(ctx, 0x004CC68C, {o, b, load<u8>(o + 16)});
}
// Node* f(s32 key, const Node* after): the first node from after->+152
// (or the list head at 0x00F66868 when after is 0), linked by +156, whose
// +128 is key; 0 when none (r4 / r5 as the loop leaves them)
DECOMP(004D54FC, Node_FindByKey_004D54FC) {
    const u32 key = arg<u32>(ctx, 0), after = arg<u32>(ctx, 1);
    u32 p = after != 0 ? load<u32>(after + 152) : load<u32>(0x00F66868);
    while (p != 0) {
        const u32 k = load<u32>(p + 128);
        ctx.gpr[5] = k;
        if (k == key) break;
        p = load<u32>(p + 156);
    }
    ctx.gpr[4] = p;
    ret(ctx, p);
}
// T::T(): vtable 0x00B23090, +4 = 1, +8 / +12 = -1, +16 / +20 / +28 = 0,
// +24 bit 0x80 cleared, +25 = 0, +26 (u16) = 0 (r4..r8 left)
DECOMP(004F88FC, Ctor_B23090_004F88FC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u8>(o + 24) & ~0x80u;
    store<u32>(o + 12, ~0u);
    store<u32>(o + 8, ~0u);
    store<u32>(o + 16, 0);
    store<u32>(o + 4, 1);
    store<u32>(o + 20, 0);
    store<u8>(o + 24, static_cast<u8>(b));
    store<u32>(o + 28, 0);
    store<u8>(o + 25, 0);
    store<u16>(o + 26, 0);
    store<u32>(o, 0x00B23090);
    ctx.gpr[4] = ~0ull;
    ctx.gpr[5] = 1;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 0x00B23090;
    ctx.gpr[8] = b;
}
// void f(Vec3* v): v *= *(f32*)0x00734AC0 / |v| (single precision, fused;
// f1..f5 / r4 left)
DECOMP(00734AC8, Vec3_Normalize_00734AC8) {
    const u32 v = arg<u32>(ctx, 0);
    constexpr u32 kK = 0x00734AC0;
    const f64 x = LfsAt(v), y = LfsAt(v + 4), z = LfsAt(v + 8), k = LfsAt(kK);
    f64 s = static_cast<f32>(y * y);
    s = static_cast<f32>(std::fma(x, x, s));
    s = static_cast<f32>(std::fma(z, z, s));
    s = static_cast<f32>(std::sqrt(s));
    s = static_cast<f32>(k / s);
    const f32 rx = static_cast<f32>(x * s), ry = static_cast<f32>(y * s), rz = static_cast<f32>(z * s);
    store<f32>(v, rx);
    store<f32>(v + 4, ry);
    store<f32>(v + 8, rz);
    ctx.gpr[4] = kK;
    ctx.fpr[1] = ry;
    ctx.fpr[2] = rx;
    ctx.fpr[3] = rz;
    ctx.fpr[4] = z;
    ctx.fpr[5] = k;
}
// bool f(const Obj* o): bit 0x20000 or 0x40000 of the low word of o->+184,
// or o->+352 > *(f32*)0x00737788 (r4..r6 / f1 / f2 left)
DECOMP(00737790, Obj_Test_00737790) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 d = load<u64>(o + 184);
    const u32 lo = static_cast<u32>(d);
    ctx.gpr[5] = 0;
    ctx.gpr[6] = lo & 0x20000u;
    ctx.gpr[4] = d;
    u32 r = 1;
    if ((lo & 0x20000u) == 0) {
        ctx.gpr[4] = lo & 0x40000u;
        if ((lo & 0x40000u) == 0) {
            const f64 a = LfsAt(o + 352), c = LfsAt(0x00737788);
            ctx.gpr[4] = 0x00737788;
            ctx.fpr[1] = a;
            ctx.fpr[2] = c;
            r = a > c ? 1u : 0u;
        }
    }
    ctx.gpr[5] = r;
    ret(ctx, r);
}
// f(Obj* o, a, b): o's virtual +352 with r4 = the OPD (left), r5 = a, r6 = b,
// f1 = o->+344
DECOMP(0074C634, Obj_VCall352_0074C634) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5];
    const u32 opd = load<u32>(load<u32>(o) + 352);
    ctx.fpr[1] = LfsAt(o + 344);
    call_ptr(ctx, opd, {o, opd, a, b, load<u32>(opd)});
}
// ~T(): +384 = 0x00B12528, +380 = 0x00B1200C, +0 = 0x00B126B8, then the base
// destructor 0x000DF934(this) (r4..r6 = the vtables, as left)
DECOMP(00758FF0, Dtor_Vt3_00758FF0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 384, 0x00B12528);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, o23fn::VtB12550_Dtor, {o, 0x00B12528, 0x00B1200C, 0x00B126B8});
}
// f(Obj* o): o's virtual +24 (o) with f1 = *(f32*)0x00764298
DECOMP(007642A0, Obj_VCall24F_007642A0) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.fpr[1] = LfsAt(0x00764298);
    VCall(ctx, o, 24, {o});
}
// f(Obj* o): sub_0092D430(o->+20, o + 8), then AnimSkel_Release7D4EA8(o)
DECOMP(007714A8, Obj_Release_007714A8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0092D430, {load<u32>(o + 20), o + 8});
    call(ctx, 0x007D4EA8, {o});
}
// u32 f(Obj* o): o's virtual +24 into a 16-byte stack result (r3 = the
// result, r4 = o, r5 passed through); returns 0
DECOMP(00788814, Obj_VCall24Discard_00788814) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 opd = load<u32>(load<u32>(o) + 24);
    call_ptr(ctx, opd, {frame.addr(), o, ctx.gpr[5], opd, load<u32>(opd)});
    ret(ctx, 0u);
}
// char* f(const Obj* o, char* dst): sprintf(dst, "%d", o->+20); returns dst
DECOMP(00793E5C, Obj_FormatInt20_00793E5C) {
    const u32 o = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1);
    const s64 v = load<s32>(o + 20);
    call(ctx, o23fn::String_Printf, {dst, 0x00B21D68, static_cast<u64>(v)});  // "%d"
    ret(ctx, dst);
}
// f(Obj* o, bool b): bit 0x20000000 of o->+136 = b, then o's virtual +148 (o)
DECOMP(0079630C, Obj_SetFlag136VCall148_0079630C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = (load<u32>(o + 136) & ~0x20000000u) | ((arg<u32>(ctx, 1) << 29) & 0x20000000u);
    store<u32>(o + 136, w);
    VCall(ctx, o, 148, {o});
}

// ---------------------------------------------------------------- sweep: 19 instructions
// f(const Obj* o): o->+244's virtual +44 (p, 1) when there is one
DECOMP(0001CCA8, Member244_VCall44_1_0001CCA8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 244);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 44, {p, 1});
}
// f(const Obj* o): o->+244's virtual +44 (p, 0) when there is one
DECOMP(0001CCF4, Member244_VCall44_0_0001CCF4) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 244);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 44, {p, 0});
}
// f(Obj* o): o's virtual +220 (o, 34) unless o->+4 is 39 (r4 left as +4 then)
DECOMP(0003BA6C, Obj_VCall220_0003BA6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 4);
    if (k == 39) {
        ctx.gpr[4] = k;
        return;
    }
    VCall(ctx, o, 220, {o, 34});
}
// f(Obj* o): o->+20 = Sound_Find(o->+16) when +20 is 0 and +16 is set
DECOMP(0006BB10, Obj_FindSound_0006BB10) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = load<u32>(o + 20);
    if (r == 0) {
        r = load<u32>(o + 16);
        if (r != 0) {
            r = static_cast<u32>(call(ctx, 0x0044C43C, {r}));
            store<u32>(o + 20, r);
        }
    }
    ret(ctx, r);
}
// f(): o = Obj_FindById(0xCD4F65FC); SetByte_1870_0005BB08(o->+244) when both are set
DECOMP(000806D4, Obj_Mark244_000806D4) {
    u32 p = static_cast<u32>(call(ctx, 0x000D6A28, {0xCD4F65FCu}));
    if (p == 0) return;
    p = load<u32>(p + 244);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x0005BB08, {p});
}
// f(const Obj* o): sub_0044A8A8(o->+12, (f32)o->+16 (s32), o->+20) (r4 = r3,
// r5 = the word unextended, as the original passes them)
DECOMP(0008C8A4, Obj_Call44A8A8_0008C8A4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 12);
    const u64 sa = static_cast<u64>(static_cast<s64>(static_cast<s32>(a)));
    const f64 x = static_cast<f32>(static_cast<f64>(load<s32>(o + 16)));
    callf(ctx, 0x0044A8A8, {sa, sa, a}, {x, LfsAt(o + 20)});
}
// f(Obj* o): o->+20 = Sound_Find(o->+16) when +20 is 0 and +16 is set
DECOMP(0009A398, Obj_FindSound_0009A398) {
    const u32 o = arg<u32>(ctx, 0);
    u32 r = load<u32>(o + 20);
    if (r == 0) {
        r = load<u32>(o + 16);
        if (r != 0) {
            r = static_cast<u32>(call(ctx, 0x0044C43C, {r}));
            store<u32>(o + 20, r);
        }
    }
    ret(ctx, r);
}
// bool f(const Obj* o, s32 key): one of the o->+345 (u8) words at o + 368 is
// key (searched from the last; r5..r7 as the loop leaves them)
DECOMP(000FC5DC, Obj_HasWord368_000FC5DC) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 n = load<u8>(o + 345);
    ctx.gpr[6] = n;
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 off = n << 2;
    do {
        off -= 4;
        --n;
        const u32 v = load<u32>(o + 368 + off);
        ctx.gpr[5] = off;
        ctx.gpr[6] = n;
        ctx.gpr[7] = v;
        if (v == key) {
            ret(ctx, 1u);
            return;
        }
    } while (n != 0);
    ret(ctx, 0u);
}
// f(Obj* o, T* d): Thunk_000E44C0(o, d); d->+208 / +212 = o->+240 / +244 (f1 left)
DECOMP(001DEB58, Obj_Copy240_001DEB58) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000E44C0, {o, d});
    Cp(d + 208, o + 240);
    ctx.fpr[1] = F32Bits(Cp(d + 212, o + 244));
}
// f(Obj* o, T* d): sub_000CDB28(o, d); d->+144 = o->+180, d->+148 = o->+184 (u8)
DECOMP(0020AB8C, Obj_Copy180_0020AB8C) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, d});
    const u32 w = load<u32>(o + 180), b = load<u8>(o + 184);
    store<u32>(d + 144, w);
    store<u8>(d + 148, static_cast<u8>(b));
    ctx.gpr[3] = w;  // r3 / r4 left
    ctx.gpr[4] = b;
}
// f(Obj* o, a): sub_0020EBC8(o, a) unless o->+428 is 0 or 3 (then r3 = a,
// r4 = o, r5 = +428 left)
DECOMP(0020FF60, Obj_Call20EBC8_0020FF60) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    const s32 s = load<s32>(o + 428);
    if (s == 0 || s == 3) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = o;
        ctx.gpr[5] = static_cast<u32>(s);
        return;
    }
    call(ctx, 0x0020EBC8, {o, a, a});
}
// bool f(Obj* o): 1 unless bit 31 of o->+412 is set, then sub_0020DFD8(o, 1) != 0
DECOMP(00218D78, Obj_Test_00218D78) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 412) >> 31;
    if (b == 0) {
        ctx.gpr[4] = 0;
        ret(ctx, 1u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0020DFD8, {o, b}));
    ret(ctx, r != 0 ? 1u : 0u);
}
// bool f(const Obj* o): o->+428 is set and SlotRec_Byte464_002D98A4(it, 0) is 1
// (r4 left as that test)
DECOMP(00220E14, Obj_Test428_00220E14) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 428);
    u32 t = 0;
    if (p != 0) t = static_cast<u32>(call(ctx, 0x002D98A4, {p, 0})) == 1 ? 1u : 0u;
    ctx.gpr[4] = t;
    ret(ctx, t);
}
// The zero vector at 0x01164B40 (name?)
constexpr u32 kVec1164B40 = 0x01164B40;
// void f(Obj* o): bit 0x40000000 of o->+608->+1268 cleared (when set); o->+448..
// +456 = the vector at 0x01164B40, +464 = +568 = 0 (r4..r6 / f1 left)
DECOMP(0023FF8C, Obj_Reset448_0023FF8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 608);
    if (p != 0) store<u32>(p + 1268, load<u32>(p + 1268) & ~0x40000000u);
    CopyW(o + 448, kVec1164B40, 0);
    CopyW(o + 448, kVec1164B40, 4);
    const u32 f = load<u32>(kVec1164B40 + 8);
    store<u32>(o + 464, 0);
    store<u32>(o + 456, f);
    store<u32>(o + 568, 0);
    ctx.gpr[4] = 0x01160000;
    ctx.gpr[5] = kVec1164B40;
    ctx.gpr[6] = 0;
    ctx.fpr[1] = F32Bits(f);
}
// void f(Vec3* out, const Obj* o): out = o->+216->+800 (raw), or the vector at
// 0x01164B40 when there is no +216 (r4 / r5 / f1 left)
DECOMP(0024D6F8, Obj_GetVec800_0024D6F8) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 p = load<u32>(arg<u32>(ctx, 1) + 216);
    const u32 s = p != 0 ? p + 800 : kVec1164B40;
    CopyW(d, s, 0);
    CopyW(d, s, 4);
    ctx.fpr[1] = F32Bits(CopyW(d, s, 8));
    ctx.gpr[4] = p;
    if (p == 0) {
        ctx.gpr[4] = 0x01160000;
        ctx.gpr[5] = kVec1164B40;
    }
}
// void f(Obj* o, bool b): b ? o->+84 = 0 : memset(o + 84, -1, 4)
DECOMP(0027C8C8, Obj_Reset84_0027C8C8) {
    const u32 p = arg<u32>(ctx, 0) + 84;
    if ((arg<u32>(ctx, 1) & 0xFFu) != 0) {
        ctx.gpr[4] = 0;  // r4 left
        store<u32>(p, 0);
        ret(ctx, p);
        return;
    }
    call(ctx, fn::memset, {p, ~0ull, 4});
}
// void f(Obj* o, u8 b): o->+624 = b; bit 0 of the byte (*(u8**)0x011950AC)[o->+240
// * 128] = b != 0 (r3..r7 left)
DECOMP(0028BB84, Obj_SetFlag624_0028BB84) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    constexpr u32 kTab = 0x011950A0;
    const u32 i = load<u32>(o + 240);
    store<u8>(o + 624, static_cast<u8>(b));
    const u32 p = load<u32>(kTab + 12) + (i << 7);
    const u32 v = b != 0 ? (load<u8>(p) | 1u) : (load<u8>(p) & ~1u);
    store<u8>(p, static_cast<u8>(v));
    ctx.gpr[3] = p;
    ctx.gpr[4] = v;
    ctx.gpr[5] = kTab;
    ctx.gpr[6] = b;
    ctx.gpr[7] = i;
}
// void f(Rec112* a, u32 i, const Vec3* v, u32 w, u16 h): a[i] (112 bytes):
// +96..+104 = v (raw), +112 = w, +116 = h (r3 / r4 / r8 / f1 left)
DECOMP(002BCA90, Rec112_Set_002BCA90) {
    const u32 e = arg<u32>(ctx, 0) + arg<u32>(ctx, 1) * 112u + 96;
    const u32 v = arg<u32>(ctx, 2);
    CopyW(e, v, 0);
    const u32 f1 = load<u32>(v + 4);
    store<u32>(e + 16, arg<u32>(ctx, 3));
    store<u16>(e + 20, static_cast<u16>(ctx.gpr[7]));
    store<u32>(e + 4, f1);
    ctx.fpr[1] = F32Bits(CopyW(e, v, 8));
    ctx.gpr[3] = e + 20;
    ctx.gpr[4] = e;
    ctx.gpr[8] = e + 16;
}
// T* f(?, s32 i): i < the world's +4296 count (u16) ? (+4288)[i] : 0; 0 for i = -1
// (r4 / r5 left)
DECOMP(002E6A3C, World_Get4288_002E6A3C) {
    const u32 i = arg<u32>(ctx, 1);
    if (i == ~0u) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(kWorldPtr) + 4208;
    const u32 n = load<u16>(w + 88);
    ctx.gpr[5] = n;
    if (i >= n) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = i << 2;
    ret(ctx, load<u32>(load<u32>(w + 80) + (i << 2)));
}
// bool f(Obj* o): unless +40 and +41 (u8) are both set: +41 = 1 and the byte
// +72 of o->+4->+0->+32->+76->+444 = 1 when that is set; returns 1 (r4..r6 left)
DECOMP(002FD768, Obj_Mark_002FD768) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = load<u8>(o + 40);
    u32 q = load<u32>(o + 4);
    ctx.gpr[5] = 1;
    if (b != 0) {
        b = load<u8>(o + 41);
        if (b != 0) {
            ctx.gpr[4] = q;
            ctx.gpr[6] = b;
            ret(ctx, 1u);
            return;
        }
    }
    ctx.gpr[6] = b;
    q = load<u32>(q);
    store<u8>(o + 41, 1);
    q = load<u32>(load<u32>(load<u32>(q + 32) + 76) + 444);
    if (q != 0) store<u8>(q + 72, 1);
    ctx.gpr[4] = q;
    ret(ctx, 1u);
}
// void f(Obj* o, const Msg* m): m->+8 == 0xF41445FE sets the byte o->+2681,
// 0x3281C093 clears it (r4 / r5 left)
DECOMP(003351D0, Obj_Msg2681_003351D0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 k = load<s32>(arg<u32>(ctx, 1) + 8);
    ctx.gpr[4] = static_cast<u32>(k);
    if (k > static_cast<s32>(0xF41445FEu)) {
        ctx.gpr[5] = 0x3281C093u;
        if (k == 0x3281C093) {
            ctx.gpr[4] = 0;
            store<u8>(o + 2681, 0);
        }
        return;
    }
    ctx.gpr[5] = 0xFFFFFFFFF41445FDull;
    if (k > static_cast<s32>(0xF41445FDu)) {
        ctx.gpr[4] = 1;
        store<u8>(o + 2681, 1);
    }
}
// f(const Obj* o): Obj_SetAnim(o->+244, 0, 1, 0x003AAB00, 0, 0, 0x01164CA4,
// *(f32*)0x003AAB00)
DECOMP(003AAB08, Obj_SetAnim_003AAB08) {
    constexpr u32 k = 0x003AAB00;
    callf(ctx, 0x00363A3C, {load<u32>(arg<u32>(ctx, 0) + 244), 0, 1, k, 0, 0, 0x01164CA4}, {LfsAt(k)});
}
// f(Obj* o, ?, a): sub_003AC070(o, p, a, &v) with p = o->+108->+96 + 144 and v
// a stack copy of p's vector (f3 left as its z; f1 / f2 passed through)
DECOMP(003AC244, Obj_Call3AC070_003AC244) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(load<u32>(o + 108) + 96) + 144;
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    Cp(v, p);
    Cp(v + 4, p + 4);
    ctx.fpr[3] = F32Bits(Cp(v + 8, p + 8));
    call(ctx, 0x003AC070, {o, p, ctx.gpr[5], v});
}
// u32 f(): the id cached at `cache`, made on first use: ++(the current
// level's +4->+116->+48) via (*(T**)0x010B19E0)->+0x1281D4 (r4 / r5 left)
static void CachedId(PPUContext& ctx, u32 cache) {
    u32 v = load<u32>(cache);
    if (v == 0) {
        const u32 t = load<u32>(load<u32>(load<u32>(0x010B19E0) + 0x001281D4u) + 4);
        const u32 u = load<u32>(t + 116);
        v = load<u32>(u + 48) + 1;
        store<u32>(u + 48, v);
        store<u32>(cache, v);
        ctx.gpr[4] = u;
        ctx.gpr[5] = 0x00F60000;
    }
    ret(ctx, v);
}
DECOMP(003DD238, CachedId_F65818_003DD238) { CachedId(ctx, 0x00F65818); }
DECOMP(003F11F0, CachedId_F6581C_003F11F0) { CachedId(ctx, 0x00F6581C); }
// u32 f(?, op, u8 k): unless k is 2, fios_Free(sub_005FC90C(k, op), op); returns 0
DECOMP(00450624, Fios_Free_00450624) {
    const u32 k = arg<u32>(ctx, 2) & 0xFFu;
    const u64 op = ctx.gpr[4];
    if (k != 2) {
        const u64 s = call(ctx, 0x005FC90C, {k, op, ctx.gpr[5]});
        call(ctx, 0x005FE784, {s, op});
    }
    ret(ctx, 0u);
}
// void f(Obj* o): o->+100..+108 = 0, then +100 = +108 = *(f32*)0x010BC170,
// +104 = *(f32*)0x010BC1C0, o->+0 |= 0x40 (r4..r7 / f1 / f2 left)
DECOMP(0045C81C, Obj_Reset100_0045C81C) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 kA = 0x010BC128, kB = 0x010BC178;
    store<u32>(o + 100, 0);
    store<u32>(o + 104, 0);
    store<u32>(o + 108, 0);
    const u32 w = load<u32>(o) | 0x40u;
    const u32 a = load<u32>(kA + 72), b = load<u32>(kB + 72);
    store<u32>(o + 100, a);
    store<u32>(o + 108, a);
    store<u32>(o + 104, b);
    store<u32>(o, w);
    ctx.gpr[4] = w;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = kB;
    ctx.gpr[7] = kA;
    ctx.fpr[1] = F32Bits(a);
    ctx.fpr[2] = F32Bits(b);
}
// s32 f(s32 key): the index of key among the 5 16-byte entries at 0x00AE812C
// (their +0), or 0xFFFFFFFF; 0 is never found (r4..r6 left)
DECOMP(00465E10, Table_IndexOf_00465E10) {
    const u32 key = arg<u32>(ctx, 0);
    constexpr u32 kTab = 0x00AE812C;
    if (key != 0) {
        ctx.gpr[4] = kTab;
        for (u32 i = 0; i < 5; ++i) {
            const u32 v = load<u32>(kTab + (i << 4));
            ctx.gpr[5] = i;
            ctx.gpr[6] = v;
            if (v == key) {
                ret(ctx, i);
                return;
            }
        }
        ctx.gpr[5] = 5;
    }
    ret(ctx, 0xFFFFFFFFu);
}
// void f(T* t, f32 a, f32 b, f32 c, f32 d): +0 = a, +4 = b, +12 = c, +16 = d,
// +20 = 0, +24 |= 0x80000000, +8 = c > *(f32*)0x0048EF4C ? a : b (r4..r6 /
// f2 / f4 left)
DECOMP(0048EF50, Rec_Set_0048EF50) {
    const u32 t = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), c = farg(ctx, 2);
    f64 b = farg(ctx, 1);
    store<f32>(t, static_cast<f32>(a));
    store<f32>(t + 4, static_cast<f32>(b));
    store<f32>(t + 12, static_cast<f32>(c));
    store<f32>(t + 16, static_cast<f32>(farg(ctx, 3)));
    const f64 k = LfsAt(0x0048EF4C);
    const u32 w = load<u32>(t + 24) | 0x80000000u;
    store<u32>(t + 20, 0);
    store<u32>(t + 24, w);
    if (c > k) b = a;
    store<f32>(t + 8, static_cast<f32>(b));
    ctx.gpr[4] = w;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0xFFFFFFFF80000000ull;
    ctx.fpr[2] = b;
    ctx.fpr[4] = k;
}
// T::T(): vtable vt, +4..+12 = 0, +16: u16 0 then bit 0 kept, +19 bit 0 set,
// +20..+32 = 0 (r4..r7 left)
static void InitVt36(PPUContext& ctx, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 16) & 1u;
    store<u32>(o + 4, 0);
    store<u32>(o + 8, 0);
    store<u16>(o + 16, 0);
    store<u32>(o + 12, 0);
    store<u32>(o + 16, w);
    store<u32>(o, vt);
    store<u8>(o + 19, static_cast<u8>(w | 1u));
    for (u32 off = 20; off <= 32; off += 4) store<u32>(o + off, 0);
    ctx.gpr[4] = 0;
    ctx.gpr[5] = vt;
    ctx.gpr[6] = w;
    ctx.gpr[7] = w | 1u;
}
DECOMP(004A0C4C, Ctor_B21B70_004A0C4C) { InitVt36(ctx, 0x00B21B70); }
DECOMP(004A0DB0, Ctor_B21B88_004A0DB0) { InitVt36(ctx, 0x00B21B88); }
// f32 f(f32 a, f32 tol, f32 b): d = |b - a|; tol < d ? k[0] : the polynomial
// k[2] - t * (k[1] * (k[3]... (fused: u = k[2] - t * k[1]; k[3] - t^2 * u)
// with t = d / tol and f32 k[4] at 0x004A6B84 (r3 left as that address)
DECOMP(004A6B98, Falloff_004A6B98) {
    constexpr u32 k = 0x004A6B84;
    const f64 tol = farg(ctx, 1);
    const f64 d = std::fabs(static_cast<f64>(static_cast<f32>(farg(ctx, 2) - farg(ctx, 0))));
    ctx.gpr[3] = k;
    if (tol < d) {
        ctx.fpr[1] = LfsAt(k);
        return;
    }
    const f64 t = static_cast<f32>(d / tol);
    const f64 k1 = LfsAt(k + 4), k2 = LfsAt(k + 8), k3 = LfsAt(k + 12);
    const f64 t2 = static_cast<f32>(t * t);
    const f64 u = static_cast<f32>(-std::fma(t, k1, -k2));
    ctx.fpr[1] = static_cast<f32>(-std::fma(t2, u, -k3));
    ctx.fpr[2] = k1;
    ctx.fpr[3] = k2;
    ctx.fpr[4] = k3;
    ctx.fpr[5] = t2;
}
// void f(): the two list heads at 0x010C2BC0 / 0x010C2CAC: +0 / +4 / +100 = 0,
// +12 = itself, +96 (u16) = 0, +8 (u8) = 1 (r3..r8 left)
DECOMP(004F32A4, Lists_Init_004F32A4) {
    constexpr u32 a = 0x010C2BC0, b = 0x010C2CAC;
    store<u32>(a, 0);
    store<u32>(a + 12, a);
    store<u32>(a + 4, 0);
    store<u32>(a + 100, 0);
    store<u16>(a + 96, 0);
    store<u8>(a + 8, 1);
    store<u32>(b, 0);
    store<u32>(b + 4, 0);
    store<u32>(b + 12, b);
    store<u16>(b + 96, 0);
    store<u32>(b + 100, 0);
    store<u8>(b + 8, 1);
    ctx.gpr[3] = 0x010C0000;
    ctx.gpr[4] = 1;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0x010C0000;
    ctx.gpr[7] = a;
    ctx.gpr[8] = b;
}
// T::T(o, a, v): the base 0x004FBFA8(o, a, v), +72 = v (u8 / u32), vtable vt
static void CtorBase4FBFA8(PPUContext& ctx, u32 vt, u32 size) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 v = ctx.gpr[5];
    call(ctx, 0x004FBFA8, {o, ctx.gpr[4], v});
    if (size == 1) store<u8>(o + 72, static_cast<u8>(v));
    else store<u32>(o + 72, static_cast<u32>(v));
    store<u32>(o, vt);
    ctx.gpr[3] = vt;  // r3 left
}
DECOMP(00735FB0, Ctor_B0D934_00735FB0) { CtorBase4FBFA8(ctx, 0x00B0D934, 1); }
DECOMP(007360A8, Ctor_B0D968_007360A8) { CtorBase4FBFA8(ctx, 0x00B0D968, 4); }
DECOMP(0073650C, Ctor_B0DA6C_0073650C) { CtorBase4FBFA8(ctx, 0x00B0DA6C, 4); }
// T::T(o, a, ?, f32 x): the base 0x004FBFA8(o, a), +72 = x, vtable 0x00B0D99C
DECOMP(0073616C, Ctor_B0D99C_0073616C) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    call(ctx, 0x004FBFA8, {o, ctx.gpr[4], ctx.gpr[5]});  // f1 still x
    store<f32>(o + 72, static_cast<f32>(x));
    store<u32>(o, 0x00B0D99C);
    ctx.gpr[3] = 0x00B0D99C;  // r3 left
}
// T::T(o, a, const Vec3* v, d): +4 = a, +8 = 7, +12 = d, +16 = +20 = 0,
// +48..+56 = v (raw), vtable 0x00B0DC5C (r4 / r7..r9 / f1 left; the caller
// 0x00067738 relies on r7 = 0 in its next call, to 0x007366F4)
DECOMP(007368F8, Ctor_B0DC5C_007368F8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    store<u32>(o + 4, arg<u32>(ctx, 1));
    store<u32>(o + 8, 7);
    store<u32>(o + 12, arg<u32>(ctx, 3));
    store<u32>(o + 16, 0);
    store<u32>(o + 20, 0);
    Cp(o + 48, v);
    const u32 y = load<u32>(v + 4);
    store<u32>(o, 0x00B0DC5C);
    store<u32>(o + 52, y);
    ctx.fpr[1] = F32Bits(Cp(o + 56, v + 8));
    ctx.gpr[4] = 0x00B0DC5C;
    ctx.gpr[7] = 0;
    ctx.gpr[8] = 7;
    ctx.gpr[9] = o + 20;
}
// f(Obj* o, a): sub_0091A228(a, o + 380) unless bit 0x08000000 of
// o->+216->+300 is set (then r3 = a, r4 = o, r5 = the bit left)
DECOMP(00747C34, Obj_Call91A228_00747C34) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    const u32 b = (load<u32>(load<u32>(o + 216) + 300) >> 27) & 1u;
    if (b != 0) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = o;
        ctx.gpr[5] = b;
        return;
    }
    call(ctx, 0x0091A228, {a, o + 380, b});
}
// bool f(): |*(f32*)0x010592F0| < *(f32*)0x0074E778 or *(f32*)0x010592EC >
// *(f32*)0x0074E77C (r4 / r5 / f1 / f2 left)
DECOMP(0074E780, Global_Test_0074E780) {
    constexpr u32 kK = 0x0074E778;
    f64 a = std::fabs(LfsAt(0x010592F0)), b = LfsAt(kK);
    ctx.gpr[4] = 0x01060000;
    u32 r = 0;
    if (a < b) {
        r = 1;
    } else {
        a = LfsAt(kK + 4);
        b = LfsAt(0x010592EC);
        ctx.gpr[4] = kK;
        ctx.gpr[5] = 0x01060000;
        if (b > a) r = 1;
    }
    ctx.fpr[1] = a;
    ctx.fpr[2] = b;
    ret(ctx, r);
}
// void f(u8* p, f32 x): p[3] = (s32)(clamp(x) * k) with f32 k / f64 lo / f64
// hi at 0x00760F18 (fsel: k - x >= 0 ? x : lo, then t >= 0 ? t : hi; r4 / f1..f5
// left)
DECOMP(00760F30, Color_SetAlpha_00760F30) {
    constexpr u32 kK = 0x00760F18;
    const f64 x = farg(ctx, 0);
    const f64 k = LfsAt(kK), lo = std::bit_cast<f64>(load<u64>(kK + 8)), hi = std::bit_cast<f64>(load<u64>(kK + 16));
    f64 t = rt::to_single(k - x);
    t = rt::to_single(t >= 0.0 ? x : lo);
    t = rt::to_single(t >= 0.0 ? t : hi);
    t = rt::to_single(t * k);
    const u64 i = rt::fctiw(t, true);
    store<u8>(arg<u32>(ctx, 0) + 3, static_cast<u8>(i));
    ctx.gpr[4] = static_cast<u32>(i);
    ctx.fpr[1] = std::bit_cast<f64>(i);  // f1 left as the fctiwz bits (high word as rt::fctiw gives it)
    ctx.fpr[2] = x;
    ctx.fpr[3] = k;
    ctx.fpr[4] = lo;
    ctx.fpr[5] = hi;
}
// ~T(): the vtables from 0x00B1F3FC (+0 / +8 / +12 / +16), then HkBase_Dtor940F24
// (r4..r7 = the vtables, as left)
DECOMP(0076EB2C, Dtor_VtB1F3FC_0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 t = 0x00B1F3FC;
    store<u32>(o, t + 8);
    store<u32>(o + 8, t + 56);
    store<u32>(o + 12, t + 80);
    store<u32>(o + 16, t + 104);
    call(ctx, 0x00940F24, {o, t + 104, t + 8, t + 56, t + 80});
}
// f(Obj* o) entered through the base at +8: the same as 0x007714A8 on o - 8
DECOMP(007714F0, Obj_Release_Adj8_007714F0) {
    const u32 o = arg<u32>(ctx, 0) - 8;
    call(ctx, 0x0092D430, {load<u32>(o + 20), o + 8});
    call(ctx, 0x007D4EA8, {o});
}
// void f(Vec3* out, const Obj* o): out = o->+1488 (u8) ? o->+1472 : o->+144
// (raw words; r4 / r5 / f1 left)
DECOMP(0077CDC0, Obj_GetVec_0077CDC0) {
    const u32 d = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 b = load<u8>(o + 1488);
    ctx.gpr[5] = b;
    const u32 s = b != 0 ? o + 1472 : o + 144;
    if (b == 0) ctx.gpr[4] = s;
    CopyW(d, s, 0);
    CopyW(d, s, 4);
    ctx.fpr[1] = F32Bits(CopyW(d, s, 8));
}
// char* f(const Obj* o, char* dst): sprintf(dst, fmt, (double)o->+20) (the
// double in r5 and f1); returns dst
static void FormatF20(PPUContext& ctx, u32 fmt) {
    const u32 o = arg<u32>(ctx, 0), dst = arg<u32>(ctx, 1);
    const f64 x = LfsAt(o + 20);
    ctx.fpr[1] = x;
    call(ctx, o23fn::String_Printf, {dst, fmt, std::bit_cast<u64>(x)});
    ret(ctx, dst);
}
DECOMP(00793EE0, Obj_FormatF20_00793EE0) { FormatF20(ctx, 0x00B21DB8); }
DECOMP(00794134, Obj_FormatF20_00794134) { FormatF20(ctx, 0x00B21F80); }
// f(Obj* o, const Vec2* v): o->+off / +off+4 = v (f1 / f2 = v), then o's virtual +148 (o)
static void SetVec2VCall148(PPUContext& ctx, u32 off) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 x = load<u32>(v), y = load<u32>(v + 4);
    store<u32>(o + off, x);
    store<u32>(o + off + 4, y);
    ctx.fpr[1] = F32Bits(x);
    ctx.fpr[2] = F32Bits(y);
    VCall(ctx, o, 148, {o});
}
DECOMP(00796354, Obj_SetVec176_00796354) { SetVec2VCall148(ctx, 176); }
DECOMP(007963A0, Obj_SetVec184_007963A0) { SetVec2VCall148(ctx, 184); }

// ---------------------------------------------------------------- sweep: 20 instructions
// f32 f(const Obj* o): *(f32*)0x0003CFA4 when o->+60 is set with bit 0x4000 of
// its +188 word, or the top nibble's low 3 bits of o->+68->+416 are 1; else
// *(f32*)0x0003CFA8 (r3 left as 0x0003CFA4, r4 as +60 / the bit)
DECOMP(0003CFB0, Obj_SelF32_0003CFB0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 60);
    bool first = false;
    ctx.gpr[4] = p;
    if (p != 0) {
        const u32 b = load<u32>(p + 188) & 0x4000u;
        ctx.gpr[4] = b;
        first = b != 0;
    }
    if (!first) first = ((load<u32>(load<u32>(o + 68) + 416) >> 28) & 7u) == 1;
    ctx.gpr[3] = 0x0003CFA4;
    ctx.fpr[1] = LfsAt(first ? 0x0003CFA4 : 0x0003CFA8);
}
// f(const Obj* o): o->+16 ? sub_000B27B4(it) : sub_000B277C(o->+12) unless that is -1
// (r4 left as +16)
DECOMP(0007C8C8, Obj_Release_0007C8C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 16);
    if (p != 0) {
        call(ctx, 0x000B27B4, {p, p});
        return;
    }
    ctx.gpr[4] = 0;
    const s32 k = load<s32>(o + 12);
    if (k == -1) {
        ret(ctx, static_cast<u32>(k));
        return;
    }
    call(ctx, 0x000B277C, {static_cast<u64>(static_cast<s64>(k)), 0});
}
// f(const Obj* o, a): r = sub_0009A190(o, o->+12, a); sub_0048FA54(r, o->+16, 1) when r is set
DECOMP(00084DE4, Obj_Call48FA54_00084DE4) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x0009A190, {o, load<u32>(o + 12), ctx.gpr[4]}));
    if (r != 0) call(ctx, 0x0048FA54, {r, load<u32>(o + 16), 1});
}
// f(const Obj* o): bit 0x100000 of sub_0035B908(*(T**)0x00F64D48)->+960 = bit 0
// of o->+12 (u8) when there is one (r4 / r5 left)
DECOMP(0008AED0, Obj_SetFlag960_0008AED0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = static_cast<u32>(call(ctx, 0x0035B908, {load<u32>(0x00F64D48)}));
    if (p == 0) return;
    const u32 b = load<u8>(o + 12);
    const u32 w = (load<u32>(p + 960) & ~0x100000u) | ((b & 1u) << 20);
    store<u32>(p + 960, w);
    ctx.gpr[4] = b;
    ctx.gpr[5] = w;
}
// f(Obj* o, a): o->+360 = a ? sub_004BDEDC(a, 0, 1, 0) : 0 (r6 = 0 left)
DECOMP(000E13D0, Obj_Set360_000E13D0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    store<u32>(o + 360, 0);
    ctx.gpr[6] = 0;
    if (a == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x004BDEDC, {a, 0, 1, 0}));
    store<u32>(o + 360, r);
}
// f(Obj* o, a2, a3, u8 a4, ?, a6, a7, a8, a9): sub_000DBE78(o, a2, a3, a4, a9,
// a6, a7, a8, a9) (r7 = a9 too, as the original passes it); clears bit 31 of
// o->+468 when that returns nonzero (r4 left)
DECOMP(000FC968, Obj_Call0DBE78_000FC968) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a9 = load<u32>(static_cast<u32>(ctx.gpr[1]) + 116);
    const u32 r = static_cast<u32>(call(ctx, 0x000DBE78, {o, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6] & 0xFFu, a9, ctx.gpr[8], ctx.gpr[9], ctx.gpr[10], a9}));
    if (r == 0) return;
    const u32 w = load<u32>(o + 468) & 0x7FFFFFFFu;
    store<u32>(o + 468, w);
    ctx.gpr[4] = w;
}
// f(Obj* o): o->+104 = (world->+0x128168)->+60 (raw), then sub_00520338(that,
// f1 = *(f32*)0x0010BCD0) (r4..r7 as the original passes them)
DECOMP(0010BCD8, Obj_Call520338_0010BCD8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 p = load<u32>(w + 0x00128168u);
    Cp(o + 104, p + 60);
    callf(ctx, 0x00520338, {load<u32>(w + 0x00128168u), w, 0x00128168, 0x0010BCD0, p}, {LfsAt(0x0010BCD0)});
}
// f(a): p = sub_000D1288(a, 23, 0); sub_00276558(p, 0) when p's +24 bits 25..29
// are 7 (r4 left as those bits)
DECOMP(00129BC0, Obj_Call276558_00129BC0) {
    const u32 p = static_cast<u32>(call(ctx, 0x000D1288, {ctx.gpr[3], 23, 0}));
    if (p == 0) return;
    const u32 k = (load<u32>(p + 24) >> 25) & 31u;
    ctx.gpr[4] = k;
    if (k != 7) return;
    call(ctx, 0x00276558, {p, 0});
}
// bool f(?, const Obj* o): 1 unless o->+192 has bit 0x200; then 0 when o->+664
// is set, else o->+668 (u8) == 0 || o->+616 == 0 (r4 left as the last read)
DECOMP(0018E9FC, Obj_Test_0018E9FC) {
    const u32 o = arg<u32>(ctx, 1);
    ctx.gpr[4] = o;
    if ((load<u32>(o + 192) & 0x200u) == 0) {
        ret(ctx, 1u);
        return;
    }
    u32 v = load<u32>(o + 664);
    ctx.gpr[4] = v;
    if (v != 0) {
        ret(ctx, 0u);
        return;
    }
    v = load<u8>(o + 668);
    ctx.gpr[4] = v;
    ret(ctx, (v == 0 || load<u32>(o + 616) == 0) ? 1u : 0u);
}
// f(Obj* o, ?, a): sub_001AF470(o, o->+548, a, 1, f1 = o->+544); o->+853 =
// +854 = 0 and bit 0x80 of +855 cleared (r3 / r4 left)
DECOMP(001CFDB0, Obj_Call1AF470_001CFDB0) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x001AF470, {o, load<u32>(o + 548), ctx.gpr[5], 1}, {LfsAt(o + 544)});
    const u32 b = load<u8>(o + 855) & ~0x80u;
    store<u8>(o + 853, 0);
    store<u8>(o + 854, 0);
    store<u8>(o + 855, static_cast<u8>(b));
    ctx.gpr[3] = b;
    ctx.gpr[4] = 0;
}
// void f(Obj* o, f32 x, u8 scale): v = scale ? x * o->+496 : x; o->+588 = +584
// = v; o->+576->+20 = +496 > *(f32*)0x001DB970 ? v / +496 : that constant
// (when +576 is set; r4 / r5 / f1..f3 left)
DECOMP(001DB978, Obj_SetScale_001DB978) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 sc = arg<u32>(ctx, 2) & 0xFFu;
    const u32 p = load<u32>(o + 576);
    const f64 s = LfsAt(o + 496);
    f64 v = farg(ctx, 0);
    if (sc != 0) v = static_cast<f32>(v * s);
    store<f32>(o + 588, static_cast<f32>(v));
    store<f32>(o + 584, static_cast<f32>(v));
    f64 r = LfsAt(0x001DB970);
    if (s > r) r = static_cast<f32>(v / s);
    if (p != 0) store<f32>(p + 20, static_cast<f32>(r));
    ctx.gpr[4] = p;
    ctx.gpr[5] = 0x001DB970;
    ctx.fpr[1] = s;
    ctx.fpr[2] = v;
    ctx.fpr[3] = r;
}
// f32 f(const Obj* o): sub_00922918(p + 224) for p = o->+off when p and p->+8
// are set, else *(f32*)k (r3 / r4 left)
static void GetF32Off(PPUContext& ctx, u32 off, u32 k) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + off);
    ctx.fpr[1] = LfsAt(k);
    ctx.gpr[4] = k;
    if (p != 0) {
        const u32 q = load<u32>(p + 8);
        ctx.gpr[4] = q;
        if (q != 0) {
            call(ctx, 0x00922918, {p + 224, q});
            return;
        }
    }
    ret(ctx, p);
}
DECOMP(00244418, Obj_GetF32_396_00244418) { GetF32Off(ctx, 396, 0x00244414); }
DECOMP(0024D9C0, Obj_GetF32_416_0024D9C0) { GetF32Off(ctx, 416, 0x0024D9B8); }
// void f(Vec3* out, const Obj* o): out = bit 31 of o->+272 ? o->+304 : the
// vector at 0x01164B60 (raw; r4 / r5 / f1 left)
DECOMP(0026D824, Obj_GetVec304_0026D824) {
    const u32 d = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 b = load<u32>(o + 272) >> 31;
    ctx.gpr[5] = b;
    u32 s = o + 304;
    if (b == 0) {
        s = 0x01164B60;
        ctx.gpr[4] = 0x01160000;
        ctx.gpr[5] = s;
    }
    CopyW(d, s, 0);
    CopyW(d, s, 4);
    ctx.fpr[1] = F32Bits(CopyW(d, s, 8));
}
// f(const Obj* o): o->+72's virtual +48 (p) when there is one, else 0
DECOMP(002E207C, Member72_VCall48_002E207C) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 72);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, p, 48, {p});
}
// f(const Obj* o): o->+48's virtual +24 (p) when there is one, else 1
DECOMP(002FE0EC, Member48_VCall24_002FE0EC) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 48);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    VCall(ctx, p, 24, {p});
}
// s32 f(Obj* o, a, b): 5 when o->+18 (u8) is 4, else sub_003252D8(o, a, b)
// (r3 = 5, r5 = o, r6 = the byte left then)
DECOMP(00333478, Obj_Call3252D8_00333478) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4], b = ctx.gpr[5];
    const u32 k = load<u8>(o + 18);
    if (k == 4) {
        ctx.gpr[5] = o;
        ctx.gpr[6] = k;
        ret(ctx, 5u);
        return;
    }
    call(ctx, 0x003252D8, {o, a, b, b});
}
// f(?, a): sub_0046C3B8(world, a, 0, 0, 0, 0, 0, 0, 0, 0)
DECOMP(004200E8, World_Call46C3B8_004200E8) {
    call(ctx, 0x0046C3B8, {load<u32>(kWorldPtr), ctx.gpr[4], 0, 0, 0, 0, 0, 0, 0, 0});
}
// T* f(s32 key): the +0 of the first of the *(u32*)0x010AA690 16-byte entries
// at *(T**)0x00F65858 whose +4 is key, or 0 (r4..r9 as the loop leaves them)
DECOMP(0042C980, Table_Find_0042C980) {
    const u32 key = arg<u32>(ctx, 0);
    const u32 n = load<u32>(0x010AA690);
    ctx.gpr[5] = 0;
    ctx.gpr[6] = n;
    ctx.gpr[8] = 0;
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    u64 c = load<u32>(0x00F65858);
    for (u32 i = 0;;) {
        const u32 e = static_cast<u32>(c);
        const u32 v = load<u32>(e + 4);
        ctx.gpr[7] = e;
        ctx.gpr[9] = v;
        ctx.gpr[4] = c;
        if (v == key) {
            ctx.gpr[8] = i;
            ret(ctx, load<u32>(e));
            return;
        }
        ++i;
        c += 16;
        ctx.gpr[4] = c;
        ctx.gpr[8] = i;
        if (!(n > i)) break;
    }
    ret(ctx, 0u);
}
// bool f(const Obj* o): o->+12 is 0 or sub_003814A8(it) is nonzero
DECOMP(00433570, Obj_Test_00433570) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 12);
    u32 r = 1;
    if (p != 0 && call(ctx, 0x003814A8, {p}) == 0) r = 0;
    ret(ctx, r);
}
// T::T(o, f32 x): the base 0x0042C9D0(o, 1), +12 = x, vtable 0x00B21220 (r3 left)
DECOMP(00438498, Ctor_B21220_00438498) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    call(ctx, 0x0042C9D0, {o, 1});
    store<f32>(o + 12, static_cast<f32>(x));
    store<u32>(o, 0x00B21220);
    ctx.gpr[3] = 0x00B21220;
}
// T::T(o, a): the base 0x0042C9D0(o, 1), +12 = a, vtable 0x00B213A0 (r3 left)
DECOMP(0043A620, Ctor_B213A0_0043A620) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    call(ctx, 0x0042C9D0, {o, 1});
    store<u32>(o + 12, a);
    store<u32>(o, 0x00B213A0);
    ctx.gpr[3] = 0x00B213A0;
}
// void f(Node* n): unless n->+100 (u8) is set, n goes on the front of the list
// at 0x00F65B64 (+104 next, +108 prev), +100 = 1 and the byte 0x00F65B68 = 1
// (r4..r8 left)
DECOMP(00441C68, List_PushFront_00441C68) {
    const u32 n = arg<u32>(ctx, 0);
    constexpr u32 kHead = 0x00F65B64;
    const u32 b = load<u8>(n + 100);
    ctx.gpr[4] = b;
    if (b != 0) return;
    u32 h = load<u32>(kHead);
    if (h != 0) {
        store<u32>(h + 108, n);
        h = load<u32>(kHead);
        ctx.gpr[5] = 0x00F60000;
    }
    store<u32>(n + 104, h);
    store<u32>(n + 108, 0);
    store<u32>(kHead, n);
    store<u8>(n + 100, 1);
    store<u8>(kHead + 4, 1);
    ctx.gpr[4] = h;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0x00F60000;
    ctx.gpr[7] = 1;
    ctx.gpr[8] = 0x00F60000;
}
// s32 f(u32 hash): 0x782CF148 -> 2, 0x0464F605 -> 1, 0x00014065 -> 0, else -1
// (r4 left as the last constant compared)
DECOMP(0044D118, Hash_ToIndex_0044D118) {
    const u32 h = arg<u32>(ctx, 0);
    s32 r;
    if (h == 0x782CF148u) {
        ctx.gpr[4] = 0x782CF148u;
        r = 2;
    } else if (h == 0x0464F605u) {
        ctx.gpr[4] = 0x0464F605u;
        r = 1;
    } else {
        ctx.gpr[4] = 0x00014065u;
        r = h == 0x00014065u ? 0 : -1;
    }
    ret(ctx, r);
}
// f(Obj* o, a): sub_004A95FC(o->+32, o), then sub_008FFE68(o, a)
DECOMP(004A9764, Obj_Call2_004A9764) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    call(ctx, 0x004A95FC, {load<u32>(o + 32), o});
    call(ctx, 0x008FFE68, {o, a});
}
// void f(Arr8* a, s32 i): --a->+8 (u16 count); unless i was the last index the
// last 8-byte entry of a->+0 moves to [i] (r3..r6 / f1 left)
DECOMP(00735A20, Arr8_RemoveSwap_00735A20) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 i = arg<u32>(ctx, 1);
    const u64 n = static_cast<u64>(load<u16>(a + 8)) - 1;
    const u32 n16 = static_cast<u32>(n) & 0xFFFFu;
    store<u16>(a + 8, static_cast<u16>(n));
    ctx.gpr[6] = n16;
    ctx.gpr[5] = ctx.gpr[4];
    ctx.gpr[4] = n;
    if (i == n16) return;
    const u32 data = load<u32>(a);
    const u32 dst = data + (i << 3), src = data + (n16 << 3);
    const u32 w = load<u32>(src), f = load<u32>(src + 4);
    store<u32>(dst, w);
    store<u32>(dst + 4, f);
    ctx.gpr[3] = src;
    ctx.gpr[4] = dst;
    ctx.gpr[5] = w;
    ctx.fpr[1] = F32Bits(f);
}
// f32 f(const Obj* o): the table o->+308->+12: when its byte at the index
// (*(T**)0x00F9486C)->+64 is set, the f32 at the index (*(T**)0x00F947F4)->+64,
// else *(f32*)0x00738A90 (r4 / r5 left)
DECOMP(00738A98, Obj_SelF32_00738A98) {
    const u32 tbl = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 off = load<u32>(load<u32>(0x00F9486C) + 64) << 2;
    const u32 b = load<u8>(tbl + off);
    ctx.gpr[3] = tbl;
    ctx.gpr[5] = 0x00738A90;
    ctx.gpr[4] = b;
    ctx.fpr[1] = LfsAt(0x00738A90);
    if (b == 0) return;
    const u32 off2 = load<u32>(load<u32>(0x00F947F4) + 64) << 2;
    ctx.gpr[4] = off2;
    ctx.fpr[1] = LfsAt(tbl + off2);
}
// f(Obj* o, a) entered through the base at +384: sub_0091A228(a, p + 380) for
// p = o - 384 unless bit 0x08000000 of p->+216->+300 is set (then r3 = a, r4 =
// p, r5 = the bit, r6 = p->+216 left)
DECOMP(00747C80, Obj_Call91A228_Adj384_00747C80) {
    const u32 p = arg<u32>(ctx, 0) - 384;
    const u64 a = ctx.gpr[4];
    const u32 q = load<u32>(p + 216);
    const u32 b = (load<u32>(q + 300) >> 27) & 1u;
    ctx.gpr[6] = q;
    if (b != 0) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = p;
        ctx.gpr[5] = b;
        return;
    }
    call(ctx, 0x0091A228, {a, p + 380, b, q});
}
// void f(T* t, u32 i, f32 x, f32 y, u8 c): the 12-byte entry i at t + 0x128588
// = {x, y, c} (r3..r5 / r7 left)
DECOMP(00760B78, Entry12_Set_00760B78) {
    const u32 e = arg<u32>(ctx, 0) + arg<u32>(ctx, 1) * 12u + 0x00128588u;
    store<f32>(e, static_cast<f32>(farg(ctx, 0)));
    store<f32>(e + 4, static_cast<f32>(farg(ctx, 1)));
    store<u8>(e + 8, static_cast<u8>(ctx.gpr[6]));
    ctx.gpr[3] = e + 8;
    ctx.gpr[4] = 0x00128590;
    ctx.gpr[5] = e + 4;
    ctx.gpr[7] = e;
}
// u32 f(u32 key): *sub_0076611C(0x0106C228, &key, key) for key != 0, else 0
DECOMP(0077D114, Map_Get_0077D114) {
    const u32 key = arg<u32>(ctx, 0);
    if (key == 0) {
        ctx.gpr[4] = 0;
        ret(ctx, 0u);
        return;
    }
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);
    const u32 p = static_cast<u32>(call(ctx, 0x0076611C, {0x0106C228, frame.addr(), key}));
    ret(ctx, load<u32>(p));
}
// f(Obj* o, const Vec2* v): o's virtual +48 (o, &copy of v) (f1 / f2 = v)
DECOMP(00795CC8, Obj_VCall48Vec2_00795CC8) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Stack frame(ctx, 16);
    const u32 l = frame.addr();
    ctx.fpr[1] = F32Bits(Cp(l, v));
    ctx.fpr[2] = F32Bits(Cp(l + 4, v + 4));
    VCall(ctx, o, 48, {o, l});
}

// ---------------------------------------------------------------- sweep: 21 instructions
// f(const Obj* o): i = sub_000D14DC(o->+48, 0x001F0BAF); sub_000DA954(o->+48, i) when i >= 0
static void Call14DC_DA954(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 i = call(ctx, 0x000D14DC, {load<u32>(o + 48), 0x001F0BAF});
    if (static_cast<s32>(i) < 0) return;
    call(ctx, 0x000DA954, {load<u32>(o + 48), i});
}
DECOMP(00027CFC, Obj_Call0DA954_00027CFC) { Call14DC_DA954(ctx); }
DECOMP(0002BD64, Obj_Call0DA954_0002BD64) { Call14DC_DA954(ctx); }
// ~T(): vtable 0x00B0FF40, sub_004432DC(+24, the vtable), +24 = 0, then the
// base 0x004BB41C(this, 0)
DECOMP(00083C8C, Dtor_B0FF40_00083C8C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 24);
    store<u32>(o, 0x00B0FF40);
    call(ctx, 0x004432DC, {p, 0x00B0FF40});
    store<u32>(o + 24, 0);
    call(ctx, 0x004BB41C, {o, 0});
}
// f(const Obj* o): o->+16 (u8) ? sub_0048C82C(world, f1 = o->+12) : sub_0048C840(world, 0)
DECOMP(0008AD90, World_Call48C8_0008AD90) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    if (load<u8>(o + 16) != 0) callf(ctx, 0x0048C82C, {w, w}, {LfsAt(o + 12)});
    else call(ctx, 0x0048C840, {w, 0});
}
// f(const Obj* o, u8 b): r = sub_0034CC34(o->+608, 0); sub_00353CC0(r, b) when both are set
DECOMP(000FCFDC, Obj_Call353CC0_000FCFDC) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 608), b = arg<u32>(ctx, 1) & 0xFFu;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0034CC34, {p, 0}));
    if (r == 0) return;
    call(ctx, 0x00353CC0, {r, b});
}
// f(Obj* o): sub_00520338(world->+0x128168, f1 = o->+104), then o->+4->+356 =
// o->+100 (raw; r3 / f1 left)
DECOMP(0010BD28, Obj_Call520338_0010BD28) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    callf(ctx, 0x00520338, {load<u32>(w + 0x00128168u), w, ctx.gpr[5]}, {LfsAt(o + 104)});
    const u32 p = load<u32>(o + 4);
    ctx.fpr[1] = F32Bits(Cp(p + 356, o + 100));
    ctx.gpr[3] = p;
}
// f(Obj* o): r = o's virtual +64 (o); sub_00252940(r) when r is set, else 0
DECOMP(00129148, Obj_Call252940_00129148) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00252940, {r});
}
// T* f(const Obj* o): 0 when the world's +29 (u8) is 1, else the world's
// player slot (1 - o->+772) +4216 object (slots of 0x20C80 bytes; r4 / r5 left)
DECOMP(0016B59C, World_OtherSlot_0016B59C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 b = load<u8>(w + 29);
    ctx.gpr[4] = w;
    if (b == 1) {
        ctx.gpr[5] = b;
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[5] = 0x00020C80;
    const u32 s = (1u - load<u32>(o + 772)) * 0x00020C80u;
    ret(ctx, load<u32>(w + s + 4208 + 8));
}
// T* f(const Obj* o): the first of the list o->+1324->+272 (linked by +268)
// whose +208->+100 is o->+1572 (0 when that is 0 or none matches; r4 / r6 left)
DECOMP(00185C9C, Obj_FindLinked_00185C9C) {
    const u32 o = arg<u32>(ctx, 0);
    u32 p = load<u32>(load<u32>(o + 1324) + 272);
    ctx.gpr[4] = p;
    ctx.gpr[5] = 0;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 k = load<u32>(o + 1572);
    if (k == 0) {
        ret(ctx, 0u);
        return;
    }
    while (p != 0) {
        const u32 v = load<u32>(load<u32>(p + 208) + 100);
        ctx.gpr[6] = v;
        if (v == k) {
            ctx.gpr[5] = p;
            ret(ctx, p);
            return;
        }
        p = load<u32>(p + 268);
        ctx.gpr[4] = p;
    }
    ret(ctx, 0u);
}
// r f(Obj* o, a2..a6, u8 a7, ...): r = sub_000D9648(o, a2..a6, a7) (f1..f4
// passed through), then o->+736 = sub_000D94F4(o); returns r
DECOMP(001CC60C, Obj_Call0D9648_001CC60C) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 r = call(ctx, 0x000D9648, {o, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9] & 0xFFu});
    store<u32>(o + 736, static_cast<u32>(call(ctx, 0x000D94F4, {o})));
    ctx.gpr[3] = r;
}
// f32 f(const Obj* o): fma(*(f32*)0x01053BF8, *(f32*)0x001F0214,
// sub_005E6578(o->+772)) (r3 / f2 / f3 left)
DECOMP(001F0218, Obj_Wave_001F0218) {
    const u32 p = arg<u32>(ctx, 0) + 768;
    const f64 y = callf(ctx, 0x005E6578, {p}, {LfsAt(p + 4)});
    const f64 b = LfsAt(0x001F0214), a = LfsAt(0x01053BB0 + 72);
    ctx.gpr[3] = 0x01053BB0;
    ctx.fpr[2] = b;
    ctx.fpr[3] = a;
    ctx.fpr[1] = static_cast<f32>(std::fma(a, b, y));
}
// s32 f(Obj* o, a): 1 when o->+428 is 0 or 3 (r4 = o, r5 = +428 left), else
// sub_0020E7A4(o, a)
DECOMP(0020FF0C, Obj_Call20E7A4_0020FF0C) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    const s32 s = load<s32>(o + 428);
    if (s == 0 || s == 3) {
        ctx.gpr[4] = o;
        ctx.gpr[5] = static_cast<u32>(s);
        ret(ctx, 1u);
        return;
    }
    call(ctx, 0x0020E7A4, {o, a, a});
}
// void f(Vec4* d, const Vec4* s): *d = *s (raw words; r3..r8 / f1 left)
DECOMP(0026B30C, Vec4_Copy_0026B30C) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) Cp(d + k, s + k);
    ctx.fpr[1] = F32Bits(Cp(d + 12, s + 12));
    ctx.gpr[3] = d + 12;
    ctx.gpr[4] = s + 12;
    ctx.gpr[5] = s + 4;
    ctx.gpr[6] = d + 4;
    ctx.gpr[7] = d + 8;
    ctx.gpr[8] = s + 8;
}
// void f(Obj* o, u32 v): every o->+200 160-byte entry at o + 208 whose +132 /
// +136 are not both *(f32*)0x0029683C gets +140 = v (r5..r7 / f1 / f2 left)
DECOMP(00296840, Obj_SetEntries_00296840) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const f64 k = LfsAt(0x0029683C);
    ctx.fpr[1] = k;
    u64 c = o;
    u32 i = 0;
    for (;;) {
        const u32 n = load<u32>(o + 200);
        ctx.gpr[6] = n;
        if (!(i < n)) break;
        const u32 e = static_cast<u32>(c + 208);
        ctx.gpr[6] = e;
        f64 x = LfsAt(e + 132);
        bool set = x != k;
        if (!set) {
            x = LfsAt(e + 136);
            set = x != k;
        }
        ctx.fpr[2] = x;
        if (set) store<u32>(e + 140, v);
        c += 160;
        ++i;
    }
    ctx.gpr[5] = c;
    ctx.gpr[7] = i;
}
// f(const Obj* o, a): r = sub_0034CC34(o->+36, a); sub_00353DC8(r) when both are
// set (r3 = a, r4 = 0, r5 = o left when there is no +36)
DECOMP(00298748, Obj_Call353DC8_00298748) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    const u32 p = load<u32>(o + 36);
    if (p == 0) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = 0;
        ctx.gpr[5] = o;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0034CC34, {p, a, p}));
    if (r == 0) return;
    call(ctx, 0x00353DC8, {r});
}
// s32 f(const Obj* o, s32 key): the index of the first of the o->+32 536-byte
// entries at o->+28 whose +468 is key, or 0xFFFFFFFF (r5..r7 left)
DECOMP(002E1278, Obj_IndexOf536_002E1278) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 32);
    ctx.gpr[5] = n;
    ctx.gpr[6] = 0;
    if (n != 0) {
        const u32 base = load<u32>(o + 28);
        for (u32 i = 0; i < n; ++i) {
            const u32 v = load<u32>(base + i * 536u + 468);
            ctx.gpr[6] = i;
            ctx.gpr[7] = v;
            if (v == key) {
                ret(ctx, i);
                return;
            }
            ctx.gpr[6] = i + 1;
        }
    }
    ret(ctx, 0xFFFFFFFFu);
}
// f(Obj* o, a): v = 0; sub_004F99BC(a, 0x03D558EF, &v, 0); o->+4 = v (r3 left as v)
DECOMP(00303298, Obj_Get3D558EF_00303298) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    store<u32>(v, 0);
    call(ctx, 0x004F99BC, {ctx.gpr[4], 0x03D558EF, v, 0, 0, 0x03D50000});
    const u32 r = load<u32>(v);
    store<u32>(o + 4, r);
    ctx.gpr[3] = r;
}
// f(Obj* o): o->+484 = world->+0x1281E0; o->+480 = sub_000D14DC(o, 0x0021C5E0, that)
DECOMP(003059EC, Obj_Init480_003059EC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(kWorldPtr);
    const u32 x = load<u32>(w + 0x001281E0u);
    store<u32>(o + 484, x);
    store<u32>(o + 480, static_cast<u32>(call(ctx, 0x000D14DC, {o, 0x0021C5E0, x, 0x00210000, w})));
}
// u32 f(const Obj* o, f32* out): with o->+2544 set and +2540 < +2536 (s32):
// *out = o->+2552 (raw, when out is set) and returns (+2544)[+2540 * 2]; else 0
// (r5..r7 / f1 left)
DECOMP(00325DF8, Obj_Current_00325DF8) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    u32 p = load<u32>(o + 2544);
    ctx.gpr[6] = p;
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    u32 i = load<u32>(o + 2540);
    const u32 n = load<u32>(o + 2536);
    ctx.gpr[5] = i;
    ctx.gpr[7] = n;
    if (!(static_cast<s32>(i) < static_cast<s32>(n))) {
        ret(ctx, 0u);
        return;
    }
    if (out != 0) {
        ctx.fpr[1] = F32Bits(Cp(out, o + 2552));
        p = load<u32>(o + 2544);
        i = load<u32>(o + 2540);
        ctx.gpr[6] = p;
        ctx.gpr[5] = i;
    }
    ret(ctx, load<u32>(p + (i << 3)));
}
// bool f(const Obj* o): o->+18 (u8) is 5, or one of the list o->+2576 (linked
// by +2564) has +2230 (u8) set and +18 == 5 (r4 left as the last byte read)
DECOMP(00328470, Obj_AnyState5_00328470) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = load<u8>(o + 18);
    ctx.gpr[4] = b;
    if (b == 5) {
        ret(ctx, 1u);
        return;
    }
    for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) {
        b = load<u8>(p + 2230);
        ctx.gpr[4] = b;
        if (b == 0) continue;
        b = load<u8>(p + 18);
        ctx.gpr[4] = b;
        if (b == 5) {
            ret(ctx, 1u);
            return;
        }
    }
    ret(ctx, 0u);
}
// r f(const Obj* o): o->+8 ? its virtual `slot` (p) : dflt (r4 left as the result)
static void Member8VCall(PPUContext& ctx, u32 slot, u64 dflt) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 8);
    u64 r = dflt;
    if (p != 0) r = VCall(ctx, p, slot, {p});
    ctx.gpr[4] = r;
    ctx.gpr[3] = r;
}
DECOMP(003478C0, Member8_VCall36_003478C0) { Member8VCall(ctx, 36, 1); }
DECOMP(003479EC, Member8_VCall48_003479EC) { Member8VCall(ctx, 48, ~0ull); }
// f(?, a): the word 0x0106C27C = String_HashNoCase(sprintf(buf, fmt 0x00ADCB68, a))
DECOMP(00369CAC, Global_SetHash_00369CAC) {
    Stack frame(ctx, 112);
    const u32 buf = frame.addr();
    call(ctx, o23fn::String_Printf, {buf, 0x00ADCB68, ctx.gpr[4], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], ctx.gpr[10]});
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {buf}));
    store<u32>(0x0106C27C, h);
    ctx.gpr[4] = 0x01070000;  // r4 left
}
// f(Obj* o, a): o->+180 = a; sub_004CF988(o->+20, a + 0x02B81827, a) when +20 is
// set (r3 = a, r4 = 0, r5 = o left otherwise)
DECOMP(0036EA80, Obj_Set180_0036EA80) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 20);
    store<u32>(o + 180, a);
    if (p == 0) {
        ctx.gpr[3] = ctx.gpr[4];
        ctx.gpr[4] = 0;
        ctx.gpr[5] = o;
        return;
    }
    call(ctx, 0x004CF988, {p, a + 0x02B81827u, a, 0x02B81827});
}
// bool f(?, const Obj* o): with o->+12 set and its +192 bit 0x8:
// sub_00129C18(it) and 1; else 0 (r4 left as the bit)
DECOMP(00396750, Obj_Call129C18_00396750) {
    const u32 p = load<u32>(arg<u32>(ctx, 1) + 12);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 b = load<u32>(p + 192) & 0x8u;
    ctx.gpr[4] = b;
    if (b == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x00129C18, {p, b});
    ret(ctx, 1u);
}
// bool f(?, const Obj* o): q = sub_000D94F4(o->+12); q->+1268 |= 0x40000000 and
// 1 when both are set, else 0 (r4 / r5 left)
DECOMP(00398D58, Obj_SetFlag1268_00398D58) {
    const u32 p = load<u32>(arg<u32>(ctx, 1) + 12);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 q = static_cast<u32>(call(ctx, 0x000D94F4, {p}));
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(q + 1268) | 0x40000000u;
    store<u32>(q + 1268, w);
    ctx.gpr[4] = q;
    ctx.gpr[5] = w;
    ret(ctx, 1u);
}
// f(Obj* o, Arr* a): o->+68 (u16) = 0; sub_004A71D0(a, o + 60, a->+8, 0) when a
// has a nonzero +8 (u16) count
DECOMP(0039FEE0, Obj_Call4A71D0_0039FEE0) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    store<u16>(o + 68, 0);
    ctx.gpr[6] = 0;
    ctx.gpr[3] = a;
    ctx.gpr[4] = o;
    ctx.gpr[5] = o;
    if (a == 0) return;
    const u32 n = load<u16>(a + 8);
    ctx.gpr[5] = n;
    if (n == 0) return;
    call(ctx, 0x004A71D0, {a, o + 60, n, 0});
}
// f(Obj* o): o->+464..+466 = 0 (u8), then o's virtual +28 (o, 0, 0)
DECOMP(003B0784, Obj_Reset464_003B0784) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 464, 0);
    store<u8>(o + 465, 0);
    store<u8>(o + 466, 0);
    VCall(ctx, o, 28, {o, 0, 0});
}
// void f(Vec3* out, T* const* pp): out = *pp ? (*pp)->+64 : the vector at
// 0x01164B40 (raw; r4 / r5 / f1 left)
DECOMP(003F0264, Ptr_GetVec64_003F0264) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 p = load<u32>(arg<u32>(ctx, 1));
    u32 s = p + 64;
    ctx.gpr[4] = s;
    if (p == 0) {
        s = kVec1164B40;
        ctx.gpr[4] = 0x01160000;
        ctx.gpr[5] = s;
    }
    CopyW(d, s, 0);
    CopyW(d, s, 4);
    ctx.fpr[1] = F32Bits(CopyW(d, s, 8));
}
// f(const Obj* o): o->+16 (u8) ? sub_0044D5BC(o->+12, f1 = o->+20) : sub_0044D614(the same)
DECOMP(0042CC80, Obj_Call44D5_0042CC80) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 x = load<u32>(o + 12);
    const u32 t = load<u8>(o + 16) != 0 ? 0x0044D5BC : 0x0044D614;
    callf(ctx, t, {x, x}, {LfsAt(o + 20)});
}
// void f(): the globals 0x010BE870 and 0x010BE820 get the vtable 0x00B0D968
// and the base init 0x004FC1A0 each
DECOMP(0049E738, Globals_Init_0049E738) {
    constexpr u32 vt = 0x00B0D968;
    store<u32>(0x010BE870, vt);
    call(ctx, 0x004FC1A0, {0x010BE870, 0x00B10000, 0x010C0000});
    store<u32>(0x010BE820, vt);
    call(ctx, 0x004FC1A0, {0x010BE820, 0x010C0000});
}
// T::T(): vtable 0x00B221A0, +4 (u16) = 1, +12..+32 = 0, +60 = 0x80, bits
// 0xC0 of +61 cleared (r4..r7 left)
DECOMP(004B9ED8, Ctor_B221A0_004B9ED8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b60 = load<u8>(o + 60);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B221A0);
    const u32 b61 = load<u8>(o + 61) & ~0xC0u;
    for (u32 off = 12; off <= 32; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 60, 0x80);
    store<u8>(o + 61, static_cast<u8>(b61));
    ctx.gpr[4] = 0x80;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = b61;
    ctx.gpr[7] = b60;
}
// f32 f(f32 x): sub_005E649C(clamp(x)) with f32 k / f64 lo / f64 hi at
// 0x007352D0 (fsel: x + k >= 0 ? x : lo, then t - k >= 0 ? hi : t; r3 left)
DECOMP(007352E8, Clamp_Call5E649C_007352E8) {
    constexpr u32 kK = 0x007352D0;
    const f64 x = farg(ctx, 0);
    const f64 k = LfsAt(kK), lo = std::bit_cast<f64>(load<u64>(kK + 8)), hi = std::bit_cast<f64>(load<u64>(kK + 16));
    f64 t = rt::to_single(x + k);
    t = rt::to_single(t >= 0.0 ? x : lo);
    const f64 u = rt::to_single(t - k);
    t = rt::to_single(u >= 0.0 ? hi : t);
    ctx.gpr[3] = kK;
    ctx.fpr[2] = u;
    ctx.fpr[3] = k;
    ctx.fpr[4] = lo;
    ctx.fpr[5] = hi;
    callf(ctx, 0x005E649C, {kK}, {t});
}
// f(const Obj* o): o->+160's virtual +12 (p) when there is one, else 0x00B0DA98
DECOMP(007372FC, Member160_VCall12_007372FC) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 160);
    if (p == 0) {
        ret(ctx, 0x00B0DA98u);
        return;
    }
    VCall(ctx, p, 12, {p});
}
// bool f(?, ?, ?, const Node* n, f32 key): the binary tree from n (key +4,
// +12 lower, +16 otherwise) holds key (f2 left as the last key read)
DECOMP(0074ACF4, Tree_HasF32_0074ACF4) {
    u32 n = arg<u32>(ctx, 3);
    const f64 key = farg(ctx, 0);
    while (n != 0) {
        const f64 k = LfsAt(n + 4);
        ctx.fpr[2] = k;
        if (k == key) {
            ret(ctx, 1u);
            return;
        }
        n = key < k ? load<u32>(n + 12) : load<u32>(n + 16);
    }
    ret(ctx, 0u);
}
// f(Obj* o, a, b, const Vec3* v): sub_0056A410(o, a, b, v) (f1 passed through),
// then o->+48..+56 = v (raw; f1 left)
DECOMP(0074C938, Obj_SetVec48_0074C938) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 3);
    call(ctx, 0x0056A410, {o, ctx.gpr[4], ctx.gpr[5], v});
    Cp(o + 48, v);
    Cp(o + 52, v + 4);
    ctx.fpr[1] = F32Bits(Cp(o + 56, v + 8));
}
// void f(T* t, const Vec3* a, const Vec3* b, u32 i): the 160-byte entry i of t:
// +224 = a, +208 = b (raw; r3 / r6 / r7 / f1 left)
DECOMP(0074DA18, Entry160_SetVecs_0074DA18) {
    const u64 i = ctx.gpr[6];
    const u32 e = arg<u32>(ctx, 0) + static_cast<u32>(i) * 160u;
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    for (u32 k = 0; k < 12; k += 4) Cp(e + 224 + k, a + k);
    for (u32 k = 0; k < 8; k += 4) Cp(e + 208 + k, b + k);
    ctx.fpr[1] = F32Bits(Cp(e + 216, b + 8));
    ctx.gpr[3] = e + 208;
    ctx.gpr[6] = e + 224;
    ctx.gpr[7] = i << 7;
}
// bool f(Obj* o): the member at o + 24's virtual +32 (m, 0) returns 2
DECOMP(0074E6CC, Member24_VCall32Is2_0074E6CC) {
    const u32 m = arg<u32>(ctx, 0) + 24;
    const u32 r = static_cast<u32>(VCall(ctx, m, 32, {m, 0}));
    ret(ctx, r == 2 ? 1u : 0u);
}
// ~T(): the vtables from 0x00B1F290 (+0 / +8 / +12 / +16 / +20), then the base
// sub_00884FB8 (r4..r8 = the vtables, as left)
DECOMP(0076EA98, Dtor_VtB1F290_0076EA98) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 t = 0x00B1F290;
    store<u32>(o, t + 8);
    store<u32>(o + 8, t + 64);
    store<u32>(o + 12, t + 84);
    store<u32>(o + 16, t + 112);
    store<u32>(o + 20, t + 132);
    call(ctx, 0x00884FB8, {o, t + 132, t + 8, t + 64, t + 84, t + 112});
}
// f(Obj* o, const Vec3* v): o->+80..+88 = v (raw; f1 = v->z), then o's virtual +148 (o)
DECOMP(00796598, Obj_SetVec80_00796598) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    Cp(o + 80, v);
    Cp(o + 84, v + 4);
    ctx.fpr[1] = F32Bits(Cp(o + 88, v + 8));
    VCall(ctx, o, 148, {o});
}

// ---------------------------------------------------------------- sweep: 22 instructions
// bool f(?, s32 k): 0 for k = 1..4, 8, 12 and 28, else 1
DECOMP(0001647C, Kind_Test_0001647C) {
    const s32 k = arg<s32>(ctx, 1);
    const bool z = (k >= 1 && k <= 4) || k == 8 || k == 12 || k == 28;
    ret(ctx, z ? 0u : 1u);
}
// f32 f(const Obj* o): *(f32*)0x0001ECCC when bit 0x20 of o->+305 is set and the
// u8 o->+308->+12 at the index (*(T**)0x00F948B4)->+64 is set, else
// *(f32*)0x0001ECC8 (r3 left as 0x0001ECC8, r4 as the bit / index * 4)
DECOMP(0001ECD0, Obj_SelF32_0001ECD0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = (load<u8>(o + 305) >> 5) & 1u;
    ctx.gpr[4] = b;
    bool second = false;
    if (b != 0) {
        const u32 tbl = load<u32>(load<u32>(o + 308) + 12);
        const u32 off = load<u32>(load<u32>(0x00F948A4 + 16) + 64) << 2;
        ctx.gpr[4] = off;
        second = load<u8>(tbl + off) != 0;
    }
    ctx.gpr[3] = 0x0001ECC8;
    ctx.fpr[1] = LfsAt(second ? 0x0001ECCC : 0x0001ECC8);
}
// void f(): the words 0x00F9CDC8..+8 = 0, sub_005516E0(0x00F9CDA8), then the
// u16 0x00F9CDAE = 16 (r3 left as 16)
DECOMP(000ABB74, Globals_Init_000ABB74) {
    constexpr u32 a = 0x00F9CDC8, b = 0x00F9CDA8;
    store<u32>(a, 0);
    store<u32>(a + 4, 0);
    store<u32>(a + 8, 0);
    call(ctx, 0x005516E0, {b, 0, a, 0x00FA0000});
    store<u16>(b + 6, 16);
    ctx.gpr[3] = 16;
}
// void f(Obj* o, bool b): sub_001241A0(o, 255) when b; bit 0x08 of o->+968 = b (r3 left)
DECOMP(00124370, Obj_SetFlag968_00124370) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 b = ctx.gpr[4];
    if ((b & 0xFFu) != 0) call(ctx, 0x001241A0, {o, 255, b & 0xFFu});
    else ctx.gpr[5] = 0;  // r5 left
    const u32 v = (load<u8>(o + 968) & ~0x08u) | ((static_cast<u32>(b) << 3) & 0x08u);
    store<u8>(o + 968, static_cast<u8>(v));
    ctx.gpr[3] = v;
}
// f32 f(Obj* o): r = o's virtual +64 (o); r ? r->+640 : *(f32*)0x001290EC (r3 left)
DECOMP(001290F0, Obj_GetF32_001290F0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(VCall(ctx, o, 64, {o}));
    if (r != 0) {
        ctx.fpr[1] = LfsAt(r + 640);
        return;
    }
    ctx.gpr[3] = 0x001290EC;
    ctx.fpr[1] = LfsAt(0x001290EC);
}
// f(Obj* o, u8 b): sub_001D81F0(o, 0) when b, then sub_000E4544(o, b)
DECOMP(001D8560, Obj_Call0E4544_001D8560) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1) & 0xFFu;
    if (b != 0) call(ctx, 0x001D81F0, {o, 0});
    call(ctx, 0x000E4544, {o, b});
}
// ~T(): vtable 0x00B17DB0; sub_00555CC0(+600, +596) when +600 is set, then the
// base 0x000E2A34(this)
DECOMP(001DC8B8, Dtor_B17DB0_001DC8B8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 600);
    store<u32>(o, 0x00B17DB0);
    if (p != 0) {
        call(ctx, 0x00555CC0, {p, load<u32>(o + 596)});
        call(ctx, 0x000E2A34, {o});
    } else {
        call(ctx, 0x000E2A34, {o, 0x00B17DB0});
    }
}
// f32 f(Obj* o): o->+336 ? sub_000D155C(it, o)->+224 : the f32 with the bits
// (o->+388->+510 (u16)) << 16 (r3 left)
DECOMP(002071AC, Obj_GetF32_002071AC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 336);
    if (p != 0) {
        const u32 r = static_cast<u32>(call(ctx, 0x000D155C, {p, o, o}));
        ctx.fpr[1] = LfsAt(r + 224);
        return;
    }
    const u32 w = static_cast<u32>(load<u16>(load<u32>(o + 388) + 510)) << 16;
    ctx.gpr[3] = w;
    ctx.gpr[4] = 0;  // r4 left
    ctx.fpr[1] = F32Bits(w);
}
// f(?, const Obj* p): p->+432's virtual +88 (q, 0) when both are set, else 0
DECOMP(0020BB9C, Member432_VCall88_0020BB9C) {
    const u32 p = arg<u32>(ctx, 1);
    const u32 q = p != 0 ? load<u32>(p + 432) : 0u;
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    VCall(ctx, q, 88, {q, 0});
}
// ~T(): vtable 0x00B1C020, sub_0075DC84(0x0105752C, this + 36), then
// sub_004432DC(this->+280)
DECOMP(00272588, Dtor_B1C020_00272588) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1C020);
    call(ctx, 0x0075DC84, {0x0105752C, o + 36, 0x00B1C020});
    call(ctx, 0x004432DC, {load<u32>(o + 280)});
}
// f(Obj* o): sub_0028BF58(o) unless the word 0x010F0ED8 is 2, then
// sub_0028D0FC(o, o->+140, o + 68)
DECOMP(0028D6A0, Obj_Update_0028D6A0) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(0x010F0E20 + 184) != 2) call(ctx, 0x0028BF58, {o});
    call(ctx, 0x0028D0FC, {o, load<u32>(o + 140), o + 68});
}
// f(Obj* o): sub_0052EFE0 on o + 4, o + 20 and o + 36
DECOMP(0028D6F8, Obj_Call52EFE0x3_0028D6F8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0052EFE0, {o + 4});
    call(ctx, 0x0052EFE0, {o + 20});
    call(ctx, 0x0052EFE0, {o + 36});
}
// f(const Obj* o, a): r = sub_0034CC34(o->+36, a); sub_00354198(r, 0) when both
// are set (r3 = a, r4 = 0, r5 = o left when there is no +36)
DECOMP(002988EC, Obj_Call354198_002988EC) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    const u32 p = load<u32>(o + 36);
    if (p == 0) {
        ctx.gpr[3] = a;
        ctx.gpr[4] = 0;
        ctx.gpr[5] = o;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x0034CC34, {p, a, p}));
    if (r == 0) return;
    call(ctx, 0x00354198, {r, 0});
}
// f(): r = sub_004A5FC8(*(T**)0x01059F78, 0, 0, 1); sub_004CF710(r, f1 =
// *(f32*)0x002ABEB0) when r is set
DECOMP(002ABEB8, Global_Call4CF710_002ABEB8) {
    const u32 r = static_cast<u32>(call(ctx, 0x004A5FC8, {load<u32>(0x01059F30 + 72), 0, 0, 1}));
    if (r == 0) return;
    callf(ctx, 0x004CF710, {r, 0x002ABEB0}, {LfsAt(0x002ABEB0)});
}
// s32 f(const Obj* o): the index of the first of the o->+32 (u16) 144-byte
// entries at o->+24 whose +48 is 0, or -1 (r4..r6 left)
DECOMP(002B92A0, Obj_FreeIndex_002B92A0) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 n = load<u16>(o + 32);
    const u32 base = load<u32>(o + 24);
    ctx.gpr[4] = static_cast<u32>(n);
    ctx.gpr[5] = 0;
    for (s32 i = 0; i < n; ++i) {
        const u32 v = load<u32>(base + static_cast<u32>(i) * 144u + 48);
        ctx.gpr[6] = v;
        ctx.gpr[7] = static_cast<u64>(static_cast<s64>(i)) << 4;
        if (v == 0) {
            ret(ctx, static_cast<u32>(i));
            return;
        }
        ctx.gpr[5] = static_cast<u32>(i + 1);
    }
    ret(ctx, -1);
}
// bool f(Obj* o, a): sub_005637D4(o, a), sub_002E8EB4(o, a), the word
// 0x01068720 = 0; returns 1 (r4 / r5 left)
DECOMP(002E9F88, Obj_Reset_002E9F88) {
    const u32 o = arg<u32>(ctx, 0);
    const u64 a = ctx.gpr[4];
    call(ctx, 0x005637D4, {o, a});
    call(ctx, 0x002E8EB4, {o, a});
    store<u32>(0x01068720, 0);
    ctx.gpr[4] = 0x01070000;
    ctx.gpr[5] = 0;
    ret(ctx, 1u);
}
// f(Obj* o, a): r = sub_004F9534(a, 0x7F1667C1); sub_00555F80(o + 4, r) when r is set
DECOMP(00302F14, Obj_Call555F80_00302F14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x004F9534, {ctx.gpr[4], 0x7F1667C1, 0x7F160000}));
    if (r == 0) return;
    call(ctx, 0x00555F80, {o + 4, r, o + 4});
}
// void f(Obj* o, s32 key, u8 v): every 64-byte entry of the array o->+104
// (+0 data, +8 u16 count) whose +0 is key gets +48 = v (r5..r10 left)
DECOMP(00352CD8, Obj_SetEntries_00352CD8) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    const u64 v = ctx.gpr[5];
    u32 i = 0, off = 0;
    for (;;) {
        const u32 arr = load<u32>(o + 104);
        const u32 n = arr != 0 ? load<u16>(arr + 8) : 0u;
        ctx.gpr[9] = arr;
        ctx.gpr[10] = n;
        if (!(i < n)) break;
        const u32 e = load<u32>(arr) + off;
        const u32 k = load<u32>(e);
        ctx.gpr[9] = e;
        ctx.gpr[10] = k;
        if (k == key) store<u8>(e + 48, static_cast<u8>(v));
        off += 64;
        ++i;
    }
    ctx.gpr[5] = 0;
    ctx.gpr[6] = off;
    ctx.gpr[7] = i;
    ctx.gpr[8] = v;
}
// T* f(const Obj* o, u32 i): with i < o->+8->+152: sub_004DC6F4(p) for p =
// o->+328[i].+12 (16-byte entries) when set; else 0 (r4 / r5 left)
DECOMP(0036C014, Obj_Entry_0036C014) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 n = load<u32>(load<u32>(o + 8) + 152);
    ctx.gpr[5] = n;
    if (!(i < n)) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = i << 4;
    const u32 p = load<u32>(load<u32>(o + 328) + (i << 4) + 12);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x004DC6F4, {p, i << 4, n});
}
// bool f(): one of the (u16 at 0x010AA69C) entries of *(T***)0x010AA694 has a
// nonzero +12 (u16) (r3..r7 as the loop leaves them)
DECOMP(0042C1D4, List_AnySet_0042C1D4) {
    constexpr u32 kL = 0x010AA694;
    u64 c = load<u32>(kL);
    const u32 n = load<u16>(kL + 8);
    ctx.gpr[4] = n;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = 0;
    for (u32 i = 0; i < n;) {
        const u32 v = load<u16>(load<u32>(static_cast<u32>(c)) + 12);
        ctx.gpr[7] = v;
        if (v != 0) {
            ret(ctx, 1u);
            return;
        }
        ++i;
        c += 4;
        ctx.gpr[6] = i;
    }
    ret(ctx, 0u);
}

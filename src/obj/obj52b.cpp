// Game code, part 52 (b): the small-function sweep continued with the 118
// and 119 instruction functions (size order). Each was checked against the
// original in the PPC simulator (memory writes, calls and their arguments,
// r3 and f1).
#include "obj52.h"

using namespace sly4;
using namespace sly4::obj52;

// ---------------------------------------------------------------- sweep: 118 instructions
// u32 f(T* o, const T* s) (copy constructor): ColObj_Ctor1F85F0(o, s); the
// rows s->+256..+312 (3 words of each 16; +288..+300 all 4) -> o->+304..,
// vtable 0x00B18F18 (stored mid-copy), s->+320..+400 -> o->+368..+448
// (gaps at +380 / +396 of o); bits 0x80000000 .. 0x00400000 (one each,
// s->+404 re-read) of +452 from s->+404. r3 = the +452 word; f1 = s->+396
// as the copy leaves it.
DECOMP(001FDF7C, ColObj_CopyConstruct_001FDF7C) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    call(ctx, 0x001F85F0u, {o, s});
    static constexpr u16 kSrc[] = {256, 260, 264, 272, 276, 280, 288, 292, 296, 300, 304, 308, 312, 320, 324};
    for (u16 a : kSrc) Cp52(o + a + 48, s + a);
    store<u32>(o, 0x00B18F18u);
    static constexpr u16 kSrc2[] = {328, 336, 340, 344, 356, 360, 352, 364, 368, 372, 380, 376, 384, 388, 392, 396, 400};
    for (u16 a : kSrc2) Cp52(o + a + 48, s + a);
    ctx.fpr[1] = Lfs52(s + 396);
    u32 w = (load<u32>(o + 452) & 0x7FFFFFFFu) | (load<u32>(s + 404) & 0x80000000u);
    store<u32>(o + 452, w);
    for (u32 m = 0x40000000u; m >= 0x00400000u; m >>= 1) {
        w = (w & ~m) | (load<u32>(s + 404) & m);
        store<u32>(o + 452, w);
    }
    ret(ctx, w);
}

// T* f(a, b, Xml* x, d) (reads a 224-byte def): the 124-byte record at
// sp+112 ({0x00400000, 2, 0, x->+12, 0x00AD366C, 0x00AD366C, 8 bytes 0, 0,
// -1.0f, -1.0f, u8 0, 6 x 0, -1.0f, name 0x00AD38CC, u8 1, 3 bytes 0, 9 x 0,
// u8 0}) filled by XmlDef_Read_00223D38(a, b, x, d, rec, 2, 1,
// 0x00400000); p = SmallAlloc(224) (0 -> r3 = 0): Thunk_000C2B90(p, rec),
// u8 +172..+175 = rec bytes +80..+83 (vtable 0x00B19510 between),
// String_CopyN(p + 140, rec->+76, 31), +176..+184 / +188..+208 = rec +84..
// +92 / +96..+116 (interleaved; vtable 0x00B1AFE0), u8 +212 = rec +120. r3 =
// p; f1 = rec +116 as the copy leaves it.
DECOMP(00223F94, Def224_ReadXml_00223F94) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    Stack fr(ctx, 128);  // the record at sp+112..235
    const u32 r = fr.addr();
    constexpr u32 kNeg1 = 0xBF800000u, kStr = 0x00AD366Cu;
    store<u32>(r + 16, kStr);
    store<u32>(r + 12, load<u32>(x + 12));
    store<u32>(r + 20, kStr);
    for (u32 k = 24; k < 32; ++k) store<u8>(r + k, 0);
    store<u32>(r + 32, 0);
    store<u32>(r + 36, kNeg1);
    store<u32>(r + 40, kNeg1);
    store<u8>(r + 44, 0);
    for (u32 k = 48; k < 72; k += 4) store<u32>(r + k, 0);
    store<u32>(r + 72, kNeg1);
    store<u32>(r + 8, 0);
    store<u32>(r + 76, 0x00AD38CCu);
    store<u8>(r + 80, 1);
    store<u8>(r + 81, 0);
    store<u8>(r + 82, 0);
    store<u8>(r + 83, 0);
    store<u32>(r, 0x00400000u);
    for (u32 k = 84; k < 120; k += 4) store<u32>(r + k, 0);
    store<u8>(r + 120, 0);
    store<u32>(r + 4, 2);
    call(ctx, 0x00223D38u, {a, b, x, d, r, 2, 1, 0x00400000u});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {224}));
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    call(ctx, 0x000C2B90u, {p, r});
    store<u8>(p + 172, load<u8>(r + 80));
    store<u8>(p + 173, load<u8>(r + 81));
    store<u32>(p, 0x00B19510u);
    store<u8>(p + 174, load<u8>(r + 82));
    store<u8>(p + 175, load<u8>(r + 83));
    call(ctx, 0x00553638u, {p + 140, load<u32>(r + 76), 31});
    Cp52(p + 188, r + 96);
    Cp52(p + 176, r + 84);
    Cp52(p + 192, r + 100);
    Cp52(p + 196, r + 104);
    Cp52(p + 180, r + 88);
    store<u32>(p, 0x00B1AFE0u);
    Cp52(p + 200, r + 108);
    Cp52(p + 184, r + 92);
    Cp52(p + 204, r + 112);
    store<u8>(p + 212, load<u8>(r + 120));
    Cp52(p + 208, r + 116);
    ctx.fpr[1] = Lfs52(r + 116);
    ret(ctx, p);
}

// The constructor body of 0x00255F18 / 0x002560F0 (void f(T* o, const Def*
// s)): vtable 0x00B1BE30, +4 / +8 / +12 = s->+4 / +8 / +12, u8 +16 = 1,
// +20 = 0, {+4, o} entered in the array 0x01056F1C (ArrInsert8_51);
// ObjBase_Construct(o + 24), vtables vt0 (+0) / vt24 (+24); +32..+52 =
// s->+20..+40 (+56 / +60 = 0); PtrVectors +64 / +84 / +104 / +124 (u16
// +70 = 4, +90 = 16, +110 = 4, +130 = 4); +56 = +52, +60 = +44 + +48. r3 =
// the last PtrVector_Construct's result, f1 = +60.
static void Range_Construct52(PPUContext& ctx, u32 vt0, u32 vt24) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    store<u32>(o, 0x00B1BE30u);
    store<u32>(o + 4, load<u32>(s + 4));
    store<u32>(o + 8, load<u32>(s + 8));
    store<u8>(o + 16, 1);
    store<u32>(o + 12, load<u32>(s + 12));
    store<u32>(o + 20, 0);
    ArrInsert8_51(ctx, 0x01056F1Cu, o + 4, o);
    call(ctx, 0x0048F27Cu, {o + 24});
    store<u32>(o, vt0);
    store<u32>(o + 24, vt24);
    for (u32 k = 0; k < 5; ++k) Cp52(o + 32 + 4 * k, s + 20 + 4 * k);
    store<u32>(o + 56, 0);
    Cp52(o + 52, s + 40);
    store<u32>(o + 60, 0);
    static constexpr u16 kGrow[4] = {4, 16, 4, 4};
    u64 r = 0;
    for (u32 k = 0; k < 4; ++k) {
        r = call(ctx, 0x005516E0u, {o + 64 + 20 * k});
        store<u16>(o + 70 + 20 * k, kGrow[k]);
    }
    const f64 sum = F52(Lfs52(o + 44) + Lfs52(o + 48));
    Cp52(o + 56, o + 52);
    store<f32>(o + 60, static_cast<f32>(sum));
    ctx.fpr[1] = sum;
    ret(ctx, r);
}

// void f(T* o, const Def* s): Range_Construct52 with vtables 0x00B1CDB0 /
// 0x00B1CDFC.
DECOMP(00255F18, Range_Construct_00255F18) { Range_Construct52(ctx, 0x00B1CDB0u, 0x00B1CDFCu); }

// void f(T* o, const Def* s): Range_Construct52 with vtables 0x00B1CCD8 /
// 0x00B1CD24.
DECOMP(002560F0, Range_Construct_002560F0) { Range_Construct52(ctx, 0x00B1CCD8u, 0x00B1CD24u); }

// u32 f(a, b, Xml* x, Tmpl* d, Rec* r) (damage-collision def check):
// DamageColDef_Read_000C85A8(a, b, x, d, r); then the defaults record on
// the stack (Def_Construct74FD80(rec, x->+12, 0x00AD9DD0, 0x00AD9DD0, 0, 1,
// 0x9260C26E, 0x01164B40; the float layout of obj45b / obj51b from the pool
// 0x002F8CD8) with its PtrVector +224 (u16 +230 = 4, PtrVector_Init(v, 0,
// 0, 0)); bit 0x00080000 of rec->+0 set; with d lacking bit 0x00080000 of
// d->+20, XmlCursor_DocPath(a); the vectors +224 and +124 (lazy, freed)
// cleared. r3 as the last call leaves it.
DECOMP(002F8CF0, DamageCol_ReadCheck_002F8CF0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2), d = arg<u32>(ctx, 3);
    call(ctx, 0x000C85A8u, {a, b, x, d, arg<u32>(ctx, 4)});
    constexpr u32 K = 0x002F8CD8u;
    const f64 k0 = Lfs52(K), k4 = Lfs52(K + 4), k8 = Lfs52(K + 8), k12 = Lfs52(K + 12), k16 = Lfs52(K + 16);
    const u64 f3w = load<u32>(K + 12), f2w = load<u32>(K + 16);
    Stack fr(ctx, 256);  // the record at sp+368 (frame of 656 bytes)
    const u32 rec = fr.addr();
    constexpr u32 kName = 0x00AD9DD0u;
    // clang-format off
    callf(ctx, 0x0074FD80u, {rec, load<u32>(x + 12), kName, kName, 0, 1, 0x9260C26Eu, 0x01164B40u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, f3w, f3w, f3w, f3w, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, f2w, f2w, 0}, {k0, k16, k12, k12, k12, k12, k4, k8, k16, k12, k12, k12, k12});
    // clang-format on
    call(ctx, 0x005516E0u, {rec + 224});
    store<u16>(rec + 230, 4);
    call(ctx, 0x00551598u, {rec + 224, 0, 0, 0});
    store<u32>(rec, load<u32>(rec) | 0x00080000u);
    if (d != 0 && (load<u32>(d + 20) & 0x00080000u) == 0) call(ctx, 0x004F8944u, {a});
    call(ctx, 0x0054EC1Cu, {rec + 224});
    u64 r3 = call(ctx, 0x0054EC18u, {rec + 224});
    const u32 v = load<u32>(rec + 124);
    if (v != 0) {
        call(ctx, 0x0054EC1Cu, {v});
        call(ctx, 0x0054EC18u, {v});
        r3 = call(ctx, 0x00552420u, {load<u32>(rec + 124), 20});
    }
    ret(ctx, r3);
}

// u32 f(Shapes* o, XmlCursor* x): the name attribute 0x0024728B
// (String_Assign(o, a); missing: the name used in messages is 0x00AD9F68),
// the hash 0x0024728B -> o->+32 (old value as default); then each child
// element with bit 0x80 of u8 +24 and hash +20 one of 0x03B880B3 /
// 0x04862DD6 / 0x8ACA5893 is read into a 17-byte shape (sp+112, zeroed;
// Shape_ReadXml_00302DA8) and memmoved onto the 20-byte entry array o->+36
// (u16 cap +40, count +44, high-water +46); any other child gets
// String_Format(0x00AD9F70, name). r3 = 0.
DECOMP(0030386C, Shapes_ReadXml_0030386C) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 kName = 0x0024728Bu;
    u32 nm = static_cast<u32>(call(ctx, 0x004F9534u, {x, kName}));
    if (nm != 0)
        call(ctx, 0x00555F80u, {o, nm});
    else
        nm = 0x00AD9F68u;
    call(ctx, 0x004FA14Cu, {x, kName, o + 32, load<u32>(o + 32)});
    u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (c != 0) {
        Stack fr(ctx, 32);  // the shape at sp+112..128
        const u32 sh = fr.addr();
        for (; c != 0; c = static_cast<u32>(call(ctx, 0x004F8D74u, {c}))) {
            const u32 h = load<u32>(c + 20);
            if ((load<u8>(c + 24) & 0x80u) == 0 || (h != 0x03B880B3u && h != 0x04862DD6u && h != 0x8ACA5893u)) {
                call(ctx, 0x005534D0u, {0x00AD9F70u, nm});
                continue;
            }
            store<u32>(sh, 0);
            store<u32>(sh + 12, 0);
            store<u8>(sh + 16, 0);
            store<u32>(sh + 4, 0);
            store<u32>(sh + 8, 0);
            call(ctx, 0x00302DA8u, {sh, c});
            if (load<u16>(o + 44) == load<u16>(o + 40))
                call(ctx, 0x00551718u, {o + 36, (static_cast<u32>(load<u16>(o + 40)) << 1) & 0x0001FFFEu});
            call(ctx, 0x0071C3F0u, {load<u32>(o + 36) + static_cast<u32>(load<u16>(o + 44)) * 20u, sh, 17});
            const u32 cnt = load<u16>(o + 44) + 1u;
            const u32 hi = load<u16>(o + 46);
            store<u16>(o + 44, static_cast<u16>(cnt));
            store<u16>(o + 46, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(cnt & 0xFFFFu) ? hi : cnt));
        }
    }
    ret(ctx, c);
}

// The inlined Handles_Destroy_0042725C + free on the socket set *g: the
// import 0x00ABDCE4 (socketclose) on the s32 handle +0 and on each of the
// +20 handles from +4 on, +0 = 0, Mem_FreeFrom(set, 0), *g = 0.
static void SocketSetFree52(PPUContext& ctx, u32 g) {
    const u32 s = load<u32>(g);
    if (s == 0) return;
    auto sx = [](u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); };
    call(ctx, 0x00ABDCE4u, {sx(load<u32>(s))});
    for (u32 i = 0; i < load<u32>(s + 20); ++i) call(ctx, 0x00ABDCE4u, {sx(load<u32>(s + 4 + 4 * i))});
    store<u32>(s, 0);
    call(ctx, 0x0054EE6Cu, {s, 0});
    store<u32>(g, 0);
}

// void f() (network shutdown): the socket sets *0x00F6584C, *0x00F65850,
// *0x00F65854 closed and freed (SocketSetFree52); r3 =
// PtrVector_Clear(0x0108A13C).
DECOMP(004272F4, Net_CloseAll_004272F4) {
    SocketSetFree52(ctx, 0x00F6584Cu);
    SocketSetFree52(ctx, 0x00F65850u);
    SocketSetFree52(ctx, 0x00F65854u);
    ret(ctx, call(ctx, 0x0054EC1Cu, {0x0108A13Cu}));
}

// ---------------------------------------------------------------- sweep: 119 instructions
// T* f(?, XmlCursor* x, u8 flag) (a group-ref def): no "name" attribute
// (XmlCursor_FindAttrByName(x, 0x00AC019C)) -> XmlCursor_DocPath(r3 as
// passed), r3 = 0. Else a 28-byte record (SmallAlloc(28); vtable
// 0x00B0DAAC, +4 = String_HashNoCase(name), +8 = 0, u8 +16 = 0, kind +12 =
// 13, entered in the 8-byte entry array 0x01056F04 (ArrInsert8_51), +20 =
// +24 = 0, vtable 0x00B0DDAC); u8 +16 = flag, the hashes 0x0027873A ->
// +24 and 0x3A034393 -> +20 (old values as defaults; with a failed
// allocation these touch address 0 + offset: kept); GroupTable_Read17124(x)
// when +24 is 0x49CC0E00. r3 = the record.
DECOMP(00017420, GroupRef_ReadXml_00017420) {
    const u32 a = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2);
    const u32 nm = static_cast<u32>(call(ctx, 0x004F9628u, {x, 0x00AC019Cu}));
    if (nm == 0) {
        call(ctx, 0x004F8944u, {a});
        ret(ctx, 0u);
        return;
    }
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {28}));
    if (p != 0) {
        store<u32>(p, 0x00B0DAACu);
        const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {nm}));
        store<u32>(p + 8, 0);
        store<u8>(p + 16, 0);
        store<u32>(p + 4, h);
        store<u32>(p + 12, 13);
        ArrInsert8_51(ctx, 0x01056F04u, p + 4, p);
        store<u32>(p + 20, 0);
        store<u32>(p + 24, 0);
        store<u32>(p, 0x00B0DDACu);
    }
    store<u8>(p + 16, static_cast<u8>(flag));
    call(ctx, 0x004FA14Cu, {x, 0x0027873Au, p + 24, load<u32>(p + 24)});
    call(ctx, 0x004FA14Cu, {x, 0x3A034393u, p + 20, load<u32>(p + 20)});
    if (load<u32>(p + 24) == 0x49CC0E00u) call(ctx, 0x00017124u, {x});
    ret(ctx, p);
}

// s32 f(XmlCursor* x, T* o, Msg* m) (resolves o's list handle, then reads
// x's first child into its entry): with no handle (low half of o->+40 0)
// KListLib_Add_00390DC8(*0x00F65798, &key (sp+116: u8 1, u8 0, +4 = 0, u8
// +8 = 1, +12 / +16 = 0, u16 +20 = o->+44, +22 = 0, +24 = o->+20), &h
// (sp+112, 0)) -> o->+40. The handle {u16 index, u16 gen} must name a live
// entry (index below list->+24, entry (list->+28)[index] word +12 bits
// 15..30 == gen, bit 0x80 of its byte +12 clear) with an object e (+8),
// else r3 = -1. When x has a second child, String_Format(0x00AC04E4,
// XmlCursor_DocPath(Xml_FindResolved_004F9434(x)), line + 1, o->+20,
// m->+4); then 0x0038DF4C(e, first child, &r (sp+144)) and r3 = s16 r.
DECOMP(000650B0, KList_ReadEntry_000650B0) {
    const u32 x = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    constexpr u32 kList = 0x00F65798u;
    Stack fr(ctx, 48);  // h at sp+112, key at sp+116..143, result at sp+144
    const u32 sp = fr.addr() - 112;
    u32 h = load<u32>(o + 40);
    if ((h & 0xFFFFu) == 0) {
        store<u32>(sp + 112, 0);
        store<u16>(sp + 114, 0);
        store<u8>(sp + 117, 0);
        store<u32>(sp + 120, 0);
        store<u8>(sp + 124, 1);
        store<u32>(sp + 128, 0);
        store<u32>(sp + 132, 0);
        store<u16>(sp + 138, 0);
        store<u8>(sp + 116, 1);
        store<u16>(sp + 136, static_cast<u16>(load<u32>(o + 44)));
        store<u32>(sp + 140, load<u32>(o + 20));
        call(ctx, 0x00390DC8u, {load<u32>(kList), sp + 116, sp + 112});
        h = load<u32>(sp + 112);
        store<u32>(o + 40, h);
    }
    const u32 L = load<u32>(kList);
    const u32 gen = h & 0xFFFFu;
    u32 e = 0;
    if (gen != 0 && static_cast<s32>(h >> 16) < static_cast<s32>(load<u32>(L + 24))) {
        const u32 ent = load<u32>(L + 28) + ((h >> 12) & 0x000FFFF0u);
        if (((load<u32>(ent + 12) >> 15) & 0xFFFFu) == gen && (load<u8>(ent + 12) & 0x80u) == 0) e = load<u32>(ent + 8);
    }
    if (e == 0) {
        ret(ctx, 0xFFFFFFFFu);
        return;
    }
    const u32 c = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    if (static_cast<u32>(call(ctx, 0x004F8D74u, {c})) != 0) {
        const u32 d = static_cast<u32>(call(ctx, 0x004F9434u, {x}));
        const u32 dp = static_cast<u32>(call(ctx, 0x004F8944u, {d}));
        const u32 d2 = static_cast<u32>(call(ctx, 0x004F9434u, {x}));
        const u32 line = load<u32>(d2 + 36) + 1;
        call(ctx, 0x005534D0u, {0x00AC04E4u, dp, static_cast<u64>(static_cast<s64>(static_cast<s32>(line))), load<u32>(o + 20), load<u32>(m + 4)});
    }
    const u32 c1 = static_cast<u32>(call(ctx, 0x004F9140u, {x}));
    call(ctx, 0x0038DF4Cu, {e, c1, sp + 144});
    ret(ctx, static_cast<u32>(static_cast<s32>(static_cast<s16>(load<u16>(sp + 144)))));
}

// u32 f(Obj* o, u8 on) (enables / disables the motion): bit 0x20000 of
// o->+172 = !on; with u8 *0x01022014 set, o->vt[68]() gives r -> r->vt[44](r,
// on); without component 32, bit 0x20 of u8 +28 of
// Peer_GetVec(WorldObj_GetPeer(o->+160)) = !on when the peer and its +136
// exist (the peer fetched for each step); then each 48-byte entry of the
// PtrVector o->+388 (re-read each step; only with o->+160) whose motion m
// (+28) has m->+8 gets Hk_SetMotion_0091D470(m, on ? s32 entry +36 : 5, 1,
// 0). r3 as the original leaves it (0 or the vector).
DECOMP(000E7B0C, Obj_EnableMotion_000E7B0C) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 off = on == 0 ? 1u : 0u;
    store<u32>(o + 172, (load<u32>(o + 172) & ~0x00020000u) | (off << 17));
    if (load<u8>(0x01021FCCu + 72) != 0) {
        u32 opd = load<u32>(load<u32>(o) + 68);
        const u32 r = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(opd)}));
        if (r != 0) {
            opd = load<u32>(load<u32>(r) + 44);
            call_ptr(ctx, opd, {r, on, opd, load<u32>(opd)});
        }
    }
    if (static_cast<u32>(call(ctx, 0x000D1288u, {o, 32, 0})) == 0 && load<u32>(o + 160) != 0 &&
        static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) != 0 &&
        load<u32>(static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)})) + 136) != 0) {
        const u32 pr = static_cast<u32>(call(ctx, 0x0057A428u, {load<u32>(o + 160)}));
        const u32 v = static_cast<u32>(call(ctx, 0x0057BE98u, {pr}));
        store<u8>(v + 28, static_cast<u8>((load<u8>(v + 28) & ~0x20u) | (off << 5)));
    }
    u32 r3 = load<u32>(o + 160);
    if (r3 != 0) {
        r3 = load<u32>(o + 388);
        if (r3 != 0 && load<u16>(r3 + 8) != 0) {
            for (u32 i = 0, e48 = 0;; ++i, e48 += 48) {
                const u32 n = r3 != 0 ? load<u16>(r3 + 8) : 0u;
                if (!(i < n)) break;
                const u32 e = load<u32>(r3) + e48;
                const u32 m = load<u32>(e + 28);
                if (m == 0 || load<u32>(m + 8) == 0) continue;
                const u64 mo = on != 0 ? static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(e + 36)))) : 5u;
                call(ctx, 0x0091D470u, {m, mo, 1, 0});
                r3 = load<u32>(o + 388);
            }
        }
    }
    ret(ctx, r3);
}

// u32 f(Obj* o, f32 dt) (charge update): the pending part o->+432 (unless
// bit 0x40000 of its +172) gets vt[40](p, 0) and +432 = 0. With bit
// 0x20000 of o->+376 and the effect o->+356: when bit 0x80000 is set and
// o->+160 too, the entry of ((o->+160)->+204) in the sorted map 0x01013B1C
// (SortedMap_Find732F38, key at sp+112) with bit 0x2 of +4 adds its args
// (Effect_AddArgs49FCC0(e, entry + 180, 0, 0, 0)); then
// Effect_SetHash4CF988(e, 0x04B6FB5B); bit 0x20000 cleared. 0 when
// Obj_UpdateBase(o, dt) fails; else +660 += dt (capped at +676), and +656
// += dt * +680 * (cap == 0 ? 0 : 1 - +660 / cap) + dt, capped at +672; r3 =
// 1, f1 = the new +656 before the cap.
DECOMP(000F4200, Obj_ChargeUpdate_000F4200) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 pa = load<u32>(o + 432);
    if (pa != 0 && (load<u32>(pa + 172) & 0x00040000u) == 0) {
        const u32 opd = load<u32>(load<u32>(pa) + 40);
        call_ptr(ctx, opd, {pa, 0, opd, load<u32>(opd)});
        store<u32>(o + 432, 0);
    }
    const u32 w = load<u32>(o + 376);
    if (w & 0x00020000u) {
        u32 nw = w;
        if (load<u32>(o + 356) != 0) {
            const u32 t = load<u32>(o + 160);
            if ((w & 0x00080000u) != 0 && t != 0) {
                Stack fr(ctx, 16);  // the key at sp+112
                store<u32>(fr.addr(), load<u32>(t + 204));
                const u32 r = load<u32>(static_cast<u32>(call(ctx, 0x00732F38u, {0x01013B1Cu, fr.addr()})));
                if (r != 0 && (load<u32>(r + 4) & 0x2u) != 0) call(ctx, 0x0049FCC0u, {load<u32>(o + 356), r + 180, 0, 0, 0});
            }
            call(ctx, 0x004CF988u, {load<u32>(o + 356), 0x04B6FB5Bu});
            nw = load<u32>(o + 376);
        }
        store<u32>(o + 376, nw & ~0x00020000u);
    }
    callf(ctx, 0x000D83C4u, {o}, {dt});
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ret(ctx, 0u);
        return;
    }
    constexpr u32 K = 0x000F41F4u;  // 0.0f, 1.0f
    f64 t = F52(Lfs52(o + 660) + dt);
    const f64 cap = Lfs52(o + 676);
    store<f32>(o + 660, static_cast<f32>(t));
    if (t >= cap) {
        t = cap;
        store<f32>(o + 660, static_cast<f32>(cap));
    }
    f64 k = Lfs52(K);
    if (cap != k) k = F52(Lfs52(K + 4) - F52(t / cap));
    const f64 a = F52(dt * Lfs52(o + 680));
    f64 v = F52(std::fma(a, k, dt));
    const f64 mx = Lfs52(o + 672);
    v = F52(v + Lfs52(o + 656));
    store<f32>(o + 656, static_cast<f32>(v));
    if (v > mx) store<f32>(o + 656, static_cast<f32>(mx));
    ctx.fpr[1] = v;
    ret(ctx, 1u);
}

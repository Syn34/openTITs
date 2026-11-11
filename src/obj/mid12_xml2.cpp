// Game code, medium sweep part 12: the XML def readers under 0x000BAAC8 and
// 0x00322D88.
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

namespace {
namespace m7fn = mid7::m7fn;
using mid5::XF32;
using mid5::XHash;
using mid5::XS32;

constexpr u32 kTagTbl3 = 0x00F646C0;  // u32[3] child tags (0x000B9C88 / 0x000BA618)
constexpr u32 kTagTbl5 = 0x00F646D8;  // u32[5] child tags (0x000BA318 / 0x000BA618)

// The index of tag in the n-word table t, or n.
u32 TagIndex(u32 t, u32 n, u32 tag) {
    u32 i = 0;
    while (i < n && load<u32>(t + 4 * i) != tag) ++i;
    return i;
}

// XmlCursor_ReadVec3(c, hash, o + off, d + off).
void XVec3D(PPUContext& ctx, u32 c, u32 hash, u32 o, u32 d, u32 off) { call(ctx, xfn::XmlCursor_ReadVec3, {c, hash, o + off, d + off}); }
// XmlCursor_ReadHash(c, hash, o + off, d->off).
void XHashD(PPUContext& ctx, u32 c, u32 hash, u32 o, u32 d, u32 off) { call(ctx, xfn::XmlCursor_ReadHash, {c, hash, o + off, load<u32>(d + off)}); }
// XmlCursor_ReadF32(c, hash, o + off) with the default d->off in f1.
void XF32D(PPUContext& ctx, u32 c, u32 hash, u32 o, u32 d, u32 off) {
    callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, o + off}, {std::bit_cast<f64>(mid1::LfsBits(load<u32>(d + off)))});
}
// XmlCursor_ReadS32(c, hash, o + off, (s32)d->off).
void XS32D(PPUContext& ctx, u32 c, u32 hash, u32 o, u32 d, u32 off) {
    call(ctx, xfn::XmlCursor_ReadS32, {c, hash, o + off, static_cast<u64>(static_cast<s64>(static_cast<s32>(load<u32>(d + off))))});
}
// XmlCursor_ReadF32(c, hash, out) with the default given (as an lfs left it).
void XF32K(PPUContext& ctx, u32 c, u32 hash, u32 out, u32 kaddr) {
    callf(ctx, xfn::XmlCursor_ReadF32, {c, hash, out}, {std::bit_cast<f64>(mid1::LfsBits(load<u32>(kaddr)))});
}
}  // namespace

// void XmlDef_ReadList_000B9908(XmlCursor* c, const T* d, T* o)
// o's 20-byte entries {u32 kind, f32 +4, u32 +8, +12, +16} from c's
// children (the entry staged zeroed on the stack): tag 0x51EA2F9B kind 2
// with hash 0x9178C4CB +12; tag 0x0A7CBA70 kind 1 with f32s 0xE6DF8EF6 +12
// / 0x43B894DB +16; tag 0xA14FA55B kind 0 with f32 0x04B70827 +12 / hash
// 0x04C14205 +16; each then f32 0x89979442 +4 and hash 0xB9B6CDC6 +8 and
// pushed onto o's vector +0 (other tags skipped). Then f32s 0x8F61D65B
// +20 / 0x564F61BC +24, s32s 0x15219E72 +28 / 0x15219D84 +32, f32s
// 0xE8624EBF +36 / 0xD585ED7A +40 / 0x535B8D4E +44 (defaults from d).
DECOMP(000B9908, XmlDef_ReadList_000B9908) {
    const u32 c = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);
    const u32 e = frame.addr();  // the original's sp + 112
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        const u32 tag = NodeTag(n);
        for (u32 k = 0; k < 20; k += 4) store<u32>(e + k, 0);
        if (tag == 0x51EA2F9Bu) {
            store<u32>(e, 2);
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x9178C4CBu, e + 12, 0});
        } else if (tag == 0x0A7CBA70u) {
            store<u32>(e, 1);
            XF32(ctx, n, 0xE6DF8EF6u, e + 12);
            XF32(ctx, n, 0x43B894DBu, e + 16);
        } else if (tag == 0xA14FA55Bu) {
            store<u32>(e, 0);
            XF32(ctx, n, 0x04B70827u, e + 12);
            XHash(ctx, n, 0x04C14205u, e + 16);
        } else {
            continue;
        }
        XF32(ctx, n, 0x89979442u, e + 4);
        XHash(ctx, n, 0xB9B6CDC6u, e + 8);
        VecPush20(ctx, o, e);
    }
    XF32D(ctx, c, 0x8F61D65Bu, o, d, 20);
    XF32D(ctx, c, 0x564F61BCu, o, d, 24);
    XS32D(ctx, c, 0x15219E72u, o, d, 28);
    XS32D(ctx, c, 0x15219D84u, o, d, 32);
    XF32D(ctx, c, 0xE8624EBFu, o, d, 36);
    XF32D(ctx, c, 0xD585ED7Au, o, d, 40);
    XF32D(ctx, c, 0x535B8D4Eu, o, d, 44);
}

// void XmlDef_Read_000B9C88(XmlCursor* c, const T* d, T* o)
// Vec3s / hashes / f32s at +576..+696 (defaults from d; +684 scaled by
// 0x000B9C80 when found), then each child whose tag is entry i of the
// 3-word table 0x00F646C0 read by XmlDef_ReadList_000B9908(child, d +
// 252 + 48 i, o + 252 + 48 i). r3 = 0.
DECOMP(000B9C88, XmlDef_Read_000B9C88) {
    const u32 c = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    XVec3D(ctx, c, 0x0A146036u, o, d, 576);
    XVec3D(ctx, c, 0xCF76A788u, o, d, 592);
    XVec3D(ctx, c, 0x63662D61u, o, d, 608);
    XHashD(ctx, c, 0x6F830E8Au, o, d, 624);
    XF32D(ctx, c, 0x7D873487u, o, d, 628);
    XF32D(ctx, c, 0x3000B249u, o, d, 632);
    XF32D(ctx, c, 0xA7E295B8u, o, d, 636);
    XF32D(ctx, c, 0xE7E0104Fu, o, d, 640);
    XF32D(ctx, c, 0xD08693FAu, o, d, 644);
    XHashD(ctx, c, 0x68A7D152u, o, d, 648);
    XHashD(ctx, c, 0x6885DB46u, o, d, 652);
    XF32D(ctx, c, 0xA83567ABu, o, d, 656);
    XF32D(ctx, c, 0x1A08DA22u, o, d, 660);
    XF32D(ctx, c, 0x706EAC4Bu, o, d, 664);
    XHashD(ctx, c, 0xA7FD77A6u, o, d, 668);
    XHashD(ctx, c, 0x1C705693u, o, d, 672);
    XF32D(ctx, c, 0xAC8D9D61u, o, d, 676);
    XF32D(ctx, c, 0x90DD5E33u, o, d, 680);
    XF32D(ctx, c, 0x65C5D708u, o, d, 684);
    if (ctx.gpr[3] == 0) StoreF(o + 684, MulS(LoadF(o + 684), LoadF(0x000B9C80)));
    XF32D(ctx, c, 0x585F7EA5u, o, d, 688);
    XF32D(ctx, c, 0x17326A5Cu, o, d, 692);
    XF32D(ctx, c, 0x1210F240u, o, d, 696);
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        const u32 i = TagIndex(kTagTbl3, 3, NodeTag(n));
        if (i < 3) call(ctx, 0x000B9908, {n, d + 252 + 48 * i, o + 252 + 48 * i});
    }
}

// void XmlDef_Read_000BA084(XmlCursor* c, const T* d, T* o)
// Vec3 0x6A8E77E1 +32, hashes 0x8FE931CE +52 / 0xAB27550F +56 / 0x9E7F122F
// +60, f32s 0x39E5B2A5 +64 / 0x562C2C1C +68 / 0xC3D02A1A +72 / 0x8078D810
// +76 (defaults from d); then children: tag 0xD03BFCFD pushes its hash
// 0x0024728B (default 0, when set) onto o's vector +0, tag 0x4856CB5F
// reads it into +48. r3 = 0.
DECOMP(000BA084, XmlDef_Read_000BA084) {
    const u32 c = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 h = frame.addr();  // the original's sp + 112
    XVec3D(ctx, c, 0x6A8E77E1u, o, d, 32);
    XHashD(ctx, c, 0x8FE931CEu, o, d, 52);
    XHashD(ctx, c, 0xAB27550Fu, o, d, 56);
    XHashD(ctx, c, 0x9E7F122Fu, o, d, 60);
    XF32D(ctx, c, 0x39E5B2A5u, o, d, 64);
    XF32D(ctx, c, 0x562C2C1Cu, o, d, 68);
    XF32D(ctx, c, 0xC3D02A1Au, o, d, 72);
    XF32D(ctx, c, 0x8078D810u, o, d, 76);
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        if (!(load<u8>(n + 24) & 0x80u)) continue;
        const u32 tag = load<u32>(n + 20);
        if (tag == 0xD03BFCFDu) {
            store<u32>(h, 0);
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x0024728Bu, h, 0});
            if (load<u32>(h) != 0) mid1::PtrVecPush(ctx, o, load<u32>(h));
        } else if (tag == 0x4856CB5Fu) {
            XHash(ctx, n, 0x0024728Bu, o + 48);
        }
    }
}

// void XmlDef_ReadList_000BA318(XmlCursor* c, T* unused, Vector* v)
// v's 20-byte entries {u32 kind, +4, +8, +12, f32 +16} from c's children
// whose tag is entry i of the 5-word table 0x00F646D8 (kind = i; the entry
// is staged on the stack and keeps what earlier entries left in the words
// a kind does not read; f32 defaults the pool word 0x000BA30C):
// 4 / 3: f32 0x00274ACD +4;
// 2: s32s 0xF0150938 +4 / 0xF015084A +8 (default 0), +4 = min(+4, +8)
//    (unsigned);
// 1: f32s 0x6C038E80 +4 / 0x2D3AC4D0 +8, +8 scaled by 0x000BA310 when
//    found;
// 0: s32s 0xC56330D7 +4 / 0xC5632FE9 +8 / 0x2751B533 +12 (default 0), f32
//    0x2A4032AA +16, +4 = min(+4, +8) (unsigned).
// r3 = 0.
DECOMP(000BA318, XmlDef_ReadList_000BA318) {
    const u32 c = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);
    const u32 e = frame.addr();  // the original's sp + 112
    constexpr u32 kPool = 0x000BA30C;
    auto umin = [&]() {
        if (load<u32>(e + 4) > load<u32>(e + 8)) store<u32>(e + 4, load<u32>(e + 8));
    };
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        const u32 i = TagIndex(kTagTbl5, 5, NodeTag(n));
        if (i >= 5) continue;
        store<u32>(e, i);
        switch (i) {
            case 4:
            case 3: XF32K(ctx, n, 0x00274ACDu, e + 4, kPool); break;
            case 2:
                call(ctx, xfn::XmlCursor_ReadS32, {n, 0xF0150938u, e + 4, 0});
                call(ctx, xfn::XmlCursor_ReadS32, {n, 0xF015084Au, e + 8, 0});
                umin();
                break;
            case 1:
                XF32K(ctx, n, 0x6C038E80u, e + 4, kPool);
                XF32K(ctx, n, 0x2D3AC4D0u, e + 8, kPool);
                if (ctx.gpr[3] == 0) {
                    const f64 s = MulS(LoadF(e + 8), LoadF(kPool + 4));
                    StoreF(e + 8, s);
                    ctx.fpr[1] = s;
                }
                break;
            default:
                call(ctx, xfn::XmlCursor_ReadS32, {n, 0xC56330D7u, e + 4, 0});
                call(ctx, xfn::XmlCursor_ReadS32, {n, 0xC5632FE9u, e + 8, 0});
                call(ctx, xfn::XmlCursor_ReadS32, {n, 0x2751B533u, e + 12, 0});
                XF32K(ctx, n, 0x2A4032AAu, e + 16, kPool);
                umin();
                break;
        }
        VecPush20(ctx, v, e);
        SetF1(ctx, e + 16);
    }
}

// void XmlDef_Read_000BA618(XmlCursor* c, const T* d, T* o)
// f32 0xA7E295B8 +424 (default d), then c's children by tag: 0x04DA9970
// pushes its hash 0x7A30D7E3 (default 0, when set) onto o's vector +128;
// 0x7B14285B pushes its hash 0x0024728B onto +148; entry i of the table
// 0x00F646C0 is read by XmlDef_ReadList_000BA318(child, d + 192 + 20 i, o +
// 192 + 20 i); entry 0 of 0x00F646D8 reads vec3 0x6A8E77E1 +432, hashes
// 0x8FE931CE +448 / 0xAEDAEEAD +452, f32s 0xB9D556E7 +456 / 0xE998BF37
// +460 / 0xA2DDBF5E +464; entry 1 hashes 0xD0A12D53 +480 / 0xF4ED3EBA +484
// and f32s 0xE0D094B8 +488 / 0x73F8C1C4 +492; entry 2 is read by
// XmlDef_Read_000BA084(child, d + 496, o + 496) (defaults from d; entries
// 3 / 4 and other tags skipped). r3 = 0.
DECOMP(000BA618, XmlDef_Read_000BA618) {
    const u32 c = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);
    const u32 h0 = frame.addr(), h1 = h0 + 4;  // the original's sp + 112 / + 116
    XF32D(ctx, c, 0xA7E295B8u, o, d, 424);
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        const u32 tag = NodeTag(n);
        if (tag == 0x04DA9970u) {
            store<u32>(h0, 0);
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x7A30D7E3u, h0, 0});
            if (load<u32>(h0) != 0) mid1::PtrVecPush(ctx, o + 128, load<u32>(h0));
            continue;
        }
        if (tag == 0x7B14285Bu) {
            store<u32>(h1, 0);
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x0024728Bu, h1, 0});
            if (load<u32>(h1) != 0) mid1::PtrVecPush(ctx, o + 148, load<u32>(h1));
            continue;
        }
        const u32 i = TagIndex(kTagTbl3, 3, tag);
        if (i < 3) {
            call(ctx, 0x000BA318, {n, d + 192 + 20 * i, o + 192 + 20 * i});
            continue;
        }
        switch (TagIndex(kTagTbl5, 3, tag)) {
            case 0:
                XVec3D(ctx, n, 0x6A8E77E1u, o, d, 432);
                XHashD(ctx, n, 0x8FE931CEu, o, d, 448);
                XHashD(ctx, n, 0xAEDAEEADu, o, d, 452);
                XF32D(ctx, n, 0xB9D556E7u, o, d, 456);
                XF32D(ctx, n, 0xE998BF37u, o, d, 460);
                XF32D(ctx, n, 0xA2DDBF5Eu, o, d, 464);
                break;
            case 1:
                XHashD(ctx, n, 0xD0A12D53u, o, d, 480);
                XHashD(ctx, n, 0xF4ED3EBAu, o, d, 484);
                XF32D(ctx, n, 0xE0D094B8u, o, d, 488);
                XF32D(ctx, n, 0x73F8C1C4u, o, d, 492);
                break;
            case 2: call(ctx, 0x000BA084, {n, d + 496, o + 496}); break;
            default: break;  // entries 3 / 4: the original keeps searching, finds nothing more to do
        }
    }
}

// Def* XmlDef_New_00332898(a0, XmlCursor* c, x)
// A stack PtrVector of f32 (capacity 4): the hash 0x7A30D7E3 and bool
// 0x4FCD42D9 read into stack temporaries (unused), then each child tagged
// 0x49F9F44B pushes its f32 0x0F9A3C75 (default the pool word 0x00332890);
// the values sorted (qsort with the compare OPD 0x00F36ED8); a 2608-byte
// object (SmallAlloc) built by 0x0033273C(p, x, &vec) when allocated; the
// vector cleared. Returns p (0 when the allocation fails).
DECOMP(00332898, XmlDef_New_00332898) {
    const u32 c = arg<u32>(ctx, 1), x = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);
    const u32 f = frame.addr(), v = f + 4, h = f + 24, b = f + 28;  // the original's sp + 112 / + 116 / + 136 / + 140
    mid1::PtrVecNew(ctx, v, 4, 4, 0);
    store<u32>(h, 0);
    store<u8>(b, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x7A30D7E3u, h, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x4FCD42D9u, b, load<u8>(b)});
    for (u32 n = static_cast<u32>(call(ctx, m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, m7fn::XmlCursor_NextSiblingAny, {n}))) {
        if (NodeTag(n) != 0x49F9F44Bu) continue;
        store<u32>(f, 0);
        XF32K(ctx, n, 0x0F9A3C75u, f, 0x00332890);
        mid1::PtrVecPush(ctx, v, load<u32>(f));
        SetF1(ctx, f);
    }
    call(ctx, 0x0071EC28, {load<u32>(v), load<u16>(v + 8), 4, 0x00F36ED8});  // qsort
    const u32 p = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {2608}));
    if (p != 0) call(ctx, 0x0033273C, {p, x, v});
    call(ctx, m1fn::PtrVector_Clear, {v});
    call(ctx, m1fn::Nop, {v});
    ret(ctx, p);
}

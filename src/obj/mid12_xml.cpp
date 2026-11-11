// Game code, medium sweep part 12: XML def readers / initialisers.
#include "mid12.h"

using namespace sly4;
using namespace sly4::mid12;

namespace {
namespace m2fn = mid2::m2fn;

// A 56-byte list item (SmallAlloc): +32 0, an inline PtrVector at +36
// (element size elem, capacity 64), memset(item, 0, 32). 0 when the
// allocation fails.
u32 NewItem56(PPUContext& ctx, u16 elem) {
    const u32 q = static_cast<u32>(call(ctx, m1fn::SmallAlloc, {56}));
    if (q == 0) return 0;
    store<u32>(q + 32, 0);
    mid1::PtrVecNew(ctx, q + 36, 64, elem, 1);
    call(ctx, m2fn::memset, {q, 0, 32});
    return q;
}
}  // namespace

// void XmlDef_Init30743C(Def* p, XmlCursor* c)
// Fills the def's three item lists from c's children (p may be 0 when the
// factory's allocation failed; the original does not check it):
// - tag 0x6155B001: a 56-byte item (Mem_Alloc(56, 0, 16); +32..+52 0,
//   memset(item, 0, 32); not checked: a failed allocation reads into 0):
//   the name attribute 0x0024728B copied (String_Assign) to +0, the hash
//   0x0024728B into +32, the f32s 0x2A786531 / 0x99F2F517 / 0xABB0AF23 /
//   0xEE3F6AE0 into +36 / +40 / +44 / +52; pushed onto +304;
// - tag 0x88B4A252: NewItem56 (element size 4) read by 0x0030339C(item,
//   child) (with 0 too), pushed onto +324;
// - tag 0xD9ABF25D: NewItem56 (element size 20) read by 0x0030386C(item,
//   child), pushed onto +344.
// r3 = 0 (the last sibling lookup's).
DECOMP(0030743C, XmlDef_Init30743C) {
    const u32 p = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kTagA = 0x6155B001u, kTagB = 0x88B4A252u, kTagC = 0xD9ABF25Du, kName = 0x0024728Bu;
    for (u32 n = static_cast<u32>(call(ctx, m2fn::XmlCursor_FirstChild, {c, kTagA})); n != 0;
         n = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {n, kTagA}))) {
        const u32 q = static_cast<u32>(call(ctx, m1fn::Mem_Alloc, {56, 0, 16}));
        if (q != 0) {
            for (u32 k = 32; k <= 52; k += 4) store<u32>(q + k, 0);
            call(ctx, m2fn::memset, {q, 0, 32});
        }
        const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {n, kName}));
        if (s != 0) call(ctx, m1fn::String_Assign, {q, s});
        mid5::XHash(ctx, n, kName, q + 32);
        mid5::XF32(ctx, n, 0x2A786531u, q + 36);
        mid5::XF32(ctx, n, 0x99F2F517u, q + 40);
        mid5::XF32(ctx, n, 0xABB0AF23u, q + 44);
        mid5::XF32(ctx, n, 0xEE3F6AE0u, q + 52);
        mid1::PtrVecPush(ctx, p + 304, q);
    }
    for (u32 n = static_cast<u32>(call(ctx, m2fn::XmlCursor_FirstChild, {c, kTagB})); n != 0;
         n = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {n, kTagB}))) {
        const u32 q = NewItem56(ctx, 4);
        call(ctx, 0x0030339C, {q, n});
        mid1::PtrVecPush(ctx, p + 324, q);
    }
    for (u32 n = static_cast<u32>(call(ctx, m2fn::XmlCursor_FirstChild, {c, kTagC})); n != 0;
         n = static_cast<u32>(call(ctx, m2fn::XmlCursor_NextSibling, {n, kTagC}))) {
        const u32 q = NewItem56(ctx, 20);
        call(ctx, 0x0030386C, {q, n});
        mid1::PtrVecPush(ctx, p + 344, q);
    }
    ctx.gpr[3] = 0;
}

// void XmlReg_ReadObjD4E28(a0, XmlCursor* c, Obj** inout)
// Reads the registry object *inout (a0 unused; *inout re-read after every
// call): the euler vec3 0x9F79F1B1 (default 0, scaled by the pool word
// 0x000D4E1C into a stack copy) turned into the quaternion at +16 (half
// angles scaled by 0x000D4E20; Math_Cosf / Math_Sinf of y, z, x); vec3s
// 0x88FDF556 +32 (default 0, then scaled), 0x9F79EA2E +48, 0x1F236753 +64,
// 0x52AFCD49 +80 (then scaled), 0xB02C44EE +96 (then scaled); then the hash
// 0xFBA47770 (default 0): 0 stops; one of the four at 0x00AC69F0 sets the
// mode bits 31..30 of +124 to its index (else Hash_ToString(hash) and
// stop); mode 1 reads the vec3 0xDCF8B269 (default 0) into +112..+120
// through the stack copy, modes 2 / 3 the f32 0x8FA16092 (default
// 0x000D4E24) into +112, mode 3 also sets byte +116 to 1, then to 0..2 when
// the attribute "FromTarget" starts with x / y / z (either case); finally
// the bool 0x7B9BA162 into bit 29 of +124. r3 = *inout (0 / Hash_ToString's
// when stopped early).
DECOMP(000D4E28, XmlReg_ReadObjD4E28) {
    const u32 c = arg<u32>(ctx, 1), io = arg<u32>(ctx, 2);
    Stack frame(ctx, 32);
    const u32 v = frame.addr(), hs = v + 16, bs = v + 20;  // the original's sp + 112 / + 128 / + 132
    constexpr u32 kPool = 0x000D4E1C;
    const u32 zero = mid11::kVecZero;
    auto scale3 = [&](u32 a, f64 k) {
        const f64 x = MulS(LoadF(a), k), y = MulS(LoadF(a + 4), k), z = MulS(LoadF(a + 8), k);
        StoreF(a, x);
        StoreF(a + 4, y);
        StoreF(a + 8, z);
        ctx.fpr[1] = x;
    };
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x9F79F1B1u, v, zero});
    const f64 k0 = LoadF(kPool), k4 = LoadF(kPool + 4);
    const f64 ex = MulS(LoadF(v), k0), ey = MulS(LoadF(v + 4), k0), ez = MulS(LoadF(v + 8), k0);
    const u32 q = load<u32>(io) + 16;
    StoreF(v, ex);
    StoreF(v + 4, ey);
    StoreF(v + 8, ez);
    EulerQuatAt(ctx, q, ex, ey, ez, k4);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x88FDF556u, load<u32>(io) + 32, zero});
    u32 o = load<u32>(io);
    scale3(o + 32, k0);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x9F79EA2Eu, o + 48, o + 48});
    o = load<u32>(io);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x1F236753u, o + 64, o + 64});
    o = load<u32>(io);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0x52AFCD49u, o + 80, o + 80});
    o = load<u32>(io);
    scale3(o + 80, k0);
    call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xB02C44EEu, o + 96, o + 96});
    o = load<u32>(io);
    scale3(o + 96, k0);
    store<u32>(hs, 0);
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xFBA47770u, hs, 0});
    const u32 h = load<u32>(hs);
    if (h == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    o = load<u32>(io);
    u32 i = 0;
    while (i < 4 && load<u32>(0x00AC69F0u + 4 * i) != h) ++i;
    if (i >= 4) {
        call(ctx, 0x0056A6E0, {h});  // Hash_ToString
        return;
    }
    store<u32>(o + 124, (load<u32>(o + 124) & 0x3FFFFFFFu) | (i << 30));
    o = load<u32>(io);
    const u32 mode = load<u32>(o + 124) >> 30;
    if (mode == 1) {
        call(ctx, xfn::XmlCursor_ReadVec3, {c, 0xDCF8B269u, v, zero});
        for (u32 k = 0; k < 12; k += 4) CopyW(load<u32>(io) + 112 + k, v + k);
        SetF1(ctx, v + 8);
    } else if (mode >= 2) {
        callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x8FA16092u, o + 112}, {LoadF(kPool + 8)});
        if (mode == 3) {
            store<u8>(load<u32>(io) + 116, 1);
            const u32 s = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {c, 0x00AC6A00u}));  // "FromTarget"
            if (s != 0) {
                u32 ch = load<u8>(s);
                const s32 sc = static_cast<s8>(ch);
                if (sc >= 65 && sc <= 90) ch += 32;
                const s32 lc = static_cast<s8>(ch);
                if (lc >= 120 && lc <= 122) store<u8>(load<u32>(io) + 116, static_cast<u8>(lc - 120));
            }
        }
    }
    o = load<u32>(io);
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x7B9BA162u, bs, (load<u32>(o + 124) & 0x20000000u) ? 1u : 0u});
    o = load<u32>(io);
    store<u32>(o + 124, PutBit(load<u32>(o + 124), 2, load<u8>(bs)));
    ctx.gpr[3] = o;
}

// void XmlCmdList_Read_0030339C(Item* item, XmlCursor* c)
// The name attribute 0x0024728B copied (String_Assign) to item +0, the
// hash 0x0024728B into +32; then each child becomes a command object by its
// tag (SmallAlloc; vtable, +4.. cleared): 0x760CB725 12 bytes (0x00B1DA60),
// 0x6A8A0D06 80 bytes (0x00B1DA00; zero vec3s +16 / +32, f32 +68 = the word
// 0x01069EF8), 0x651BDBA8 8 bytes (0x00B1DA30), 0x60BB2C95 36 bytes
// (0x00B1D9D0, memset(+4, 0, 32)), 0x04862DD6 8 bytes (0x00B1DA48),
// 0x044DC6AB 8 bytes (0x00B1D9E8), 0xD9ABF25D 112 bytes (0x00B1DA18; as the
// 80-byte one plus +80..+100 cleared); other tags are reported
// (String_Format(0x00AD9F3C, tag)). Each object built reads the child
// (vt[2](obj, child)) and is pushed onto item +36. r3 = 0.
DECOMP(0030339C, XmlCmdList_Read_0030339C) {
    const u32 item = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 kName = 0x0024728Bu, kK68 = 0x01069EF8u;
    const u32 vz = mid11::kVecZero;
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, kName}));
    if (s != 0) call(ctx, m1fn::String_Assign, {item, s});
    mid5::XHash(ctx, c, kName, item + 32);
    auto head80 = [&](u32 o, u32 vt) {  // the 80-byte command's fields, as both builders store them
        store<u32>(o, vt);
        store<u32>(o + 4, 0);
        if (vt == 0x00B1DA18u) {
            store<u32>(o + 8, 0);
            store<u32>(o + 12, 0);
        }
        CopyW(o + 16, vz);
        CopyW(o + 20, vz + 4);
        CopyW(o + 24, vz + 8);
        CopyW(o + 32, vz);
        CopyW(o + 36, vz + 4);
        store<u32>(o + 48, 0);
        CopyW(o + 40, vz + 8);
        for (u32 k = 52; k <= 64; k += 4) store<u32>(o + k, 0);
    };
    for (u32 n = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_FirstChildAny, {c})); n != 0;
         n = static_cast<u32>(call(ctx, mid7::m7fn::XmlCursor_NextSiblingAny, {n}))) {
        const u32 tag = NodeTag(n);
        auto alloc = [&](u32 size) { return static_cast<u32>(call(ctx, m1fn::SmallAlloc, {size})); };
        auto small = [&](u32 size, u32 vt) {
            const u32 o = alloc(size);
            if (o != 0) {
                store<u32>(o, vt);
                for (u32 k = 4; k < size; k += 4) store<u32>(o + k, 0);
            }
            return o;
        };
        u32 o;
        switch (tag) {
            case 0x760CB725u: o = small(12, 0x00B1DA60u); break;
            case 0x6A8A0D06u:
                o = alloc(80);
                if (o != 0) {
                    head80(o, 0x00B1DA00u);
                    CopyW(o + 68, kK68);
                    SetF1(ctx, kK68);
                }
                break;
            case 0x651BDBA8u: o = small(8, 0x00B1DA30u); break;
            case 0x60BB2C95u:
                o = alloc(36);
                if (o != 0) {
                    store<u32>(o, 0x00B1D9D0u);
                    call(ctx, mid2::m2fn::memset, {o + 4, 0, 32});
                }
                break;
            case 0x04862DD6u: o = small(8, 0x00B1DA48u); break;
            case 0x044DC6ABu: o = small(8, 0x00B1D9E8u); break;
            case 0xD9ABF25Du:
                o = alloc(112);
                if (o != 0) {
                    head80(o, 0x00B1DA18u);
                    store<u32>(o + 80, 0);
                    CopyW(o + 68, kK68);
                    store<u32>(o + 92, 0);
                    store<u8>(o + 96, 0);
                    for (u32 k = 84; k < 92; ++k) store<u8>(o + k, 0);
                    store<u8>(o + 100, 0);
                    SetF1(ctx, kK68);
                }
                break;
            default:
                call(ctx, m1fn::String_Format, {0x00AD9F3Cu, tag});
                continue;
        }
        if (o == 0) continue;
        obj19::VCall(ctx, o, 8, {o, n});
        mid1::PtrVecPush(ctx, item + 36, o);
    }
}

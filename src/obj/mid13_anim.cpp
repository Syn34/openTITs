// Game code, medium sweep part 13: anim clip container readers (elements
// taken from a record pool) and an instance setup that walks a node tree.
#include "mid13.h"

using namespace sly4;
using namespace sly4::mid13;

namespace {

constexpr u32 AnimClip_ReadBase = 0x004B31D8;  // (o, c, pool, r6) (reads r3..r6)
constexpr u32 StrBuf_Append = 0x00566F1C;      // (StrBuf*, const char*) -> char*
constexpr u32 Xml_Where = 0x004F61C4;          // (node) -> const char*
constexpr u32 kGame = 0x00F65F84;              // Game* (its +0 is the string buffer here)

// The inlined pool record allocation: the pool {+4 header {u16 capacity,
// u16 count}, +8 optional u32[] cleared per slot, +12 76-byte records}
// hands out slot (s16)count (count raised) while count < capacity, else
// -1. Returns the record with its +0 = kind, or 0 when the slot is
// negative or not below the new count. Quirk: a count of 0x8000 or more is
// raised but gives 0.
u32 PoolRec(u32 pool, u32 kind, s32& idx) {
    u32 hdr = load<u32>(pool + 4);
    const u32 cnt = load<u16>(hdr + 2);
    if (static_cast<s32>(cnt) < static_cast<s32>(load<u16>(hdr))) {
        const u32 d = load<u32>(pool + 8);
        if (d != 0) {
            store<u32>(d + ((cnt << 2) & 0x3FFFCu), 0);
            hdr = load<u32>(pool + 4);
        }
        store<u16>(hdr + 2, static_cast<u16>(load<u16>(hdr + 2) + 1));
        idx = static_cast<s16>(cnt);
    } else {
        idx = -1;
    }
    if (idx < 0 || idx >= static_cast<s32>(load<u16>(hdr + 2))) return 0;
    const u32 e = load<u32>(pool + 12) + 76u * static_cast<u32>(idx);
    store<u32>(e, kind);
    return e;
}

// The containers' tally: the first element's slot into u16 o+68, o->60
// counted, f32 o->64 += the element's weight.
void Tally(PPUContext& ctx, u32 o, s32 idx, u32 w) {
    const u32 n = load<u32>(o + 60);
    if (n == 0) store<u16>(o + 68, static_cast<u16>(idx));
    const f64 s = static_cast<f32>(LoadF(o + 64) + LoadF(w));
    store<u32>(o + 60, n + 1);
    StoreF(o + 64, s);
    ctx.fpr[1] = s;  // the fadds result, as the original leaves f1
}

// The allocation failure: a message with o->4, bit 0x80 of byte o+20 set;
// returns that byte.
u32 PoolFail(PPUContext& ctx, u32 o, u32 fmt) {
    call(ctx, m1fn::String_Format, {fmt, load<u32>(o + 4)});
    const u32 b = load<u8>(o + 20) | 0x80u;
    store<u8>(o + 20, static_cast<u8>(b));
    return b;
}

}  // namespace

// u32 AnimClip_Read4B5918(AnimClip* o, XmlNode* c, Pool* pool)
// (via Pool_NewRec_004B5C70) AnimClip_ReadBase(o, c, pool, r6 as left by
// the caller), then for each "AnimClip" child n a kind-12 record from the
// pool: byte +4 0, +8 the clip name (attribute 0x6565E5DB, interned by
// StrBuf_Append on the game's buffer; a warning with n's location when
// missing), +12 its no-case hash, f32 +16 the weight (0x98643678, default
// 100), tallied into o (+60 count, +64 total weight, +68 first slot).
// Returns 0, or byte o+20 with bit 0x80 set when the pool is exhausted.
DECOMP(004B5918, AnimClip_Read4B5918) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), pool = arg<u32>(ctx, 2);
    call(ctx, AnimClip_ReadBase, {o, c, pool, ctx.gpr[6]});
    constexpr u32 kTag = 0x00AEE330;  // "AnimClip"
    const f64 w0 = LoadF(0x004B5914);
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_FindChildren, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_NextSibling_004F9078, {n, kTag}))) {
        s32 idx;
        const u32 e = PoolRec(pool, 12, idx);
        if (e == 0) {
            ret(ctx, PoolFail(ctx, o, 0x00AEE9A4u));  // "Fail to allocate anim clip element ..."
            return;
        }
        store<u32>(e + 8, 0);
        store<u32>(e + 12, 0);
        store<u32>(e + 16, 0);
        store<u8>(e + 4, 0);
        const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {n, 0x6565E5DBu}));
        store<u32>(e + 8, s);
        if (s == 0) {
            const u32 where = static_cast<u32>(call(ctx, Xml_Where, {n}));
            call(ctx, m1fn::String_Format, {0x00AEE95Cu, where});  // "Null clip name ..."
        } else {
            const u32 t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), s}));
            store<u32>(e + 8, t);
            store<u32>(e + 12, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})));
            callf(ctx, xfn::XmlCursor_ReadF32, {n, 0x98643678u, e + 16}, {w0});
        }
        Tally(ctx, o, idx, e + 16);
    }
    ret(ctx, 0u);
}

// u32 AnimRandom_Read4B46A8(AnimClip* o, XmlNode* c, Pool* pool)
// (via Pool_NewRec_004B49E0) As AnimClip_Read4B5918 for the "AnimRandom"
// children: kind-6 records with byte +4 (set when the weight is missing),
// +8 / +12 the "AnimName" attribute interned and hashed, +16 / +20 the
// "Map" attribute likewise (when there is a name), f32 +24 the weight
// (0x98643678, default 0; a warning with the name when missing).
DECOMP(004B46A8, AnimRandom_Read4B46A8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), pool = arg<u32>(ctx, 2);
    call(ctx, AnimClip_ReadBase, {o, c, pool, ctx.gpr[6]});
    constexpr u32 kTag = 0x00AEE348;  // "AnimRandom"
    const f64 w0 = LoadF(0x004B46A4);
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_FindChildren, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_NextSibling_004F9078, {n, kTag}))) {
        s32 idx;
        const u32 e = PoolRec(pool, 6, idx);
        if (e == 0) {
            ret(ctx, PoolFail(ctx, o, 0x00AEE7DCu));  // "Fail to allocate random anim element ..."
            return;
        }
        for (u32 k = 8; k <= 20; k += 4) store<u32>(e + k, 0);
        store<u8>(e + 4, 0);
        store<u32>(e + 24, 0);
        const u32 s = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {n, 0x00AEE734u}));  // "AnimName"
        store<u32>(e + 8, s);
        if (s == 0) {
            const u32 where = static_cast<u32>(call(ctx, Xml_Where, {n}));
            call(ctx, m1fn::String_Format, {0x00AEE744u, where});  // "Null anim name ..."
        } else {
            const u32 t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), s}));
            store<u32>(e + 8, t);
            store<u32>(e + 12, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})));
            const u32 m = static_cast<u32>(call(ctx, m11fn::XmlCursor_FindAttrByName, {n, 0x00AEE784u}));  // "Map"
            store<u32>(e + 16, m);
            if (m != 0) {
                const u32 t2 = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), m}));
                store<u32>(e + 16, t2);
                store<u32>(e + 20, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t2})));
            }
        }
        callf(ctx, xfn::XmlCursor_ReadF32, {n, 0x98643678u, e + 24}, {w0});
        if (static_cast<u32>(ctx.gpr[3]) != 0) {  // not found
            call(ctx, m1fn::String_Format, {0x00AEE788u, load<u32>(e + 8)});  // "Cannot get the weight ..."
            store<u8>(e + 4, 1);
        }
        Tally(ctx, o, idx, e + 24);
    }
    ret(ctx, 0u);
}

// u32 AnimInst_Init4B69B8(AnimInst* o, a1, a2, XmlNode* c)
// Instance_Base4B71BC(o, a1, a2, c), vtable 0x00B21CC0, +32..+44 0, bytes
// +48..+51 0; reads a float (0xB7EA3D94, default 0x004B69B0) into o+36 and
// bools (0x0023BEA4, 0x0021ECBF) into bytes +48 / +49. Then the tree
// iterator o->20 {+12 tree (root at +0), +16 u32 stack[], +96 s16 depth,
// +100 current} is reset to the leftmost node (nodes: +8 payload, +12
// left, +16 right; ancestors pushed) and walked in order, raising f32 o+36
// to each payload's +28 when that is greater. Returns o->20.
DECOMP(004B69B8, AnimInst_Init4B69B8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 3);
    call(ctx, 0x004B71BC, {o, ctx.gpr[4], ctx.gpr[5], c});  // Instance_Base4B71BC
    store<u32>(o, 0x00B21CC0u);
    store<u32>(o + 32, 0);
    store<u32>(o + 36, 0);
    store<u8>(o + 48, 0);
    store<u8>(o + 50, 0);
    store<u8>(o + 51, 0);
    store<u8>(o + 49, 0);
    store<u32>(o + 40, 0);
    store<u32>(o + 44, 0);
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0xB7EA3D94u, o + 36}, {LoadF(0x004B69B0)});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0023BEA4u, o + 48, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0021ECBFu, o + 49, 0});
    const u32 it = load<u32>(o + 20);
    auto slot = [it](u32 d) { return it + 16 + 4u * static_cast<u32>(static_cast<s32>(static_cast<s16>(d))); };
    if (it != 0) {
        store<u16>(it + 96, 0);
        u32 n = load<u32>(load<u32>(it + 12));
        u32 cur = 0;
        while (n != 0) {  // descend left, pushing
            cur = n;
            const u32 l = load<u32>(n + 12);
            if (l != 0) {
                const u32 d = load<u16>(it + 96);
                store<u16>(it + 96, static_cast<u16>(d + 1));
                store<u32>(slot(d), n);
            }
            n = l;
        }
        store<u32>(it + 100, cur);
    }
    while (it != 0 && load<u32>(it + 100) != 0) {
        const u32 cur = load<u32>(it + 100);
        const u32 p = load<u32>(cur + 8);
        ctx.fpr[1] = LfsF(load<u32>(p + 28));  // as the original leaves f1
        if (ctx.fpr[1] > LoadF(o + 36)) CopyW(o + 36, p + 28);
        u32 r = load<u32>(cur + 16);
        if (r == 0) {  // up: pop until arriving from a left child
            u32 d = load<u16>(it + 96), child = cur, next = 0;
            while ((d & 0xFFFFu) != 0) {
                d -= 1;
                store<u16>(it + 96, static_cast<u16>(d));
                const u32 par = load<u32>(slot(d));
                if (child == load<u32>(par + 12)) {
                    next = par;
                    break;
                }
                store<u32>(it + 100, par);
                child = par;
            }
            store<u32>(it + 100, next);
            continue;
        }
        {  // push cur, then down the right child's left spine
            const u32 d = load<u16>(it + 96);
            store<u16>(it + 96, static_cast<u16>(d + 1));
            store<u32>(slot(d), cur);
        }
        while (load<u32>(r + 12) != 0) {
            const u32 d = load<u16>(it + 96);
            store<u32>(slot(d), r);
            r = load<u32>(r + 12);
            store<u16>(it + 96, static_cast<u16>(d + 1));
        }
        store<u32>(it + 100, r);
    }
    ret(ctx, it);
}

namespace {

// The containers' control value type: the no-case hash of name s looked up
// in the game's table (+264 {u32, u32 hash}[], +268 count); u16 o+56 = its
// index, or -1 with a "Fail to find the anim control data type" message
// when absent (or at 0x8000 or more). Returns the original's r3 (the index,
// or the message call's result).
void CtlType(PPUContext& ctx, u32 o, u32 s) {
    const u32 game = load<u32>(kGame);
    const u32 h = static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {s}));
    const u32 n = load<u32>(game + 268);
    if (n != 0) {
        u32 p = load<u32>(game + 264), i = 0;
        do {
            if (load<u32>(p + 4) == h) {
                if (static_cast<s16>(i) >= 0) {
                    store<u16>(o + 56, static_cast<u16>(i));
                    ret(ctx, i);
                    return;
                }
                break;
            }
            ++i;
            p += 8;
        } while (i < n);
    }
    call(ctx, m1fn::String_Format, {0x00B21C08u, s});
    store<u16>(o + 56, 0xFFFFu);
}

// The selection / blend elements' names: +8 / +12 the clip name (attribute 0xFF45BC1C) interned and hashed and
// +16 / +20 the attribute 0x0001293C likewise, or a warning (fmt with n's
// location) when the name is missing. Returns false then.
bool SelHead(PPUContext& ctx, u32 e, u32 n, u32 fmt) {
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {n, 0xFF45BC1Cu}));
    store<u32>(e + 8, s);
    if (s == 0) {
        const u32 where = static_cast<u32>(call(ctx, Xml_Where, {n}));
        call(ctx, m1fn::String_Format, {fmt, where});
        return false;
    }
    const u32 t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), s}));
    store<u32>(e + 8, t);
    store<u32>(e + 12, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})));
    const u32 m = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {n, 0x0001293Cu}));
    store<u32>(e + 16, m);
    if (m != 0) {
        const u32 t2 = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), m}));
        store<u32>(e + 16, t2);
        store<u32>(e + 20, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t2})));
    }
    return true;
}

// The selection / blend tally: the first element's slot into u16 o+64,
// o->60 counted.
void SelTally(u32 o, s32 idx) {
    const u32 k = load<u32>(o + 60);
    if (k == 0) store<u16>(o + 64, static_cast<u16>(idx));
    store<u32>(o + 60, k + 1);
}

}  // namespace

// u32 AnimSelection_Read4B4EC8(AnimClip* o, XmlNode* c, Pool* pool)
// AnimClip_ReadBase(o, c, pool, r6 as left by the caller), then for each
// "AnimSelection" child n a kind-8 record from the pool (SelHead; when
// named: f32 +24 (0x04DA9951, default 0x004B4EC0), hash +28 (0x10D018A4,
// default 0x34212D3C), bool +32 (0x0023BEA4)), counted into o (+60, first
// slot +64). Then the control value type (attribute 0x0027873A, CtlType;
// when missing a message and bit 0x80 of byte o+20). Returns the last r3
// (the type index, a message result or byte o+20).
DECOMP(004B4EC8, AnimSelection_Read4B4EC8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), pool = arg<u32>(ctx, 2);
    call(ctx, AnimClip_ReadBase, {o, c, pool, ctx.gpr[6]});
    constexpr u32 kTag = 0x00AEE358;  // "AnimSelection"
    const f64 w0 = LoadF(0x004B4EC0);
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_FindChildren, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_NextSibling_004F9078, {n, kTag}))) {
        s32 idx;
        const u32 e = PoolRec(pool, 8, idx);
        if (e == 0) {
            ret(ctx, PoolFail(ctx, o, 0x00AEE890u));  // "Fail to allocate anim clip element ..."
            return;
        }
        store<u8>(e + 4, 0);
        for (u32 k = 8; k <= 24; k += 4) store<u32>(e + k, 0);
        store<u32>(e + 28, 0x34212D3Cu);
        if (SelHead(ctx, e, n, 0x00AEE828u)) {  // "Null clip name for animation selection element ..."
            callf(ctx, xfn::XmlCursor_ReadF32, {n, 0x04DA9951u, e + 24}, {w0});
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x10D018A4u, e + 28, 0x34212D3Cu});
            call(ctx, xfn::XmlCursor_ReadBool, {n, 0x0023BEA4u, e + 32, 0});
        }
        SelTally(o, idx);
    }
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x0027873Au}));
    if (s == 0) {
        // r4 is left over from the call above (the format takes no argument).
        call(ctx, m1fn::String_Format, {0x00AEE8D0u, ctx.gpr[4]});  // "...: Null anim control value type pass in!"
        const u32 b = load<u8>(o + 20) | 0x80u;
        store<u8>(o + 20, static_cast<u8>(b));
        ret(ctx, b);
        return;
    }
    CtlType(ctx, o, s);
}

// u32 AnimBlend_Read4B6120(AnimClip* o, XmlNode* c, Pool* pool)
// As AnimSelection_Read4B4EC8 for the "AnimBlend" children: kind-10
// records (when named: f32 +24 (0x95FB67C0, default 0x004B6118), hash +28
// (0x044DDD31)); after the control value type a bool (0xA7795607) into
// byte o+66 (also when the type attribute is missing). Returns its result
// (byte o+20 when the pool is exhausted).
DECOMP(004B6120, AnimBlend_Read4B6120) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), pool = arg<u32>(ctx, 2);
    call(ctx, AnimClip_ReadBase, {o, c, pool, ctx.gpr[6]});
    constexpr u32 kTag = 0x00AEE368;  // "AnimBlend"
    const f64 w0 = LoadF(0x004B6118);
    for (u32 n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_FindChildren, {c, kTag})); n != 0;
         n = static_cast<u32>(call(ctx, mid2::m2fn::Xml_NextSibling_004F9078, {n, kTag}))) {
        s32 idx;
        const u32 e = PoolRec(pool, 10, idx);
        if (e == 0) {
            ret(ctx, PoolFail(ctx, o, 0x00AEEA38u));  // "Fail to allocate anim clip element ..."
            return;
        }
        store<u8>(e + 4, 0);
        for (u32 k = 8; k <= 28; k += 4) store<u32>(e + k, 0);
        if (SelHead(ctx, e, n, 0x00AEE9F4u)) {  // "Null clip name for animation blend element ..."
            callf(ctx, xfn::XmlCursor_ReadF32, {n, 0x95FB67C0u, e + 24}, {w0});
            call(ctx, xfn::XmlCursor_ReadHash, {n, 0x044DDD31u, e + 28, 0});
        }
        SelTally(o, idx);
    }
    const u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x0027873Au}));
    if (s == 0) {
        // r4 is left over from the call above (the format takes no argument).
        call(ctx, m1fn::String_Format, {0x00AEEA78u, ctx.gpr[4]});  // "...: Null anim control value type pass in!"
        store<u8>(o + 20, static_cast<u8>(load<u8>(o + 20) | 0x80u));
    } else {
        CtlType(ctx, o, s);
    }
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xA7795607u, o + 66, 0});
}

namespace {
// Byte o+20 with bit m set from the low bit of b.
void Bit20(u32 o, u32 m, u32 b) {
    const u32 k = std::countr_zero(m);
    store<u8>(o + 20, static_cast<u8>((load<u8>(o + 20) & ~m) | ((b << k) & m)));
}

// Bit 0x80 of byte o+20 set (the readers' failure mark); returns the byte.
u32 Fail20(PPUContext& ctx, u32 o) {
    const u32 b = load<u8>(o + 20) | 0x80u;
    store<u8>(o + 20, static_cast<u8>(b));
    ret(ctx, b);
    return b;
}
}  // namespace

// u32 AnimClip_ReadBase(AnimClip* o, XmlNode* c, a2, a3)
// The anim clip node head: o->4 = the name (attribute 0x0024728B) interned
// by StrBuf_Append on the game's buffer, o->8 its no-case hash; hashes
// 0x03D3B158 / 0xA9D45179 into o+12 / +16; bools 0x0023BEA4, 0x57BAF55F,
// 0xC8FE1FDD and "ScaleExtMotion" into bits 0x40 / 0x04 / 0x20 / 0x08 of
// byte o+20; floats 0x28FC6916 / 0x04B70827 / 0x273C260F into o+24 / +28 /
// +32 (defaults the pool 0x004B31CC); the play node name (0x9EAD6F36)
// interned into o+44 and hashed into +48; u16 o+58 = the index of hash
// 0x9AFDD455 in the game's table (+272 {u32, u32 hash}[], +276 count) or
// -1; byte o+21 = 1 / 2 for the hash 0xDB8BAEEB = 0x85D98325 / 0x0026E201
// (else 0); the transition name (0xCA170197) hashed into o+40 (0 when
// missing) and interned into o+36. Then the "AnimEvent" children are read
// by Xml_AllChildren_004F4494(c, "AnimEvent", 0x00F39F98, &{o, a2, a3}, 0),
// whose result is returned. A missing name, or a full string buffer (with
// a message), sets bit 0x80 of byte o+20 and returns that byte.
DECOMP(004B31D8, AnimClip_ReadBase4B31D8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3);
    Stack frame(ctx, 32);
    const u32 sp = frame.addr();  // the original's sp + 112
    u32 s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x0024728Bu}));
    store<u32>(o + 4, s);
    if (s == 0) {
        Fail20(ctx, o);
        return;
    }
    u32 t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), s}));
    store<u32>(o + 4, t);
    if (t == 0) {
        call(ctx, m1fn::String_Format, {0x00AEE608u, t});  // "Fail to add the clip name (%s) ..."
        Fail20(ctx, o);
        return;
    }
    store<u32>(o + 8, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})));
    constexpr u32 kPool = 0x004B31CC;
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x03D3B158u, o + 12, 0});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xA9D45179u, o + 16, 0});
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x0023BEA4u, sp + 20, 0});
    Bit20(o, 0x40u, load<u8>(sp + 20));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0x57BAF55Fu, sp + 21, 0});
    Bit20(o, 0x04u, load<u8>(sp + 21));
    call(ctx, xfn::XmlCursor_ReadBool, {c, 0xC8FE1FDDu, sp + 22, 0});
    Bit20(o, 0x20u, load<u8>(sp + 22));
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x28FC6916u, o + 24}, {LoadF(kPool)});
    call(ctx, 0x004FA504, {c, 0x00AEE564u, sp + 23, 0});  // XmlCursor_ReadBoolByName("ScaleExtMotion")
    Bit20(o, 0x08u, load<u8>(sp + 23));
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x04B70827u, o + 28}, {LoadF(kPool + 4)});
    s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0x9EAD6F36u}));
    store<u32>(o + 44, s);
    if (s != 0) {
        t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), s}));
        store<u32>(o + 44, t);
        if (t == 0) {
            call(ctx, m1fn::String_Format, {0x00AEE648u, load<u32>(o + 4), t});  // "Run out anim clip string buffer ..."
            Fail20(ctx, o);
            return;
        }
        store<u32>(o + 48, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {t})));
    }
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0x9AFDD455u, sp, 0});
    {
        const u32 game = load<u32>(kGame), n = load<u32>(game + 276);
        u32 idx = 0xFFFFFFFFu;
        if (n != 0) {
            u32 p = load<u32>(game + 272), i = 0;
            do {
                if (load<u32>(p + 4) == load<u32>(sp)) {
                    idx = i;
                    break;
                }
                ++i;
                p += 8;
            } while (i < n);
        }
        store<u16>(o + 58, static_cast<u16>(idx));
    }
    callf(ctx, xfn::XmlCursor_ReadF32, {c, 0x273C260Fu, o + 32}, {LoadF(kPool + 8)});
    call(ctx, xfn::XmlCursor_ReadHash, {c, 0xDB8BAEEBu, sp + 4, 0});
    {
        const s32 v = load<s32>(sp + 4);
        u32 k = 0;
        if (v > static_cast<s32>(0x85D98325u)) k = v == 0x0026E201 ? 2 : 0;
        else if (v > static_cast<s32>(0x85D98324u)) k = 1;
        store<u8>(o + 21, static_cast<u8>(k));
    }
    s = static_cast<u32>(call(ctx, xfn::XmlCursor_FindAttr, {c, 0xCA170197u}));
    store<u32>(o + 36, s);
    if (s != 0) {
        store<u32>(o + 40, static_cast<u32>(call(ctx, m1fn::String_HashNoCase, {s})));
        t = static_cast<u32>(call(ctx, StrBuf_Append, {load<u32>(kGame), load<u32>(o + 36)}));
        store<u32>(o + 36, t);
        if (t == 0) {
            call(ctx, m1fn::String_Format, {0x00AEE698u, t});  // "Run out of string buffer for transition name %s"
            Fail20(ctx, o);
            return;
        }
    } else {
        store<u32>(o + 40, 0);
    }
    store<u32>(sp + 8, o);
    store<u32>(sp + 12, a2);
    store<u32>(sp + 16, a3);
    call(ctx, 0x004F4494, {c, 0x00AEE320u, 0x00F39F98u, sp + 8, 0});  // Xml_AllChildren_004F4494("AnimEvent")
}

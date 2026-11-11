// Game code, large-function sweep part 16: the XML def factory (one def
// from an XML node, dispatched on the node's tag hash).
#include "big16.h"

using namespace sly4;
using namespace sly4::big16;

namespace {
namespace hca0 {
namespace m1 = mid1::m1fn;
constexpr u32 kPool = 0x000CA018;        // f32 {-1, 0, 100, 0.3, 1, 10, 0.5}
constexpr u32 kDefTable = 0x01013B04;    // the def table (by name hash)
constexpr u32 sName = 0x00AC689C;        // "name"
constexpr u32 sFrom = 0x00AC68B4;        // "from"
constexpr u32 sObjDefName = 0x00AC68C4;  // "objDefName"
constexpr u32 sObjInstName = 0x00AC68D4; // "objInstName"
constexpr u32 String_HashNoCase = 0x0056A710;     // (s) -> u32
constexpr u32 Table_Find74493C = 0x0074493C;      // (table, const u32* key) -> slot (*slot = def or 0)
constexpr u32 Hash_ToString = 0x0056A6E0;         // (hash) -> char*
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;  // (c, tag) -> child or 0
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8; // (c, tag) -> sibling or 0
constexpr u32 XmlNode_ReadTree = 0x000C9AB8;      // (a0, node, PtrVector** out)
constexpr u32 Vector_Add733964 = 0x00733964;      // (vec, const void* 192-byte element)
constexpr u32 DamageDef_Construct = 0x00745D88;   // (rec, a, b, c, ...) the damage def defaults record
constexpr u32 Def_Construct74596C = 0x0074596C;   // (rec, a, b, c, w144, ...) the leaf def defaults record
constexpr u32 DamageDef_Ctor_000C79C0 = 0x000C79C0;  // (Def* p 304 bytes, const Record* rec)

struct Factory {
    u32 tag, fn;
};
// The tags whose def comes from a factory fn(a0, c, x, from) -> Def*.
constexpr Factory kFactories[] = {
    {0x00227313u, 0x001F9BA0},
    {0x002678F2u, 0x001BB6C4},
    {0x0026B78Du, 0x001E6B18},
    {0x0353CB09u, 0x0011E2B8},
    {0x03D1DEA1u, 0x001A7AB0},
    {0x044344C9u, 0x001DBE80},
    {0x04B6FABDu, 0x00167080},
    {0x0E5B671Du, 0x00203BE4},
    {0x109C1EFAu, 0x000C5DC8},
    {0x184A4683u, 0x00212DC8},
    {0x1B733030u, 0x000C1780},
    {0x201FA54Fu, 0x001AA158},
    {0x2190D8D7u, 0x001C28E8},
    {0x21F55D9Cu, 0x000BB4B0},
    {0x24A57117u, 0x001C0658},
    {0x35DF6E97u, 0x001FE170},
    {0x3F0EA991u, 0x00176CC0},
    {0x4F590DD4u, 0x001C8780},
    {0x554FC0D9u, 0x001BD668},
    {0x55B67D7Cu, 0x001ECD08},
    {0x5697EEFBu, 0x002217E0},
    {0x57CCFDA0u, 0x00223F94},
    {0x5EFDA0B2u, 0x0022F630},
    {0x633A7385u, 0x001D3E58},
    {0x6927A078u, 0x001F5BA8},
    {0x6E2FB705u, 0x002203F0},
    {0x6F429D34u, 0x001EA3B8},
    {0x70542137u, 0x001D71D8},
    {0x707C673Cu, 0x002221D8},
    {0x71FE40F4u, 0x0017D640},
    {0x75F206BDu, 0x00214E1C},
    {0x77A6CF07u, 0x0021A310},
    {0x786E8593u, 0x001B4394},
    {0x7CDC084Cu, 0x001AD7F0},
    {0x7F6CE46Au, 0x0021E23C},
    {0x871F883Au, 0x0020A298},
    {0x8CD0E8E8u, 0x001B24D0},
    {0x906A55EAu, 0x001B0120},
    {0x942B64F0u, 0x001CA7C0},
    {0x9F7993AFu, 0x0021D3CC},
    {0xA09DEEA4u, 0x002126B4},
    {0xA0BCC96Fu, 0x002198A8},
    {0xA0C4F7BCu, 0x0021B898},
    {0xA273E6E4u, 0x00245A30},
    {0xA5900924u, 0x002251DC},
    {0xA7CFB837u, 0x0016ADC4},
    {0xA8F71270u, 0x0023FA28},
    {0xAEF4C388u, 0x001A5AD8},
    {0xB2221492u, 0x001AB548},
    {0xB266AD6Cu, 0x00307A78},
    {0xB3748711u, 0x001DE610},
    {0xB47D3A95u, 0x001AC858},
    {0xB9F78EFDu, 0x000C63B0},
    {0xBC13B5BCu, 0x000C58A8},
    {0xBC6FA23Bu, 0x001C5408},
    {0xC475D118u, 0x001EB2B8},
    {0xCCFB1F40u, 0x0018F8E8},
    {0xDCC028E7u, 0x002210C8},
    {0xDF511592u, 0x001BED98},
    {0xE65DDB0Fu, 0x002102BC},
    {0xEB9EE198u, 0x0020C164},
    {0xEBE54755u, 0x000C77E4},
    {0xEE9D8477u, 0x001D0208},
    {0xF004220Bu, 0x0001BFE0},
    {0xF8ADA400u, 0x002F8EE0},
    {0xFDC69C0Fu, 0x002183FC},
};

// The record's PtrVector at slot (a pointer to it) destroyed when set
// (PtrVector_Clear, Nop, SmallFree(.., 20)).
inline void FreeVec(PPUContext& ctx, u32 slot) {
    const u32 v = load<u32>(slot);
    if (v == 0) return;
    call(ctx, m1::PtrVector_Clear, {v});
    call(ctx, m1::Nop, {v});
    call(ctx, m1::SmallFree, {load<u32>(slot), 20});
}
}  // namespace hca0
}  // namespace

// Def* XmlDef_NewFromNodeCA038(a0, XmlCursor* c, u8 flag)
// One def from the XML node c: its "name" attribute (missing, or a name
// already in the def table 0x01013B04: XmlCursor_DocPath(a0), 0) heads the
// on-stack header x (+12 the name, +16 / +20 the "objDefName" /
// "objInstName" attributes, default the parent's (or 0x00AC6790), bools
// +24..+31, f32s +36 / +40 (default the parent's +36 / +40, else -1), +44
// the flag byte, hash +48, s32 +52, hash +56, f32s +60 / +72 (0 / -1),
// +64 / +68 the child vectors when non-empty, +32 0). The "from"
// attribute names the parent def (looked up by name hash; not found:
// XmlCursor_DocPath(a0), 0), whose vfunc 3 on +4 / +8 gives the default
// names and whose +32 flag bits / +36 / +40 / +120 / +124 the defaults of
// x. The hashes 0x0024728B of the 0x52A72CDD children of the 0x023E6F16
// child are pushed onto a 20-byte PtrVector (capacity 4) whose address goes
// to x +64 when not empty; the 0xCDA892E2 child's tree (XmlNode_ReadTree)
// goes to x +68 when not empty. The node's tag then selects the def:
// one of 66 factories fn(a0, c, x, parent); 0x93DEF0C4 an inline damage
// def (DamageDef_Construct, DamageColDef_Read_000C85A8, a 304-byte def
// built by 000C79C0); 0xA77EB5E9 an inline leaf def (Def_Construct74596C,
// DefLeaf_ReadBase_000C4020, a 416-byte def built by
// DefLeaf_Construct_000C30E8); 0x03D3E53D none; any other tag notes c's
// document path and Hash_ToString(tag), no def. A def gets the 192-byte
// elements of the tree vector added to its PtrVector +136 (created with
// capacity 192 when unset). Both vectors are destroyed; returns the def (0
// when none).
DECOMP(000CA038, XmlDef_NewFromNodeCA038) {
    using namespace b16fn;
    using namespace hca0;
    const u32 a0 = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    const u64 flag = ctx.gpr[5];
    Stack frame(ctx, 1296);
    const u32 sp = frame.addr();
    const u32 rec = sp + 416, leaf = sp + 640, kName = sp + 992, vKids = sp + 996, vTree = sp + 1000, kHash = sp + 1004,
              kFrom = sp + 1008, x = sp + 1012;
    u32 def = 0;
    const u32 tag = (load<u8>(c + 24) & 0x80u) ? load<u32>(c + 20) : 0u;
    const u32 name = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sName}));
    if (name == 0) {
        call(ctx, XmlCursor_DocPath, {a0});
        ret(ctx, 0u);
        return;
    }
    store<u32>(kName, static_cast<u32>(call(ctx, String_HashNoCase, {name})));
    if (load<u32>(static_cast<u32>(call(ctx, Table_Find74493C, {kDefTable, kName}))) != 0) {
        call(ctx, XmlCursor_DocPath, {a0});
        ret(ctx, 0u);
        return;
    }
    const f64 kM1 = LfsX(kPool), k0 = LfsX(kPool + 4);
    store<u32>(x + 12, name);
    u32 defName = mid11::kStrAC6790, instName = mid11::kStrAC6790;
    store<u32>(x + 16, defName);
    store<u32>(x + 20, instName);
    for (u32 k = 0; k < 8; ++k) store<u8>(x + 24 + k, 0);
    store<u32>(x + 32, 0);
    store<u8>(x + 44, 0);
    store<u32>(x + 48, 0);
    store<u32>(x + 36, 0xBF800000u);
    store<u32>(x + 40, 0xBF800000u);
    for (u32 k = 52; k <= 68; k += 4) store<u32>(x + k, 0);
    store<u32>(x + 72, 0xBF800000u);
    for (u32 k = 0; k < 3; ++k) store<u32>(x + 4 * k, 0);
    store<u64>(sp + 1096, mid11::kStrAC6790);
    store<u64>(sp + 1104, 0);
    store<u64>(sp + 1088, flag);
    u32 parent = 0, s120 = 0, s124 = 0;
    u32 b[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    f64 f36 = kM1, f40 = kM1;
    const u32 from = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sFrom}));
    if (from != 0 && load<u8>(from) != 0) {
        const u32 h = static_cast<u32>(call(ctx, String_HashNoCase, {from}));
        store<u32>(kFrom, h);
        if (h != 0) parent = load<u32>(static_cast<u32>(call(ctx, Table_Find74493C, {kDefTable, kFrom})));
        if (parent == 0) {
            call(ctx, XmlCursor_DocPath, {a0});
            ret(ctx, 0u);
            return;
        }
        if (load<u32>(parent + 4) != 0) defName = static_cast<u32>(obj19::VCall(ctx, load<u32>(parent + 4), 12, {load<u32>(parent + 4)}));
        if (load<u32>(parent + 8) != 0) instName = static_cast<u32>(obj19::VCall(ctx, load<u32>(parent + 8), 12, {load<u32>(parent + 8)}));
        const u32 w = load<u32>(parent + 32);
        for (u32 k = 0; k < 8; ++k) b[k] = Bit(w, k);
        f36 = LfsX(parent + 36);
        f40 = LfsX(parent + 40);
        s120 = load<u32>(parent + 120);
        s124 = load<u32>(parent + 124);
    }
    store<u8>(x + 44, static_cast<u8>(load<u64>(sp + 1088)));
    u32 s = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sObjDefName}));
    store<u32>(x + 16, s != 0 ? s : defName);
    s = static_cast<u32>(call(ctx, XmlCursor_FindAttrByName, {c, sObjInstName}));
    store<u32>(x + 20, s != 0 ? s : instName);
    constexpr u32 kBools[8] = {0xCA8DBA6Bu, 0xA8C8CC1Cu, 0x71362F02u, 0x6611E802u, 0x3395D35Bu, 0x1E0C8C0Cu, 0xE90F4FE5u, 0xC8CB4F86u};
    for (u32 k = 0; k < 8; ++k) call(ctx, XmlCursor_ReadBool, {c, kBools[k], x + 24 + k, b[k]});
    callf(ctx, XmlCursor_ReadF32, {c, 0x465FBA18u, x + 36}, {f36});
    callf(ctx, XmlCursor_ReadF32, {c, 0x01B6B516u, x + 40}, {f40});
    call(ctx, XmlCursor_ReadHash, {c, 0x161EABC1u, x + 48, 0});
    call(ctx, XmlCursor_ReadS32, {c, 0xFB89446Eu, x + 52, Sx64(s120)});
    call(ctx, XmlCursor_ReadHash, {c, 0x46F408DBu, x + 56, s124});
    callf(ctx, XmlCursor_ReadF32, {c, 0xDD1DC180u, x + 60}, {k0});
    callf(ctx, XmlCursor_ReadF32, {c, 0xE2EA9FD0u, x + 72}, {kM1});
    store<u32>(vKids, 0);
    u32 ch = static_cast<u32>(call(ctx, XmlCursor_FirstChild, {c, 0x023E6F16u}));
    if (ch != 0) {
        for (u32 n = static_cast<u32>(call(ctx, XmlCursor_FirstChild, {ch, 0x52A72CDDu})); n != 0;
             n = static_cast<u32>(call(ctx, XmlCursor_NextSibling, {n, 0x52A72CDDu}))) {
            call(ctx, XmlCursor_ReadHash, {n, 0x0024728Bu, kHash, 0});
            u32 v = load<u32>(vKids);
            if (v == 0) {
                v = static_cast<u32>(call(ctx, m1::SmallAlloc, {20}));
                store<u32>(vKids, v);
                if (v != 0) {
                    call(ctx, m1::PtrVector_Construct, {v});
                    store<u16>(v + 6, 4);
                    v = load<u32>(vKids);
                }
                // Quirk: a failed allocation is pushed onto anyway (v = 0).
            }
            big15::PushFrom(ctx, v, kHash);
        }
    }
    {
        const u32 v = load<u32>(vKids);
        store<u32>(x + 64, (v != 0 && load<u16>(v + 8) != 0) ? vKids : 0u);
    }
    store<u32>(vTree, 0);
    ch = static_cast<u32>(call(ctx, XmlCursor_FirstChild, {c, 0xCDA892E2u}));
    if (ch != 0) call(ctx, XmlNode_ReadTree, {a0, ch, vTree});
    {
        const u32 v = load<u32>(vTree);
        if (v != 0 && load<u16>(v + 8) != 0) store<u32>(x + 68, vTree);
    }
    bool made = false;
    if (tag == 0x93DEF0C4u) {
        const u64 w1 = load<u32>(kPool + 16), wm1 = load<u32>(kPool);
        const f64 k100 = LfsX(kPool + 8), k03 = LfsX(kPool + 12), k1 = LfsX(kPool + 16);
        const u32 nm = load<u32>(x + 12), dn = load<u32>(x + 16), in = load<u32>(x + 20);
        callf(ctx, DamageDef_Construct,
              {rec, nm, dn, in, 1, in, 0x9260C26Eu, kVecZero,
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // slots 112..192 (not set by the original)
               w1, w1, w1, w1,                   // s204..s228
               0, 0, 0, 0, 0, 0, 0, 0,           // s232..s288
               1,                                // s300
               0, 0, 0, 0,                       // s304..s328
               wm1, wm1, 0},                     // s340, s348, s352
              {k100, kM1, k1, k1, k1, k1, k0, k03, kM1, k1, k1, k1, k1});
        call(ctx, DamageColDef_Read_000C85A8, {a0, c, x, parent, rec});
        const u32 p = static_cast<u32>(call(ctx, m1::Mem_Alloc, {304, 0, 16}));
        if (p != 0) {
            call(ctx, DamageDef_Ctor_000C79C0, {p, rec});
            def = p;
        }
        DmgDestroy(ctx, rec);
        made = true;
    } else if (tag == 0xA77EB5E9u) {
        const u64 w0 = load<u32>(kPool), w4 = load<u32>(kPool + 4), w20 = load<u32>(kPool + 20), w24 = load<u32>(kPool + 24);
        const f64 k100 = LfsX(kPool + 8), k1 = LfsX(kPool + 16);
        callf(ctx, Def_Construct74596C,
              {leaf, load<u32>(x + 12), load<u32>(x + 16), load<u32>(x + 20), load<u64>(sp + 1096), 1, mid11::kVec2_4AFC, kVecZero,
               0, 0, 0, 0, 0,                        // slots 112..144
               0, 0, 1,                              // s156, s164, s172
               0, 0, 0, 0,                           // slots 176..200
               kVecZero, 0,                          // s212, slot 216
               w0, mid11::kVec2_4AFC,                // s228, s236
               w20, w20, w24, w4, w0, w0, w0,        // s244..s292
               0, 0x7641A763u,                       // s300, s308
               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // s316..s412
              {kM1, kM1, kM1, kM1, kM1, k100, k1, kM1, k1, k1, k1, kM1, k0});
        call(ctx, mid11::m11fn::DefLeaf_ReadBase_000C4020, {a0, c, x, parent, leaf});
        const u32 p = static_cast<u32>(call(ctx, m1::Mem_Alloc, {416, 0, 16}));
        if (p != 0) {
            call(ctx, mid11::m11fn::DefLeaf_Construct_000C30E8, {p, leaf});
            def = p;
        }
        made = true;
    } else if (tag != 0x03D3E53Du) {
        for (const Factory& f : kFactories)
            if (f.tag == tag) {
                def = static_cast<u32>(call(ctx, f.fn, {a0, c, x, parent}));
                made = true;
                break;
            }
        if (!made) {
            call(ctx, XmlCursor_DocPath, {a0});
            call(ctx, Hash_ToString, {(load<u8>(c + 24) & 0x80u) ? load<u32>(c + 20) : 0u});
        }
    }
    if (made && def != 0 && load<u32>(x + 68) != 0) {
        for (u32 i = 0, off = 0;; ++i, off += 192) {
            const u32 v = load<u32>(load<u32>(x + 68));
            const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
            if (i >= n) break;
            const u32 e = load<u32>(v) + off;
            u32 vec = load<u32>(def + 136);
            if (vec == 0) {
                vec = static_cast<u32>(call(ctx, m1::SmallAlloc, {20}));
                store<u32>(def + 136, vec);
                if (vec != 0) {
                    call(ctx, m1::PtrVector_Construct, {vec});
                    store<u16>(vec + 6, 192);
                    vec = load<u32>(def + 136);
                }
            }
            call(ctx, Vector_Add733964, {vec, e});
        }
    }
    FreeVec(ctx, vTree);
    FreeVec(ctx, vKids);
    ret(ctx, def);
}

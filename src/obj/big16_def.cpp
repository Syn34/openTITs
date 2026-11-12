// Game code, large-function sweep part 16: the XML def factory (one def
// from an XML node, dispatched on the node's tag hash).
#include "big16.h"
#include "big18.h"

namespace sly4::obj {

namespace {
constexpr u32 kPoolCA0 = 0x000CA018;        // f32 {-1, 0, 100, 0.3, 1, 10, 0.5}
constexpr u32 kDefTableCA0 = 0x01013B04;    // the def table (by name hash)
constexpr u32 sName = 0x00AC689C;        // "name"
constexpr u32 sFrom = 0x00AC68B4;        // "from"
constexpr u32 sObjDefName = 0x00AC68C4;  // "objDefName"
constexpr u32 sObjInstName = 0x00AC68D4; // "objInstName"
constexpr u32 String_HashNoCaseCA0 = 0x0056A710;  // (s) -> u32
constexpr u32 Table_Find74493C = 0x0074493C;      // (table, const u32* key) -> slot (*slot = def or 0)
constexpr u32 Hash_ToStringCA0 = 0x0056A6E0;      // (hash) -> char*
constexpr u32 XmlCursor_FirstChild = 0x004F92BC;  // (c, tag) -> child or 0
constexpr u32 XmlCursor_NextSibling = 0x004F8FC8; // (c, tag) -> sibling or 0
constexpr u32 XmlNode_ReadTree = 0x000C9AB8;      // (a0, node, PtrVector** out)
constexpr u32 Vector_Add733964 = 0x00733964;      // (vec, const void* 192-byte element)
constexpr u32 DamageDef_Construct = 0x00745D88;   // (rec, a, b, c, ...) the damage def defaults record
constexpr u32 Def_Construct74596C = 0x0074596C;   // (rec, a, b, c, w144, ...) the leaf def defaults record
constexpr u32 DefLeaf_ReadBase_000C4020 = 0x000C4020;  // (a0, c, x, tmpl, rec)
constexpr u32 kStrAC6790 = 0x00AC6790;   // the leaf def name (three string arguments)
constexpr u32 kVec2_4AFCa = 0x01164AFC;  // f32[2]

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
inline void FreeVec(be_u32* slot) {
    const u32 v = *slot;
    if (v == 0) return;
    call(fn::PtrVector_Clear, v);
    call(fn::Nop54EC18, v);
    call(fn::SmallFree, slot->get(), 20u);
}
}  // namespace

// void XmlDef_NewFromNodeCA038(a0, XmlCursor* c, u8 flag) (the caller
// does not read r3): one def from the XML node c: its "name" attribute
// (missing, or a name already in the def table 0x01013B04:
// XmlCursor_DocPath(a0), no def) heads the on-stack header x (+12 the name,
// +16 / +20 the "objDefName" / "objInstName" attributes, default the
// parent's (or 0x00AC6790), bools +24..+31, f32s +36 / +40 (default the
// parent's +36 / +40, else -1), +44 the flag byte, hash +48, s32 +52, hash
// +56, f32s +60 / +72 (0 / -1), +64 / +68 the child vectors when
// non-empty, +32 0). The "from" attribute names the parent def (looked up
// by name hash; not found: XmlCursor_DocPath(a0), no def), whose vfunc 3
// on +4 / +8 gives the default names and whose +32 flag bits / +36 / +40 /
// +120 / +124 the defaults of x. The hashes 0x0024728B of the 0x52A72CDD
// children of the 0x023E6F16 child are pushed onto a 20-byte PtrVector
// (capacity 4) whose address goes to x +64 when not empty; the 0xCDA892E2
// child's tree (XmlNode_ReadTree) goes to x +68 when not empty. The node's
// tag then selects the def: one of 66 factories fn(a0, c, x, parent);
// 0x93DEF0C4 an inline damage def (DamageDef_Construct,
// DamageColDef_Read_000C85A8, a 304-byte def built by 000C79C0);
// 0xA77EB5E9 an inline leaf def (Def_Construct74596C,
// DefLeaf_ReadBase_000C4020, a 416-byte def built by
// DefLeaf_Construct_000C30E8); 0x03D3E53D none; any other tag notes c's
// document path and Hash_ToString(tag), no def. A def gets the 192-byte
// elements of the tree vector added to its PtrVector +136 (created with
// capacity 192 when unset). Both vectors are destroyed. The floats of the
// two defaults-record constructors sit in stack argument slots the
// original leaves 0.
void XmlDef_NewFromNodeCA038(u32 a0, u32 c, u8 flag) {
    GuestFrame frame(1296);
    const u32 sp = frame.addr();
    const u32 rec = sp + 416, leaf = sp + 640, x = sp + 1012;
    be_u32* kName = frame.as<be_u32>(992);
    be_u32* vKids = frame.as<be_u32>(996);
    be_u32* vTree = frame.as<be_u32>(1000);
    be_u32* kHash = frame.as<be_u32>(1004);
    be_u32* kFrom = frame.as<be_u32>(1008);
    auto W = [](u32 base, u32 off) -> be_u32& { return gm<be_u32>(base + off); };
    auto B = [](u32 base, u32 off) -> u8& { return gm<u8>(base + off); };
    auto F = [](u32 ea) { return LfsW(gm<be_u32>(ea)); };
    u32 def = 0;
    const u32 tag = (B(c, 24) & 0x80u) ? W(c, 20).get() : 0u;
    const u32 name = call<u32>(fn::XmlCursor_FindAttrByName, c, sName);
    if (name == 0) {
        call(fn::XmlCursor_DocPath, a0);
        return;
    }
    *kName = call<u32>(String_HashNoCaseCA0, name);
    if (gm<be_u32>(call<u32>(Table_Find74493C, kDefTableCA0, kName)) != 0u) {
        call(fn::XmlCursor_DocPath, a0);
        return;
    }
    const f64 kM1 = F(kPoolCA0), k0 = F(kPoolCA0 + 4);
    W(x, 12) = name;
    u32 defName = kStrAC6790, instName = kStrAC6790;
    W(x, 16) = defName;
    W(x, 20) = instName;
    for (u32 k = 0; k < 8; ++k) B(x, 24 + k) = 0;
    W(x, 32) = 0u;
    B(x, 44) = 0;
    W(x, 48) = 0u;
    W(x, 36) = 0xBF800000u;
    W(x, 40) = 0xBF800000u;
    for (u32 k = 52; k <= 68; k += 4) W(x, k) = 0u;
    W(x, 72) = 0xBF800000u;
    for (u32 k = 0; k < 3; ++k) W(x, 4 * k) = 0u;
    u32 parent = 0, s120 = 0, s124 = 0;
    u32 b[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    f64 f36 = kM1, f40 = kM1;
    const u32 from = call<u32>(fn::XmlCursor_FindAttrByName, c, sFrom);
    if (from != 0 && gm<u8>(from) != 0) {
        const u32 h = call<u32>(String_HashNoCaseCA0, from);
        *kFrom = h;
        if (h != 0) parent = gm<be_u32>(call<u32>(Table_Find74493C, kDefTableCA0, kFrom));
        if (parent == 0) {
            call(fn::XmlCursor_DocPath, a0);
            return;
        }
        if (const u32 o4 = W(parent, 4); o4 != 0) defName = vcall<u32>(host<u8>(o4), 12);
        if (const u32 o8 = W(parent, 8); o8 != 0) instName = vcall<u32>(host<u8>(o8), 12);
        const u32 w = W(parent, 32);
        for (u32 k = 0; k < 8; ++k) b[k] = (w >> (31 - k)) & 1u;
        f36 = F(parent + 36);
        f40 = F(parent + 40);
        s120 = W(parent, 120);
        s124 = W(parent, 124);
    }
    B(x, 44) = flag;
    u32 s = call<u32>(fn::XmlCursor_FindAttrByName, c, sObjDefName);
    W(x, 16) = s != 0 ? s : defName;
    s = call<u32>(fn::XmlCursor_FindAttrByName, c, sObjInstName);
    W(x, 20) = s != 0 ? s : instName;
    constexpr u32 kBools[8] = {0xCA8DBA6Bu, 0xA8C8CC1Cu, 0x71362F02u, 0x6611E802u, 0x3395D35Bu, 0x1E0C8C0Cu, 0xE90F4FE5u, 0xC8CB4F86u};
    for (u32 k = 0; k < 8; ++k) call(fn::XmlCursor_ReadBool, c, kBools[k], x + 24 + k, b[k]);
    call(fn::XmlCursor_ReadF32, c, 0x465FBA18u, x + 36, f36);
    call(fn::XmlCursor_ReadF32, c, 0x01B6B516u, x + 40, f40);
    call(fn::XmlCursor_ReadHash, c, 0x161EABC1u, x + 48, 0u);
    call(fn::XmlCursor_ReadS32, c, 0xFB89446Eu, x + 52, static_cast<s32>(s120));
    call(fn::XmlCursor_ReadHash, c, 0x46F408DBu, x + 56, s124);
    call(fn::XmlCursor_ReadF32, c, 0xDD1DC180u, x + 60, k0);
    call(fn::XmlCursor_ReadF32, c, 0xE2EA9FD0u, x + 72, kM1);
    *vKids = 0u;
    u32 ch = call<u32>(XmlCursor_FirstChild, c, 0x023E6F16u);
    if (ch != 0) {
        for (u32 n = call<u32>(XmlCursor_FirstChild, ch, 0x52A72CDDu); n != 0; n = call<u32>(XmlCursor_NextSibling, n, 0x52A72CDDu)) {
            call(fn::XmlCursor_ReadHash, n, 0x0024728Bu, kHash, 0u);
            u32 v = *vKids;
            if (v == 0) {
                v = call<u32>(fn::SmallAlloc, 20u);
                *vKids = v;
                if (v != 0) {
                    call(fn::PtrVector_Construct, v);
                    gm<be_u16>(v + 6) = 4;
                    v = *vKids;
                }
                // Quirk: a failed allocation is pushed onto anyway (v = 0).
            }
            PtrVecPush(gm<PtrVec<u8>>(v), *kHash);
        }
    }
    {
        const u32 v = *vKids;
        W(x, 64) = (v != 0 && gm<PtrVec<u8>>(v).count != 0) ? guest(vKids) : 0u;
    }
    *vTree = 0u;
    ch = call<u32>(XmlCursor_FirstChild, c, 0xCDA892E2u);
    if (ch != 0) call(XmlNode_ReadTree, a0, ch, vTree);
    {
        const u32 v = *vTree;
        if (v != 0 && gm<PtrVec<u8>>(v).count != 0) W(x, 68) = guest(vTree);
    }
    bool made = false;
    if (tag == 0x93DEF0C4u) {
        const u32 w1 = W(kPoolCA0, 16), wm1 = W(kPoolCA0, 0);
        const f64 k100 = F(kPoolCA0 + 8), k03 = F(kPoolCA0 + 12), k1 = F(kPoolCA0 + 16);
        const u32 nm = W(x, 12), dn = W(x, 16), in = W(x, 20);
        call(DamageDef_Construct, rec, nm, dn, in, 1u, in, 0x9260C26Eu, kZeroVec,
             k100, kM1, k1, k1, k1, k1, k0, k03, kM1, k1, k1,  // f1..f11 (slots 112..192)
             w1, w1, w1, w1,                                   // s204..s228
             k1, k1,                                           // f12, f13 (s232 / s240)
             0u, 0u, 0u, 0u, 0u, 0u,                           // s248..s288
             1u,                                               // s300
             0u, 0u, 0u, 0u,                                   // s304..s328
             wm1, wm1, 0u);                                    // s340, s348, s352
        call(fn::DamageColDef_Read_000C85A8, a0, c, x, parent, rec);
        DamageDef17* p = call<DamageDef17*>(fn::Mem_Alloc, 304u, 0u, 16u);
        if (p != nullptr) {
            DamageDef_Ctor_000C79C0(p, host<DamageRec17>(rec));
            def = guest(p);
        }
        DmgDef_Destroy(rec);
        made = true;
    } else if (tag == 0xA77EB5E9u) {
        const u32 w0 = W(kPoolCA0, 0), w4 = W(kPoolCA0, 4), w20 = W(kPoolCA0, 20), w24 = W(kPoolCA0, 24);
        const f64 k100 = F(kPoolCA0 + 8), k1 = F(kPoolCA0 + 16);
        call(Def_Construct74596C, leaf, W(x, 12).get(), W(x, 16).get(), W(x, 20).get(), kStrAC6790, 1u, kVec2_4AFCa, kZeroVec,
             kM1, kM1, kM1, kM1, kM1,               // f1..f5 (slots 112..144)
             0u, 0u, 1u,                            // s156, s164, s172
             k100, k1, kM1, k1,                     // f6..f9 (slots 176..200)
             kZeroVec,                              // s212
             k1,                                    // f10 (slot 216)
             w0, kVec2_4AFCa,                       // s228, s236
             w20, w20, w24, w4, w0, w0, w0,         // s244..s292
             0u, 0x7641A763u,                       // s300, s308
             k1, kM1, k0,                           // f11..f13 (s316..s332)
             0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);  // s340..s412
        call(DefLeaf_ReadBase_000C4020, a0, c, x, parent, leaf);
        LeafDef17* p = call<LeafDef17*>(fn::Mem_Alloc, 416u, 0u, 16u);
        if (p != nullptr) {
            DefLeaf_Construct_000C30E8(p, host<LeafRec17>(leaf));
            def = guest(p);
        }
        made = true;
    } else if (tag != 0x03D3E53Du) {
        for (const Factory& f : kFactories)
            if (f.tag == tag) {
                def = call<u32>(f.fn, a0, c, x, parent);
                made = true;
                break;
            }
        if (!made) {
            call(fn::XmlCursor_DocPath, a0);
            call(Hash_ToStringCA0, (B(c, 24) & 0x80u) ? W(c, 20).get() : 0u);
        }
    }
    if (made && def != 0 && W(x, 68) != 0u) {
        for (u32 i = 0, off = 0;; ++i, off += 192) {
            const u32 v = gm<be_u32>(W(x, 68));
            const u32 n = v != 0 ? gm<PtrVec<u8>>(v).count.get() : 0u;
            if (i >= n) break;
            const u32 e = gm<be_u32>(v) + off;
            u32 vec = W(def, 136);
            if (vec == 0) {
                vec = call<u32>(fn::SmallAlloc, 20u);
                W(def, 136) = vec;
                if (vec != 0) {
                    call(fn::PtrVector_Construct, vec);
                    gm<be_u16>(vec + 6) = 192;
                    vec = W(def, 136);
                }
            }
            call(Vector_Add733964, vec, e);
        }
    }
    FreeVec(vTree);
    FreeVec(vKids);
}

}  // namespace sly4::obj

NATIVE(000CA038, sly4::obj::XmlDef_NewFromNodeCA038)

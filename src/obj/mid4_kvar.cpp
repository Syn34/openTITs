// Game code, medium sweep part 4: the typed KVarData lookups of the
// action-script nodes (four instances: int / float / bool / hash).
//
// A node's variable reference a {+2 / +4 s16 record indices (-1: none),
// +6 s16 the index of its KVarData slot} is resolved against the script
// b {+144 u16 count, +146 u16 the current entry, +152 132-byte entries:
// +0 s32 (valid below 10), +4 s16 the type (0x00F65100 + 44 * type: +40
// -> the type's name), +6 u16 flags (0x8000: error)}. The variable's name
// comes from the node records (as obj20's TNodeParamByte: the byte at +0
// of record 2's sub-block, times 4, plus *(u16*)0x0106CF70, indexes
// record 1); its hash is looked up in the current entry's hash list (+0
// u16 count, the u32 hashes at +*(u16*)+2, the u32 type tags at
// +*(u16*)+4) and the index (-1: not found) stored to the KVarData slot
// (the entry + *(u16*)0x0106CF80 + 4). Errors are reported through
// String_Format ("%s: (Error!) %s" with the type's name) and flag the
// entry.
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

namespace {

namespace kfn {
constexpr u32 String_Format = 0x005534D0;  // (fmt, ...) -> rotating static buffer
constexpr u32 kTypeTable = 0x00F65100;     // 44 bytes per type, +40 -> const char** name
constexpr u32 kSlotOffset = 0x0106CF80;    // u16
constexpr u32 kSubOffset = 0x0106CF7E;     // u16
constexpr u32 kNameOffset = 0x0106CF70;    // u16
constexpr u32 kFmtError = 0x00ADD2EC;      // "%s: (Error!) %s"
constexpr u32 kFmtNotFound = 0x00ADDA24;   // "Cannot find KVarData with name (%s) to s.."
constexpr u32 kStrNullName = 0x00ADDA7C;   // "Null var data name pass in!"
constexpr u32 kStrNull = 0x00B1FBAC;       // "NULL"
}  // namespace kfn

// b's entry i when i is below its count and the entry's +0 below 10, else 0.
u32 KVarEntry(u32 b, s32 i) {
    if (!(i < static_cast<s32>(load<u16>(b + 144)))) return 0;
    const u32 e = b + 152 + static_cast<u32>(i) * 132;
    return static_cast<s32>(load<u32>(e)) < 10 ? e : 0;
}

// The error report for entry e: "%s: (Error!) %s" with the name of e's
// type ("NULL" without one) and msg; e's +6 |= 0x8000 (r3 = the new value).
void KVarError(PPUContext& ctx, u32 e, u32 msg) {
    const s32 type = static_cast<s16>(load<u16>(e + 4));
    u32 name = load<u32>(load<u32>(kfn::kTypeTable + static_cast<u32>(type) * 44 + 40));
    if (name == 0) name = kfn::kStrNull;
    call(ctx, kfn::String_Format, {kfn::kFmtError, name, msg});
    const u32 f = static_cast<u32>(load<u16>(e + 6)) | 0x8000u;
    store<u16>(e + 6, static_cast<u16>(f));
    ctx.gpr[3] = f;
}

// void f(VarRef* a, Script* b): the lookup expecting type tag `want`
// (else the message fmt_type with the name).
void KVarResolve(PPUContext& ctx, u32 want, u32 fmt_type) {
    const u32 a = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1);
    const u32 e1 = KVarEntry(b, static_cast<s16>(load<u16>(a + 6)));
    const u32 slot = e1 + load<u16>(kfn::kSlotOffset) + 4;
    const u32 tbl = load<u32>(mid4::nfn::kNodeTable);
    const s32 i1 = static_cast<s16>(load<u16>(a + 4));
    const u32 rec1 = i1 == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(i1) << 7);
    const s32 i2 = static_cast<s16>(load<u16>(a + 2));
    const u32 rec2 = i2 == -1 ? 0 : load<u32>(tbl + 24) + (static_cast<u32>(i2) << 7);
    u32 sub = 0;  // address 0 without a sub-block, as the original
    if (static_cast<s32>(load<u8>(rec2 + 2)) > 0) sub = rec2 + load<u16>(kfn::kSubOffset);
    const u32 name = load<u32>(rec1 + load<u16>(kfn::kNameOffset) + (static_cast<u32>(load<u8>(sub)) << 2));
    if (name == 0) {
        KVarError(ctx, e1, kfn::kStrNullName);
        store<u16>(slot, 0xFFFF);
        return;
    }
    const u32 cur = b + 152 + static_cast<u32>(load<u16>(b + 146)) * 132;
    const u32 h = static_cast<u32>(call(ctx, mid4::m4fn::String_HashNoCase, {name}));
    const s32 n = load<u16>(cur);
    s32 k = -1;
    if (n > 0) {
        // s16 index, as the original (a count above 0x7FFF never ends a miss)
        for (s32 j = 0;;) {
            if (h == load<u32>(cur + load<u16>(cur + 2) + ((static_cast<u32>(j) << 2) & 0x3FFFCu))) {
                k = j;
                break;
            }
            j = static_cast<s16>(j + 1);
            if (!(j < n)) break;
        }
    }
    store<u16>(slot, static_cast<u16>(k));
    if (k == -1) {
        const u32 e = KVarEntry(b, static_cast<s16>(load<u16>(a + 6)));
        const u32 s = static_cast<u32>(call(ctx, kfn::String_Format, {kfn::kFmtNotFound, name}));
        KVarError(ctx, e, s);
        return;
    }
    const u32 cur2 = b + 152 + static_cast<u32>(load<u16>(b + 146)) * 132;
    const u32 tag = load<u32>(cur2 + load<u16>(cur2 + 4) + ((static_cast<u32>(k) << 2) & 0x3FFFCu));
    ctx.gpr[3] = tag;
    if (tag == want) return;
    const u32 e = KVarEntry(b, static_cast<s16>(load<u16>(a + 6)));
    const u32 s = static_cast<u32>(call(ctx, kfn::String_Format, {fmt_type, name}));
    KVarError(ctx, e, s);
}

}  // namespace

// void f(VarRef* a, Script* b): resolve a's int KVarData (type tag 0):
// "The KVarData (%s) is not type int!".
DECOMP(00391B80, KVar_ResolveInt_00391B80) { KVarResolve(ctx, 0, 0x00ADDA54); }

// void f(VarRef* a, Script* b): the same for a float (type tag 2).
DECOMP(00392130, KVar_ResolveFloat_00392130) { KVarResolve(ctx, 2, 0x00ADDA98); }

// void f(VarRef* a, Script* b): the same for a bool (type tag 3).
DECOMP(003926E0, KVar_ResolveBool_003926E0) { KVarResolve(ctx, 3, 0x00ADDAC0); }

// void f(VarRef* a, Script* b): the same for a hash (type tag 5).
DECOMP(00392C90, KVar_ResolveHash_00392C90) { KVarResolve(ctx, 5, 0x00ADDAE4); }

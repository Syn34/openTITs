// The reference-counted base and the UiRes base class (vtables 0x00B225E0,
// 0x00B225F8).
#include "res.h"

using namespace sly4;
using namespace sly4::font;

namespace sly4 {
namespace font {

void Res_Unlink(PPUContext& ctx, u32 r) {
    u32 prev = 0;
    for (u32 it = load<u32>(kResList); it != 0; it = load<u32>(it + kResNext)) {
        if (it == r) {
            if (prev != 0)
                store<u32>(prev + kResNext, load<u32>(r + kResNext));
            else
                store<u32>(kResList, load<u32>(r + kResNext));
            break;
        }
        prev = it;
    }
    ctx.gpr[4] = prev;
}

}  // namespace font
}  // namespace sly4

namespace {

inline bool IsNameChar(u8 c) {
    const s8 s = static_cast<s8>(c);
    return (s >= 'A' && s <= 'Z') || (s >= 'a' && s <= 'z') || (s >= '0' && s <= '9') || c == '_';
}

}  // namespace

// void Ref_Destruct(Ref* r)  [0x00B225E0 +0]
DECOMP(00794F20, Ref_Destruct) {
    store<u32>(arg<u32>(ctx, 0), kRefVtbl);
    ctx.gpr[4] = kRefVtbl;
}

// void Ref_Delete(Ref* r)  [+4]
DECOMP(00794F30, Ref_Delete) {
    const u32 r = arg<u32>(ctx, 0);
    if (r == 0) return;
    store<u32>(r, kRefVtbl);
    call(ctx, rfn::Mem_FreeFrom, {r, 0, r, kRefVtbl});  // r5 / r6 as in the original
}

// void Ref_Release(Ref* r)  [+8]: deletes it when the count drops to 0.
DECOMP(00794F74, Ref_Release) {
    const u32 r = arg<u32>(ctx, 0);
    const u16 n = static_cast<u16>(load<u16>(r + kRefCount) - 1);
    store<u16>(r + kRefCount, n);
    if (static_cast<s16>(n) > 0 || r == 0) return;
    const u32 opd = load<u32>(load<u32>(r) + kVfnDelete);
    call_ptr(ctx, opd, {r, opd, load<u32>(opd)});  // r4 / r5 = OPD and entry
}

// void UiRes_Construct(UiRes* r, const char* name, const char* path, (unused), u32 arg)
// Copies the path, expanding "$<var>" (only the variable whose name hashes
// to 0x243084BD; others are dropped), hashes the name and the file name,
// and puts the resource at the head of the resource list.
DECOMP(004D5274, UiRes_Construct) {
    const u32 r = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    u32 path = arg<u32>(ctx, 2);
    const u32 a84 = arg<u32>(ctx, 4);

    constexpr u32 kFrameBase = 112;
    Stack frame(ctx, 256 - kFrameBase);
    const u32 sp = frame.addr() - kFrameBase;
    const u32 var = sp + 112, file = sp + 144;

    store<u32>(r + kResFileHash, 0);
    store<u16>(r + kRefCount, 1);
    store<u32>(r, kResVtbl);
    store<u32>(r + kResHash, static_cast<u32>(call(ctx, rfn::String_HashNoCase, {name})));
    store<u16>(r + kResUsers, 0);
    store<u16>(r + kResLoaded, 0);
    store<u32>(r + kResArg, a84);
    store<u32>(r + kResNext, 0);

    u32 out = r + kResPath;
    u32 slash = out - 1;  // where the last '/' or '\' went (one before the start when there is none)
    u32 count = 0;
    while (path != 0 && load<u8>(path) != 0) {
        const u8 c = load<u8>(path);
        if (c == '/' || c == '\\') {
            slash = out;
        } else if (c == '$') {
            ++path;
            u32 v = var;
            while (IsNameChar(load<u8>(path))) store<u8>(v++, load<u8>(path++));
            store<u8>(v, 0);
            if (static_cast<u32>(call(ctx, rfn::String_HashNoCase, {var})) != kResPathVarHash) continue;
            for (u32 s = load<u32>(kResPathVar); load<u8>(s) != 0 && count < kResPathVarMax; ++s) {
                store<u8>(out++, load<u8>(s));
                ++count;
            }
            continue;
        }
        store<u8>(out++, c);
        ++path;
        ++count;
    }
    store<u8>(out, 0);
    // The file name starts after the last slash (or at the start).
    call(ctx, rfn::String_Assign, {file, slash + 1});
    store<u32>(r + kResFileHash, static_cast<u32>(call(ctx, rfn::String_HashNoCase, {file})));
    store<u32>(r + kResNext, load<u32>(kResList));
    store<u32>(kResList, r);
}

// Thunk: the same with the fourth argument cut to a byte.
DECOMP(004D54D4, UiRes_ConstructThunk) {
    ctx.gpr[6] &= 0xFF;
    call(ctx, rfn::UiRes_Construct, {arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2),
                                     static_cast<u32>(ctx.gpr[6]), arg<u32>(ctx, 4)});
}

// void UiRes_Destruct(UiRes* r)  [0x00B225F8 +0]
DECOMP(004D5878, UiRes_Destruct) {
    const u32 r = arg<u32>(ctx, 0);
    store<u32>(r, kResVtbl);
    Res_Unlink(ctx, r);
    store<u32>(r, kRefVtbl);
}

// void UiRes_Delete(UiRes* r)  [+4]
DECOMP(004D58E4, UiRes_Delete) {
    const u32 r = arg<u32>(ctx, 0);
    if (r == 0) return;
    store<u32>(r, kResVtbl);
    Res_Unlink(ctx, r);
    store<u32>(r, kRefVtbl);
    call(ctx, rfn::Mem_FreeFrom, {r, 0, r, kRefVtbl});
}

// void UiRes_Unuse(UiRes* r)  [+8]
// One user fewer: unloads it when the last one goes, then releases the
// reference.
DECOMP(004D5990, UiRes_Unuse) {
    const u32 r = arg<u32>(ctx, 0);
    const u16 users = static_cast<u16>(load<u16>(r + kResUsers) - 1);
    store<u16>(r + kResUsers, users);
    if (static_cast<s16>(users) <= 0 && load<u16>(r + kResLoaded) > 1) {
        const u32 opd = load<u32>(load<u32>(r) + kVfnUnload);
        call_ptr(ctx, opd, {r, opd, load<u32>(opd)});
        store<u16>(r + kResLoaded, 0);
    }
    const u16 refs = static_cast<u16>(load<u16>(r + kRefCount) - 1);
    store<u16>(r + kRefCount, refs);
    if (static_cast<s16>(refs) > 0 || r == 0) return;
    const u32 opd = load<u32>(load<u32>(r) + kVfnDelete);
    call_ptr(ctx, opd, {r, opd, load<u32>(opd)});
}

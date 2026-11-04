// World_HandleEvent callees, part 7a: small leaves under the hevent6
// helpers (sub-mesh / surface / render target set-up, map find, sound
// release, object placement).
#include "hevent7.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;

namespace sly4 {
namespace hevent7a {
constexpr u32 kSubMeshNext = 112;
constexpr u32 kLineScrollUnit = 0x004E20A0;  // f32 0.0 (used when the line has no glyph run)
constexpr u32 kPoolMain = 5, kPoolLocal = 4; // Mem_AllocPool pools: main memory / RSX local
constexpr u32 kSoundRefHead = 0x00F65B7C;    // u32 head of the 24-byte shared-sample list (name?)
constexpr u32 kSoundRefSize = 24;
constexpr u32 kPlaceEps = 0x003626DC;        // f32 1e-5
constexpr u32 kPlaceRate = 0x003626E0;       // f32 0.5
}  // namespace hevent7a
}  // namespace sly4

using namespace sly4::hevent7a;

// SubMesh* SubMesh_Construct5386DC(SubMesh* m) (name?): the base
// constructor, then the AglSubMesh vtable and +112 = 0. Returns the vtable
// address in r3 as the original does.
DECOMP(005386DC, SubMesh_Construct5386DC) {
    const u32 m = arg<u32>(ctx, 0);
    call(ctx, h7fn::SubMesh_ConstructBase, {m});
    store<u32>(m + kSubMeshNext, 0);
    store<u32>(m, kSubMeshVtable);
    ret(ctx, kSubMeshVtable);
}

// u32 Surface_Init5370A4(Surface* s, u32 w, u32 h, u32 unused, s32 format,
// Target** out) (name?): a new target (Target_Init(&t, w, h, format, 0, 0))
// bound to s; *out = t when out is given. r3 is the target then, else what
// Surface_SetBuffer returned.
DECOMP(005370A4, Surface_Init5370A4) {
    const u32 s = arg<u32>(ctx, 0), out = arg<u32>(ctx, 5);
    const u64 w = ctx.gpr[4], h = ctx.gpr[5], format = ctx.gpr[7];
    Stack frame(ctx, 16);
    call(ctx, h7fn::Target_Init, {frame.addr(), w, h, format, 0, 0});
    const u64 r = call(ctx, h7fn::Surface_SetBuffer, {s, load<u32>(frame.addr())});
    if (out != 0) {
        const u32 t = load<u32>(frame.addr());
        store<u32>(out, t);
        ret(ctx, t);
        return;
    }
    ctx.gpr[3] = r;
}

// bool Tree_Find78E260(Map*, u32 key, u32* out, Node* root) (name?): binary
// search from root (key < node key, unsigned, goes left at +12, else right
// at +16); on a hit *out = the node's value (+8) and 1, else 0.
DECOMP(0078E260, Tree_Find78E260) {
    const u32 key = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    u32 n = arg<u32>(ctx, 3);
    while (n != 0) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            store<u32>(out, load<u32>(n + 8));
            ret(ctx, 1u);
            return;
        }
        n = key < k ? load<u32>(n + 12) : load<u32>(n + 16);
    }
    ret(ctx, 0u);
}

// f32 Line_Scroll4E20A8(Line* l) (name?): the line's virtual metrics
// (+168, into a stack record) times its glyph run's row height (+168 font:
// u16 at run +102 times f32 at +96, when the font has both +100 and a run at
// +104; else 0.0) times the scale at +172.
DECOMP(004E20A8, Line_Scroll4E20A8) {
    const u32 l = arg<u32>(ctx, 0);
    Stack frame(ctx, 48);  // the record lives at r1+112..; +4 is read
    hevent::vcall(ctx, l, 168, {frame.addr(), l});
    const u32 font = load<u32>(l + 168);
    const f32 scale = load<f32>(l + 172);
    f32 unit;
    u32 run;
    if (load<u32>(font + 100) != 0 && (run = load<u32>(font + 104)) != 0) {
        const u32 rows = load<u16>(run + 102);
        unit = fmuls(static_cast<f32>(rows), load<f32>(font + 96));
        ctx.gpr[3] = font + 96;  // r3 as the original leaves it
    } else {
        unit = load<f32>(kLineScrollUnit);
        ctx.gpr[3] = kLineScrollUnit;
    }
    ret(ctx, fmuls(fmuls(unit, load<f32>(frame.addr() + 4)), scale));
}

// u32 Buffer_Alloc52F198(Buffer* b, u32 bytes, u32 align, s32 where,
// u32 flags) (name?): records the size (+8 bits 4..31) and flags (+12 top
// nibble), allocates from pool 5 (where == 1, main memory) or 4 (RSX local),
// then +0 = address and +4 = RSX offset << 1 | (where == 1). Returns the
// offset.
DECOMP(0052F198, Buffer_Alloc52F198) {
    const u32 b = arg<u32>(ctx, 0), bytes = arg<u32>(ctx, 1);
    const u64 align = ctx.gpr[5], flags = ctx.gpr[7];
    const bool main = arg<s32>(ctx, 3) == 1;
    store<u32>(b + 8, (load<u32>(b + 8) & 0xF) | (bytes << 4));
    store<u32>(b + 12, (load<u32>(b + 12) & 0x0FFFFFFF) | (static_cast<u32>(flags) << 28));
    const u64 p = call(ctx, h7fn::Mem_AllocPool, {align, bytes & 0x0FFFFFFF, main ? kPoolMain : kPoolLocal, flags});
    store<u32>(b, static_cast<u32>(p));
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), 0);
    call(ctx, h7fn::cellGcmAddressToOffset, {p, frame.addr()});
    const u32 off = load<u32>(frame.addr());
    store<u32>(b + 4, (off << 1) | (main ? 1u : 0u));
    ret(ctx, off);
}

// RenderSurface* Target_Bind51B4C4(u32 color, u32 depth, Texture* t)
// (name?): a new render surface for t: +28 = t, +56/+60 = its width and
// height (u16 at +52/+54), +52 flags bits 26..28 = 0b010 | (depth != 0) <<
// 27 | (color != 0) << 26 (bits 30..31 and 25 cleared), +24 = depth and
// +16 = color when given, +44 = 243 (depth; 3 if &+16 were 0) or 240.
DECOMP(0051B4C4, Target_Bind51B4C4) {
    const u32 color = arg<u32>(ctx, 0), depth = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2);
    const u32 s = static_cast<u32>(call(ctx, h7fn::RenderSurface_Alloc));
    u32 f = load<u32>(s + 52);
    store<u32>(s + 28, t);
    f = (f & 0x01FFFFFF) | 0x10000000 | (depth != 0 ? 0x08000000u : 0u) | (color != 0 ? 0x04000000u : 0u);
    store<u32>(s + 52, f);
    const u32 w = load<u16>(t + 52), h = load<u16>(t + 54);
    store<u32>(s + 56, w);
    store<u32>(s + 60, h);
    if (depth != 0) store<u32>(s + 24, depth);
    if (color != 0) store<u32>(s + 16, color);
    const bool targets = s + 16 != 0;  // quirk: the address of +16 is tested
    if (f & 0x08000000)
        store<u32>(s + 44, targets ? 243u : 3u);
    else if (targets)
        store<u32>(s + 44, 240u);
    ret(ctx, s);
}

// void Sound_Free445390(SoundMgr* m, Sound* s) (name?): drops the sound's
// shared sample (+0: refcount at +20; at 0 it is unlinked from the list at
// 0x00F65B7C (+4 prev, +8 next) and freed), releases the two streams at
// +76/+80 (virtual +40 with 0), clears +72..+80 and pushes s on the
// manager's free list (+8192, linked through +120). Returns 0.
DECOMP(00445390, Sound_Free445390) {
    const u32 m = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 r = load<u32>(s);
    if (r != 0) {
        const s32 c = load<s32>(r + 20);
        if (c > 0) {
            store<u32>(r + 20, static_cast<u32>(c - 1));
            if (c - 1 == 0) {
                const u32 next = load<u32>(r + 8);
                if (next != 0)
                    store<u32>(next + 4, load<u32>(r + 4));
                else
                    store<u32>(kSoundRefHead, load<u32>(r + 4));
                const u32 prev = load<u32>(r + 4);
                if (prev != 0) store<u32>(prev + 8, load<u32>(r + 8));
                call(ctx, h7fn::SmallFree, {r, kSoundRefSize});
            }
        }
    }
    if (const u32 o = load<u32>(s + 76); o != 0) hevent::vcall(ctx, o, 40, {o, 0});
    const u32 o2 = load<u32>(s + 80);
    store<u32>(s + 76, 0);
    if (o2 != 0) hevent::vcall(ctx, o2, 40, {o2, 0});
    store<u32>(s + 80, 0);
    const u32 head = load<u32>(m + 8192);
    store<u32>(s + 72, 0);
    store<u32>(s + 120, head);
    store<u32>(m + 8192, s);
    ret(ctx, 0u);
}

// void Obj_Pos3628B4(vec3* out, Object* o) (name?): out = the sum of the
// positions (+96 of each member's +8, or 0x01164B40 without one) of the
// members listed before o in the object list of o's kind (+8 -> +84; none
// for kind 5), then out = Obj_Offset362830(kind, out). Leaves f1 = out.z.
DECOMP(003628B4, Obj_Pos3628B4) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    store<u32>(out, load<u32>(kZeroVec));
    store<u32>(out + 4, load<u32>(kZeroVec + 4));
    store<u32>(out + 8, load<u32>(kZeroVec + 8));
    f32 x = load<f32>(out), y = load<f32>(out + 4), z = load<f32>(out + 8);
    u32 kind = kKindNone;
    if (const u32 c = load<u32>(o + 8); c != 0) {
        kind = load<u32>(c + 84);
        if (kind != kKindNone) {
            const u32 list = kObjLists + kind * kObjListStride;
            const u32 n = load<u16>(list + 8);
            for (u32 i = 0; i < n; ++i) {
                const u32 e = load<u32>(load<u32>(list) + 4 * i);
                if (e == o) break;
                const u32 p = load<u32>(e + 8);
                const u32 src = p != 0 ? p + 96 : kZeroVec;
                const f32 sx = load<f32>(src), sy = load<f32>(src + 4), sz = load<f32>(src + 8);
                x = static_cast<f32>(static_cast<f64>(x) + sx);
                y = static_cast<f32>(static_cast<f64>(y) + sy);
                z = static_cast<f32>(static_cast<f64>(z) + sz);
                store<f32>(out, x);
                store<f32>(out + 4, y);
                store<f32>(out + 8, z);
            }
        }
    }
    Stack frame(ctx, 16);
    call(ctx, h7fn::Obj_Offset362830, {frame.addr(), kind, out});
    store<u32>(out, load<u32>(frame.addr()));
    store<u32>(out + 4, load<u32>(frame.addr() + 4));
    store<u32>(out + 8, load<u32>(frame.addr() + 8));
    ctx.fpr[1] = load<f32>(frame.addr() + 8);
}

// void Obj_Place3626E8(Object* o, const vec3* p, u8 snap) (name?): snap:
// when p differs from the position (+32) by 1e-5 or more on an axis, the
// world object (+24) gets p at +64 and is marked dirty; then +32 = p and the
// motion at +48 stops. Otherwise a blend starts: +128 = &+32, +52 = 0, the
// motion at +48 set to 0.5, +80 = the old position and +96 = p. Leaves
// f1 = p->z.
DECOMP(003626E8, Obj_Place3626E8) {
    const u32 o = arg<u32>(ctx, 0), p = arg<u32>(ctx, 1);
    if ((arg<u32>(ctx, 2) & 0xFF) != 0) {
        const f32 eps = load<f32>(kPlaceEps);
        auto near = [&](u32 k) {
            return std::fabs(static_cast<f32>(static_cast<f64>(load<f32>(o + 32 + k)) - load<f32>(p + k))) < eps;
        };
        if (!(near(0) && near(4) && near(8))) {
            const u32 w = load<u32>(o + 24);
            store<u32>(w + 64, load<u32>(p));
            store<u32>(w + 68, load<u32>(p + 4));
            store<u32>(w + 72, load<u32>(p + 8));
            call(ctx, h7fn::WorldObj_MarkDirty, {w});
        }
        store<u32>(o + 32, load<u32>(p));
        store<u32>(o + 36, load<u32>(p + 4));
        store<u32>(o + 40, load<u32>(p + 8));
        call(ctx, h7fn::Motion_Stop4A0C08, {o + 48});
    } else {
        store<u32>(o + 128, o + 32);
        store<u32>(o + 52, 0);
        callf(ctx, h7fn::Motion_Set4A0C20, {o + 48}, {load<f32>(kPlaceRate)});
        const u32 x = load<u32>(o + 32), y = load<u32>(o + 36);
        store<u32>(o + 80, x);
        const u32 z = load<u32>(o + 40);
        store<u32>(o + 84, y);
        store<u32>(o + 88, z);
        store<u32>(o + 96, load<u32>(p));
        store<u32>(o + 100, load<u32>(p + 4));
        store<u32>(o + 104, load<u32>(p + 8));
    }
    ctx.fpr[1] = load<f32>(p + 8);
}

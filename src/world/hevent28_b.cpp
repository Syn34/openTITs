// World_HandleEvent callees, part 28b: the delayed spawn queue
// (SpawnQueue_Update4AE498).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;
using namespace sly4::hevent28;

namespace {
// Cooldown timer (f32) and the queue of pending spawns: a vector at
// 0x010C2480 (+0 data, u16 +8 count) of 128-byte records.
constexpr u32 kCooldown = 0x00F65F80;
constexpr u32 kQueue = 0x010C2480;
constexpr u32 kRecSize = 128;
// Record: +0 3x4 matrix (rows of 16 bytes, three words each), +64 vec3
// position, +80 vec3, +96 Profile* (spawner), +104 u32 tag, +108 f32 delay,
// +112 u8 flag.
constexpr u32 kRecPos = 64, kRecDir = 80, kRecProf = 96, kRecTag = 104, kRecDelay = 108, kRecFlag = 112;
// Constants in front of the function (0x004AE48C): 0, -1, 100.
constexpr u32 kC = 0x004AE48C;
constexpr u32 kZero = kC + 0, kMinusOne = kC + 4, kHundred = kC + 8;
constexpr u32 kZeroVec = 0x01164B30, kVecB40 = 0x01164B40;  // vec3 constants (name?)
constexpr u32 kSpawnRange = 0x010C23D8;                     // f32 (name?)
constexpr u32 kSpawnFade = 0x010C21D8;                      // f32 (name?)
// Spawn queries (as SpawnQuery_Construct0CC188, hevent25_a.cpp).
constexpr u32 kQueryVtable = 0x00B12260, kSpawnQueryVtable = 0x00B12868, kAttachQueryVtable = 0x00B18B30;
constexpr u32 kQueryRecs = 16, kQueryFlags = 132, kQueryName = 140, kQueryNameSize = 40;
constexpr u32 kProfRecs = 136, kProfRecSize = 192;
constexpr u32 kSpawnVirtual = 28;  // Profile vt +28: (prof, query) -> spawned object
// The original's 736-byte frame: the plain query at r1 +144 and the spawn
// query at r1 +336. Both read a flags word before writing it, so the
// frame is kept at the original's size and those are the same stack words.
constexpr u32 kFrame = 736, kQueryAt = 144, kSpawnAt = 336;

void copy32(u32 dst, u32 src) { store<u32>(dst, load<u32>(src)); }

// Frees a query's PtrVector held at slot.
void free_vec(PPUContext& ctx, u32 slot) {
    const u32 v = load<u32>(slot);
    if (v == 0) return;
    call(ctx, h28fn::PtrVector_Clear, {v});
    call(ctx, h28fn::Nop, {v});
    call(ctx, h28fn::SmallFree, {load<u32>(slot), 20});
}
}  // namespace

// u16 SpawnQueue_Update4AE498(; f32 dt) (name?): the cooldown at
// 0x00F65F80 runs down to 0; every queued spawn's delay is reduced by dt
// and, once it reaches 0, the spawn is made through its profile's vt +28:
// with a spawn query (vtable 0x00B12868, SpawnQuery_Init) when the
// profile's +20 bit 5 is set, else a plain query (vtable 0x00B12260)
// carrying the record's matrix and the profile's records. The object made
// gets the tag (+428 when +192 bit 5, +400 when bit 2) and, with bit 6,
// the fade at +588 / +432 bit 6 / +592 bit 27. The record is then removed
// (the last one moved into its place). Returns the queue's count; f1 is
// left as the original's last load.
DECOMP(004AE498, SpawnQueue_Update4AE498) {
    const f64 dt = farg(ctx, 0);
    const f32 zero = lf(kZero);
    // f1 is written where the original leaves a value in it.
    {
        const f32 g = lf(kCooldown);
        const f32 d = fsubs(g, dt);
        const f32 m = g < d ? g : d;
        const f32 v = m > zero ? m : zero;
        store<f32>(kCooldown, v);
        ctx.fpr[1] = v;
    }
    Stack frame(ctx, kFrame);
    const u32 q = frame.addr() + kQueryAt, sq = frame.addr() + kSpawnAt;
    s32 i = 0;
    for (;;) {
        const u32 n = load<u16>(kQueue + 8);
        if (!(i < static_cast<s32>(n))) {
            ctx.gpr[3] = n;
            return;
        }
        const u32 e = load<u32>(kQueue) + static_cast<u32>(i) * kRecSize;
        const f32 t = fsubs(lf(e + kRecDelay), dt);
        store<f32>(e + kRecDelay, t);
        ctx.fpr[1] = t;
        if (t > zero) {
            ++i;
            continue;
        }
        const u32 prof = load<u32>(e + kRecProf);
        u32 spawned;
        if (prof == 0 || (load<u32>(prof + 20) & 0x20u) == 0) {
            const u32 name = static_cast<u32>(call(ctx, h28fn::Obj_InstanceName, {prof}));
            const u32 p = load<u32>(e + kRecProf);
            store<u32>(q + kQueryRecs, 0);
            store<u32>(q + 0, kQueryVtable);
            for (u32 row = 0; row < 4; ++row)
                for (u32 c = 0; c < 12; c += 4) copy32(q + 32 + 16 * row + c, e + 16 * row + c);
            const u32 old = load<u32>(q + kQueryFlags);  // stack word read before written (as the original)
            for (u32 c = 0; c < 12; c += 4) copy32(q + 96 + c, kZeroVec + c);
            store<u32>(q + 112, 0);
            store<u32>(q + 116, 0);
            store<u32>(q + 120, p);
            const u32 dflt = load<u32>(kMinusOne);
            store<u32>(q + 124, p != 0 ? load<u32>(p + 36) : dflt);
            const u32 b = p != 0 ? load<u32>(p + 40) : dflt;
            store<u32>(q + 128, b);
            u32 flags = (old | 0x80000000u) & ~0x40000000u;
            const u32 bit29 = p != 0 ? (load<u8>(p + 32) >> 2) & 1u : 0u;
            flags = (flags & ~0x20000000u) | (bit29 << 29);
            store<u32>(q + 136, 0);
            store<u32>(q + kQueryFlags, flags | 0x1FE00000u);
            store<u32>(q + 4, 0);
            store<u32>(q + 8, 0);
            store<u32>(q + 12, 0);
            store<u8>(q + kQueryName, 0);
            ctx.fpr[1] = std::bit_cast<f32>(b);
            if (name != 0) call(ctx, h28fn::String_Copy, {q + kQueryName, name, kQueryNameSize});
            if (p != 0) {
                for (u32 j = 0;; ++j) {
                    const u32 recs = load<u32>(p + kProfRecs);
                    if (j >= (recs != 0 ? static_cast<u32>(load<u16>(recs + 8)) : 0u)) break;
                    const u32 rec = load<u32>(recs) + j * kProfRecSize;
                    u32 v = load<u32>(q + kQueryRecs);
                    if (v == 0) {
                        v = static_cast<u32>(call(ctx, h28fn::SmallAlloc, {20}));
                        store<u32>(q + kQueryRecs, v);
                        if (v != 0) {
                            call(ctx, h28fn::PtrVector_Construct, {v});
                            store<u16>(v + 6, kProfRecSize);
                            v = load<u32>(q + kQueryRecs);
                        }
                    }
                    call(ctx, h28fn::Vector_Add, {v, rec});  // also with a failed allocation (as the original)
                }
            }
            const u32 o = load<u32>(e + kRecProf);  // may be 0 (as the original)
            spawned = static_cast<u32>(vcall(ctx, o, kSpawnVirtual, {o, q}));
            store<u32>(q + 0, kQueryVtable);
            free_vec(ctx, q + kQueryRecs);
        } else {
            const u32 name = static_cast<u32>(call(ctx, h28fn::Obj_InstanceName, {prof}));
            callf(ctx, h28fn::SpawnQuery_Init, {sq, name, prof, 0, e, kZeroVec, 1, 0, 0, 0, 0}, {static_cast<f64>(lf(kHundred))});
            for (u32 c = 0; c < 12; c += 4) copy32(sq + 240 + c, kVecB40 + c);
            store<u32>(sq + 0, kAttachQueryVtable);
            for (u32 c = 0; c < 12; c += 4) copy32(sq + 256 + c, kVecB40 + c);
            store<u32>(sq + 272, 0);
            store<u32>(sq + 276, 0xBF800000u);  // -1.0f
            store<u32>(sq + 280, 0x7F7FFFFFu);  // FLT_MAX
            store<u8>(sq + 284, 0);
            const u32 w4 = load<u32>(sq + 4);
            {  // prof is non-null here; the original tests it again
                const u32 bit = load<u32>(prof + 300) >> 31;
                store<u32>(sq + kQueryFlags, (load<u32>(sq + kQueryFlags) & ~0x40000000u) | (bit << 30));
            }
            store<u32>(sq + 4, w4 | 0x20u);
            for (u32 c = 0; c < 12; c += 4) copy32(sq + 256 + c, e + kRecPos + c);
            for (u32 c = 0; c < 12; c += 4) copy32(sq + 240 + c, e + kRecDir + c);
            copy32(sq + 280, kSpawnRange);
            store<u32>(sq + 272, load<u32>(e + kRecTag));
            ctx.fpr[1] = lf(e + kRecDir + 8);
            const u32 o = load<u32>(e + kRecProf);
            spawned = static_cast<u32>(vcall(ctx, o, kSpawnVirtual, {o, sq}));
            // ~SpawnQuery (inlined, as VtB12868_Dtor)
            store<u32>(sq + 0, kSpawnQueryVtable);
            call(ctx, h28fn::PtrVector_Clear, {sq + 200});
            call(ctx, h28fn::Nop, {sq + 200});
            call(ctx, h28fn::PtrVector_Clear, {sq + 180});
            call(ctx, h28fn::Nop, {sq + 180});
            store<u32>(sq + 0, kQueryVtable);
            free_vec(ctx, sq + kQueryRecs);
        }
        if (spawned != 0) {
            if ((load<u32>(spawned + 192) & 0x20u) != 0) {
                store<u32>(spawned + 428, load<u32>(e + kRecTag));
                if ((load<u32>(spawned + 192) & 0x40u) != 0) {
                    const u32 fade = load<u32>(kSpawnFade);
                    store<u32>(spawned + 588, fade);
                    ctx.fpr[1] = std::bit_cast<f32>(fade);
                    const u32 on = std::bit_cast<f32>(fade) > zero ? 1u : 0u;
                    store<u16>(spawned + 432, static_cast<u16>((load<u16>(spawned + 432) & ~0x40u) | (on << 6)));
                    if (load<u8>(e + kRecFlag) != 0) store<u32>(spawned + 592, load<u32>(spawned + 592) | 0x08000000u);
                }
            }
            if ((load<u32>(spawned + 192) & 0x4u) != 0) store<u32>(spawned + 400, load<u32>(e + kRecTag));
        }
        // Remove the record: the last one is moved into its place.
        const u32 last = (load<u16>(kQueue + 8) - 1u) & 0xFFFFu;
        store<u16>(kQueue + 8, static_cast<u16>(last));
        if (i != static_cast<s32>(last)) {
            const u32 data = load<u32>(kQueue);
            const u32 src = data + last * kRecSize, dst = data + static_cast<u32>(i) * kRecSize;
            for (u32 row = 0; row < 6; ++row)
                for (u32 c = 0; c < 12; c += 4) copy32(dst + 16 * row + c, src + 16 * row + c);
            for (u32 c = 96; c < 112; c += 4) copy32(dst + c, src + c);
            ctx.fpr[1] = lf(src + kRecDelay);
            store<u8>(dst + kRecFlag, load<u8>(src + kRecFlag));
        }
    }
}

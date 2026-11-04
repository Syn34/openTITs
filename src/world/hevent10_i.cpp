// World_HandleEvent callees, part 10i: the physics world step (contact
// velocity fix-up and fixed sub-steps). The Havok functions it uses
// (0x00867058, 0x0091A4C8, 0x0092F0C8, 0x009D19C8) stay recompiled.
#include <cmath>

#include "hevent10.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent10;

namespace sly4 {
namespace hevent10i {
namespace ifn10 {
constexpr u32 Hk_GetBody9D19C8 = 0x009D19C8;     // (entity) -> rigid body (Havok, name?)
constexpr u32 Hk_GetAgents867058 = 0x00867058;   // (collidable) -> {+0 8-byte entries, +4 s32 count} (Havok, name?)
constexpr u32 Hk_Activate91A4C8 = 0x0091A4C8;    // (body) (Havok, name?)
constexpr u32 Hk_StepDeltaTime92F0C8 = 0x0092F0C8;  // (world, a, b; f32 dt) (Havok, name?)
}  // namespace ifn10

// Physics wrapper: +4 hkpWorld*, +32 hkArray of entities {+0 data, +4 s32
// size, +8 capacity | 0x80000000 (not owned)}.
constexpr u32 kPwWorld = 4, kPwEnts = 32, kPwEntCount = 36, kPwEntCap = 40;
constexpr u32 kArrNotOwned = 0x80000000u;
// Rigid body: +16 collidable, +224 motion (vtable +68: set velocity), +288
// vec4 position, +432 vec4 velocity. Agent entry: {+0 contact entry (+8
// manager: vtable +48 (id) -> point, +52 (hkArray<u16>* ids)), +4 agent}.
// Agent: +16 s8 offset to the other body, +24 s8 kind (1: has one). The
// other body's +232 u8 type (7: the kind that gets fixed up). Contact
// point +16 vec4 normal.
constexpr u32 kBodyColl = 16, kBodyMotion = 224, kBodyPos = 288, kBodyVel = 432;
constexpr u32 kVGetPoint = 48, kVGetIds = 52, kVSetVel = 68;
constexpr u32 kOtherType = 232, kTypeFixUp = 7;
constexpr u32 kHkMem = 0x011DF31C;   // Havok memory object (vtable +20: free(p, bytes)) (name?)
constexpr u32 kVFree = 20;
constexpr u32 kStepFlag = 0x00F64CB0;      // u8 cleared on a step (name?)
constexpr u32 kNoFixUp = 0x0106B9A4 + 72;  // u8: skip the contact pass (name?)
constexpr u32 kStepArgA = 0x00F64C94, kStepArgB = 0x00F64C90;  // u32 (name?)
constexpr u32 kConsts = 0x00345E90;  // f32 {1e-5 min dt, 0 (min dot / end), 1/15 sub-step}
}  // namespace hevent10i
}  // namespace sly4

using namespace sly4::hevent10i;

namespace {
f32 lane(u32 ea, u32 k) { return load<f32>((ea & ~15u) + 4 * k); }  // lvx element k
// Free through the Havok memory object (r6 = entry, r7 = OPD as in the
// two-argument sequence of the original).
void hk_free(PPUContext& ctx, u32 p, u64 bytes) {
    const u32 opd = load<u32>(load<u32>(kHkMem) + kVFree);
    ctx.gpr[6] = load<u32>(opd);
    ctx.gpr[7] = opd;
    call_ptr(ctx, opd, {kHkMem, p, bytes});
}

// For every entity's rigid body: each contact with a fix-up body (+232 ==
// 7) whose relative position (other - body) has a non-negative component
// along the body's velocity (moving towards it) removes the velocity's
// component along the contact normal (velocity -= n * (velocity . n); Activate, then motion vtable +68).
void contact_pass(PPUContext& ctx, u32 pw) {
    Stack frame(ctx, 48);
    const u32 ids = frame.addr();        // hkArray<u16> {data, size, capacity}
    const u32 vel = frame.addr() + 16;   // vec4
    for (s32 i = 0; i < static_cast<s32>(load<u32>(pw + kPwEntCount)); ++i) {
        const u32 body = static_cast<u32>(call(ctx, ifn10::Hk_GetBody9D19C8, {load<u32>(load<u32>(pw + kPwEnts) + static_cast<u32>(i) * 4)}));
        const u32 coll = body + kBodyColl;
        const s32 n = static_cast<s32>(load<u32>(static_cast<u32>(call(ctx, ifn10::Hk_GetAgents867058, {coll})) + 4));
        for (s32 j = 0; j < n; ++j) {
            const u32 off = static_cast<u32>(j) * 8;
            const u32 agent = load<u32>(load<u32>(static_cast<u32>(call(ctx, ifn10::Hk_GetAgents867058, {coll}))) + off + 4);
            u32 other = 0;
            if (static_cast<s8>(load<u8>(agent + 24)) == 1) other = agent + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(agent + 16))));
            if (load<u8>(other + kOtherType) != kTypeFixUp) continue;
            const u32 entry = load<u32>(load<u32>(static_cast<u32>(call(ctx, ifn10::Hk_GetAgents867058, {coll}))) + off);
            const u32 mgr = load<u32>(entry + 8);
            store<u32>(ids + 0, 0);
            store<u32>(ids + 4, 0);
            store<u32>(ids + 8, kArrNotOwned);
            vcall(ctx, mgr, kVGetIds, {mgr, ids});
            f32 d[4];
            for (u32 k = 0; k < 4; ++k) d[k] = lane(other + kBodyPos, k) - lane(body + kBodyPos, k);
            const f64 lim = lfs_f64(kConsts + 4);
            for (s32 k = 0; k < static_cast<s32>(load<u32>(ids + 4)); ++k) {
                const u32 id = load<u16>(load<u32>(ids) + static_cast<u32>(k) * 2);
                const u32 pt = static_cast<u32>(vcall(ctx, mgr, kVGetPoint, {mgr, id}));
                f32 v[4], p[4];
                for (u32 e = 0; e < 4; ++e) {
                    v[e] = lane(body + kBodyVel, e);
                    p[e] = std::fma(v[e], d[e], 0.0f);
                }
                const f32 dot = (p[0] + p[1]) + p[2];
                if (dot < lim) continue;
                f32 nrm[4], q[4];
                for (u32 e = 0; e < 4; ++e) {
                    nrm[e] = lane(pt + 16, e);
                    q[e] = std::fma(v[e], nrm[e], 0.0f);
                }
                const f32 s = (q[0] + q[1]) + q[2];
                for (u32 e = 0; e < 4; ++e) store<f32>(vel + 4 * e, v[e] - std::fma(nrm[e], s, 0.0f));
                call(ctx, ifn10::Hk_Activate91A4C8, {body});
                vcall(ctx, body + kBodyMotion, kVSetVel, {body + kBodyMotion, vel});
            }
            store<u32>(ids + 4, 0);
            const u32 cap = load<u32>(ids + 8);
            if ((cap & kArrNotOwned) == 0) hk_free(ctx, load<u32>(ids), (cap << 1) & 0x7FFFFFFEu);
            store<u32>(ids + 0, 0);
            store<u32>(ids + 8, kArrNotOwned);
        }
    }
}
}  // namespace

// u32 Physics_Step345EB8(PhysicsWorld* pw, f32 dt) (name?): with a world and
// dt >= 1e-5: clears the flag at 0x00F64CB0, runs the contact pass (unless
// 0x0106B9EC is set), then steps the world in ceil(dt * 15) equal sub-steps
// (0x0092F0C8). Always empties the entity array (freeing an owned buffer).
// Returns 0; f1 as the original leaves it.
DECOMP(00345EB8, Physics_Step345EB8) {
    const u32 pw = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const f64 c0 = lfs_f64(kConsts);
    if (load<u32>(pw + kPwWorld) != 0 && (dt > c0 || dt == c0)) {
        if (load<u8>(kStepFlag) != 0) store<u8>(kStepFlag, 0);
        if (load<u8>(kNoFixUp) == 0) contact_pass(ctx, pw);
        const f64 q = static_cast<f32>(dt / lfs_f64(kConsts + 8));
        u32 n = static_cast<u32>(rt::fctid(q, true));
        if (static_cast<f32>(q - static_cast<f32>(static_cast<f64>(n))) > c0) n += 1;
        const f64 step = static_cast<f32>(static_cast<f32>(dt / static_cast<f32>(static_cast<f64>(n))) + c0);
        const f64 end = lfs_f64(kConsts + 4);
        ctx.fpr[1] = dt;
        f64 left = dt;
        while (left > end) {
            const f64 h = step > left ? left : step;
            callf(ctx, ifn10::Hk_StepDeltaTime92F0C8, {load<u32>(pw + kPwWorld), load<u32>(kStepArgA), load<u32>(kStepArgB)}, {h});
            left = static_cast<f32>(left - h);
        }
    }
    const u32 cap = load<u32>(pw + kPwEntCap);
    store<u32>(pw + kPwEntCount, 0);
    if ((cap & kArrNotOwned) == 0)
        vcall(ctx, kHkMem, kVFree, {kHkMem, load<u32>(pw + kPwEnts), static_cast<u64>(static_cast<s64>(static_cast<s32>(cap << 2)))});
    store<u32>(pw + kPwEnts, 0);
    store<u32>(pw + kPwEntCap, kArrNotOwned);
    ret(ctx, 0u);
}

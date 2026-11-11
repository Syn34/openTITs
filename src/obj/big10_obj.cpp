// Game code, large-function sweep part 10: object / camera-target helpers
// named by part 9 (00115B90, 004A22D8, ...).
#include "big10.h"

using namespace sly4;
using namespace sly4::big10;

namespace {
constexpr u32 kCamTargetVt = 0x00B14078u;   // CamTarget vtable
constexpr u32 kIdentity = 0x01164BD0u;      // f32[16] identity matrix (the first three rows are copied)
// f32 tuning values (each +72 unless noted) copied into the target twice.
constexpr u32 kTune0 = 0x010350B8u;
constexpr u32 kTune1 = 0x01035104u;
constexpr u32 kTune2 = 0x01035150u;
constexpr u32 kTune3 = 0x010351A0u;         // +80
constexpr u32 kTune4 = 0x010351F0u;         // +4, +8
constexpr u32 kTune5 = 0x01035200u;
constexpr u32 kTune6 = 0x0103524Cu;
constexpr u32 kTune7 = 0x01035298u;
constexpr u32 kTune8 = 0x010352E4u;
}  // namespace

// void CamTarget_Construct(CamTarget* t, Camera* cam, Obj* a, Obj* b; f32 blend)
// Fills a 224-byte camera target: the camera, the identity rotation rows, a
// set of tuning floats (current and default copies), the two referenced
// objects (each gets ++ on its u16 reference count at +178) and the blend
// at +144 / +208.
DECOMP(00115B90, CamTarget_Construct115B90) {
    const u32 t = arg<u32>(ctx, 0), cam = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const f64 blend = farg(ctx, 0);
    store<u32>(t + 4, cam);
    store<u8>(t + 8, 0);
    CopyW(t + 32, kIdentity + 0);
    CopyW(t + 36, kIdentity + 4);
    CopyW(t + 40, kIdentity + 8);
    CopyW(t + 48, kIdentity + 16);
    CopyW(t + 52, kIdentity + 20);
    CopyW(t + 56, kIdentity + 24);
    const u32 i32 = load<u32>(kIdentity + 32);
    store<u32>(t + 0, kCamTargetVt);
    store<u32>(t + 64, i32);
    CopyW(t + 68, kIdentity + 36);
    CopyW(t + 72, kIdentity + 40);
    CopyW(t + 80, kIdentity + 48);
    CopyW(t + 84, kIdentity + 52);
    CopyW(t + 88, kIdentity + 56);
    const u32 w0 = load<u32>(kTune0 + 72);
    store<u32>(t + 12, 0);
    store<u32>(t + 96, w0);
    const u32 w1 = load<u32>(kTune1 + 72);
    store<u32>(t + 16, 0);
    store<u32>(t + 100, w1);
    const u32 w2 = load<u32>(kTune2 + 72);
    store<u32>(t + 104, w2);
    const u32 w3 = load<u32>(kTune3 + 80);
    store<u32>(t + 112, w3);
    const u32 w4 = load<u32>(kTune4 + 4);
    store<u32>(t + 116, w4);
    const u32 w5 = load<u32>(kTune4 + 8);
    store<u32>(t + 120, w5);
    const u32 w6 = load<u32>(kTune5 + 72);
    store<u32>(t + 128, w6);
    const u32 w7 = load<u32>(kTune6 + 72);
    store<u32>(t + 132, w7);
    const u32 w8 = load<u32>(kTune7 + 72);
    store<u32>(t + 136, w8);
    const u32 w9 = load<u32>(kTune8 + 72);
    store<u32>(t + 160, w0);
    store<u32>(t + 140, w9);
    store<u32>(t + 164, w1);
    store<u32>(t + 168, w2);
    store<u32>(t + 176, w3);
    store<u32>(t + 180, w4);
    store<u32>(t + 184, w5);
    store<u32>(t + 192, w6);
    store<u32>(t + 196, w7);
    store<u32>(t + 200, w8);
    store<u32>(t + 204, w9);
    store<u32>(t + 12, a);
    if (a != 0) AddRef178(a);
    store<u32>(t + 16, b);
    if (b != 0) AddRef178(b);
    Stfs(t + 208, blend);
    Stfs(t + 144, blend);
}

namespace {
constexpr u32 kSplineNew = 0x005A7CB4u;     // SplineInstance_New (key) -> instance
constexpr u32 kSetName = 0x0057D368u;       // WorldObj_SetName (o, name, 1)
constexpr u32 kTypeIndex = 0x004A1244u;     // Obj_TypeIndex4A1244 (o) -> u8
constexpr u32 kSetMode = 0x005A7B38u;       // SplineInstance_SetMode (inst, u8)
constexpr u32 kMemAlloc = 0x0054ED38u;      // Mem_Alloc (size, heap, align)
constexpr u32 kMemFreeFrom = 0x0054EE6Cu;   // Mem_FreeFrom (p, heap)
constexpr u32 kRefRelease = 0x00557C80u;    // RefBlock_Release (o)

// RefBlock (28 bytes): u16 refs, u32 slot[4] (+4..+16), u32 +20, u8 used (+24).
constexpr u32 kInstRefs = 12;               // instance: RefBlock* (owned, refcounted)

// The block's refcount drops by one; at <= 0 its four slots are released
// (last first) and it is freed.
void BlockDrop(PPUContext& ctx, u32 blk) {
    const u16 n = static_cast<u16>(load<u16>(blk) - 1);
    store<u16>(blk, n);
    if (static_cast<s16>(n) > 0) return;
    for (s32 k = 3; k >= 0; --k) {
        const u32 s = load<u32>(blk + 4 + 4 * static_cast<u32>(k));
        if (s != 0) call(ctx, kRefRelease, {s});
    }
    call(ctx, kMemFreeFrom, {blk, 0});
}
}  // namespace

// SplineInstance* SplineInst_FromKey(Obj* key, RefObj* ref)
// A new spline instance for key, named after key's vfunc +12; its RefBlock
// (made on first use) gets ref in the next slot (++ of the u16 count at
// +24, with the slot's old object released), then the mode is set from
// Obj_TypeIndex.
DECOMP(004A22D8, SplineInst_FromKey4A22D8) {
    const u32 key = arg<u32>(ctx, 0), ref = arg<u32>(ctx, 1);
    const u32 inst = static_cast<u32>(call(ctx, kSplineNew, {key}));
    const u32 name = static_cast<u32>(VCall(ctx, key, 12, {key}));
    call(ctx, kSetName, {inst, name, 1});
    u32 blk = load<u32>(inst + kInstRefs);
    if (blk == 0) {
        // Inlined smart-pointer assignment of a new block.
        u32 nb = static_cast<u32>(call(ctx, kMemAlloc, {28, 0, 16}));
        if (nb != 0) {
            store<u32>(nb + 4, 0);
            store<u32>(nb + 20, 0);
            store<u32>(nb + 8, 0);
            store<u16>(nb + 0, 1);
            store<u32>(nb + 12, 0);
            store<u32>(nb + 16, 0);
            store<u8>(nb + 24, 0);
        }
        const u32 old = load<u32>(inst + kInstRefs);
        if (old != nb) {
            store<u32>(inst + kInstRefs, nb);
            if (nb != 0) store<u16>(nb, static_cast<u16>(load<u16>(nb) + 1));
            if (old != 0) BlockDrop(ctx, old);
        }
        // The temporary's reference. Quirk: the count is decremented before
        // the null test, so a failed allocation writes the u16 at address 0.
        blk = load<u32>(inst + kInstRefs);
        const u16 n = static_cast<u16>(load<u16>(blk) - 1);
        store<u16>(blk, n);
        if (!(static_cast<s16>(n) > 0 || blk == 0)) {
            for (s32 k = 3; k >= 0; --k) {
                const u32 s = load<u32>(blk + 4 + 4 * static_cast<u32>(k));
                if (s != 0) call(ctx, kRefRelease, {s});
            }
            call(ctx, kMemFreeFrom, {blk, 0});
            blk = load<u32>(inst + kInstRefs);
        }
    }
    const u8 idx = load<u8>(blk + 24);
    const u32 slot = blk + 4 + 4 * static_cast<u32>(idx);
    store<u8>(blk + 24, static_cast<u8>(idx + 1));
    const u32 old = load<u32>(slot);
    if (old != ref) {
        store<u32>(slot, ref);
        if (ref != 0) store<u16>(ref, static_cast<u16>(load<u16>(ref) + 1));
        if (old != 0) call(ctx, kRefRelease, {old});
    }
    const u32 ti = static_cast<u32>(call(ctx, kTypeIndex, {inst}));
    call(ctx, kSetMode, {inst, ti & 0xFFu});
    ret(ctx, inst);
}

namespace {
// Havok helpers (names from what the code does with them, name?).
constexpr u32 kTransformInvPos = 0x007D170Cu;  // (vec4* out, const hkTransform*, const vec4* in): to local space
constexpr u32 kTransformInvDir = 0x007D17E0u;  // (vec4* out, const hkTransform*, const vec4* in): to local direction
constexpr u32 kRemoveRef = 0x007D4EA8u;        // hkReferencedObject::removeReference (o)
constexpr u32 kHingeCtor = 0x0090CE9Cu;        // limited hinge constraint data ctor (256 bytes)
constexpr u32 kHingeSetWorld = 0x0090CC18u;    // (data, transform A, transform B, const vec4* pivot, const vec4* axis)
constexpr u32 kHingeSetMotor = 0x0090CA30u;    // (data, motor)
constexpr u32 kHingeMotorOn = 0x0090CBC0u;     // (data, constraint instance, u8 on)
constexpr u32 kMotorCtor = 0x00915E60u;        // motor ctor (28 bytes; f1, f2, f3 max force)
constexpr u32 kInstanceCtor = 0x0090AA54u;     // constraint instance ctor (56 bytes: inst, entity A, entity B, data, priority)
constexpr u32 kWorldAddConstraint = 0x00933938u;  // (hkpWorld*, constraint instance)

constexpr s32 kTlsRouter = -28332;             // r13 +: hkMemoryRouter* (+44 the allocator, vt +8 alloc(a, size))
constexpr u32 kAttachK = 0x000E2A60u;          // .text: vsel / vandc lane masks (+0 also f32 0), f32 constants below
constexpr u32 kHalf = kAttachK + 48;           // 0.5
constexpr u32 kNegDeg = kAttachK + 52;         // -pi / 180
constexpr u32 kDeg = kAttachK + 56;            // pi / 180
constexpr u32 kTwo = kAttachK + 60;            // 2
constexpr u32 kMaxForce = kAttachK + 64;       // ~FLT_MAX
constexpr u32 kNoLimit = kAttachK + 68;        // -1
constexpr u32 kLimitScale = kAttachK + 72;     // 1.25

// vmaddfp lane: the fused product-sum rounded to single.
inline f32 Vmadd(f32 a, f32 c, f32 b) { return static_cast<f32>(std::fma(static_cast<f64>(a), static_cast<f64>(c), static_cast<f64>(b))); }

u32 HkAlloc(PPUContext& ctx, u32 size) {
    const u32 al = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + 44);
    const u32 p = static_cast<u32>(VCall(ctx, al, 8, {al, size}));
    // Quirk: the size is stored at +4 before the null test.
    store<u16>(p + 4, static_cast<u16>(size));
    return p;
}
}  // namespace

// void PhysObj_AttachHingeMotor(PhysObj* o, u32 index, const vec4* axis)
// Body index gets a motorised limited hinge to the body its template's
// +100 index names: pivot = the item's position (+48), axis in world space;
// the limit (+80, or +72 * 1.25, none when negative) in degrees; the motor
// strength from the item's +64 / +68 and the body's inertia about the axis
// (halved unless the other body's motion type is 4 or 5). The constraint is
// added to the body's world and its motor switched on.
DECOMP(000E2AB0, PhysObj_AttachHingeMotor0E2AB0) {
    const u32 o = arg<u32>(ctx, 0), idx = arg<u32>(ctx, 1), axis = arg<u32>(ctx, 2);
    const u32 entry = load<u32>(load<u32>(o + 388)) + idx * 48;   // body entries (48 bytes)
    const u32 df = load<u32>(o + 216);
    const u32 o160 = load<u32>(o + 160);
    const s32 ti = static_cast<s32>(load<u32>(entry + 20));
    u32 tmpl = 0;
    if (o160 != 0) {
        const u32 r = static_cast<u32>(VCall(ctx, o160, 28, {o160}));
        if (r != 0 && ti >= 0) tmpl = load<u32>(r + 32) + (static_cast<u32>(ti) << 4);
    }
    // Quirk: a missing template reads address 0.
    const u32 tdef = load<u32>(tmpl);
    const u32 item = load<u32>(load<u32>(df + 192)) + idx * 160 + 48;
    const s32 other = static_cast<s16>(load<u16>(tdef + 100));
    const u32 bodyB = load<u32>(load<u32>(load<u32>(o + 388)) + static_cast<u32>(other) * 48 + 28);
    const u32 bodyA = load<u32>(entry + 28);

    Stack frame(ctx, 112);
    const u32 sp = frame.addr();
    const u32 vPos = sp, vLocal = sp + 16, vAxis = sp + 32, mInertia = sp + 48;
    store<u32>(vPos + 0, load<u32>(item + 0));
    store<u32>(vPos + 4, load<u32>(item + 4));
    store<u32>(vPos + 8, load<u32>(item + 8));
    store<u32>(vPos + 12, 0);
    call(ctx, kTransformInvPos, {vLocal, bodyA + 240, vPos});
    VCall(ctx, load<u32>(entry + 28) + 224, 24, {load<u32>(entry + 28) + 224, mInertia});   // motion: inertia (local)
    call(ctx, kTransformInvDir, {vAxis, load<u32>(entry + 28) + 240, axis});

    // axis . (I axis) with the vector unit: rows scaled by the axis lanes.
    f32 s[4], p[4];
    for (u32 k = 0; k < 4; ++k) s[k] = load<f32>(vAxis + 4 * k);
    for (u32 k = 0; k < 4; ++k) {
        f32 x = Vmadd(s[0], load<f32>(mInertia + 4 * k), 0.0f);
        x = Vmadd(s[1], load<f32>(mInertia + 16 + 4 * k), x);
        x = Vmadd(s[2], load<f32>(mInertia + 32 + 4 * k), x);
        p[k] = Vmadd(x, s[k], 0.0f);
    }
    f32 inertia = (p[0] + p[1]) + p[2];
    const u8 motion = load<u8>(bodyB + 232);
    const f32 f30 = F(item + 16), f29 = F(item + 20);
    if (motion != 5 && motion != 4) inertia = inertia * F(kHalf);

    const u32 hinge = HkAlloc(ctx, 256);
    u32 data = 0;
    if (hinge != 0) {
        call(ctx, kHingeCtor, {hinge});
        data = hinge;
    }
    call(ctx, kHingeSetWorld, {data, bodyB + 240, load<u32>(entry + 28) + 240, vLocal, axis});
    const f32 zero = F(kAttachK);
    f32 lim = F(item + 32);
    if (!(lim >= zero)) {
        const f32 alt = F(item + 24);
        lim = alt >= zero ? alt * F(kLimitScale) : F(kNoLimit);
    }
    if (lim >= zero) {
        Stfs(data + 212, lim * F(kNegDeg));
        Stfs(data + 216, lim * F(kDeg));
    }
    const f32 strength = (f30 * f30) * inertia;
    const f32 damping = ((f30 * F(kTwo)) * f29) * inertia;
    ctx.fpr[1] = inertia;   // as the original (fmr f1, f31 before the allocation; left there when it fails)
    const u32 mem = HkAlloc(ctx, 28);
    u32 motor = 0;
    if (mem != 0) {
        callf(ctx, kMotorCtor, {mem}, {strength, damping, F(kMaxForce)});
        motor = mem;
    }
    call(ctx, kHingeSetMotor, {data, motor});
    call(ctx, kRemoveRef, {motor});
    const u32 im = HkAlloc(ctx, 56);
    u32 inst = 0;
    if (im != 0) {
        call(ctx, kInstanceCtor, {im, bodyB, load<u32>(entry + 28), data, 1});
        inst = im;
    }
    call(ctx, kWorldAddConstraint, {load<u32>(load<u32>(entry + 28) + 8), inst});
    call(ctx, kHingeMotorOn, {data, inst, 1});
    call(ctx, kRemoveRef, {data});
    call(ctx, kRemoveRef, {inst});
}

namespace {
constexpr u32 kSmCtor = 0x0045CBE8u;        // StateMachine_ConstructThunk (o, u32, u32, 0)
constexpr u32 kHashNoCase = 0x0056A710u;    // String_HashNoCase (const char*) -> u32
constexpr u32 kApplyQuery = 0x000D2F04u;    // Obj_ApplyQuery0D2F04 (o, tmpl)
constexpr u32 kSetAnimSet = 0x000D1B10u;    // Obj_SetAnimSet0D1B10 (o, tmpl)
constexpr u32 kCompCreate = 0x002635E8u;    // Comp_Create2635E8 (o, def) -> component or 0
constexpr u32 kFindComp = 0x000D1288u;      // Obj_FindComponent (o, kind, 0)
constexpr u32 kAnimReset = 0x002980ACu;     // Anim_Reset2980AC (anim component, u32)
constexpr u32 kWorldCounter = 0x004A2560u;  // World_Counter_004A2560 () -> u32 (a fresh id)
constexpr u32 kSmallAlloc = 0x0055238Cu;    // SmallAlloc (size)
constexpr u32 kPtrVecCtor = 0x005516E0u;    // PtrVector_Construct (v)
constexpr u32 kSetFlags64 = 0x007348D8u;    // (o, u64 mask, 1, 1) (name?)

constexpr u32 kObjVt = 0x00B12278u;         // object base vtable
constexpr u32 kBaseK = 0x000D3560u;         // .text: f32 0, f32 -1, u64 flag masks (+8, +16)
constexpr u32 kNoParentList = 0x01013B34u;  // PtrVector of objects whose +172 bit 0x10 is clear (name?)

// Template fields.
constexpr u32 kTAnim = 0;                   // anim set (u16 +8 count) or 0
constexpr u32 kTPos = 4;                    // f32[3]
constexpr u32 kTSmA = 16;
constexpr u32 kTFlags = 20;                 // bits 17..31: state machine arg; flag bits below
constexpr u32 kTByte = 24;
constexpr u32 kTName = 28;                  // const char*
constexpr u32 kTDef = 32;                   // class def (u8 +32 flags, +16 PtrVector of component defs)
constexpr u32 kTRadius = 128;
constexpr u32 kTRange = 132;                // f32: squared into +208 (or -1 when negative)
constexpr u32 kTId = 140;                   // 0 / -1: a fresh id

// The flags word at +172 built from the template flags, one stw per step as
// the original writes it.
u32 BaseFlags(u32 o, u32 t) {
    const u8 b = load<u8>(t + kTByte);
    u32 w = load<u32>(o + 172);
    store<u8>(o + 172, b);
    w = (w & 0x00FFFFFFu) | (static_cast<u32>(b) << 24);
    w = (w & ~0x00C00000u) | 0x00800000u;
    w &= ~0x00380000u;
    w |= 0x00040000u;
    store<u32>(o + 172, w);
    w = (w & ~0x00020000u) | ((load<u32>(t + kTFlags) & 0x800u) << 6);
    store<u32>(o + 172, w);
    w = (w & ~0x00010000u) | ((load<u32>(t + kTFlags) & 0x200u) << 7);
    store<u32>(o + 172, w);
    {
        const u32 f = load<u32>(t + kTFlags);
        w = (w & ~0x8000u) | ((f & 0x100u) << 7);
        w = (w & ~0x4000u) | ((f & 0x100u) << 6);
        w &= ~0x2000u;
    }
    store<u32>(o + 172, w);
    w = (w & ~0x1000u) | ((load<u32>(t + kTFlags) & 0x80u) << 5);
    w &= ~0x0E00u;
    store<u32>(o + 172, w);
    w = (w & ~0x100u) | ((load<u32>(t + kTFlags) & 0x2000u) >> 5);
    store<u32>(o + 172, w);
    w = (w & ~0x80u) | ((load<u32>(t + kTFlags) & 0x1000u) >> 5);
    w &= ~0x60u;
    store<u32>(o + 172, w);
    w = (w & ~0x10u) | ((load<u32>(t + kTFlags) & 0x40u) >> 2);
    w &= ~0x0Fu;
    store<u32>(o + 172, w);
    return w;
}
}  // namespace

// void Obj_BaseInit(Obj* o, const ObjTemplate* t)
// Base object construction from a template: the state machine, the
// transform (two copies), the name hash, the flag words (+172 / +176), the
// tuning fields, the four (0, -1, 0) links, the template position, the
// query, an id, the class flags, the anim set and the class's components
// (each made by Comp_Create and kept in a lazily created PtrVector at +248),
// then the anim component reset and two flag masks set.
DECOMP(000D3578, Obj_BaseInit0D3578) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1);
    call(ctx, kSmCtor, {o, (load<u32>(t + kTFlags) >> 17) & 0x7FFFu, load<u32>(t + kTSmA), 0});
    // Two copies of the 4x3 transform (rows at +32 / +96).
    u32 m[12];
    static constexpr u32 kSrc[12] = {48, 52, 56, 64, 68, 72, 80, 84, 88, 96, 100, 104};
    static constexpr u32 kDst[12] = {32, 36, 40, 48, 52, 56, 64, 68, 72, 80, 84, 88};
    for (u32 k = 0; k < 6; ++k) { m[k] = load<u32>(t + kSrc[k]); store<u32>(o + kDst[k], m[k]); }
    m[6] = load<u32>(t + kSrc[6]);
    store<u32>(o + 0, kObjVt);
    for (u32 k = 6; k < 11; ++k) {
        if (k > 6) m[k] = load<u32>(t + kSrc[k]);
        store<u32>(o + kDst[k], m[k]);
    }
    m[11] = load<u32>(t + kSrc[11]);
    store<u32>(o + 96, m[0]);
    store<u32>(o + 88, m[11]);
    store<u32>(o + 100, m[1]);
    store<u32>(o + 104, m[2]);
    store<u32>(o + 112, m[3]);
    store<u32>(o + 116, m[4]);
    store<u32>(o + 120, m[5]);
    store<u32>(o + 128, m[6]);
    store<u32>(o + 132, m[7]);
    store<u32>(o + 136, m[8]);
    store<u32>(o + 160, 0);
    store<u32>(o + 144, m[9]);
    store<u32>(o + 164, 0);
    store<u32>(o + 148, m[10]);
    store<u32>(o + 152, m[11]);
    store<u32>(o + 168, static_cast<u32>(call(ctx, kHashNoCase, {load<u32>(t + kTName)})));

    u32 w = BaseFlags(o, t);
    u32 h = load<u16>(o + 176);
    h &= ~0x3800u;
    h |= 0x400u;
    h &= ~0x1F0u;
    store<u16>(o + 176, static_cast<u16>(h));
    store<u16>(o + 178, 1);
    store<u64>(o + 184, 0);
    store<u32>(o + 204, load<u32>(t + kTId));
    const f32 zero = F(kBaseK + 0);
    const f32 r = F(t + kTRange);
    u32 range = load<u32>(kBaseK + 4);
    if (r >= zero) range = FBits(r * r);
    SetF1Bits(ctx, range);   // f1 is left holding it, as in the original
    store<u32>(o + 208, range);
    store<u32>(o + 216, load<u32>(t + kTDef));
    store<u32>(o + 212, 0x3F800000u);
    store<u32>(o + 220, load<u32>(t + kTRadius));
    store<u32>(o + 224, 0x3F800000u);
    store<u32>(o + 228, 0x3F800000u);
    store<u32>(o + 232, 0x3F800000u);
    store<u32>(o + 240, 0x3F800000u);
    store<u32>(o + 244, 0);
    store<u32>(o + 248, 0);
    store<u32>(o + 252, 0);
    for (u32 k = 0; k < 2; ++k) {
        for (u32 l : {256u, 268u}) {
            const u32 a = o + 24 * k + l;
            store<u32>(a + 0, 0);
            store<u16>(a + 2, 0);
            store<u32>(a + 4, 0xFFFFFFFFu);
            store<u32>(a + 8, 0);
        }
    }
    const u32 def0 = load<u32>(o + 216);
    h = load<u16>(o + 176);
    store<u32>(o + 336, 0);
    h &= ~0xC000u;
    store<u32>(o + 340, 0);
    const u32 bit = def0 != 0 ? (load<u32>(def0 + 32) >> 23) & 1u : 0u;
    h = (h & ~0x4000u) | (bit << 14);
    store<u16>(o + 176, static_cast<u16>(h));
    const u32 p0 = load<u32>(t + kTPos), p1 = load<u32>(t + kTPos + 4), p2 = load<u32>(t + kTPos + 8);
    store<u32>(o + 192, p0);
    store<u32>(o + 196, p1);
    store<u32>(o + 200, p2);
    for (u32 l : {256u, 268u, 280u, 292u}) {
        store<u32>(o + l + 0, 0);
        store<u32>(o + l + 4, 0xFFFFFFFFu);
        store<u32>(o + l + 8, 0);
    }
    call(ctx, kApplyQuery, {o, t});
    w = load<u32>(o + 172);
    if (load<u8>(o + 172) != 255) {
        const u64 q = load<u64>(o + 184) | 1u;
        w &= ~0x00C00000u;
        store<u32>(o + 172, w);
        store<u64>(o + 184, q);
    }
    if ((w & 0x10u) == 0) PtrVecPush(ctx, kNoParentList, o);
    const u32 id = load<u32>(t + kTId);
    store<u32>(o + 204, id);
    if (id == 0 || id == 0xFFFFFFFFu) store<u32>(o + 204, static_cast<u32>(call(ctx, kWorldCounter, {})));

    u32 def = load<u32>(o + 216);
    if (def != 0) {
        if (load<u8>(def + 32) & 0x04u) store<u32>(o + 172, load<u32>(o + 172) | 0x00020000u);
        const u32 an = load<u32>(t + kTAnim);
        if (an != 0 && load<u16>(an + 8) != 0) {
            call(ctx, kSetAnimSet, {o, t});
            def = load<u32>(o + 216);
        }
        h = load<u16>(o + 176);
        h = (h & ~0x40u) | ((static_cast<u32>(load<u8>(def + 32)) << 5) & 0x40u);
        store<u16>(o + 176, static_cast<u16>(h));
        for (u32 i = 0;; ++i) {
            const u32 vec = load<u32>(load<u32>(o + 216) + 16);
            const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
            if (!(i < n)) break;
            const u32 cd = load<u32>(load<u32>(vec) + 4 * i);
            store<u32>(cd + 8, o);
            const u32 c = static_cast<u32>(call(ctx, kCompCreate, {o, cd}));
            if (c != 0) {
                u32 lst = load<u32>(o + 248);
                if (lst == 0) {
                    lst = static_cast<u32>(call(ctx, kSmallAlloc, {20}));
                    store<u32>(o + 248, lst);
                    if (lst != 0) {
                        call(ctx, kPtrVecCtor, {lst});
                        store<u16>(lst + 6, 4);
                        lst = load<u32>(o + 248);
                    }
                }
                // Quirk: a failed allocation pushes onto a null vector.
                PtrVecPush(ctx, lst, c);
            }
            store<u32>(cd + 8, 0);
        }
    }
    const u32 anim = static_cast<u32>(call(ctx, kFindComp, {o, 2, 0}));
    if (anim != 0) call(ctx, kAnimReset, {anim, load<u32>(o + 164)});
    call(ctx, kSetFlags64, {o, load<u64>(kBaseK + 8), 1, 1});
    call(ctx, kSetFlags64, {o, load<u64>(kBaseK + 16), 1, 1});
}

// World_HandleEvent callees, part 22d: an object's Havok rigid body set-up
// (named by Obj_Construct3430F8). The Havok calls stay recompiled; this is
// the game-side glue that fills the construction info on the stack.
#include "hevent22.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent22;

namespace {
// Havok entry points (recompiled; names are guesses from their use here).
constexpr u32 kHkBodyCinfoCtor = 0x0093FB34;     // (RigidBodyCinfo*) (name?)
constexpr u32 kHkCinfoSetMotion = 0x0093F940;    // (RigidBodyCinfo*, 5) (name?)
constexpr u32 kHkCinfoSetMass = 0x0093FAF8;      // (RigidBodyCinfo*; f32) (name?)
constexpr u32 kHkRigidBodyCtor = 0x00933DF8;     // (RigidBody*, const RigidBodyCinfo*, 0x0132DBDC) (name?)
constexpr u32 kHkBodyAttach8807E8 = 0x008807E8;  // (body +120 object) (name?)
constexpr u32 kHkWorldLock94059C = 0x0094059C;   // (*0x00F64C94) (name?)
constexpr u32 kHkShapeCinfoCtor = 0x0091FB50;    // (ShapeCinfo*) (name?)
constexpr u32 kHkBoxShapeCtor = 0x00899720;      // (BoxShape*, const Vector4* half extents; f32 radius) (name?)
constexpr u32 kHkShapeCtor91E100 = 0x0091E100;   // (Shape*, const ShapeCinfo*) (name?)
constexpr u32 kHkRemoveReference = 0x007D4EA8;   // (obj) (symbols.txt AnimSkel_Release7D4EA8)
constexpr u32 kHkAddEntity = 0x0092F2A8;         // (world, entity, 1) (symbols.txt Hk_AddEntity92F2A8)
constexpr u32 kHkBodyClass = 0x0132DBDC;         // third argument of the rigid body constructor (name?)
constexpr u32 kHkWorldArg = 0x00F64C94;          // u32 given to 0x0094059C (name?)
constexpr u32 kHkCinfoVtable = 0x00B1F280;       // stored into the cinfo when it goes out of scope
constexpr u32 kBoxRadius = 0x00F9203C;           // f32 0.05

// Tuning words read here (data at 0x0106B9F0..): the cinfo +16 vector's y,
// the mass, the box half extents, the shape cinfo vector and its scalar.
constexpr u32 kTuneY = 0x0106BA38, kTuneMass = 0x0106BA84;
constexpr u32 kTuneHalfExt = 0x0106BAE0, kTuneVec = 0x0106BB40, kTuneScalar = 0x0106BB98;

// The thread's Havok memory router: *(r13 - 28332) +44, vt +8 alloc(size).
constexpr s32 kTlsRouter = -28332;
constexpr u32 kRouterAllocator = 44;
// hkReferencedObject: u16 +4 memory size, stored after every allocation
// (also when the allocation failed, at address 4, as the original does).
constexpr u32 kHkObjSize = 4;

// Frame layout (offsets from the 656-byte frame's base, as the original).
constexpr u32 kFrameSize = 656;
constexpr u32 kHalfExt = 112;   // Vector4 box half extents
constexpr u32 kShapeCi = 128;   // shape cinfo: +4 shape, +16 vector, +168 f32, +180 u8
constexpr u32 kBodyCi = 336;    // rigid body cinfo: +16 vector, +41 u8, +84 / +88 / +104 refs, +217 u8

u32 hk_alloc(PPUContext& ctx, u32 size) {
    const u32 a = load<u32>(load<u32>(static_cast<u32>(ctx.gpr[13]) + static_cast<u32>(kTlsRouter)) + kRouterAllocator);
    const u32 p = static_cast<u32>(vcall(ctx, a, 8, {a, size}));
    store<u16>(p + kHkObjSize, static_cast<u16>(size));
    return p;
}
// The splatted-and-masked vectors: (x, y, z, 0) from three tuning words.
void store_xyz0(u32 dst, u32 src) {
    store<u32>(dst + 0, load<u32>(src + 0));
    store<u32>(dst + 4, load<u32>(src + 4));
    store<u32>(dst + 8, load<u32>(src + 8));
    store<u32>(dst + 12, 0);
}
}  // namespace

// void Obj_Start342A30(Obj* o) (name?): builds o's Havok rigid body: a
// stack rigid body cinfo (motion type 5, +16 = (0, y, 0, 0), two u8 fields
// 3, mass) makes a 944-byte body (o +4; its +16..+24 position copied to o
// +80..+88), then a 48-byte box shape (half extents (x, y, z, 0), radius
// 0.05) and a 544-byte shape built from a stack shape cinfo holding it; the
// body is added with Hk_AddEntity92F2A8(body, shape, 1) and the
// references released, then the cinfo's own references (+104, +88, +84).
DECOMP(00342A30, Obj_Start342A30) {
    const u32 o = arg<u32>(ctx, 0);
    Stack frame(ctx, kFrameSize);
    const u32 s = frame.addr();
    const u32 bci = s + kBodyCi;
    call(ctx, kHkBodyCinfoCtor, {bci});
    call(ctx, kHkCinfoSetMotion, {bci, 5});
    store<u8>(s + 553, 3);
    store<u8>(s + 377, 3);
    store<u32>(bci + 16, 0);  // vand / vandc masks keep word 1 only
    store<u32>(bci + 20, load<u32>(kTuneY));
    store<u32>(bci + 24, 0);
    store<u32>(bci + 28, 0);
    callf(ctx, kHkCinfoSetMass, {bci}, {hevent7::lfs_f64(kTuneMass)});

    u32 body = hk_alloc(ctx, 944);
    if (body != 0) {
        call(ctx, kHkRigidBodyCtor, {body, bci, kHkBodyClass});
    }
    store<u32>(o + 4, body);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 80 + k, load<u32>(body + 16 + k));  // lfs / stfs: raw copy
    call(ctx, kHkBodyAttach8807E8, {load<u32>(load<u32>(o + 4) + 120)});
    call(ctx, kHkWorldLock94059C, {load<u32>(kHkWorldArg)});
    call(ctx, kHkShapeCinfoCtor, {s + kShapeCi});

    store_xyz0(s + kHalfExt, kTuneHalfExt);
    const u32 box_mem = hk_alloc(ctx, 48);
    u32 box = 0;
    if (box_mem != 0) {
        callf(ctx, kHkBoxShapeCtor, {box_mem, s + kHalfExt}, {hevent7::lfs_f64(kBoxRadius)});
        box = box_mem;
    }
    store<u32>(s + kShapeCi + 4, box);
    store<u8>(s + 308, 5);
    store<u32>(s + 296, load<u32>(kTuneScalar));  // lfs / stfs: raw copy
    ctx.fpr[1] = hevent7::lfs_f64(kTuneScalar);    // and left in f1
    store_xyz0(s + 144, kTuneVec);
    const u32 shape_mem = hk_alloc(ctx, 544);
    u32 shape = 0;
    if (shape_mem != 0) {
        call(ctx, kHkShapeCtor91E100, {shape_mem, s + kShapeCi});
        shape = shape_mem;
    }
    call(ctx, kHkRemoveReference, {box});
    call(ctx, kHkAddEntity, {load<u32>(o + 4), shape, 1});
    call(ctx, kHkRemoveReference, {shape});

    // The cinfo's destructor, inlined: vtable, then its three references.
    const u32 ref104 = load<u32>(bci + 104);
    store<u32>(bci, kHkCinfoVtable);
    if (ref104 != 0) call(ctx, kHkRemoveReference, {ref104});
    const u32 ref88 = load<u32>(bci + 88);
    store<u32>(bci + 104, 0);
    if (ref88 != 0) call(ctx, kHkRemoveReference, {ref88});
    const u32 ref84 = load<u32>(bci + 84);
    if (ref84 != 0) {
        store<u32>(bci + 88, 0);
        call(ctx, kHkRemoveReference, {ref84});
    } else {
        ctx.gpr[3] = 0;
    }
}

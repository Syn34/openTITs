// World_Destruct (vtable +0): tears the game down in the reverse order of
// World_Construct / World_Init.
#include "vt_world.h"

using namespace sly4;
using namespace sly4::world;

namespace {

u64 vcall(PPUContext& ctx, u32 obj, u32 slot, std::initializer_list<u64> args) {
    const u32 opd = load<u32>(load<u32>(obj) + slot);
    ctx.gpr[3 + args.size()] = opd;
    ctx.gpr[4 + args.size()] = load<u32>(opd);
    return call_ptr(ctx, opd, args);
}

constexpr u32 kFlowGetMovieOwner = 0x00458D6C;
constexpr u32 kMemFreeFrom = 0x0054EE6C, kSmallFree = 0x00552420, kMemset = 0x00726E54;
constexpr u32 kPtrVectorClear = 0x0054EC1C, kNop = 0x0054EC18;

constexpr u32 kMovieRelease = 0x004D22B0;   // (owner) (name?)
constexpr u32 kSaveWrite = 0x000AE784;      // (path, data, size) (name?)
constexpr u32 kSaveDat = 0x00AE84A4;        // "save.dat"
constexpr u32 kWorldUnload = 0x00466678;    // (World*, 0) (name?)
constexpr u32 kSub42A740 = 0x0042A740;
constexpr u32 kSub425BA8 = 0x00425BA8, kSub424F24 = 0x00424F24;  // (obj, 0) / (obj): 176-byte object
constexpr u32 kSub0CE5B0 = 0x000CE5B0;
constexpr u32 kSub596E3C = 0x00596E3C;
constexpr u32 kSub4E3040 = 0x004E3040;      // (0, 0)
constexpr u32 kSub4D3798 = 0x004D3798;
constexpr u32 kSub0AE660 = 0x000AE660, kSub0B2814 = 0x000B2814;
constexpr u32 kSub449A98 = 0x00449A98, kTrophyDestruct = 0x004FB388;
constexpr u32 kSub4EBF5C = 0x004EBF5C, kSub4272F4 = 0x004272F4, kSub3550BC = 0x003550BC;

// Global singletons destroyed (and freed) here: {global, destructor}.
struct Owned { u32 global, destruct; };
constexpr Owned kGlobals[] = {
    {0x00F65BE8, 0x00451A6C}, {0x00F64C38, 0x002EE3C8}, {0x00F657FC, 0x003A01E4}, {0x00F64C20, 0x002DD604}};
constexpr u32 kGlobalFirst = 0x00F557F0, kGlobalFirstDestruct = 0x00018B30;
constexpr u32 kGlobalBuffer = 0x010B7834;   // u32 freed and cleared
constexpr u32 kFadeUi = 0x00F65C88;
constexpr u32 kTrophy = 0x00F668E8;
constexpr u32 kScratch = 0x010BB809;        // 64 bytes, then u8 +64, +65 (name?)

// World fields.
constexpr u32 kSaveFlags = 44;              // u8, bit 7: write save.dat on exit
constexpr u32 kSaveData = 80, kSaveSize = 4064;
constexpr u32 kOwned176 = 0x12847C;
constexpr u32 kVectors = 0x128480, kVectorCount = 13, kVectorSize = 20;
constexpr u32 kObj200 = 0x128200, kObj204 = 0x128204, kObj1FC = 0x1281FC, kWord208 = 0x128208;
constexpr u32 kObj36 = 36;
constexpr u32 kObjs194 = 0x128194, kObj168 = 0x128168, kObjs184 = 0x128184;
constexpr u32 kObjs16C[] = {0x12816C, 0x128170, 0x128174, 0x128178, 0x12817C, 0x128180};
constexpr u32 kVector444 = 0x128444, kVector20C = 0x12820C;
constexpr u32 kBlocks = 0x107470, kBlockCount = 9, kBlockStride = 0x20C80;  // +8 object, +80 vector

void DestroyGlobal(PPUContext& ctx, u32 global, u32 destruct) {
    const u32 p = load<u32>(global);
    if (p == 0) return;
    call(ctx, destruct, {p});
    call(ctx, kMemFreeFrom, {p, 0});
}

void FreeVector(PPUContext& ctx, u32 v) {
    call(ctx, kPtrVectorClear, {v});
    call(ctx, kNop, {v});
}

}  // namespace

// void World_Destruct(World* w)
DECOMP(00466860, World_Destruct) {
    const u32 w = arg<u32>(ctx, 0);
    store<u32>(w, kWorldVtable);
    const u32 owner = static_cast<u32>(call(ctx, kFlowGetMovieOwner, {w}));
    if (owner != 0) call(ctx, kMovieRelease, {owner});
    vcall(ctx, w, kVfnSetState, {w, 0, kSmNone});

    for (u32 i = 0; (ref<u8>(w + kSaveFlags) >> 7) != 0 && i < 1; ++i)
        call(ctx, kSaveWrite, {kSaveDat, w + kSaveData + i * kSaveSize, kSaveSize});
    call(ctx, kWorldUnload, {w, 0});
    call(ctx, kSub42A740);

    if (load<u32>(w + kOwned176) != 0) {
        call(ctx, kSub425BA8, {load<u32>(w + kOwned176), 0});
        const u32 o = load<u32>(w + kOwned176);
        if (o != 0) {
            call(ctx, kSub424F24, {o});
            call(ctx, kSmallFree, {o, 176});
            store<u32>(w + kOwned176, 0);
        }
    }
    call(ctx, kSub0CE5B0);
    if (const u32 p = load<u32>(kGlobalBuffer)) {
        call(ctx, kMemFreeFrom, {p, 0});
        store<u32>(kGlobalBuffer, 0);
    }
    for (u32 k = 0; k < kVectorCount; ++k) store<u16>(w + kVectors + k * kVectorSize + 8, 0);

    DestroyGlobal(ctx, kGlobalFirst, kGlobalFirstDestruct);
    for (const Owned& g : kGlobals) DestroyGlobal(ctx, g.global, g.destruct);

    if (const u32 o = load<u32>(w + kObj200)) {
        call(ctx, kSub596E3C, {o});
        store<u32>(w + kObj200, 0);
    }
    call(ctx, kSub4E3040, {0, 0});
    for (u32 off : {kObj204, kObj1FC}) {
        if (const u32 o = load<u32>(w + off)) {
            vcall(ctx, o, 8, {o});
            store<u32>(w + off, 0);
        }
    }
    call(ctx, kSub4D3798);
    const u32 o36 = load<u32>(w + kObj36);
    store<u32>(w + kWord208, 0);
    if (o36 != 0) {
        vcall(ctx, o36, 4, {o36});
        store<u32>(w + kObj36, 0);
    }
    call(ctx, kSub0AE660);
    call(ctx, kSub0B2814);
    store<u32>(kFadeUi, 0);
    for (u32 i = 0; i < 4; ++i) {
        const u32 slot = w + kObjs194 + i * 4;
        if (const u32 o = load<u32>(slot)) {
            vcall(ctx, o, 8, {o});
            store<u32>(slot, 0);
        }
    }
    // These are destroyed but their slots are left as they were.
    if (const u32 o = load<u32>(w + kObj168)) vcall(ctx, o, 8, {o});
    for (u32 i = 0; i < 4; ++i)
        if (const u32 o = load<u32>(w + kObjs184 + i * 4)) vcall(ctx, o, 8, {o});
    for (u32 off : kObjs16C)
        if (const u32 o = load<u32>(w + off)) vcall(ctx, o, 8, {o});

    call(ctx, kSub449A98);
    DestroyGlobal(ctx, kTrophy, kTrophyDestruct);
    call(ctx, kSub4EBF5C);
    call(ctx, kSub4272F4);
    call(ctx, kMemset, {kScratch, 0, 64});
    ref<u8>(kScratch + 64) = 0;
    ref<u8>(kScratch + 65) = 0;
    call(ctx, kSub3550BC);

    for (s32 k = kVectorCount - 1; k >= 0; --k) FreeVector(ctx, w + kVectors + static_cast<u32>(k) * kVectorSize);
    FreeVector(ctx, w + kVector444);
    FreeVector(ctx, w + kVector20C);
    for (s32 k = 0; k < static_cast<s32>(kBlockCount); ++k) {
        const u32 b = w + kBlocks - static_cast<u32>(k) * kBlockStride;
        if (const u32 o = load<u32>(b + 8)) {
            vcall(ctx, o, 40, {o, 0});
            store<u32>(b + 8, 0);
        }
        FreeVector(ctx, b + 80);
    }

    // ~StateMachine.
    const u32 ids = load<u32>(w + kSmIds);
    store<u32>(w, kSmVtable);
    if (ids != 0) call(ctx, kSmallFree, {ids, load<u32>(w + kSmBytes)});
}

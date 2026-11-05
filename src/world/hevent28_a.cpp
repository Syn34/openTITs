// World_HandleEvent callees, part 28a: the geometry set's scene load
// (GeomSet_LoadScene4F2678, called from World_EnterLevel0 and for every
// child scene).
#include "hevent28.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent27;
using namespace sly4::hevent28;

namespace {
// GeomSet (as hevent27_g.cpp): +0 Bank*, +4 / +8 cameras.
constexpr u32 kSetBank = 0, kSetCamB = 4, kSetCamA = 8;
// Scene definition.
constexpr u32 kSceneChildren = 256;   // PtrVector of child scene name hashes (+0 data, u16 +8 count)
constexpr u32 kSceneLoads = 276;      // const char* names, u16 count at +284
constexpr u32 kSceneBanks = 296;      // Entry* (+0 name, +16 f32[3]), u16 count at +304
constexpr u32 kSceneXml = 316;        // const char* xml paths, u16 count at +324
constexpr u32 kSceneHasFog = 656;     // u8: take the four values below
constexpr u32 kSceneFog = 660;        // f32[4]: near, far, start, end
// Display configuration block (0x010C5A20) fog / depth fields.
constexpr u32 kCfg = 0x010C5A20;
constexpr u32 kCfgNear = 136, kCfgFar = 140, kCfgOutNear = 144, kCfgOutFar = 148;
constexpr u32 kCfgStart = 112, kCfgEnd = 116, kCfgOutStart = 120, kCfgOutEnd = 124, kCfgStartSq = 128, kCfgEndSq = 132;
constexpr u32 kBankPos = 0x011944E0;  // f32[3]: position handed to Bank_Open (name?)
constexpr u32 kCamDefKey = 0xFCCCF4CC;     // the default camera def
constexpr u32 kFreeListName = 0x00AEFC1C;  // "ApeTFreeList<T> &ApeTListAllocator<T>::GetAllocator() [with T = ApeCameraDef]"
constexpr u32 kCamFov = 0x425C0000;        // 55.0f at camera +340
constexpr u32 kDefRot = 0x01164C50, kDefPos = 0x01164B40;  // default camera rotation / position (name?)
// Constants in front of the function (0x004F2660): 1e-5, 300, 50, 0, 7, 1.
constexpr u32 kC = 0x004F2660;
constexpr u32 kEps = kC + 0, kDefNear = kC + 4, kDefFar = kC + 8, kZero = kC + 12, kSeven = kC + 16, kOne = kC + 20;

// lfs + fabs + fcmpu: v unless |v| < eps (not for NaN), then the default.
u32 or_default(u32 v, u32 def) {
    return std::fabs(std::bit_cast<f32>(v)) < lf(kEps) ? load<u32>(def) : v;
}
}  // namespace

// void GeomSet_LoadScene4F2678(GeomSet* set, Scene* scene, u32 a) (name?):
// with a bank (+0): the display config's depth / fog fields (from the
// scene when +656, else from the config's own, near / far defaulting to
// 300 / 50 when ~0), each child scene found in the node map loaded
// recursively, banks opened at their positions, xml paths added to set 0,
// the two cameras (+8 / +4) created and given the default camera def
// (0xFCCCF4CC; when there is none, a fresh def from the camera def
// allocator, or else a looking-down-z matrix at (0, 0, 7) and fov 55),
// geometry instances hooked, the +276 scenes loaded (Geom_LoadScene, the
// first with bit 1), the scene itself (top: bit 1 when it had none), and
// the class textures bound. Returns Geom_BindClassTextures' r3 (0 when
// there is no bank).
DECOMP(004F2678, GeomSet_LoadScene4F2678) {
    const u32 set = arg<u32>(ctx, 0), scene = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    if (load<u32>(set + kSetBank) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    if (load<u8>(scene + kSceneHasFog) == 0) {
        const u32 n = or_default(load<u32>(kCfg + kCfgNear), kDefNear);
        const u32 f = or_default(load<u32>(kCfg + kCfgFar), kDefFar);
        store<u32>(kCfg + kCfgOutNear, n);
        const u32 s = load<u32>(kCfg + kCfgStart), e = load<u32>(kCfg + kCfgEnd);
        store<u32>(kCfg + kCfgOutFar, f);
        store<u32>(kCfg + kCfgOutStart, s);
        store<u32>(kCfg + kCfgOutEnd, e);
        store<f32>(kCfg + kCfgStartSq, fmuls(std::bit_cast<f32>(s), std::bit_cast<f32>(s)));
        store<f32>(kCfg + kCfgEndSq, fmuls(std::bit_cast<f32>(e), std::bit_cast<f32>(e)));
        ctx.fpr[1] = std::bit_cast<f32>(f);  // f1 left as the original (lfs)
    } else {
        store<u32>(kCfg + kCfgOutNear, load<u32>(scene + kSceneFog + 0));
        store<u32>(kCfg + kCfgOutFar, load<u32>(scene + kSceneFog + 4));
        const u32 s = load<u32>(scene + kSceneFog + 8);
        store<u32>(kCfg + kCfgOutStart, s);
        store<f32>(kCfg + kCfgStartSq, fmuls(std::bit_cast<f32>(s), std::bit_cast<f32>(s)));
        const u32 e = load<u32>(scene + kSceneFog + 12);
        store<u32>(kCfg + kCfgOutEnd, e);
        store<f32>(kCfg + kCfgEndSq, fmuls(std::bit_cast<f32>(e), std::bit_cast<f32>(e)));
        ctx.fpr[1] = std::bit_cast<f32>(e);
    }

    // Child scenes: looked up twice (as the original); the value found
    // is loaded recursively.
    const u32 kids = scene + kSceneChildren;
    for (u32 i = 0; i < load<u16>(kids + 8); ++i) {
        const u32 key = load<u32>(load<u32>(kids) + 4 * i);
        if (node_lookup(key) == 0) continue;
        const u32 n = node_lookup(key);
        if (n == 0) continue;
        const u32 child = load<u32>(n + hevent6::kNodeValue);
        if (child != 0) call(ctx, 0x004F2678, {set, child, a});
    }

    for (u32 i = 0; i < load<u16>(scene + kSceneBanks + 8); ++i) {
        const u32 bank = load<u32>(set + kSetBank);
        const u32 e = load<u32>(load<u32>(scene + kSceneBanks) + 4 * i);
        for (u32 k = 0; k < 12; k += 4) store<u32>(kBankPos + k, load<u32>(e + 16 + k));
        ctx.fpr[1] = lf(e + 24);  // f1 as the copy leaves it
        call(ctx, h28fn::Bank_Open, {bank, load<u32>(e), 0});
    }

    for (u32 i = 0; i < load<u16>(scene + kSceneXml + 8); ++i) {
        const u32 path = load<u32>(load<u32>(scene + kSceneXml) + 4 * i);
        const u32 s = static_cast<u32>(call(ctx, h28fn::Set_Find, {0}));
        call(ctx, h28fn::Set_AddXmlPath, {s, path, 0});
    }

    if (load<u32>(set + kSetCamA) == 0) store<u32>(set + kSetCamA, static_cast<u32>(call(ctx, h28fn::CameraInstance_New, {0})));
    if (load<u32>(set + kSetCamB) == 0) store<u32>(set + kSetCamB, static_cast<u32>(call(ctx, h28fn::CameraInstance_New, {0})));

    Stack frame(ctx, 80);  // +0 Mat34 (64 bytes), +64 key
    const u32 m = frame.addr(), keyp = m + 64;
    store<u32>(keyp, kCamDefKey);
    u32 def = load<u32>(static_cast<u32>(call(ctx, h28fn::CameraDefMap_Find, {load<u32>(load<u32>(kCamDefs) + 16), keyp})));
    if (def == 0) {
        const u32 alloc = camdef_allocator(ctx, kFreeListName);
        const u32 node = static_cast<u32>(call(ctx, h27fn::Obj_GetField4, {alloc}));
        def = node != 0 ? node + 16 : 0;
    }

    if (def != 0) {
        const u32 cam_a = load<u32>(set + kSetCamA);
        vcall(ctx, cam_a, 44, {cam_a, def});
        const u32 cam_b = load<u32>(set + kSetCamB);
        vcall(ctx, cam_b, 44, {cam_b, def});
    } else {
        // No camera def: a camera at (0, 0, 7) looking along (0, 0, 7)
        // normalised, its up taken from the default rotation.
        const f32 zero = lf(kZero), seven = lf(kSeven), one = lf(kOne);
        ctx.fpr[1] = seven;  // f1 as the original leaves it (not read)
        call(ctx, h28fn::Mat34_FromRotPos, {m, kDefRot, kDefPos});
        f32 x = -fsubs(zero, zero), z = -fsubs(zero, seven);
        const f32 u0 = lf(m + 16), u1 = lf(m + 20), u2 = lf(m + 24);
        f32 len = fmadds(z, z, fmadds(x, x, fmuls(x, x)));
        len = fsqrts(len);
        f32 inv = fdivs(one, len);
        z = fmuls(z, inv);
        x = fmuls(x, inv);
        store<u32>(m + 48, 0);
        store<u32>(m + 52, 0);
        store<u32>(m + 56, std::bit_cast<u32>(seven));
        // side = up x forward, forward = (x, x, z)
        f32 sy = fmsubs(u2, x, fmuls(u0, z));
        f32 sx = fmsubs(u1, z, fmuls(u2, x));
        f32 sz = fmsubs(u0, x, fmuls(u1, x));
        store<f32>(m + 32, x);
        store<f32>(m + 36, x);
        store<f32>(m + 40, z);
        len = fsqrts(fmadds(sz, sz, fmadds(sx, sx, fmuls(sy, sy))));
        inv = fdivs(one, len);
        sy = fmuls(sy, inv);
        sz = fmuls(sz, inv);
        sx = fmuls(sx, inv);
        store<f32>(m + 4, sy);
        store<f32>(m + 8, sz);
        store<f32>(m + 0, sx);
        store<f32>(m + 16, fmsubs(x, sz, fmuls(z, sy)));
        store<f32>(m + 20, fmsubs(z, sx, fmuls(x, sz)));
        store<f32>(m + 24, fmsubs(x, sy, fmuls(x, sx)));
        for (const u32 off : {kSetCamA, kSetCamB}) {
            const u32 cam = load<u32>(set + off);
            for (u32 row = 0; row < 4; ++row)
                for (u32 k = 0; k < 12; k += 4) store<u32>(cam + 16 + 16 * row + k, load<u32>(m + 16 * row + k));
            ctx.fpr[1] = lf(m + 56);  // f1: the last word copied
            call(ctx, h28fn::WorldObj_MarkDirty, {cam});
        }
        store<u32>(load<u32>(set + kSetCamA) + 340, kCamFov);
        store<u32>(load<u32>(set + kSetCamB) + 340, kCamFov);
    }

    call(ctx, h28fn::GeomInstances_HookEffects, {set, scene, a});
    u32 n = 0;
    for (u32 i = 0; i < (n = load<u16>(scene + kSceneLoads + 8)); ++i) {
        const u32 hash = static_cast<u32>(call(ctx, h28fn::String_HashNoCase, {load<u32>(load<u32>(scene + kSceneLoads) + 4 * i)}));
        if (node_lookup(hash) == 0) continue;
        const u32 nd = node_lookup(hash);
        if (nd == 0) continue;
        const u32 sub = load<u32>(nd + hevent6::kNodeValue);
        if (sub != 0) call(ctx, h28fn::Geom_LoadScene, {set, sub, a, i == 0 ? 1u : 0u, 0});
    }
    // The scene itself last (top), flagged first when it loaded no others.
    call(ctx, h28fn::Geom_LoadScene, {set, scene, a, n == 0 ? 1u : 0u, 1});
    call(ctx, h28fn::Geom_BindClassTextures, {set, scene});
}

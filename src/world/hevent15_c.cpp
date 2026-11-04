// World_HandleEvent callees, part 15c: under Player_Respawn12C250 and
// Obj_Release12BF70: the target settings reset, the animator layer pop, the
// physics body mode switch and the coach HUD mode.
#include "hevent15.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent7;
using namespace sly4::hevent15;

namespace {
namespace cfn {
constexpr u32 Quat_FromMatrix = 0x00555478;         // (f32 out[4], const f32 m[4][4]) (symbols.txt)
constexpr u32 Mat34_FromRotPos = 0x00553B4C;        // (Mat34* out, const Mat33*, const vec3*) (symbols.txt)
constexpr u32 Body_SetPose260D70 = 0x00260D70;      // (Body*, const Mat34*, 1) (name?)
constexpr u32 Hk_GetBody = 0x009D19C8;              // (entity) -> rigid body: Havok range (symbols.txt)
constexpr u32 Hk_AddEntity92F2A8 = 0x0092F2A8;      // (world, rigid body, 1): Havok range (name?)
constexpr u32 Hk_RemoveEntity92FFC8 = 0x0092FFC8;   // (u8* out, world, rigid body): Havok range (name?)
constexpr u32 Body_Add931D10 = 0x00931D10;          // (world, body): Havok range (symbols.txt)
constexpr u32 Body_Remove931F00 = 0x00931F00;       // (world, body): Havok range (symbols.txt)
constexpr u32 Obj_Notify24 = 0x000D949C;            // (Object*) -> u32 (symbols.txt)
constexpr u32 Obj_Get0D95EC = 0x000D95EC;           // (Object*) -> u32 (name?)
constexpr u32 Map_Get = 0x0075EA74;                 // (map, const u32* key) -> slot (symbols.txt)
constexpr u32 Game_SetPaused = 0x00365A54;          // (bool resume, 0) (symbols.txt)
constexpr u32 Coach_Leave2A2998 = 0x002A2998;       // (Coach*, u32 old mode) (name?)
constexpr u32 Coach_Enter2A1658 = 0x002A1658;       // (Coach*, u32 new mode) (name?)
constexpr u32 Coach_Refresh2A0F5C = 0x002A0F5C;     // (Coach*) (name?)
}  // namespace cfn

// fsel: a >= 0 ? c : b (NaN a picks b).
inline f64 sel_ge(f64 a, f64 c, f64 b) { return a >= 0.0 ? c : b; }

// ---- Target_Reset0FEACC
// Target: +16 Object* (+192 bit 3: owner settings at owner +1008, +200 bit
// 10: owner settings at owner +432; owner = +216). The settings block (36
// words) lands at +736: three vec3 (w slots skipped) at +736 / +752, then
// +768 .. +852 words, +856 u8, +860 word.
constexpr u32 kTgtObject = 16, kTgtSettings = 736;
constexpr u32 kObjFlags200 = 200, kObjFlag200Bit10 = 0x400;
constexpr u32 kOwnerSettingsA = 1008, kOwnerSettingsB = 432;
// Defaults when there is no owner block: f32 tunables (each value at +72 of
// a 76-byte record, one at +80 / a vec3), in the original's store order.
struct Default { u32 dst, src; };
constexpr Default kTargetDefaults[] = {
    {736, 0x01032418}, {740, 0x010324B0}, {744, 0x010324FC}, {752, 0x01032550}, {756, 0x01032554},
    {760, 0x01032558}, {768, 0x010325A8}, {772, 0x010325F4}, {776, 0x01032640}, {780, 0x0103268C},
    {784, 0x010326D8}, {788, 0x010327BC}, {792, 0x01032808}, {796, 0x01032724}, {800, 0x01032770},
    {804, 0x01032854}, {808, 0x010328A0}, {812, 0x010328EC}, {816, 0x01032938}, {820, 0x01032984},
    {824, 0x010329D0}, {828, 0x01032A1C}, {836, 0x01032FE0}, {840, 0x0103302C}, {832, 0x01035410},
    {844, 0x01033078}, {848, 0x010330C4}, {852, 0x01033110},
};

// ---- Obj_PopLayer (Object animator layers, as hevent9_c.cpp)
constexpr u32 kObjMatrix = 96, kObjLayer = 345, kObjQuery = 356, kObjAnimators = 364;
constexpr u32 kBlendA = 768, kBlendB = 864, kBlendC = 900;
constexpr u32 kZeroConsts = 0x000FBB78;  // f32 0.0; f64 0.0 at +0 / +8
constexpr u32 kQueryTail = 0x01164CA8;   // 4 bytes copied to the query's +96
constexpr u32 kVAnimPop = 36, kVAnimResume = 28, kVAnimBlend = 32, kVPose = 48;
constexpr u32 kQSize = 104, kQPair = 64;

// ---- Body_SetMode
// Body: +28 owner (+4 Havok world), +32 current entity, +36 / +40 / +44
// entities for modes 1-2 / 4 / 5, +48 extra body, +232 flags (bit 29: mode
// 2, bit 30 cleared), +416 flags (bit 31: in the world, bits 28..30 mode,
// bits 26..27 cleared), +420 0, +436 f32 time for modes 1-2, +464 f32 time,
// +480 vec3, +512 Mat33.
constexpr u32 kBodyOwner = 28, kOwnerWorld = 4, kBodyEntity = 32, kBodyExtra = 48;
constexpr u32 kBodyEnt12 = 36, kBodyEnt4 = 40, kBodyEnt5 = 44;
constexpr u32 kBodyFlags232 = 232, kBodyFlags416 = 416, kBody420 = 420;
constexpr u32 kBodyTime12 = 436, kBodyTime = 464, kBodyPos = 480, kBodyRot = 512;
constexpr u32 kInWorld = 0x80000000u, kModeMask = 0x70000000u, kBits26_27 = 0x0C000000u;
constexpr u32 kMode2Flag = 0x20000000u, kFlag232Bit30 = 0x40000000u;
constexpr u32 kTime4 = 0x0105C360, kTime5 = 0x0105C424;  // f32 tunables (0x0105C310 +80, 0x0105C3D0 +84)
constexpr u32 kHkEntityAdded = 8;                          // rigid body / extra body +8: in a world

// ---- Coach_SetMode2A8AE0
// Coach: +8 Object*, +176 speaker object, +184 text element (+212 u32
// chars, +224 f32 fade), +244 flags (bits 28..31 sub-mode, 20..27 mode, bit
// 15, bit 14: owner flag passed on, bit 13: pass it, bit 11: locked), +400
// f32 time.
constexpr u32 kCoachObj = 8, kCoachSpeaker = 176, kCoachText = 184, kCoachFlags = 244, kCoachTime = 400;
constexpr u32 kCoachLocked = 0x800, kCoachPass = 0x2000, kCoachOwnerBit = 0x4000, kCoachBit15 = 0x8000;
constexpr u32 kModeShift = 20, kModeField = 0x0FF00000u, kSubShift = 28;
constexpr u32 kOwner2338 = 2338;
constexpr u8 kOwner2338Bit6 = 0x40;
constexpr u32 kSpeakerMap = 0x0106C2A8;   // key -> speaker (+20 u8[4] colour) (as hevent5_f.cpp)
constexpr u32 kDefaultColor = 0x01164CC4; // u8[4]
constexpr u32 kEmptyString = 0x00AD4B7C;  // ""
constexpr u32 kPerChar = 0x002A8AD8, kTimeBase = 0x002A8ADC;  // f32 0.08, 0.01
constexpr u32 kTextFade = 0x3DA3D70A;     // f32 0.08
constexpr u32 kOne = 0x3F800000;

u32 mode_of(u32 f) { return (f >> kModeShift) & 0xFF; }
u32 with_mode(u32 f, u32 m) { return (f & ~kModeField) | ((m << kModeShift) & kModeField); }
}  // namespace

// void Target_Reset0FEACC(Target* t) (name?): copies the target's settings
// (+736 .. +860) from its object's owner (+216) block at +1008 when the
// object has +192 bit 3, at +432 when it has +200 bit 10, else (no object
// either) from the default tunables (no +856 / +860 then). f1 / r4 as the
// original leaves them.
DECOMP(000FEACC, Target_Reset0FEACC) {
    const u32 t = arg<u32>(ctx, 0);
    const u32 o = load<u32>(t + kTgtObject);
    u32 src = 0;
    if (o != 0) {
        if ((load<u32>(o + hevent14::kObjFlags192) & hevent14::kObjFlag192Bit3) != 0)
            src = load<u32>(o + hevent14::kObjOwner) + kOwnerSettingsA;
        else if ((load<u32>(o + kObjFlags200) & kObjFlag200Bit10) != 0)
            src = load<u32>(o + hevent14::kObjOwner) + kOwnerSettingsB;
    }
    if (src == 0) {
        for (const Default& d : kTargetDefaults) store<u32>(t + d.dst, load<u32>(d.src));
        ctx.fpr[1] = lfs_f64(0x01033110);
        ctx.gpr[4] = 0x010330C8;
        return;
    }
    for (u32 k : {0u, 4u, 8u, 16u, 20u, 24u}) store<u32>(t + kTgtSettings + k, load<u32>(src + k));
    for (u32 k = 32; k <= 116; k += 4) store<u32>(t + kTgtSettings + k, load<u32>(src + k));
    store<u8>(t + kTgtSettings + 120, load<u8>(src + 120));
    const u32 last = load<u32>(src + 124);
    store<u32>(t + kTgtSettings + 124, last);
    ctx.fpr[1] = lfs_f64(src + 116);
    ctx.gpr[4] = last;
}

// Target* Obj_PopLayer(Object* o, Animator* a; f32 t) (name?):
// removes animator a from o's layer stack (+364, +345 entries); 0 when it is
// not there. Below the top it is just taken out. On top: a is popped (vt
// +36); with none left all three blends get max(t, 0). Otherwise the new top
// resumes, blending (vt +32 with a query of o's matrix) when t > 0 or not
// (vt +28), is asked for its pose (vt +48, 0.0) and the three blends restart
// at max(t, 0) from it (as Obj_BlendPose0FA660). Returns a.
DECOMP(000FBB88, Obj_PopLayer) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const f64 t = farg(ctx, 0);
    const u32 n = load<u8>(o + kObjLayer);
    u32 i = n;
    while (i != 0 && load<u32>(o + kObjAnimators + 4 * i) != a) --i;
    if (i == 0) return ret(ctx, 0u);
    if (i != n) {
        // Shift the entries above down (the count is re-read every step).
        u32 cnt, j = i;
        do {
            cnt = load<u8>(o + kObjLayer);
            store<u32>(o + kObjAnimators + 4 * j, load<u32>(o + kObjAnimators + 4 + 4 * j));
            ++j;
        } while (j < cnt);
        const u32 top = cnt - 1;
        store<u8>(o + kObjLayer, static_cast<u8>(top));
        store<u32>(o + kObjAnimators + 4 + ((top << 2) & 0x3FC), 0);
        return ret(ctx, a);
    }
    vcall(ctx, a, kVAnimPop, {a});
    const u32 c = (load<u8>(o + kObjLayer) - 1u) & 0xFF;
    store<u8>(o + kObjLayer, static_cast<u8>(c));
    if (c == 0) {
        const f32 v = static_cast<f32>(sel_ge(-t, std::bit_cast<f64>(load<u64>(kZeroConsts)), t));
        store<f32>(o + kBlendA, v);
        store<f32>(o + kBlendB, v);
        store<f32>(o + kBlendC, v);
        ctx.fpr[1] = v;
        return ret(ctx, a);
    }

    const f64 zero = lfs_f64(kZeroConsts);
    Stack frame(ctx, 16 + kQSize);  // quat out at r1+112, query at r1+128
    const u32 quat = frame.addr(), q = frame.addr() + 16;
    // The identity matrix and 55.0 / 7.5 the original writes first are
    // overwritten before any use (dead stores, left out).
    for (u32 row = 0; row < 4; ++row)
        for (u32 k = 0; k < 12; k += 4) store<u32>(q + 16 * row + k, load<u32>(o + kObjMatrix + 16 * row + k));
    store<u32>(q + kQPair + 0, load<u32>(o + kObjQuery + 0));
    store<u32>(q + kQPair + 4, load<u32>(o + kObjQuery + 4));
    store<u8>(q + 72, 0);
    store<u32>(q + 76, 0);
    store<u32>(q + 80, 0);
    store<u32>(q + 84, 0);
    store<u8>(q + 88, 0);
    store<u32>(q + 92, 0);
    for (u32 k = 0; k < 4; ++k) store<u8>(q + 96 + k, load<u8>(kQueryTail + k));
    {
        const u32 top = load<u32>(o + kObjAnimators + ((c << 2) & 0x3FC));
        if (t > zero) vcall(ctx, top, kVAnimBlend, {top, q});
        else vcall(ctx, top, kVAnimResume, {top});
    }
    {
        // (this, f32 0.0, Query*): the float takes r4's slot, which holds
        // the OPD as in the original; r6 its entry.
        const u32 top = load<u32>(o + kObjAnimators + ((static_cast<u32>(load<u8>(o + kObjLayer)) << 2) & 0x3FC));
        const u32 opd = load<u32>(load<u32>(top) + kVPose);
        ctx.fpr[1] = zero;
        ctx.gpr[6] = load<u32>(opd);
        call_ptr(ctx, opd, {top, opd, q});
    }
    const f32 w = static_cast<f32>(sel_ge(-t, std::bit_cast<f64>(load<u64>(kZeroConsts + 8)), t));
    const bool restart = static_cast<f64>(w) <= zero;
    store<f32>(o + kBlendA, w);
    const u32 tx = load<u32>(q + 48), ty = load<u32>(q + 52), tz = load<u32>(q + 56);
    store<u32>(o + 736, tx);
    store<u32>(o + 740, ty);
    store<u32>(o + 744, tz);
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 752 + k, load<u32>(kZeroVec + k));
    if (restart) {
        store<u32>(o + 704, tx);
        store<u32>(o + 708, ty);
        store<u32>(o + 712, tz);
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 720 + k, load<u32>(kZeroVec + k));
    }
    call(ctx, cfn::Quat_FromMatrix, {quat, q});
    store<f32>(o + kBlendB, w);
    u32 qv[4];
    for (u32 k = 0; k < 4; ++k) {
        qv[k] = load<u32>(quat + 4 * k);
        store<u32>(o + 832 + 4 * k, qv[k]);
    }
    for (u32 k = 0; k < 12; k += 4) store<u32>(o + 848 + k, load<u32>(kZeroVec + k));
    const u32 pair = load<u32>(q + kQPair);
    if (restart) {
        for (u32 k = 0; k < 4; ++k) store<u32>(o + 800 + 4 * k, qv[k]);
        for (u32 k = 0; k < 12; k += 4) store<u32>(o + 816 + k, load<u32>(kZeroVec + k));
    }
    store<f32>(o + kBlendC, w);
    store<u32>(o + 892, pair);
    store<u32>(o + 896, 0);
    if (restart) {
        store<u32>(o + 884, pair);
        store<u32>(o + 888, 0);
    }
    ctx.fpr[1] = lfs_f64(q + kQPair);  // f1 as the original leaves it
    ret(ctx, a);
}

// u32 Body_SetMode(Body* b, u32 mode) (name?): switches the physics body
// to mode (0 off, 1 / 2 normal, 4, 5). With an entity (+32): modes 4 / 5
// (and 1 / 2 coming from 4 / 5) take the entity out of the Havok world when
// it is in (+416 bit 31), switch to that mode's entity (+40 / +44 / +36)
// and time (+464), set the pose from +512 / +480 and put it back; mode 2
// sets +232 bit 29. Then the entity goes in (mode != 0) or out (mode 0) when
// that differs from bit 31, which then follows. Finally +416 takes the mode
// in bits 28..30 (bits 26..27 cleared), +232 bit 30 is cleared and +420 = 0.
// Returns 0.
DECOMP(00261328, Body_SetMode) {
    const u32 b = arg<u32>(ctx, 0), mode = arg<u32>(ctx, 1);
    u32 f = load<u32>(b + kBodyFlags416);
    if (load<u32>(b + kBodyEntity) != 0) {
        Stack frame(ctx, 80);  // r1+112 Mat34, r1+176..179 out bytes
        const u32 mat = frame.addr(), outb = frame.addr() + 64;
        call(ctx, cfn::Mat34_FromRotPos, {mat, b + kBodyRot, b + kBodyPos});
        const u32 f0 = load<u32>(b + kBodyFlags416), h0 = load<u32>(b + kBodyEntity);
        auto world = [&]() { return load<u32>(load<u32>(b + kBodyOwner) + kOwnerWorld); };
        // Takes the current entity out (and the extra body) when it is in.
        auto take_out = [&](u32 out) {
            if (h0 == 0 || (f0 & kInWorld) == 0) return;
            const u32 hb = static_cast<u32>(call(ctx, cfn::Hk_GetBody, {h0}));
            const u32 wd = world();
            if (load<u32>(hb + kHkEntityAdded) != 0) call(ctx, cfn::Hk_RemoveEntity92FFC8, {out, wd, hb});
            const u32 x = load<u32>(b + kBodyExtra);
            if (x != 0 && load<u32>(x + kHkEntityAdded) != 0) call(ctx, cfn::Body_Remove931F00, {wd, x});
        };
        // Switches to entity e with the time word at timea, sets the pose and
        // puts the new entity in.
        auto swap = [&](u32 e, u32 timea) {
            const u32 fl = load<u32>(b + kBodyFlags416) & ~kInWorld;
            const u32 tw = load<u32>(timea);
            store<u32>(b + kBodyEntity, e);
            store<u32>(b + kBodyFlags416, fl);
            store<u32>(b + kBodyTime, tw);
            ctx.fpr[1] = lfs_f64(timea);
            call(ctx, cfn::Body_SetPose260D70, {b, mat, 1});
            const u32 h = load<u32>(b + kBodyEntity);
            if (h != 0 && (load<u32>(b + kBodyFlags416) & kInWorld) == 0) {
                const u32 hb = static_cast<u32>(call(ctx, cfn::Hk_GetBody, {h}));
                const u32 wd = world();
                const u32 x = load<u32>(b + kBodyExtra);
                if (x != 0) call(ctx, cfn::Body_Add931D10, {wd, x});
                call(ctx, cfn::Hk_AddEntity92F2A8, {wd, hb, 1});
            }
            store<u32>(b + kBodyFlags416, load<u32>(b + kBodyFlags416) | kInWorld);
        };
        if (mode == 5) {
            take_out(outb + 1);
            swap(load<u32>(b + kBodyEnt5), kTime5);
        } else if (mode == 4) {
            take_out(outb + 2);
            swap(load<u32>(b + kBodyEnt4), kTime4);
        } else if (mode == 2 || mode == 1) {
            if (mode == 2) store<u32>(b + kBodyFlags232, load<u32>(b + kBodyFlags232) | kMode2Flag);
            const u32 cur = (f0 >> 28) & 7;
            if (cur == 4 || cur == 5) {
                take_out(outb + 3);
                swap(load<u32>(b + kBodyEnt12), b + kBodyTime12);
            }
        }
        const u32 on = mode != 0 ? 1u : 0u;
        const u32 h = load<u32>(b + kBodyEntity);
        if (h != 0 && (on ^ (load<u32>(b + kBodyFlags416) >> 31)) != 0) {
            const u32 hb = static_cast<u32>(call(ctx, cfn::Hk_GetBody, {h}));
            const u32 wd = world();
            if (mode != 0) {
                const u32 x = load<u32>(b + kBodyExtra);
                if (x != 0) call(ctx, cfn::Body_Add931D10, {wd, x});
                call(ctx, cfn::Hk_AddEntity92F2A8, {wd, hb, 1});
            } else {
                if (load<u32>(hb + kHkEntityAdded) != 0) call(ctx, cfn::Hk_RemoveEntity92FFC8, {outb, wd, hb});
                const u32 x = load<u32>(b + kBodyExtra);
                if (x != 0 && load<u32>(x + kHkEntityAdded) != 0) call(ctx, cfn::Body_Remove931F00, {wd, x});
            }
        }
        f = (load<u32>(b + kBodyFlags416) & ~kInWorld) | (on << 31);
    }
    f = (f & ~kModeMask) | ((mode & 7) << 28);
    f &= ~kBits26_27;
    const u32 w232 = load<u32>(b + kBodyFlags232) & ~kFlag232Bit30;
    store<u32>(b + kBody420, 0);
    store<u32>(b + kBodyFlags416, f);
    store<u32>(b + kBodyFlags232, w232);
    ctx.gpr[4] = w232;
    ret(ctx, 0u);
}

// u32 Coach_SetMode2A8AE0(Coach* c, u8 enter, u32 sub, u8 pause) (name?):
// does nothing when leaving (!enter) a locked coach (+244 bit 11). Passes
// the owner flag (+244 bit 14, once, when bit 13 is set) to the object's
// Obj_Notify24 target (vt +44) and marks it (+2338 bit 6); shows the speaker
// (vt +88, 1), colours the text element by the speaker's map colour (vt
// +56), clears its text and times it by its length (+400 = chars * 0.08 +
// 0.01, else 1.0). Entering stores the sub-mode and switches to mode 1
// (pause: unpauses the game first) or 3. Leaving with pause: from mode 1 / 2
// only sets bit 15, from 3 goes to mode 4 (sub != 0) or 5; without pause it
// goes to mode 0; then the sub-mode is stored, the object's 0x000D95EC
// target gets vt +44 (0) and the coach refreshes.
DECOMP(002A8AE0, Coach_SetMode2A8AE0) {
    const u32 c = arg<u32>(ctx, 0);
    const u8 enter = arg<u8>(ctx, 1);
    const u32 sub = arg<u32>(ctx, 2);
    const u8 pause = arg<u8>(ctx, 3);
    if (enter == 0 && (load<u32>(c + kCoachFlags) & kCoachLocked) != 0) return ret(ctx, 1u);
    const u32 obj = load<u32>(c + kCoachObj);
    if (obj != 0) {
        const u32 n = static_cast<u32>(call(ctx, cfn::Obj_Notify24, {obj}));
        if (n != 0) {
            const u32 fl = load<u32>(c + kCoachFlags);
            if ((fl & kCoachPass) != 0) {
                vcall(ctx, n, 44, {n, (fl & kCoachOwnerBit) != 0 ? 1u : 0u});
                store<u32>(c + kCoachFlags, load<u32>(c + kCoachFlags) & ~kCoachPass);
            }
            store<u8>(n + kOwner2338, load<u8>(n + kOwner2338) | kOwner2338Bit6);
        }
    }
    const u32 spk = load<u32>(c + kCoachSpeaker);
    if (spk != 0) {
        vcall(ctx, spk, 88, {spk, 1});
        Stack frame(ctx, 16);  // the key at r1+112
        store<u32>(frame.addr(), 0);
        u32 col = load<u32>(kDefaultColor);
        const u32 slot = static_cast<u32>(call(ctx, cfn::Map_Get, {kSpeakerMap, frame.addr(), kDefaultColor, 0}));
        const u32 sp = load<u32>(slot);
        const u32 text = load<u32>(c + kCoachText);
        if (sp != 0) col = load<u32>(sp + 20);
        store<u32>(c + kCoachTime, kOne);
        if (text != 0) {
            const u32 copd = load<u32>(load<u32>(text) + 56);
            ctx.gpr[5] = load<u32>(copd);  // the entry; r4 carries the colour
            call_ptr(ctx, copd, {text, col});
            const u32 t2 = load<u32>(c + kCoachText);
            vcall(ctx, t2, 192, {t2, kEmptyString});
            const u32 t3 = load<u32>(c + kCoachText);
            const f32 nch = static_cast<f32>(static_cast<f64>(load<u32>(t3 + 212)));
            const f64 per = lfs_f64(kPerChar), base = lfs_f64(kTimeBase);
            const f32 tm = static_cast<f32>(std::fma(static_cast<f64>(nch), per, base));
            store<f32>(c + kCoachTime, tm);
            ctx.fpr[1] = tm;  // f1 as the original leaves it
            store<u32>(t3 + 224, kTextFade);
        }
    }
    if (enter != 0) {
        const u32 old = load<u32>(c + kCoachFlags);
        const u32 fl = (old & 0x0FFFFFFFu) | (sub << kSubShift);
        store<u32>(c + kCoachFlags, fl);
        if (pause == 0) {
            const u32 m = mode_of(fl);
            if (m == 3) return ret(ctx, old);
            call(ctx, cfn::Coach_Leave2A2998, {c, m});
            call(ctx, cfn::Coach_Enter2A1658, {c, 3});
            const u32 v = with_mode(load<u32>(c + kCoachFlags), 3);
            store<u32>(c + kCoachFlags, v);
            return ret(ctx, v);
        }
        call(ctx, cfn::Game_SetPaused, {0, 0});
        const u32 m = mode_of(load<u32>(c + kCoachFlags));
        if (m == 1) return ret(ctx, 1u);
        call(ctx, cfn::Coach_Leave2A2998, {c, m});
        call(ctx, cfn::Coach_Enter2A1658, {c, 1});
        store<u32>(c + kCoachFlags, with_mode(load<u32>(c + kCoachFlags), 1));
        return ret(ctx, 1u);
    }
    u32 fl = load<u32>(c + kCoachFlags);
    const u32 m = mode_of(fl);
    auto go = [&](u32 to) {
        call(ctx, cfn::Coach_Leave2A2998, {c, m});
        call(ctx, cfn::Coach_Enter2A1658, {c, to});
        fl = with_mode(load<u32>(c + kCoachFlags), to);
    };
    if (pause != 0) {
        if (m == 1 || m == 2) fl |= kCoachBit15;
        else if (m == 3) go(sub != 0 ? 4u : 5u);
    } else if (m != 0) {
        go(0);
    }
    const u32 nf = (fl & 0x0FFFFFFFu) | (sub << kSubShift);
    const u32 o2 = load<u32>(c + kCoachObj);
    store<u32>(c + kCoachFlags, nf);
    if (o2 != 0) {
        const u32 x = static_cast<u32>(call(ctx, cfn::Obj_Get0D95EC, {o2}));
        if (x != 0) vcall(ctx, x, 44, {x, 0});
    }
    call(ctx, cfn::Coach_Refresh2A0F5C, {c});
}

// World_HandleEvent callees, part 29c: the radar HUD object's constructor
// (one of the +80 bit 15 kinds made by Objects_CreateFromDefs35AC7C).
#include "hevent29.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent29;

namespace {
namespace fn29c {
constexpr u32 Obj_BaseConstruct = 0x0035695C;    // (Object*, u32 kind, Def*) (Obj_BaseConstruct35695C)
constexpr u32 String_Format = 0x005534D0;        // (fmt, ...) -> rotating static buffer
constexpr u32 String_HashNoCase = 0x0056A710;    // (const char*) -> u32
constexpr u32 WorldObj_FindByHash = 0x005801DC;  // (u32 hash, 0) -> WorldObj*
constexpr u32 Obj_CollectLodGroup = 0x00590FEC;  // (obj, u8 recurse)
constexpr u32 WorldObj_SetName = 0x0057D368;     // (obj, const char*, u8)
constexpr u32 Body_SetMode = 0x004A5C20;         // (obj, u32 group) (Body_SetMode4A5C20)
}  // namespace fn29c

constexpr u32 kVtRadarHud = 0x00B1F9C8;  // vtable (name?)
// Object fields.
constexpr u32 kObjFlags4 = 4;        // +4 bit 0x8000 set
constexpr u32 kObjAnimHashes = 164;  // u32[6] at +168: intro, on-update, idle, outro, alert intro / outro (+164 cleared too)
constexpr u32 kObjScale = 192;       // f32 from def +148
constexpr u32 kObjF196 = 196;        // f32 -1
constexpr u32 kObjTarget = 208;      // kind-8 object found by def +160
constexpr u32 kObjPlayerBlips = 212;    // WorldObj*[9]
constexpr u32 kObjEnemyBlips = 248;     // WorldObj*[12]
constexpr u32 kObjGoalBlips = 296;      // WorldObj*[12]
constexpr u32 kObjGoalEdgeBlips = 344;  // WorldObj*[12]
constexpr u32 kObjHideoutBlip = 392, kObjFollowBlip = 396;
constexpr u32 kObjBottleBlips = 400;  // WorldObj*[32]
constexpr u32 kObjSafeBlip = 528;
constexpr u32 kObjBits = 532;  // bit 31 / 30: def +152 bit 0; bits 29..19 cleared
// Definition fields.
constexpr u32 kDefName = 4, kDefAltName = 36;  // names (+36 when set)
constexpr u32 kDefScale = 148, kDefFlag = 152;
constexpr u32 kDefPlayerTpl = 156, kDefTarget = 160, kDefEnemyTpl = 164, kDefGoalTpl = 168, kDefGoalEdgeTpl = 172;
constexpr u32 kDefHideoutTpl = 176, kDefFollowTpl = 180, kDefBottleTpl = 184, kDefSafeTpl = 188;
constexpr u32 kKindTemplate = 8;  // (obj +4 >> 10) & 63
constexpr u32 kGroupHidden = 13;  // Body_SetMode group for bottle / safe blips (name?)

// Strings (rodata).
constexpr u32 kFmtIntro = 0x00ADC768, kFmtOnUpdate = 0x00ADC778, kFmtIdle = 0x00ADC788, kFmtOutro = 0x00ADC7A8;
constexpr u32 kFmtAlertIntro = 0x00ADC85C, kFmtAlertOutro = 0x00ADC86C;
constexpr u32 kFmtPlayer = 0x00ADC87C;    // "RadarBlipPlayer_%02d"
constexpr u32 kFmtEnemy = 0x00ADC894;     // "RadarBlipEnemy_%02d"
constexpr u32 kFmtGoal = 0x00ADC8AC;      // "RadarBlipGoal_%02d"
constexpr u32 kFmtGoalEdge = 0x00ADC8C4;  // "RadarBlipGoalEdge_%02d"
constexpr u32 kStrHideout = 0x00ADC8DC;   // "RadarBlipHideout"
constexpr u32 kStrFollow = 0x00ADC8F4;    // "RadarBlipFollow"
constexpr u32 kFmtBottle = 0x00ADC904;    // "RadarBlipBottle_%02d"
constexpr u32 kStrSafe = 0x00ADC91C;      // "RadarBlipSafe_01"

u32 kind_of(u32 o) { return (load<u32>(o + 4) >> 10) & 63; }

// The template lookup every blip set starts with. r3 is what the original
// leaves when the template is missing (0) or of the wrong kind (its kind).
bool find_template(PPUContext& ctx, u32 hash, u32& o) {
    o = static_cast<u32>(call(ctx, fn29c::WorldObj_FindByHash, {hash, 0}));
    if (o == 0) return false;
    const u32 k = kind_of(o);
    if (k != kKindTemplate) {
        ctx.gpr[3] = k;
        return false;
    }
    return true;
}

// count instances (vt +56 with kind) of the template, each stored in its
// slot, its LOD group collected and named fmt % (i + 1); bottles also get
// the hidden group.
void make_blips(PPUContext& ctx, u32 tpl, u32 slots, u32 count, u32 kind, u32 fmt, bool group) {
    vcall(ctx, tpl, 40, {tpl, 0, 1});
    for (u32 i = 0; i < count; ++i) {
        const u32 slot = slots + 4 * i;
        const u32 b = static_cast<u32>(vcall(ctx, tpl, 56, {tpl, kind}));
        store<u32>(slot, b);
        call(ctx, fn29c::Obj_CollectLodGroup, {b, 1});
        const u32 obj = load<u32>(slot);  // read before the name is formatted
        const u32 name = static_cast<u32>(call(ctx, fn29c::String_Format, {fmt, i + 1}));
        call(ctx, fn29c::WorldObj_SetName, {obj, name, 1});
        if (group) call(ctx, fn29c::Body_SetMode, {load<u32>(slot), kGroupHidden});
    }
}

// The single blips: one instance in its slot, named name.
void make_blip(PPUContext& ctx, u32 tpl, u32 slot, u32 kind, u32 name, bool group) {
    vcall(ctx, tpl, 40, {tpl, 0, 1});
    const u32 b = static_cast<u32>(vcall(ctx, tpl, 56, {tpl, kind}));
    store<u32>(slot, b);
    call(ctx, fn29c::Obj_CollectLodGroup, {b, 1});
    call(ctx, fn29c::WorldObj_SetName, {load<u32>(slot), name, 1});
    if (group) call(ctx, fn29c::Body_SetMode, {load<u32>(slot), kGroupHidden});
}

u32 anim_hash(PPUContext& ctx, u32 fmt, u32 name) {
    const u32 s = static_cast<u32>(call(ctx, fn29c::String_Format, {fmt, name}));
    return static_cast<u32>(call(ctx, fn29c::String_HashNoCase, {s}));
}
}  // namespace

// void RadarHud_Construct357DC4(Object* this, Def* def) (name?): the base
// object, then the radar fields: the def's +152 flag in bits 31 / 30 of
// +532, scale (+148), the six "<name>_Intro" .. "<name>_AlertOutro" hashes
// when the def is named, and the blip sets instanced from their kind-8
// template objects: 9 player, 12 enemy, 12 goal, 12 goal-edge, the hideout
// and follow blips, 32 bottles and the safe (the last two in group 13).
// The +156, +160, +184 and +188 templates are skipped when 0; the others
// are looked up even then. r3 is what the last step leaves (see below).
DECOMP(00357DC4, RadarHud_Construct357DC4) {
    const u32 self = arg<u32>(ctx, 0), def = arg<u32>(ctx, 1);
    call(ctx, fn29c::Obj_BaseConstruct, {self, 0, def});

    u32 bits = load<u32>(self + kObjBits);
    const u32 scale = load<u32>(def + kDefScale);
    bits = (bits & 0x7FFFFFFFu) | ((load<u8>(def + kDefFlag) & 1u) << 31);
    store<u32>(self + kObjBits, bits);
    bits = (bits & ~0x40000000u) | ((load<u8>(def + kDefFlag) & 1u) << 30);
    store<u32>(self + 0, kVtRadarHud);
    store<u32>(self + kObjScale, scale);
    store<u32>(self + 200, 0);
    store<u32>(self + 204, 0);
    store<u32>(self + kObjF196, 0xBF800000u);  // -1.0f
    store<u32>(self + kObjTarget, 0);
    store<u32>(self + kObjHideoutBlip, 0);
    store<u32>(self + kObjFollowBlip, 0);
    bits &= ~0x3FF80000u;  // bits 29..19
    store<u32>(self + kObjBits, bits);
    store<u32>(self + kObjFlags4, load<u32>(self + kObjFlags4) | 0x8000u);
    for (u32 k = 0; k < 20; k += 4) store<u32>(self + kObjAnimHashes + k, 0);
    ctx.fpr[1] = lfs_reg(scale);  // f1 as the copy leaves it

    u32 name = def + kDefAltName;
    if (load<u8>(name) == 0) name = def + kDefName;
    if (name != 0 && load<u8>(name) != 0) {
        const u32 h = self + kObjAnimHashes + 4;
        store<u32>(h + 0, anim_hash(ctx, kFmtIntro, name));
        store<u32>(h + 4, anim_hash(ctx, kFmtOnUpdate, name));
        store<u32>(h + 8, anim_hash(ctx, kFmtIdle, name));
        store<u32>(h + 12, anim_hash(ctx, kFmtOutro, name));
        store<u32>(h + 16, anim_hash(ctx, kFmtAlertIntro, name));
        store<u32>(h + 20, anim_hash(ctx, kFmtAlertOutro, name));
    }

    u32 tpl;
    u32 hash = load<u32>(def + kDefPlayerTpl);
    if (hash != 0 && find_template(ctx, hash, tpl)) make_blips(ctx, tpl, self + kObjPlayerBlips, 9, 7, kFmtPlayer, false);

    hash = load<u32>(def + kDefTarget);
    if (hash != 0) {
        const u32 o = static_cast<u32>(call(ctx, fn29c::WorldObj_FindByHash, {hash, 0}));
        if (o != 0 && kind_of(o) == kKindTemplate) store<u32>(self + kObjTarget, o);
    }

    for (const u32 blips : {kObjEnemyBlips, kObjGoalBlips, kObjGoalEdgeBlips})
        for (u32 k = 0; k < 48; k += 8) store<u64>(self + blips + k, 0);
    if (find_template(ctx, load<u32>(def + kDefEnemyTpl), tpl)) make_blips(ctx, tpl, self + kObjEnemyBlips, 12, 7, kFmtEnemy, false);
    if (find_template(ctx, load<u32>(def + kDefGoalTpl), tpl)) make_blips(ctx, tpl, self + kObjGoalBlips, 12, 7, kFmtGoal, false);
    if (find_template(ctx, load<u32>(def + kDefGoalEdgeTpl), tpl)) make_blips(ctx, tpl, self + kObjGoalEdgeBlips, 12, 7, kFmtGoalEdge, false);
    if (find_template(ctx, load<u32>(def + kDefHideoutTpl), tpl)) make_blip(ctx, tpl, self + kObjHideoutBlip, 7, kStrHideout, false);
    if (find_template(ctx, load<u32>(def + kDefFollowTpl), tpl)) make_blip(ctx, tpl, self + kObjFollowBlip, 7, kStrFollow, false);

    // Cleared with eight stvx, which drop the low four address bits.
    const u32 vbase = (self + kObjBottleBlips) & ~15u;
    for (u32 k = 0; k < 128; k += 4) store<u32>(vbase + k, 0);
    hash = load<u32>(def + kDefBottleTpl);
    if (hash != 0 && find_template(ctx, hash, tpl)) make_blips(ctx, tpl, self + kObjBottleBlips, 32, 5, kFmtBottle, true);

    // r3 at exit: 0 without a safe template (or when it is not found), its
    // kind when that is not 8, else Body_SetMode's result.
    hash = load<u32>(def + kDefSafeTpl);
    store<u32>(self + kObjSafeBlip, 0);
    ctx.gpr[3] = 0;
    if (hash != 0 && find_template(ctx, hash, tpl)) make_blip(ctx, tpl, self + kObjSafeBlip, 5, kStrSafe, true);
}

// Game code, medium sweep part 34: the scripted sequence starters of three
// state objects (each fires its sequence once: the progress record bumped,
// the scene started, the done-notify sent).
#include "mid34.h"

namespace sly4::obj {

// The 76-byte progress record (Obj_Clear68_0049E860 / Rec_Update).
struct ProgressRec {
    be_s32 count;   // +0
    pad<64> _4;
    be_u32 hash;    // +68
    u8 kind;        // +72
    pad<3> _73;
};
static_assert(sizeof(ProgressRec) == 76 && offsetof(ProgressRec, kind) == 72);

struct SeqObj3FA970 {
    pad<53> _0;
    u8 inactive;    // +53
    pad<33> _54;
    u8 fired;       // +87
    pad<265> _88;
    u8 done;        // +353
    pad<6> _354;
    u8 flag360;     // +360
    u8 flag361;     // +361
};
static_assert(offsetof(SeqObj3FA970, done) == 353 && offsetof(SeqObj3FA970, flag361) == 361);

struct SeqTarget;  // has vfunc 24 (x, 0, 0)
struct SeqObj41DCBC {
    pad<53> _0;
    u8 inactive;    // +53
    pad<33> _54;
    u8 fired;       // +87
    pad<392> _88;
    u8 started;     // +480
    u8 flag481;     // +481
    pad<14> _482;
    gp<SeqTarget> target;  // +496
};
static_assert(offsetof(SeqObj41DCBC, started) == 480 && offsetof(SeqObj41DCBC, target) == 496);

struct EffectOwner;  // vfunc 272 () -> effect set
struct SeqObj411AB0 {
    pad<53> _0;
    u8 inactive;    // +53
    pad<33> _54;
    u8 fired;       // +87
    pad<104> _88;
    gp<EffectOwner> effects;  // +192
    pad<268> _196;
    u8 effect_on;   // +464
    pad<11> _465;
    u8 flag476;     // +476
    u8 once;        // +477
    pad<2> _478;
    gp<SeqTarget> target;  // +480
};
static_assert(offsetof(SeqObj411AB0, effects) == 192 && offsetof(SeqObj411AB0, effect_on) == 464 &&
              offsetof(SeqObj411AB0, once) == 477 && offsetof(SeqObj411AB0, target) == 480);

namespace {
constexpr u32 RecClear34 = 0x0049E860u;       // Obj_Clear68_0049E860(rec)
constexpr u32 RecFind34 = 0x002D9994u;        // Obj_FindRecord76_002D9994(mgr, rec)
constexpr u32 RecUpdate34 = 0x002D98DCu;      // Rec_Update_002D98DC(mgr, rec)
constexpr u32 ResolveBit1_34 = 0x003568E0u;   // Handle_ResolveBit1(hash) -> handle
constexpr u32 PlayerSet34 = 0x003632C8u;      // Player_Set3632C8(handle, text, k)
constexpr u32 StartScene34 = 0x0046C3B8u;     // World_StartScene46C3B8(world, hash, 0 x 6; stack 0, 0)
constexpr u32 HandleNotify34 = 0x0036742Cu;   // Handle_Notify36742C(hash, 0, 1, 0)
constexpr u32 SetSelect34 = 0x00491000u;      // SetSelect_491000(o, hash)
constexpr u32 EffectRelease34 = 0x004CF3C4u;  // Effect_Release(set, hash)
constexpr u32 kRecMgr34 = 0x00F64C20u;        // its +40: the progress record manager
constexpr u32 kFmtNum34 = 0x00ADF3D8u;        // the number format
constexpr u32 kCounterHash34 = 0x35B15B25u;
constexpr u32 kHashStart34 = 0x002237D4u;     // "start"

struct RecMgrHolder {
    pad<40> _0;
    be_u32 mgr;  // +40
};

// The progress record (76 bytes on the stack: cleared, +0 = 1, +68 =
// 0x1B2EEF42, +72 = 13) looked up and its +0 count bumped (Rec_Update),
// then the counter text shown twice (the count with k = 0, then 6 with k
// = 1). Nothing without a record manager.
void BumpRecord34() {
    const u32 m = global<gp<RecMgrHolder>>(kRecMgr34)->mgr;
    if (m == 0) return;
    GuestFrame f(80);
    ProgressRec* rec = f.as<ProgressRec>();
    call(RecClear34, rec);
    rec->kind = 13;
    rec->count = 1;
    rec->hash = 0x1B2EEF42u;
    call(RecFind34, m, rec);
    rec->count = rec->count + 1;
    call(RecUpdate34, m, rec);
    u32 h = call<u32>(ResolveBit1_34, kCounterHash34);
    u32 s = call<u32>(fn::String_Format, kFmtNum34, s32(rec->count));
    call(PlayerSet34, h, s, 0u);
    h = call<u32>(ResolveBit1_34, kCounterHash34);
    s = call<u32>(fn::String_Format, kFmtNum34, 6u);
    call(PlayerSet34, h, s, 1u);
}

void StartScene34f(u32 hash) {
    call(StartScene34, global<be_u32>(kWorld).get(), hash, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
}
}  // namespace

// Fire(o) of the 0x40877911 sequence: skipped (only +360 / +361 set) when
// +53 is set; else +87 = 1 and, the first time (+353 clear), the record
// bump and the scene. Then the notify 0x811E9BA0, SetSelect(o,
// 0x93330CE9), +353 = 1, the object's slot 76 (o), +360 = +361 = 1.
void SeqObj_Fire3FA970(SeqObj3FA970* o) {
    if (o->inactive != 0) {
        o->flag361 = 1;
        o->flag360 = 1;
    } else {
        const u8 done = o->done;
        o->fired = 1;
        if (done == 0) {
            BumpRecord34();
            StartScene34f(0x40877911u);
        }
    }
    call(HandleNotify34, 0x811E9BA0u, 0u, 1u, 0u);
    call(SetSelect34, o, 0x93330CE9u);
    o->done = 1;
    vcall(o, 76);
    o->flag360 = 1;
    o->flag361 = 1;
}

// Fire(o) of the 0x48E4C38D sequence: unless +53 or +480 is set, the
// record bump and the scene (+480 / +481 / +87 = 1 first); with +53 only
// +481 / +480 = 1. Then the notify 0x251B46E4, SetSelect(o, 0x20EB352D)
// and the +496 object's slot 24 (x, 0, 0).
void SeqObj_Fire41DCBC(SeqObj41DCBC* o) {
    if (o->inactive != 0) {
        o->flag481 = 1;
        o->started = 1;
    } else if (o->started == 0) {
        BumpRecord34();
        o->started = 1;
        o->flag481 = 1;
        o->fired = 1;
        StartScene34f(0x48E4C38Du);
    }
    call(HandleNotify34, 0x251B46E4u, 0u, 1u, 0u);
    call(SetSelect34, o, 0x20EB352Du);
    vcall(o->target.get(), 24, 0u, 0u);
}

// Fire(o) of the 0x02E6CA58 sequence (once: nothing when +477 is set):
// with +53 only +477 / +476 = 1, else the record bump, +477 / +87 = 1 and
// the scene. Then the notify 0xBB9F16F3, SetSelect(o, 0xB3000DC2), the
// +480 object's slot 24 (x, 0, 0); when the +192 object's slot 272 gives
// an effect set: +464 = 0 and Effect_Release(slot 272 again, "start").
void SeqObj_Fire411AB0(SeqObj411AB0* o) {
    if (o->once != 0) return;
    if (o->inactive != 0) {
        o->once = 1;
        o->flag476 = 1;
    } else {
        BumpRecord34();
        o->once = 1;
        o->fired = 1;
        StartScene34f(0x02E6CA58u);
    }
    call(HandleNotify34, 0xBB9F16F3u, 0u, 1u, 0u);
    call(SetSelect34, o, 0xB3000DC2u);
    vcall(o->target.get(), 24, 0u, 0u);
    EffectOwner* u = o->effects;
    if (u == nullptr) return;
    if (vcall<u32>(u, 272) == 0) return;
    o->effect_on = 0;
    EffectOwner* u2 = o->effects;
    const u32 e = vcall<u32>(u2, 272);
    call(EffectRelease34, e, kHashStart34);
}

}  // namespace sly4::obj

NATIVE(003FA970, sly4::obj::SeqObj_Fire3FA970)
NATIVE(0041DCBC, sly4::obj::SeqObj_Fire41DCBC)
NATIVE(00411AB0, sly4::obj::SeqObj_Fire411AB0)

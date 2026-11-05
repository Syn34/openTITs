// World_HandleEvent callees, part 20c: small leaves under the travel /
// level / mode paths (a ref-counted target slot, an object search, an
// audio release, a position helper, a pause counter, a layer pop, a line
// trigger, the session's current part and a heap block allocation).
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kTargetReset = 0x000FEACC;        // (Target*) (symbols.txt Target_Reset0FEACC)
constexpr u32 kTargetUpdate0FE828 = 0x000FE828; // (Target*) (name?)
constexpr u32 kWorldObjForEach = 0x0057F5D8;    // (obj, fn opd, arg, bool recurse) (symbols.txt)
constexpr u32 kFindByIdOpd = 0x00F39F58;        // OPD of the WorldObj_ForEach callback below (name?)
constexpr u32 kAudioRelease = 0x00446384;       // (SoundMgr*, Sound*) (symbols.txt Audio_Release446384)
constexpr u32 kSoundMgr = 0x010ACC40;           // SoundMgr given to Audio_Release446384
constexpr u32 kAudioBlock = 0x010C1B88;         // +136 Object* (vt +188), +144 Sound*
constexpr u32 kEmptyName = 0x00AEB4C4;          // "" (rodata)
constexpr u32 kOffset26A42C = 0x0026A42C;       // (f32 out[2+], Obj*, const f32* v) (name?)
constexpr u32 kPauseList = 0x00F64A58;          // u32 first object (next +2568)
constexpr u32 kPauseCount = 0x010592E8;         // s32 pause nesting count
constexpr u32 kPause2AAEBC = 0x002AAEBC;        // (Obj*, u8 paused) (name?)
constexpr u32 kObjPopLayer = 0x000FBB88;        // (Object*, Animator*; f32) (symbols.txt Obj_PopLayer)
constexpr u32 kPopTime = 0x002169BC;            // f32 -1.0 (text constant)
constexpr u32 kTextLookup = 0x0034A398;         // (u32 id) -> Text* (symbols.txt Text_Lookup34A398)
constexpr u32 kSpeakerSay = 0x003514AC;         // (Speaker*, Text*, u8, ..., u32 voice; f1-f3) (symbols.txt)
constexpr u32 kSayZero = 0x004A5C8C;            // f32 0.0 (text constant)
constexpr u32 kWorldLayer = 0x1281E0;           // World +: the layer object (u32)
constexpr u32 kWorldPartKey = 0x128114;         // World +: the current part key (u32)
constexpr u32 kBlockVtable = 0x00B1F4EC;        // the 16-byte block's vtable base (+8 / +40 / +60 stored)
constexpr u32 kBlockPtr = 0x00F64C9C;           // u32 the allocated block (0 on failure)
constexpr u32 kBlockFlag = 0x0106BBE4;          // u8 set before the allocation
constexpr u32 kHeapTls = 28332;                 // r13 - 28332: the thread's heap object pointer
}  // namespace

// void Target_SetRef0FEEBC(Target* t, Ref* x) (name?): t +16 = x (old vt
// +40 (old, 0)); when x is set, its u16 +178 is incremented, then
// Target_Reset0FEACC(t) and 0x000FE828(t).
DECOMP(000FEEBC, Target_SetRef0FEEBC) {
    const u32 t = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 old = load<u32>(t + 16);
    if (old != 0) vcall(ctx, old, 40, {old, 0});
    else ctx.gpr[3] = 0;
    store<u32>(t + 16, x);
    if (x == 0) return;
    const u32 c = load<u16>(x + 178) + 1;
    store<u16>(x + 178, static_cast<u16>(c));
    call(ctx, kTargetReset, {t, x + 178, c});
    call(ctx, kTargetUpdate0FE828, {t});
}

// WorldObj* WorldObj_FindById4A5AE8(WorldObj* o, s32 id) (name?): o itself
// when its +204 == id, else the first match WorldObj_ForEach's callback
// (0x00F39F58) leaves in the {id, found} pair; 0 when o or id is 0.
DECOMP(004A5AE8, WorldObj_FindById4A5AE8) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    if (o == 0 || id == 0) {
        ret(ctx, 0);
        return;
    }
    if (static_cast<s32>(load<u32>(o + 204)) == static_cast<s32>(id)) {
        ret(ctx, o);
        return;
    }
    Stack frame(ctx, 16);
    const u32 pair = static_cast<u32>(ctx.gpr[1]);
    store<u32>(pair, id);
    store<u32>(pair + 4, 0);
    call(ctx, kWorldObjForEach, {o, kFindByIdOpd, pair, 0});
    ret(ctx, load<u32>(pair + 4));
}

// void Audio_Stop4A9084() (name?): the block at 0x010C1B88: object +136 gets
// vt +188 (obj, "", 0) and is cleared; sound +144 is released
// (Audio_Release446384) and cleared.
DECOMP(004A9084, Audio_Stop4A9084) {
    const u32 obj = load<u32>(kAudioBlock + 136);
    if (obj != 0) vcall(ctx, obj, 188, {obj, kEmptyName, 0});
    const u32 snd = load<u32>(kAudioBlock + 144);
    store<u32>(kAudioBlock + 136, 0);
    if (snd != 0) {
        call(ctx, kAudioRelease, {kSoundMgr, snd});
        store<u32>(kAudioBlock + 144, 0);
    }
    ctx.gpr[4] = 0;
    ret(ctx, 0);
}

// f32* Obj_Position26A540(f32 out[3], Obj* o) (name?): out = o +480; when
// o +32 is set, out += (o +528 vec3) x the scale 0x0026A42C writes to the
// second word of its out pair (fmadds).
DECOMP(0026A540, Obj_Position26A540) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) store<u32>(out + k, load<u32>(o + 480 + k));
    ctx.fpr[1] = hevent7::lfs_f64(o + 488);
    const u32 parent = load<u32>(o + 32);
    ctx.gpr[6] = parent;
    if (parent == 0) {
        ctx.gpr[3] = o;
        return;
    }
    Stack frame(ctx, 16);
    const u32 tmp = static_cast<u32>(ctx.gpr[1]);
    const u32 v = o + 528;
    call(ctx, kOffset26A42C, {tmp, o, v});
    const f32 s = load<f32>(tmp + 4);
    const f32 x = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(v)), s, load<f32>(out)));
    const f32 y = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(v + 4)), s, load<f32>(out + 4)));
    const f32 z = static_cast<f32>(std::fma(static_cast<f64>(load<f32>(v + 8)), s, load<f32>(out + 8)));
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = z;
}

// void Pause_Set2AB340(u8 paused) (name?): 0x002AAEBC(obj, paused) for every
// object of the list at 0x00F64A58 (next +2568); then the pause count at
// 0x010592E8 is incremented when pausing, else decremented unless 0.
DECOMP(002AB340, Pause_Set2AB340) {
    const u32 paused = arg<u32>(ctx, 0) & 0xFF;
    for (u32 o = load<u32>(kPauseList); o != 0; o = load<u32>(o + 2568)) call(ctx, kPause2AAEBC, {o, paused});
    u32 n = load<u32>(kPauseCount);
    if (paused != 0) {
        store<u32>(kPauseCount, ++n);
    } else if (n != 0) {
        store<u32>(kPauseCount, --n);
    }
    ctx.gpr[4] = n;
    ctx.gpr[3] = 0x01060000u;  // lis r3, 0x106 (left)
}

// void Obj_DropLayer2169C0(Obj* o) (name?): the animator o +440, when set:
// popped from the world's layer object (Obj_PopLayer(layer, anim; -1.0))
// when there is one, then its vt +4 (anim) when still set; +440 = 0.
DECOMP(002169C0, Obj_DropLayer2169C0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 layer = load<u32>(load<u32>(kCurrentWorld) + kWorldLayer);
    u32 a = load<u32>(o + 440);
    if (a == 0) {
        ctx.gpr[4] = layer;
        ret(ctx, 0);
        return;
    }
    if (layer != 0) {
        callf(ctx, kObjPopLayer, {layer, a, layer, kPopTime}, {hevent7::lfs_f64(kPopTime)});
        a = load<u32>(o + 440);
    }
    if (a != 0) vcall(ctx, a, 4, {a});
    store<u32>(o + 440, 0);
    ret(ctx, 0);
}

// bool Speaker_SayId4A5C90(Speaker* s, u32 text id, u8 flag; f32 t)
// (name?): Speaker_Say3514AC(s, Text_Lookup34A398(id), flag, 0, -, -, 0, 0,
// voice 0; t, 0, 0) when s and the text exist; returns whether it spoke.
// The original leaves r7 / r8 as Text_Lookup34A398 left them (unread).
DECOMP(004A5C90, Speaker_SayId4A5C90) {
    const u32 s = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1), flag = arg<u32>(ctx, 2) & 0xFF;
    const f64 t = farg(ctx, 0);
    if (s == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 text = static_cast<u32>(call(ctx, kTextLookup, {id, ctx.gpr[4], ctx.gpr[5], s}));
    if (text == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 zero = hevent7::lfs_f64(kSayZero);
    callf(ctx, kSpeakerSay, {s, text, flag, 0, ctx.gpr[7], ctx.gpr[8], 0, 0, 0}, {t, zero, zero});
    ret(ctx, 1);
}

// Part* Session_CurrentPart2DDD24(Session* s) (name?): s +36 when set, else
// the first of the s +24 860-byte parts (s +20) that is not done (u8 +826)
// and lists the world's current part key among its u8 +820 keys (+748).
DECOMP(002DDD24, Session_CurrentPart2DDD24) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 cur = load<u32>(s + 36);
    if (cur != 0) {
        ctx.gpr[4] = cur;
        ret(ctx, cur);
        return;
    }
    const u32 key = load<u32>(load<u32>(kCurrentWorld) + kWorldPartKey);
    const u32 n = load<u32>(s + 24);
    u32 p = load<u32>(s + 20);
    for (u32 i = 0; i < n; ++i, p += 860) {
        if (load<u8>(p + 826) != 0) continue;
        const u32 cnt = load<u8>(p + 820);
        for (u32 j = 0; j < cnt; ++j)
            if (static_cast<s32>(load<u32>(p + 748 + 4 * j)) == static_cast<s32>(key)) {
                ret(ctx, p);
                return;
            }
    }
    ctx.gpr[4] = 0;
    ret(ctx, 0);
}

// void Block_Create348B54() (name?): u8 0x0106BBE4 = 1; a 16-byte block from
// the thread's heap (vt +8 (heap, 16)): +4 u16 16 (stored even when the
// allocation failed, at address 4), +6 u16 1, +0 / +8 / +12 vtable words
// 0x00B1F4F4 / 0x00B1F514 / 0x00B1F528. 0x00F64C9C = the block or 0.
DECOMP(00348B54, Block_Create348B54) {
    const u32 tls = load<u32>(static_cast<u32>(ctx.gpr[13]) - kHeapTls);
    store<u8>(kBlockFlag, 1);
    const u32 heap = load<u32>(tls + 44);
    const u32 b = static_cast<u32>(vcall(ctx, heap, 8, {heap, 16}));
    store<u16>(b + 4, 16);  // quirk: written before the null check
    if (b != 0) {
        store<u16>(b + 6, 1);
        store<u32>(b, kBlockVtable + 8);
        store<u32>(b + 8, kBlockVtable + 40);
        store<u32>(b + 12, kBlockVtable + 60);
        ctx.gpr[4] = kBlockVtable + 60;
        ctx.gpr[5] = kBlockVtable + 40;
        ctx.gpr[6] = kBlockVtable + 8;
    } else {
        ctx.gpr[4] = 16;
    }
    store<u32>(kBlockPtr, b);
    ctx.gpr[3] = kBlockPtr & 0xFFFF0000u;  // lis r3, 0xF6 (left)
}

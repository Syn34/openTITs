// World_HandleEvent callees, part 24c: two more object constructors of the
// level spawn table (under 0x0035AC7C), as hevent20_k.cpp.
#include "hevent19.h"

using namespace sly4;
using namespace sly4::hevent;

namespace {
constexpr u32 kObjBaseConstruct = 0x0035695C;  // (Object*, u32 kind, Def*) (symbols.txt Obj_BaseConstruct35695C)
constexpr u32 kStringFormat = 0x005534D0;      // (fmt, ...) -> const char* (symbols.txt)
constexpr u32 kStringHashNoCase = 0x0056A710;  // (const char*) -> u32 (symbols.txt)
constexpr u32 kFmtIntro = 0x00ADC768, kFmtIdle = 0x00ADC788, kFmtOutro = 0x00ADC7A8;  // "%s_Intro" / "%s_Idle" / "%s_Outro"
constexpr u32 kFindById = 0x004A5AE8;          // (WorldObj*, s32 id) -> WorldObj* (WorldObj_FindById4A5AE8)
constexpr u32 kMemAlloc = 0x0054ED38, kMemset = 0x00726E54;
constexpr u32 kPtrVectorConstruct = 0x005516E0, kPtrVectorInit = 0x00551598;  // (v) / (v, n, 0, 0) (symbols.txt)
constexpr u32 kVtable35A3A0 = 0x00B1F838;
constexpr u32 kVtable35A5B0 = 0x00B1F720;
// The six named children of 0x0035A5B0's objects (+188..+208).
constexpr u32 kChildIds35A5B0[6] = {0x0E272E73, 0x92B78972, 0xBA018A9B, 0x92A4B2F9, 0x5CC0B718, 0x47DA3E5C};

// The intro / idle / outro hashes of the def's name (+36, else +4) into
// o + offs[k]; r3 as the original leaves it when there is no name.
void anim_hashes(PPUContext& ctx, u32 o, u32 d, const u32 (&offs)[3]) {
    u32 name = d + 36;
    u32 c = load<u8>(name);
    if (c == 0) name = d + 4;
    if (name == 0 || (c = load<u8>(name)) == 0) {
        ret(ctx, c);
        return;
    }
    const u32 fmts[3] = {kFmtIntro, kFmtIdle, kFmtOutro};
    for (u32 k = 0; k < 3; ++k) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
        store<u32>(o + offs[k], static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
    }
}
}  // namespace

// Object* Obj_Construct35A3A0(Object* o, Def* d) (name?): base object, then
// vtable 0x00B1F838, a PtrVector at +164 (+170 = 12, Init(6)), +200 / +204
// = 0, +208 bit 31 cleared, +4 |= 4; with an owner (+24) +204 = the object
// of id d +156. d +152 ids (u32 at d +148 + 16 i) are looked up into a
// zeroed Mem_Alloc(4 n, 0, 16) array at +200 (looked up even without an
// owner). +184..+199 zeroed and +188 / +192 / +196 the "<name>_Intro" /
// "_Idle" / "_Outro" hashes.
DECOMP(0035A3A0, Obj_Construct35A3A0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable35A3A0);
    call(ctx, kPtrVectorConstruct, {o + 164});
    store<u16>(o + 170, 12);
    call(ctx, kPtrVectorInit, {o + 164, 6, 0, 0});
    const u32 w = load<u32>(o + 208) & 0x7FFFFFFFu;
    store<u32>(o + 200, 0);
    store<u32>(o + 204, 0);
    store<u32>(o + 208, w);
    store<u32>(o + 4, load<u32>(o + 4) | 4);
    if (const u32 owner = load<u32>(o + 24); owner != 0) store<u32>(o + 204, static_cast<u32>(call(ctx, kFindById, {owner, load<u32>(d + 156)})));
    if (const u32 n = load<u32>(d + 152); n != 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {n << 2, 0, 16}));
        const u32 a = static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 152) << 2}));
        store<u32>(o + 200, a);
        for (u32 i = 0; i < load<u32>(d + 152); ++i) {
            const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(load<u32>(d + 148) + 16 * i)}));
            store<u32>(load<u32>(o + 200) + 4 * i, r);
        }
    }
    store<u64>(o + 184, 0);
    store<u64>(o + 192, 0);
    anim_hashes(ctx, o, d, {188, 192, 196});
}

// Object* Obj_Construct35A5B0(Object* o, Def* d) (name?): base object, then
// vtable 0x00B1F720, +184..+208 and +212 (u8) zeroed, +4 |= 2, +164..+180
// zeroed; +168 / +176 / +180 the "<name>_Intro" / "_Idle" / "_Outro"
// hashes. d +152 32-byte records (d +148) become a zeroed Mem_Alloc(16 n,
// 0, 16) array at +184 of {the object of id +4, +8, +12, +16}; then +188..
// +208 the six named children (WorldObj_FindById4A5AE8 on the owner +24).
DECOMP(0035A5B0, Obj_Construct35A5B0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    store<u32>(o, kVtable35A5B0);
    for (u32 k = 184; k <= 208; k += 4) store<u32>(o + k, 0);
    store<u8>(o + 212, 0);
    store<u32>(o + 4, load<u32>(o + 4) | 2);
    for (u32 k = 164; k <= 180; k += 4) store<u32>(o + k, 0);
    anim_hashes(ctx, o, d, {168, 176, 180});
    if (const u32 n = load<u32>(d + 152); n != 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {n << 4, 0, 16}));
        const u32 a = static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 152) << 4}));
        store<u32>(o + 184, a);
        for (u32 i = 0; i < load<u32>(d + 152); ++i) {
            const u32 src = load<u32>(d + 148) + 32 * i, dst = load<u32>(o + 184) + 16 * i;
            store<u32>(dst, static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(src + 4)})));
            store<u32>(dst + 4, load<u32>(src + 8));
            store<u32>(dst + 8, load<u32>(src + 12));
            store<u32>(dst + 12, load<u32>(src + 16));
        }
    }
    for (u32 k = 0; k < 6; ++k) store<u32>(o + 188 + 4 * k, static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), kChildIds35A5B0[k]})));
}

namespace {
// Clips_Release4B0E70: a handle table of clips. +8 / +50 / +74 / +80 the
// clip vector (u16 count +74, 8-byte entries {key, Clip*} at +80); +224;
// +228 / +232 the free list's first / last node, +236 / +240 the used
// list's first / last, +244 used count, +248, +252 s32 capacity, +256
// 16-byte nodes {+0 prev, +4 next, +8 item, +12 bit 31 free, bits 15..30
// generation}. Items: +0, +4 handle, +6 u16.
constexpr u32 kHtFree = 228, kHtFreeLast = 232, kHtFirst = 236, kHtLast = 240, kHtCount = 244, kHtCap = 248, kHtNodes = 252 + 4;
constexpr u32 kHtCapacity = 252;
constexpr u32 kNodeSize = 16, kNodePrev = 0, kNodeNext = 4, kNodeItem = 8, kNodeFlags = 12;
constexpr u32 kNodeFreeByte = 0x80, kNodeGenMask = 0x7FFF8000;
constexpr u32 kGame = 0x00F65F84;       // u32 Game*: +292 OPD of the item release callback (handle) (name?)
constexpr u32 kGameItemRelease = 292;
constexpr u32 kClipRelease4B0C70 = 0x004B0C70;  // (Clip*) (name?)
constexpr u32 kBufferSyncC = 0x00566318, kMemFreeFromC = 0x0054EE6C;
}  // namespace

// u32 Clips_Release4B0E70(ClipTable* t) (name?): every used node's item is
// released (+0 = 0, the game's release callback (*0x00F65F84 +292) on +4
// when set, +6 = 0), all nodes are put back on the free list (prev / next
// indices, free bit set, generation cleared; the ends -1, as written even
// for an empty table), the used list emptied (+244 = 0, +248 = capacity),
// 0x00566318(t, t->+8), and the clip vector's clips released
// (0x004B0C70, Mem_FreeFrom) and emptied (+50 / +74 / +224 = 0).
// Returns the clip count as last read.
DECOMP(004B0E70, Clips_Release4B0E70) {
    const u32 t = arg<u32>(ctx, 0);
    u32 n = load<u32>(t + kHtCount);
    // The iterator {u16 index, u16 generation} of the first used node.
    u32 it = 0;
    if (n != 0) {
        const u32 head = load<u32>(t + kHtFirst);
        const u32 gen = (load<u32>(load<u32>(t + kHtNodes) + (head << 4) + kNodeFlags) >> 15) & 0xFFFF;
        it = ((head & 0xFFFF) << 16) | gen;
    }
    if (static_cast<s32>(n) > 0) {
        for (s32 i = 0; i < static_cast<s32>(n); ++i) {
            const u32 off = (it >> 12) & 0xFFFF0u;  // index * 16
            u32 nodes = load<u32>(t + kHtNodes);
            const u32 item = load<u32>(nodes + off + kNodeItem);
            store<u32>(item, 0);
            if (const u32 cb = load<u32>(load<u32>(kGame) + kGameItemRelease); cb != 0) {
                call_ptr(ctx, cb, {load<u32>(item + 4)});
                nodes = load<u32>(t + kHtNodes);
                n = load<u32>(t + kHtCount);
            }
            store<u16>(item + 6, 0);
            const u32 idx = it >> 16;
            it = 0;
            if (static_cast<s32>(idx) != static_cast<s32>(load<u32>(t + kHtLast))) {
                const u32 next = load<u32>(nodes + off + kNodeNext);
                it = (next << 16) | ((load<u32>(nodes + (next << 4) + kNodeFlags) >> 15) & 0xFFFF);
            }
        }
    }
    s32 cap = static_cast<s32>(load<u32>(t + kHtCapacity));
    if (cap > 0) {
        for (s32 j = 0; j < cap; cap = static_cast<s32>(load<u32>(t + kHtCapacity))) {
            const u32 nodes = load<u32>(t + kHtNodes), nd = nodes + 16 * static_cast<u32>(j);
            store<u32>(nd + kNodePrev, static_cast<u32>(j - 1));
            store<u32>(nd + kNodeNext, static_cast<u32>(j + 1));
            store<u8>(nd + kNodeFlags, static_cast<u8>(load<u8>(nd + kNodeFlags) | kNodeFreeByte));
            const u32 f = load<u32>(load<u32>(t + kHtNodes) + 16 * static_cast<u32>(j) + kNodeFlags);
            store<u32>(load<u32>(t + kHtNodes) + 16 * static_cast<u32>(j) + kNodeFlags, f & ~kNodeGenMask);
            ++j;
        }
    }
    const u32 nodes = load<u32>(t + kHtNodes);
    store<u32>(nodes + kNodePrev, 0xFFFFFFFFu);
    store<u32>(nodes + (static_cast<u32>(cap) << 4) - 12, 0xFFFFFFFFu);  // the last node's next (nodes - 12 when empty)
    store<u32>(t + kHtFirst, 0xFFFFFFFFu);
    store<u32>(t + kHtLast, 0xFFFFFFFFu);
    store<u32>(t + kHtFree, 0);
    store<u32>(t + kHtFreeLast, cap > 1 ? static_cast<u32>(cap - 1) : 0xFFFFFFFFu);
    store<u32>(t + kHtCount, 0);
    store<u32>(t + kHtCap, static_cast<u32>(cap));
    call(ctx, kBufferSyncC, {t, load<u32>(t + 8)});
    store<u16>(t + 50, 0);
    u32 cnt;
    for (u32 k = 0; static_cast<s32>(k) < static_cast<s32>(cnt = load<u16>(t + 74)); ++k) {
        const u32 c = load<u32>(load<u32>(t + 80) + 8 * k + 4);
        if (c == 0) continue;
        call(ctx, kClipRelease4B0C70, {c});
        call(ctx, kMemFreeFromC, {c, 0});
    }
    store<u16>(t + 74, 0);
    store<u32>(t + 224, 0);
    ret(ctx, cnt);
}

namespace {
// Request_Do369D98: the request block (0x0106C330): +0 / +16 / +32 vec3
// positions, +48 vec3, +96 key, +100 Object*, +108 / +112 / +116 /
// +124 / +128 f32, +132 u8 busy.
constexpr u32 kReqConsts = 0x00369D78;  // f32 {..., [3] 0.0, [6] -1.0}
constexpr u32 kObjectsReset = 0x0035B470, kObjSetAnimR = 0x00363A3C, kSaveValue = 0x00465D5C, kStringFormatR = 0x005534D0;
constexpr u32 kMat34FromRotPos = 0x00553B4C, kWorldObjMarkDirty = 0x0057D000, kWorldObjSetScale = 0x0057D100;
constexpr u32 kFmtI = 0x00ADCACC;        // "%i"
constexpr u32 kSetAnimArgR = 0x01164CA4;
constexpr u32 kReqRot = 0x01164C50;      // the rotation given to Mat34_FromRotPos553B4C (name?)
constexpr u32 kWorldObjSetFlags = 52;    // vtable slot: (w, 255, 0)
// Tuning values (f32) copied into the block (name?).
constexpr u32 kTune124 = 0x0106C668, kTune128 = 0x0106C84C, kTuneRate = 0x0106C6B4, kTuneVec48 = 0x0106C710, kTuneYOff = 0x0106C768;
}  // namespace

// u32 Request_Do369D98(Request* b, u32 key, -, const vec3* pos; f32 t)
// (name?): nothing while busy (+132; r3: the byte). Otherwise
// Objects_Reset35B470(b), +96 = key, busy; the block's object (+100) plays
// the "%i" animation of (s64)t (Obj_SetAnim, -1.0), and its +24 world
// object gets scale {1, 1, 1}, vtable +52 (w, 255, 0), the 3x4 matrix of
// Mat34_FromRotPos553B4C(0x01164C50, pos) and WorldObj_MarkDirty. Then the
// block's timings (+108 = Save_Value465D5C(key), +112 = that + t, +116 = t
// / rate when rate > 0, else 0) and positions (+0 / +16 = pos, +48 / +32
// the tuning vector, +20 / +36 raised by the tuning offset) are set.
// Returns b + 20 (r3 as the original leaves it).
DECOMP(00369D98, Request_Do369D98) {
    const u32 b = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), pos = arg<u32>(ctx, 3);
    const f64 t = farg(ctx, 0);
    if (const u32 busy = load<u8>(b + 132); busy != 0) return ret(ctx, busy);
    call(ctx, kObjectsReset, {b});
    store<u32>(b + 96, key);
    const u32 obj = load<u32>(b + 100);
    store<u8>(b + 132, 1);
    if (obj != 0) {
        const u32 s = static_cast<u32>(call(ctx, kStringFormatR, {kFmtI, static_cast<u32>(rt::fctid(t, true))}));
        // r6 = the constant pool (Obj_SetAnim does not read it).
        callf(ctx, kObjSetAnimR, {obj, s, 1, kReqConsts, 0, 0, kSetAnimArgR}, {static_cast<f64>(load<f32>(kReqConsts + 24))});
        if (const u32 w = load<u32>(load<u32>(b + 100) + 24); w != 0) {
            // Stack (original r1 +112..+192): the scale, then the 4x4 matrix at +16.
            Stack frame(ctx, 80);
            const u32 scale = frame.addr(), m = scale + 16;
            for (u32 k = 0; k < 12; k += 4) store<u32>(scale + k, 0x3F800000u);
            call(ctx, kWorldObjSetScale, {w, scale, 0});
            vcall(ctx, w, kWorldObjSetFlags, {w, 255, 0});
            call(ctx, kMat34FromRotPos, {m, kReqRot, pos});
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 12; k += 4) store<u32>(w + 16 + 16 * r + k, load<u32>(m + 16 * r + k));
            call(ctx, kWorldObjMarkDirty, {w});
        }
    }
    store<u32>(b + 124, load<u32>(kTune124));
    store<u32>(b + 128, load<u32>(kTune128));
    const s32 v = static_cast<s32>(call(ctx, kSaveValue, {load<u32>(b + 96)}));
    const f64 start = static_cast<f32>(static_cast<f64>(static_cast<s64>(v)));
    store<f32>(b + 108, static_cast<f32>(start));
    store<f32>(b + 112, static_cast<f32>(start + t));
    const f64 rate = static_cast<f64>(load<f32>(kTuneRate));
    if (rate > static_cast<f64>(load<f32>(kReqConsts + 12))) store<f32>(b + 116, static_cast<f32>(t / rate));
    else store<u32>(b + 116, load<u32>(kReqConsts + 12));
    const u32 px = load<u32>(pos);
    store<u32>(b + 0, px);
    const u32 py = load<u32>(pos + 4);
    store<u32>(b + 4, py);
    const u32 pz = load<u32>(pos + 8);
    store<u32>(b + 8, pz);
    store<u32>(b + 48, load<u32>(kTuneVec48));
    store<u32>(b + 52, load<u32>(kTuneVec48 + 4));
    const u32 v56 = load<u32>(kTuneVec48 + 8);
    store<u32>(b + 16, px);
    store<u32>(b + 56, v56);
    store<u32>(b + 20, py);
    store<u32>(b + 24, pz);
    const f64 yoff = static_cast<f64>(load<f32>(kTuneYOff));
    store<f32>(b + 20, static_cast<f32>(static_cast<f64>(load<f32>(b + 20)) + yoff));
    const u32 w48 = load<u32>(b + 48), w52 = load<u32>(b + 52);
    store<u32>(b + 32, w48);
    const u32 w56 = load<u32>(b + 56);
    store<u32>(b + 36, w52);
    store<u32>(b + 40, w56);
    const f32 y36 = static_cast<f32>(static_cast<f64>(load<f32>(b + 36)) + yoff);
    store<f32>(b + 36, y36);
    ctx.fpr[1] = y36;  // left in f1 by the original
    ret(ctx, b + 20);
}

namespace {
// Attach_Build31F44C: the attachment instances of a model (name?).
constexpr u32 kHashToStringC = 0x0056A6E0, kSortedMapFind768AC0 = 0x00768AC0, kSmallAlloc = 0x0055238C;
constexpr u32 kAttachInit31E1D8 = 0x0031E1D8;  // (Attach*) (name?)
constexpr u32 kAttachDefaultMatrix = 0x01164BD0;  // 4 x 16-byte rows, xyz used (name?)
constexpr u32 kAttachDefaultVec = 0x01164B70;     // vec3 (name?)
constexpr u32 kAttachSize = 144, kEntrySize64 = 64;
}  // namespace

// u32 Attach_Build31F44C(PtrVector* out, Map* defs) (name?): for each
// 64-byte entry of out's model vector (*(out +20) -> +0 data, +8 u16
// count): an entry without +52 only gets Hash_ToString(model +4) (the
// result unused); otherwise its def (SortedMap_Find768AC0(defs, entry +32))
// makes a 144-byte attachment: a PtrVector (+6 = 4, Init(n, 0, 1)) with n =
// def +24 -> +8, a SmallAlloc(4 n) buffer as +20 / +28 / +32 (+24 its end,
// +36 n, +40 0), the default matrix rows' xyz at +48 / +64 / +80 / +96,
// the default vector at +112, +128 = 0, +132 = entry, +136 = def. Each
// attachment (or 0 when SmallAlloc fails) is pushed onto out and given to
// 0x0031E1D8. Returns out +20 (r3 as the original leaves it).
DECOMP(0031F44C, Attach_Build31F44C) {
    const u32 out = arg<u32>(ctx, 0), defs = arg<u32>(ctx, 1);
    for (u32 i = 0;; ++i) {
        const u32 h = load<u32>(out + 20);
        const u32 vec = load<u32>(h);
        const u32 n = vec != 0 ? load<u16>(vec + 8) : 0u;
        if (!(i < n)) return ret(ctx, h);
        const u32 e = load<u32>(vec) + kEntrySize64 * i;
        if (load<u32>(e + 52) == 0) {
            call(ctx, kHashToStringC, {load<u32>(h + 4)});
            continue;
        }
        const u32 def = load<u32>(static_cast<u32>(call(ctx, kSortedMapFind768AC0, {defs, e + 32})));
        if (def == 0) continue;
        u32 a = static_cast<u32>(call(ctx, kSmallAlloc, {kAttachSize}));
        if (a != 0) {
            const u32 cnt = load<u32>(load<u32>(def + 24) + 8);
            call(ctx, kPtrVectorConstruct, {a});
            store<u16>(a + 6, 4);
            call(ctx, kPtrVectorInit, {a, cnt, 0, 1});
            const u32 cnt2 = load<u32>(load<u32>(def + 24) + 8);
            const u32 sz = cnt2 << 2;
            const u32 buf = static_cast<u32>(call(ctx, kSmallAlloc, {sz}));
            store<u32>(a + 20, buf);
            store<u32>(a + 28, buf);
            store<u32>(a + 32, buf);
            store<u32>(a + 36, cnt2);
            store<u32>(a + 24, buf + sz);
            store<u32>(a + 40, 0);
            for (u32 r = 0; r < 4; ++r)
                for (u32 k = 0; k < 12; k += 4) store<u32>(a + 48 + 16 * r + k, load<u32>(kAttachDefaultMatrix + 16 * r + k));
            store<u32>(a + 112, load<u32>(kAttachDefaultVec));
            store<u32>(a + 116, load<u32>(kAttachDefaultVec + 4));
            const u32 z = load<u32>(kAttachDefaultVec + 8);
            store<u32>(a + 128, 0);
            store<u32>(a + 120, z);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(z));  // left in f1 by the original's lfs / stfs copy
            store<u32>(a + 132, e);
            store<u32>(a + 136, def);
        }
        sly4::hevent18::pv_push(ctx, out, a);
        call(ctx, kAttachInit31E1D8, {a});
    }
}

namespace {
constexpr u32 kVtable3599C4 = 0x00B1F8A8;
constexpr u32 kFmtOnUpdate = 0x00ADC778;      // "%s_OnUpdate"
constexpr u32 kWorldObjFindByHash = 0x005801DC;  // (u32 hash, 0) -> WorldObj* (symbols.txt)
constexpr u32 kChildId3599C4 = 0x6D1B6072;
}  // namespace

// Object* Obj_Construct3599C4(Object* o, Def* d) (name?): base object, then
// vtable 0x00B1F8A8, +216 flag 0x40000000 set and 0x3F800000 cleared,
// +164..+232 fields cleared, +212 = d +160, +4 |= 0x400; +184 / +188 /
// +192 / +196 the "<name>_Intro" / "_OnUpdate" / "_Idle" / "_Outro"
// hashes; +168 = WorldObj_FindByHash(d +156, 0), +164 the 0x6D1B6072
// child; d +200 ids (28-byte records at d +196, id at +4) looked up into a
// zeroed Mem_Alloc(4 n, 0, 16) array at +220; d +152 zeroed 12-byte
// records (Mem_Alloc(12 n, 0, 16), byte stores) at +208; +224 = the
// object of id d +212.
DECOMP(003599C4, Obj_Construct3599C4) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, kObjBaseConstruct, {o, 0, d});
    const u32 w = (load<u32>(o + 216) | 0x40000000u) & ~0x3F800000u;
    store<u32>(o + 172, 0);
    store<u32>(o + 176, 0);
    store<u32>(o, kVtable3599C4);
    store<u32>(o + 168, 0);
    store<u32>(o + 200, 0);
    const u32 d160 = load<u32>(d + 160);
    store<u32>(o + 204, 0);
    store<u32>(o + 208, 0);
    store<u32>(o + 212, d160);
    store<u32>(o + 220, 0);
    store<u32>(o + 216, w);
    for (u32 k = 224; k <= 232; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 4, load<u32>(o + 4) | 0x400);
    for (u32 k = 180; k <= 196; k += 4) store<u32>(o + k, 0);
    u32 name = d + 36;
    if (load<u8>(name) == 0) name = d + 4;
    if (name != 0 && load<u8>(name) != 0) {
        const u32 fmts[4] = {kFmtIntro, kFmtOnUpdate, kFmtIdle, kFmtOutro};
        for (u32 k = 0; k < 4; ++k) {
            const u32 s = static_cast<u32>(call(ctx, kStringFormat, {fmts[k], name}));
            store<u32>(o + 184 + 4 * k, static_cast<u32>(call(ctx, kStringHashNoCase, {s})));
        }
    }
    store<u32>(o + 168, static_cast<u32>(call(ctx, kWorldObjFindByHash, {load<u32>(d + 156), 0})));
    store<u32>(o + 164, static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), kChildId3599C4})));
    if (const u32 n = load<u32>(d + 200); n != 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {n << 2, 0, 16}));
        store<u32>(o + 220, static_cast<u32>(call(ctx, kMemset, {p, 0, load<u32>(d + 200) << 2})));
        for (u32 i = 0; i < load<u32>(d + 200); ++i) {
            const u32 r = static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(load<u32>(d + 196) + 28 * i + 4)}));
            store<u32>(load<u32>(o + 220) + 4 * i, r);
        }
    }
    if (const s32 k = static_cast<s32>(load<u32>(d + 152)); k > 0) {
        const u32 p = static_cast<u32>(call(ctx, kMemAlloc, {static_cast<u32>(k) * 12, 0, 16}));
        if (p != 0)
            for (s32 i = 0; i < k; ++i)
                for (u32 b = 0; b < 12; ++b) store<u8>(p + 12 * static_cast<u32>(i) + b, 0);
        store<u32>(o + 208, p);
    }
    store<u32>(o + 224, static_cast<u32>(call(ctx, kFindById, {load<u32>(o + 24), load<u32>(d + 212)})));
}

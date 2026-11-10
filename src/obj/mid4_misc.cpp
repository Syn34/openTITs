// Game code, medium sweep part 4: assorted template instances.
#include "mid4.h"

using namespace sly4;
using namespace sly4::mid4;

namespace {

// void f(Obj* o, u8 a, u8 b): forwards to the component of o's kind (o->+24
// bits 25..29: 1 -> +60, 2 -> +76, 3 -> +92, 4 -> +108, 5 -> +124, 6 ->
// +140, 8 -> +160; others nothing): c->vfn[slot / 4](c, a, b) when it is
// set (r3 = 0 without one, as left).
void ForwardToKind(PPUContext& ctx, u32 slot) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    u32 off;
    switch ((load<u32>(o + 24) >> 25) & 0x1Fu) {
    case 8: off = 160; break;
    case 6: off = 140; break;
    case 5: off = 124; break;
    case 4: off = 108; break;
    case 3: off = 92; break;
    case 2: off = 76; break;
    case 1: off = 60; break;
    default: return;
    }
    const u32 c = load<u32>(o + off);
    ctx.gpr[3] = c;
    if (c != 0) VCall(ctx, c, slot, {c, a, b});
}

}  // namespace

// void f(Obj* o, u8 a, u8 b): ForwardToKind through vfn[10].
DECOMP(002916C0, Obj_ForwardToKind_002916C0) { ForwardToKind(ctx, 40); }

// void f(Obj* o, u8 a, u8 b): ForwardToKind through vfn[13].
DECOMP(002918C8, Obj_ForwardToKind_002918C8) { ForwardToKind(ctx, 52); }

// ---------------------------------------------------------------- tracked object lists

namespace {

namespace lfn {
constexpr u32 PtrVector_Reserve = 0x00551718;  // (vec, capacity)
constexpr u32 kRandomLast = 0x01164888;        // u64: the low 16 bits of the last random mix
constexpr u32 kRandomState = 0x01164890;       // u32
}  // namespace lfn

// Room for one more element in the vector {+0 data, +4 u16 capacity, +8
// u16 count, +10 u16 high-water} at v (doubled when full); returns the
// count.
u32 VecRoom(PPUContext& ctx, u32 v) {
    if (load<u16>(v + 8) == load<u16>(v + 4)) {
        call(ctx, lfn::PtrVector_Reserve, {v, (static_cast<u32>(load<u16>(v + 4)) << 1) & 0x1FFFEu});
    }
    return load<u16>(v + 8);
}
// count + 1 and the high-water mark after a push; returns the new mark (as
// left in a register: count + 1 unmasked when it is the larger).
u32 VecPushed(u32 v) {
    const u32 n = static_cast<u32>(load<u16>(v + 8)) + 1;
    const u32 hi = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    const u32 m = static_cast<s32>(hi) > static_cast<s32>(n & 0xFFFF) ? hi : n;
    store<u16>(v + 10, static_cast<u16>(m));
    return m;
}

// void f(List* l, Obj* o): o tracked in four parallel vectors of l (+64
// Obj*, +84 16-byte positions, +104 u32 states, +124 f32 timers): an
// already tracked o only gets its state set to 1; a new one is appended
// with its position (o +144..+152), state 2 and a random timer
// (Random_Step's previous low 16 bits * c[0] * c[1]: 0..5 seconds; c the
// f32 pair the original keeps in its text, before the function).
void TrackObject(PPUContext& ctx, u32 c) {
    const u32 l = arg<u32>(ctx, 0);
    const u32 o = arg<u32>(ctx, 1);
    {
        const u32 data = load<u32>(l + 64), n = load<u16>(l + 72);
        for (u32 i = 0; i < n; ++i) {
            if (load<u32>(data + 4 * i) == o) {
                const u32 p = load<u32>(l + 104) + (i << 2);
                store<u32>(p, 1);
                ctx.gpr[3] = p;
                return;
            }
        }
    }
    u32 n = VecRoom(ctx, l + 64);
    store<u32>(load<u32>(l + 64) + ((n << 2) & 0x3FFFCu), o);
    VecPushed(l + 64);
    n = VecRoom(ctx, l + 84);
    const u32 dst = load<u32>(l + 84) + ((n << 4) & 0xFFFF0u);
    for (u32 k = 0; k < 12; k += 4) store<u32>(dst + k, load<u32>(o + 144 + k));  // lfs / stfs pairs
    VecPushed(l + 84);
    n = VecRoom(ctx, l + 104);
    store<u32>(load<u32>(l + 104) + ((n << 2) & 0x3FFFCu), 2);
    VecPushed(l + 104);
    // the inlined random step: its previous low 16 bits (as s64) scaled
    const s64 last = static_cast<s64>(load<u64>(lfn::kRandomLast));
    const u32 s = load<u32>(lfn::kRandomState);
    const u32 mix = (std::rotl(s, 4) + std::rotl(s, 16)) ^ 0x87654321u;
    store<u32>(lfn::kRandomState, std::rotl(mix, 16));
    store<u64>(lfn::kRandomLast, mix & 0xFFFFu);
    const f32 r = static_cast<f32>(static_cast<f64>(last));
    const f32 f1 = static_cast<f32>(static_cast<f64>(r) * std::bit_cast<f64>(LfsBits(load<u32>(c))));
    const f32 t = static_cast<f32>(static_cast<f64>(f1) * std::bit_cast<f64>(LfsBits(load<u32>(c + 4))));
    n = VecRoom(ctx, l + 124);
    store<u32>(load<u32>(l + 124) + ((n << 2) & 0x3FFFCu), std::bit_cast<u32>(t));
    ctx.gpr[3] = VecPushed(l + 124);
    ctx.fpr[1] = static_cast<f64>(f1);
}

}  // namespace

// void f(List* l, Obj* o): TrackObject (constants at 0x002B2CEC).
DECOMP(002B2CF8, List_TrackObject_002B2CF8) { TrackObject(ctx, 0x002B2CEC); }

// void f(List* l, Obj* o): TrackObject (constants at 0x002B5350).
DECOMP(002B5358, List_TrackObject_002B5358) { TrackObject(ctx, 0x002B5350); }

// ---------------------------------------------------------------- talk state

namespace {

namespace tsfn {
constexpr u32 Speaker_SayId = 0x004A5C90;   // (Speaker*, u32 text id, u8; f32) -> bool
constexpr u32 Effect_SetHash = 0x004CF988;  // (Effect*, u32 hash) -> bool
constexpr u32 SortedMap_Find3 = 0x0076611C; // (map, const u32* key) -> entry*
constexpr u32 Array_Reserve = 0x00558F3C;   // (Array*, u16 capacity)
constexpr u32 kPending = 0x0106C240;        // {+0 u16 capacity, +2 u16 count, +4 u16 high-water, +6 u16 flags, +8 {key, T*}[]}
}  // namespace tsfn

// void f(T* t, u8 start, u8 stop): t's flags +252 (0x40000000 enabled,
// 0x02000000 busy, 0x20000000 / 0x10000000 the start / stop requests,
// 0x01000000 said, 0x00800000 effect set) and state +160 (bits 1..31; 6:
// talking). Nothing unless enabled and not busy (r3 = t->+12, as left).
// In state 0 / 6: a start request (flag or argument) in state 0 calls
// t->vfn[3](t); else a stop request in state 0
// queues t (keyed by t->+12, else by its +8 owner's +68) in kPending
// unless it is already there; then t->+248 = 0. In another state: the
// request bits set from the raw arguments' bit 0, state 6, +240 = -1.0f,
// the line t->+224 said by the speaker t->+28 (f1 the 1.0f the original
// keeps in its text) into the said flag, the effect +20 restarted with the hash
// at fx_table->+24 into the effect flag, +24's vfn[10](c, 1, 1), and every
// part of the owner (+8: +180 28-byte parts, +184 count) with a +12
// object gets vfn[22](obj, part +16 != 0); then +248 = 0. Returns 0.
void TalkUpdate(PPUContext& ctx, u32 one, u32 fx_table) {
    const u32 t = arg<u32>(ctx, 0);
    const u64 ra = ctx.gpr[4], rb = ctx.gpr[5];
    const u32 a = static_cast<u32>(ra) & 0xFFu, b = static_cast<u32>(rb) & 0xFFu;
    const u32 f8 = load<u32>(t + 252);
    const u32 p12 = load<u32>(t + 12);
    if ((f8 & 0x02000000u) != 0 || (f8 & 0x40000000u) == 0) {
        ctx.gpr[3] = p12;
        return;
    }
    u32 s = load<u32>(t + 160);
    const u32 owner = load<u32>(t + 8);
    const u32 st = s >> 1;
    if (st != 0 && st != 6) {
        const u32 f = (f8 & ~0x30000000u) | ((static_cast<u32>(ra) & 1) << 29) | ((static_cast<u32>(rb) & 1) << 28);
        store<u32>(t + 252, f);
        s = (s & 1) | 12;
        store<u32>(t + 240, 0xBF800000u);
        store<u32>(t + 160, s);
        callf(ctx, tsfn::Speaker_SayId, {load<u32>(t + 28), load<u32>(t + 224), 0}, {std::bit_cast<f64>(LfsBits(load<u32>(one)))});
        const u32 r = static_cast<u32>(ctx.gpr[3]);
        store<u32>(t + 252, (load<u32>(t + 252) & ~0x01000000u) | ((r & 1) << 24));
        if (const u32 e = load<u32>(t + 20); e != 0) {
            const u32 r2 = static_cast<u32>(call(ctx, tsfn::Effect_SetHash, {e, load<u32>(fx_table + 24)}));
            store<u32>(t + 252, (load<u32>(t + 252) & ~0x00800000u) | ((r2 & 1) << 23));
        }
        if (const u32 c = load<u32>(t + 24); c != 0) VCall(ctx, c, 40, {c, 1, 1});
        u32 n = load<u32>(owner + 184);
        for (u32 i = 0; i < n; ++i) {
            const u32 part = load<u32>(owner + 180) + 28 * i;
            const u32 obj = load<u32>(part + 12);
            if (obj == 0) continue;
            VCall(ctx, obj, 88, {obj, load<u32>(part + 16) != 0 ? 1u : 0u});
            n = load<u32>(owner + 184);
        }
    } else if (((f8 & 0x20000000u) != 0 || a != 0) && st == 0) {
        VCall(ctx, t, 12, {t});  // (the vtable as read on entry)
    } else if (((f8 & 0x10000000u) != 0 || b != 0) && st == 0 && t != 0) {
        u32 key = p12;
        if (key == 0) key = owner != 0 ? load<u32>(owner + 68) : 0;
        Stack frame(ctx, 16);
        store<u32>(frame.addr(), key);
        const u32 found = static_cast<u32>(call(ctx, tsfn::SortedMap_Find3, {tsfn::kPending, frame.addr()}));
        if (load<u32>(found) == 0) {
            u32 k = load<u32>(t + 12);
            if (k == 0) {
                const u32 ow = load<u32>(t + 8);
                k = ow != 0 ? load<u32>(ow + 68) : 0;
            }
            const u32 arr = tsfn::kPending;
            u32 cnt = load<u16>(arr + 2);
            const u32 cap = load<u16>(arr);
            if (cnt == cap) {
                call(ctx, tsfn::Array_Reserve, {arr, (cap << 1) & 0xFFFEu});
                cnt = load<u16>(arr + 2);
            }
            const u32 data = load<u32>(arr + 8);
            store<u32>(data + ((cnt << 3) & 0x7FFF8u), k);
            store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, t);
            const u32 n2 = load<u16>(arr + 2) + 1;
            const u32 hi = load<u16>(arr + 4), af = load<u16>(arr + 6);
            store<u16>(arr + 2, static_cast<u16>(n2));
            store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(n2 & 0xFFFF) ? hi : n2));
            store<u16>(arr + 6, static_cast<u16>(af | 1));
        }
    }
    store<u32>(t + 248, 0);
    ctx.gpr[3] = 0;
}

}  // namespace

// u32 f(T* t, u8 start, u8 stop): TalkUpdate (1.0f at 0x0037C7D8, effect hash table 0x00F64FE0).
DECOMP(0037C7E0, Talk_Update_0037C7E0) { TalkUpdate(ctx, 0x0037C7D8, 0x00F64FE0); }

// u32 f(T* t, u8 start, u8 stop): TalkUpdate (1.0f at 0x0037F5E4, effect hash table 0x00F64FFC).
DECOMP(0037F5E8, Talk_Update_0037F5E8) { TalkUpdate(ctx, 0x0037F5E4, 0x00F64FFC); }

// ---------------------------------------------------------------- KVarData notifications

namespace {

constexpr u32 kNodeKindTable = 0x00F6503C;  // 28 bytes per node kind: OPDs (name?)
constexpr u32 kVarTypeTable = 0x00F65100;   // 44 bytes per KVarData type (as mid4_kvar.cpp)

// The OPD in slot `off` of the table entry of the s16 kind at p, called
// with (p, s) when set; returns whether it was.
bool KindCall(PPUContext& ctx, u32 table, u32 stride, u32 off, u32 p, u32 s) {
    const s32 kind = static_cast<s16>(load<u16>(p));
    const u32 opd = load<u32>(table + static_cast<u32>(kind) * stride + off);
    ctx.gpr[3] = opd;
    if (opd == 0) return false;
    call_ptr(ctx, opd, {p, s, opd, load<u32>(opd)});
    return true;
}

// void f(Node* n, Script* s): the node kind's handler (kNodeKindTable slot
// k1) with (&n->+18, s); then, for a node with its u8 +16 set, the type
// handler (kVarTypeTable slot k2) of each of the +28 count (s16) KVarData
// entries of s from index n->+46 + 1 (as KVarEntry in mid4_kvar.cpp: 0
// when invalid, the handler then read at address 4, as the original) with
// (&entry->+4, s), and the entry's +12 cleared; for a node without, the
// type handler of n->+28 with (&n->+28, s).
void NotifyVars(PPUContext& ctx, u32 k1, u32 k2) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 s = arg<u32>(ctx, 1);
    KindCall(ctx, kNodeKindTable, 28, k1, n + 18, s);
    const u32 x = n + 28;
    if (load<u8>(n + 16) == 0) {
        KindCall(ctx, kVarTypeTable, 44, k2, x, s);
        return;
    }
    u32 cnt = load<u16>(x);
    ctx.gpr[3] = cnt;
    if (!(static_cast<s32>(static_cast<s16>(cnt)) > 0)) return;
    u32 j = 0;
    do {
        const s32 i = static_cast<s16>(load<u16>(x + 18) + j + 1);
        u32 e = 0;
        if (i < static_cast<s32>(load<u16>(s + 144))) {
            e = s + 152 + static_cast<u32>(i) * 132;
            if (!(static_cast<s32>(load<u32>(e)) < 10)) e = 0;
        }
        if (KindCall(ctx, kVarTypeTable, 44, k2, e + 4, s)) cnt = load<u16>(x);
        ctx.gpr[3] = cnt;
        j = (j + 1) & 0xFFFF;
        store<u32>(e + 12, 0);
    } while (static_cast<s32>(j) < static_cast<s32>(static_cast<s16>(cnt)));
}

}  // namespace

// void f(Node* n, Script* s): NotifyVars (kind slot +12, type slot +16).
DECOMP(0038B588, Node_NotifyVars_0038B588) { NotifyVars(ctx, 12, 16); }

// void f(Node* n, Script* s): NotifyVars (kind slot +16, type slot +20).
DECOMP(0038B78C, Node_NotifyVars_0038B78C) { NotifyVars(ctx, 16, 20); }

// ---------------------------------------------------------------- factory registries

namespace {

namespace rfn {
constexpr u32 Array_Reserve = 0x00558F3C;   // (Array*, u16 capacity)
constexpr u32 qsort = 0x0071EC28;           // (base, n, size, cmp OPD)
constexpr u32 Mem_Alloc = 0x0054ED38;       // (size, pool, align)
constexpr u32 NodeOwner_Init = 0x004D2BFC;  // (p, name, 0, 0)
}  // namespace rfn

// u16 f(Registry* r, Factory* const* f, u32 n): each factory f[i] called
// (r3 = its OPD, r4 its entry, as the original leaves them) for a new
// object, keyed by its vfn[3](obj) and pushed onto r's array at a {+0 u16
// capacity, +2 count, +4 high-water, +6 flags (bit 0 unsorted), +8 {key,
// obj}[], +20 the compare function} (grown when full); an unsorted array
// is sorted (qsort, 8-byte entries) when it is not empty. Then every
// object gets a 52-byte binding owner (Mem_Alloc(52, 0, 16), NodeOwner_
// Init(p, obj->vfn[2](obj), 0, 0); 0 when out of memory) at its +8 and its
// vfn[last / 4]() called. Returns the count.
void RegisterFactories(PPUContext& ctx, u32 a, u32 last) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 f = arg<u32>(ctx, 1);
    const u32 n = arg<u32>(ctx, 2);
    const u32 arr = r + a;
    u32 fl = load<u16>(arr + 6);
    u32 cnt = load<u16>(arr + 2);
    for (u32 i = 0; i < n; ++i) {
        const u32 opd = load<u32>(f + 4 * i);
        const u32 obj = static_cast<u32>(call_ptr(ctx, opd, {opd, load<u32>(opd)}));
        const u32 key = static_cast<u32>(VCall(ctx, obj, 12, {obj}));
        u32 c = load<u16>(arr + 2);
        const u32 cap = load<u16>(arr);
        if (c == cap) {
            call(ctx, rfn::Array_Reserve, {arr, (cap << 1) & 0xFFFEu});
            c = load<u16>(arr + 2);
        }
        const u32 data = load<u32>(arr + 8);
        store<u32>(data + ((c << 3) & 0x7FFF8u), key);
        store<u32>(data + (static_cast<u32>(load<u16>(arr + 2)) << 3) + 4, obj);
        cnt = load<u16>(arr + 2) + 1;
        const u32 hi = load<u16>(arr + 4);
        fl = load<u16>(arr + 6) | 1;
        store<u16>(arr + 2, static_cast<u16>(cnt));
        store<u16>(arr + 4, static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(cnt & 0xFFFF) ? hi : cnt));
        store<u16>(arr + 6, static_cast<u16>(fl));
    }
    if ((fl & 1) != 0) {
        store<u16>(arr + 6, static_cast<u16>(fl & 0xFFFE));
        if (static_cast<s32>(cnt & 0xFFFF) > 0) call(ctx, rfn::qsort, {load<u32>(arr + 8), cnt & 0xFFFF, 8, load<u32>(arr + 20)});
    }
    for (u32 j = 0;; ++j) {
        const u32 c = load<u16>(arr + 2);
        ctx.gpr[3] = c;
        if (!(static_cast<s32>(j) < static_cast<s32>(c))) break;
        const u32 obj = load<u32>(load<u32>(arr + 8) + 8 * j + 4);
        const u32 p = static_cast<u32>(call(ctx, rfn::Mem_Alloc, {52, 0, 16}));
        u32 owner = 0;
        if (p != 0) {
            const u32 name = static_cast<u32>(VCall(ctx, obj, 8, {obj}));
            call(ctx, rfn::NodeOwner_Init, {p, name, 0, 0});
            owner = p;
        }
        store<u32>(obj + 8, owner);
        VCall(ctx, obj, last, {obj});
    }
}

}  // namespace

// u16 f(Registry* r, Factory* const* f, u32 n): RegisterFactories (array +56, then vfn[4]).
DECOMP(004F32F4, Registry_AddFactories_004F32F4) { RegisterFactories(ctx, 56, 16); }

// u16 f(Registry* r, Factory* const* f, u32 n): RegisterFactories (array +8, then vfn[5]).
DECOMP(004F3554, Registry_AddFactories_004F3554) { RegisterFactories(ctx, 8, 20); }

// ---------------------------------------------------------------- handle pools

namespace {

// The free mark of a handle-pool entry's +12 word: its top byte | 0x80,
// the generation (bits 1..16) cleared; byte then word stored.
void HandleEntryFree(u32 a) {
    const u32 b = load<u8>(a) | 0x80u;
    const u32 w = (((load<u32>(a) & 0x00FFFFFFu) | (b << 24)) & ~0x7FFF8000u);
    store<u8>(a, static_cast<u8>(b));
    store<u32>(a, w);
}

// void f(u32* out, HandlePool* p, u32 handle): frees handle (index << 16 |
// generation) in p {+0 / +4 the free list's first / last index (-1:
// none), +8 / +12 the used list's first / last, +16 count, +20 frees,
// +24 capacity, +28 16-byte entries {+0 next, +4 prev, +12 bit 31 free,
// bits 15..30 generation}}: a live entry of that generation is unlinked
// from the used list and appended to the free list. *out = the handle of
// the used list's previous entry (0xFFFF0000 for the last; 0 for an
// invalid handle).
void HandlePoolFree(PPUContext& ctx) {
    const u32 out = arg<u32>(ctx, 0);
    const u32 p = arg<u32>(ctx, 1);
    const u32 h = arg<u32>(ctx, 2);
    const u32 cnt = load<u32>(p + 16);
    const s32 tail = static_cast<s32>(load<u32>(p + 4));
    const s32 last = static_cast<s32>(load<u32>(p + 12));
    u32 res = 0;
    const u32 gen = h & 0xFFFF;
    const s32 idx = static_cast<s32>(h >> 16);
    if (cnt != 0 && gen != 0) {
        const s32 cap = static_cast<s32>(load<u32>(p + 24));
        const u32 base = load<u32>(p + 28);
        const u32 e = base + (static_cast<u32>(idx) << 4);
        if (idx < cap && ((load<u32>(e + 12) >> 15) & 0xFFFF) == gen && (load<u8>(e + 12) & 0x80) == 0) {
            if (idx != static_cast<s32>(load<u32>(p + 8))) {
                const u32 next = load<u32>(e), prev = load<u32>(e + 4);
                store<u32>(base + (next << 4) + 4, prev);
            } else {
                store<u32>(p + 8, load<u32>(e + 4));
            }
            if (idx == last) {
                const u32 next = load<u32>(e);
                res = 0xFFFF0000u;
                store<u32>(p + 12, next);
            } else {
                const u32 prev = load<u32>(e + 4), next = load<u32>(e);
                store<u32>(base + (prev << 4), next);
                const u32 prev2 = load<u32>(e + 4);
                const u32 w = load<u32>(base + ((prev2 << 4) & 0xFFFF0u) + 12);
                res = (prev2 << 16) | ((w >> 15) & 0xFFFF);
            }
            if (tail == -1) {
                store<u32>(e, 0xFFFFFFFFu);
                store<u32>(e + 4, 0xFFFFFFFFu);
                HandleEntryFree(load<u32>(p + 28) + (static_cast<u32>(idx) << 4) + 12);
                store<u32>(p + 4, static_cast<u32>(idx));
                store<u32>(p, static_cast<u32>(idx));
            } else {
                store<u32>(base + (static_cast<u32>(tail) << 4) + 4, static_cast<u32>(idx));
                store<u32>(e, static_cast<u32>(tail));
                const u32 e2 = load<u32>(p + 28) + (static_cast<u32>(idx) << 4);
                store<u32>(e2 + 4, 0xFFFFFFFFu);
                HandleEntryFree(e2 + 12);
                store<u32>(p + 4, static_cast<u32>(idx));
                if (cap == 1) store<u32>(p, static_cast<u32>(idx));
            }
            const u32 c1 = cnt - 1;
            store<u32>(p + 16, c1);
            store<u32>(p + 20, load<u32>(p + 20) + 1);
            if (c1 == 0) {
                store<u32>(p + 12, 0xFFFFFFFFu);
                store<u32>(p + 8, 0xFFFFFFFFu);
            }
        }
    }
    store<u32>(out, res);
}

}  // namespace

// void f(u32* out, HandlePool* p, u32 handle): HandlePoolFree (template instance).
DECOMP(00773C8C, HandlePool_Free_00773C8C) { HandlePoolFree(ctx); }

// void f(u32* out, HandlePool* p, u32 handle): HandlePoolFree (template instance).
DECOMP(0078967C, HandlePool_Free_0078967C) { HandlePoolFree(ctx); }

namespace {

// u32 f(HandlePool* p, s32 cap): p cleared (+0..+20), +24 = cap; for a
// non-zero cap the entries come from Mem_Alloc(cap * 16 + 4, 0, 16) (a
// count word ahead of them; 0 when out of memory), each with an 8-byte
// zeroed block (Mem_Alloc(8, 0, 16), 0 on failure; v2: +4, +0 and the u16
// +6 cleared) at its +8; +28 = the entries, u8 +32 = 1; then for a
// non-zero cap the entries are chained into the free list (+0 = i - 1, +4
// = i + 1, +12 free and generation 0; the first's +0 and the last's +4 =
// -1), +8 / +12 = -1 (no used entry), +0 = 0, +4 = cap - 1 (-1 for a cap
// up to 1), +16 = 0, +20 = cap. Returns +4 (1 for a zero cap).
void HandlePoolInit(PPUContext& ctx, bool v2) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 cap = arg<u32>(ctx, 1);
    for (u32 k = 0; k <= 20; k += 4) store<u32>(p + k, 0);
    store<u32>(p + 24, cap);
    u32 ents = 0;
    if (cap != 0) {
        const u32 a = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {(cap << 4) + 4, 0, 16}));
        if (a != 0) {
            store<u32>(a, cap);
            ents = a + 4;
            for (s32 i = 0; i < static_cast<s32>(cap); ++i) {
                const u32 x = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {8, 0, 16}));
                if (x != 0) {
                    if (v2) {
                        store<u32>(x + 4, 0);
                        store<u32>(x, 0);
                        store<u16>(x + 6, 0);
                    } else {
                        store<u32>(x, 0);
                        store<u32>(x + 4, 0);
                    }
                }
                store<u32>(ents + 16 * static_cast<u32>(i) + 8, x);
            }
        }
    }
    store<u32>(p + 28, ents);
    store<u8>(p + 32, 1);
    if (cap == 0) {
        ret(ctx, 1u);
        return;
    }
    s32 n = static_cast<s32>(load<u32>(p + 24));
    for (s32 i = 0; i < n;) {
        const u32 e = load<u32>(p + 28) + 16 * static_cast<u32>(i);
        store<u32>(e, static_cast<u32>(i - 1));
        store<u32>(e + 4, static_cast<u32>(i + 1));
        store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80));
        const u32 e2 = load<u32>(p + 28) + 16 * static_cast<u32>(i) + 12;
        store<u32>(e2, load<u32>(e2) & ~0x7FFF8000u);
        ++i;
        n = static_cast<s32>(load<u32>(p + 24));
    }
    store<u32>(load<u32>(p + 28), 0xFFFFFFFFu);
    store<u32>(load<u32>(p + 28) + (static_cast<u32>(n) << 4) - 12, 0xFFFFFFFFu);
    const s32 c = static_cast<s32>(load<u32>(p + 24));
    store<u32>(p + 8, 0xFFFFFFFFu);
    store<u32>(p + 12, 0xFFFFFFFFu);
    store<u32>(p, 0);
    const u32 r = c > 1 ? static_cast<u32>(c - 1) : 0xFFFFFFFFu;
    store<u32>(p + 4, r);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, static_cast<u32>(c));
    ret(ctx, r);
}

}  // namespace

// u32 f(HandlePool* p, s32 cap): HandlePoolInit (template instance).
DECOMP(00773A90, HandlePool_Init_00773A90) { HandlePoolInit(ctx, false); }

// u32 f(HandlePool* p, s32 cap): HandlePoolInit (the blocks' u16 +6 cleared too).
DECOMP(00788EF4, HandlePool_Init_00788EF4) { HandlePoolInit(ctx, true); }

namespace {

constexpr u32 kMemmove = 0x0071C3F0;     // (dst, src, n)
constexpr u32 kMemFreeFrom = 0x0054EE6C; // (p, pool)

// u32 f(HandlePool* p, s32 cap): grows / shrinks p to cap entries (as
// HandlePoolInit: new entries with their 8-byte blocks, v2 clearing the
// u16 +6 too), the old ones copied over (memmove of the old cap *
// 16 bytes) and freed with their blocks, last first); the new entries
// are chained after the old free list's last (+4) entry, +4 = cap - 1,
// +20 (free count) += cap - old cap, +24 = cap (and +0 = the old cap
// when the free list was empty). Returns the free count (the cap when
// unchanged).
void HandlePoolResize(PPUContext& ctx, bool v2) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 cap = arg<u32>(ctx, 1);
    const u32 cur = load<u32>(p + 24);
    ctx.gpr[3] = cur;
    if (cap == cur) return;
    u32 ents = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {(cap << 4) + 4, 0, 16}));
    if (ents != 0) {
        store<u32>(ents, cap);
        ents += 4;
        for (s32 i = 0; i < static_cast<s32>(cap); ++i) {
            const u32 x = static_cast<u32>(call(ctx, m4fn::Mem_Alloc, {8, 0, 16}));
            if (x != 0) {
                if (v2) {
                    store<u32>(x + 4, 0);
                    store<u32>(x, 0);
                    store<u16>(x + 6, 0);
                } else {
                    store<u32>(x, 0);
                    store<u32>(x + 4, 0);
                }
            }
            store<u32>(ents + 16 * static_cast<u32>(i) + 8, x);
        }
    }
    call(ctx, kMemmove, {ents, load<u32>(p + 28), load<u32>(p + 24) << 4});
    if (const u32 old = load<u32>(p + 28); old != 0) {
        const u32 hdr = old - 4;
        for (s32 k = static_cast<s32>(load<u32>(hdr)) - 1; k >= 0; --k) {
            const u32 e = old + 16 * static_cast<u32>(k);
            if (const u32 b = load<u32>(e + 8); b != 0) {
                call(ctx, kMemFreeFrom, {b, 0});
                store<u32>(e + 8, 0);
            }
        }
        call(ctx, kMemFreeFrom, {hdr, 0});
    }
    store<u32>(p + 28, ents);
    const u32 oc = load<u32>(p + 24);
    store<u32>(ents + (oc << 4), load<u32>(p + 4));
    u32 nfree = load<u32>(p + 20);
    u32 oc2 = load<u32>(p + 24);
    u32 base = load<u32>(p + 28);
    if (nfree != 0) store<u32>(base + (load<u32>(p + 4) << 4) + 4, oc);
    store<u32>(base + (oc2 << 4) + 4, oc2 + 1);
    if (static_cast<s32>(oc2 + 1) < static_cast<s32>(cap)) {
        for (u32 i = oc2 + 1; i != cap; ++i) {
            const u32 e = load<u32>(p + 28) + 16 * i;
            store<u32>(e, i - 1);
            store<u32>(e + 4, i + 1);
            store<u8>(e + 12, static_cast<u8>(load<u8>(e + 12) | 0x80));
            const u32 e2 = load<u32>(p + 28) + 16 * i + 12;
            store<u32>(e2, load<u32>(e2) & ~0x7FFF8000u);
        }
        nfree = load<u32>(p + 20);
        oc2 = load<u32>(p + 24);
        base = load<u32>(p + 28);
    }
    store<u32>(base + (cap << 4) - 12, 0xFFFFFFFFu);
    if (nfree == 0) store<u32>(p, oc2);
    store<u32>(p + 4, cap - 1);
    const u32 f = nfree + (cap - oc2);
    store<u32>(p + 20, f);
    store<u32>(p + 24, cap);
    ctx.gpr[3] = f;
}

}  // namespace

// u32 f(HandlePool* p, s32 cap): HandlePoolResize (template instance).
DECOMP(00773E8C, HandlePool_Resize_00773E8C) { HandlePoolResize(ctx, false); }

// u32 f(HandlePool* p, s32 cap): HandlePoolResize (the blocks' u16 +6 cleared too).
DECOMP(007893B0, HandlePool_Resize_007893B0) { HandlePoolResize(ctx, true); }

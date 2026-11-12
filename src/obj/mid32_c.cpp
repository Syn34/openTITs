// Game code, medium sweep part 32: collectible bookkeeping (masks and
// treasures) of the per-episode progress record.
//
// The session object o: +20 / +24 players (860 bytes each: +724 id, +748
// u32 ids[u8 +820], u8 +822 slot, u8 +824 mask slot, u8 +825 progress slot,
// u8 +826 inactive, u8 +828 counted), +36 the local player, +60 mask count.
// The world *0x00F65CA0: u8 +4208 episode, +4216 player object,
// +0x128114 current key; its episode record rec = world + episode * 4056.
#include "mid32.h"

using namespace sly4;
using namespace sly4::mid32;

namespace {
using namespace m32fn;
constexpr u32 kWorld32 = 0x00F65CA0u;      // World*
constexpr u32 kRoster32 = 0x00F64C20u;     // roster: +20 players (860 bytes), +24 count
constexpr u32 kOwnedTab32 = 0x01068184u;   // u16 +2 count, +8 8-byte entries (+4 record*, u8 +0 owner, u8 +1 bit)
constexpr u32 kMaskTab32 = 0x0106819Cu;    // mask info map (Map_Find76642C)
constexpr u32 TrophyAward32 = 0x000B27B4u; // Trophy_Award0B27B4(hash)
constexpr u32 CountTrophy32 = 0x002E619Cu; // Players_CountTrophy_002E619C(o)
constexpr u32 StrFormat32 = 0x005534D0u;   // String_Format(fmt, ...)
constexpr u32 HashStr32 = 0x0056A6E0u;     // Hash_ToString(hash)
constexpr u32 MapFindA32 = 0x007662A4u;    // Map_Find7662A4(map, const u32* key) -> &value
constexpr u32 MapFindB32 = 0x0076642Cu;    // Map_Find76642C(map, const u32* key) -> &value
constexpr u32 SMapFind32 = 0x0073B608u;    // SortedMap_Find73B608(map, const u32* key) -> &value
constexpr u32 PlayerCount32 = 0x002DAC24u; // Player_Count2DAC24(player, rec + 80)
constexpr u32 StartAction32 = 0x00489A60u; // Player_StartAction(world, obj, a2..a7, u8, u8)

u32 EpRec32(u32 w) { return w + load<u8>(w + 4208) * 4056u; }

// Players of o (inactive ones skipped) whose ids include the world's
// current key: the first such player, or 0.
u32 KeyPlayer32(u32 o, u32 w) {
    const u32 key = load<u32>(w + 0x128114u);
    const u32 n = load<u32>(o + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 p = load<u32>(o + 20) + 860 * i;
        if (load<u8>(p + 826) != 0) continue;
        const u32 m = load<u8>(p + 820);
        for (u32 j = 0; j < m; ++j)
            if (load<u32>(p + 748 + 4 * j) == key) return p;
    }
    return 0;
}

// The owned-record count of the roster player with this id: its roster
// index (-1 when absent) counted among the owners (u8) of kOwnedTab32.
u32 OwnedCount32(u32 id) {
    const u32 g = load<u32>(kRoster32);
    s32 idx = -1;
    const u32 n = load<u32>(g + 24);
    for (u32 k = 0; k < n; ++k)
        if (load<u32>(load<u32>(g + 20) + 860 * k + 724) == id) {
            idx = static_cast<s32>(k);
            break;
        }
    u32 c = 0;
    for (s32 k = 0; k < static_cast<s32>(load<u16>(kOwnedTab32 + 2)); ++k)
        if (static_cast<s32>(load<u8>(load<u32>(load<u32>(kOwnedTab32 + 8) + 8 * k + 4))) == idx) ++c;
    return c;
}

u32 PopU16_32(u32 a) { return static_cast<u32>(std::popcount(static_cast<u32>(load<u16>(a)))); }

// u16 at a |= 1 << b (slw: 0 for b & 63 >= 32).
void OrBit32(u32 a, u32 b) {
    const u32 sh = b & 63;
    store<u16>(a, static_cast<u16>(load<u16>(a) | (sh < 32 ? 1u << sh : 0u)));
}
}  // namespace

// u32 Mask_Recount_002E6320(Session* o, Player* local): total = the bits
// set in the progress words rec+204[u8 +825] of o's players, sum = their
// owned-record counts. The local player (when set) gets bit 0x40 of
// rec+496[4 * u8 +822] once its owned count is nonzero and covered by its
// own progress bits. Trophy 0xA2474384 when total reaches *0x0106846C; when
// 0 < sum <= total, every player gets bit 0x40 too. Tail:
// r3 = Players_CountTrophy_002E619C(o).
DECOMP(002E6320, Mask_Recount_002E6320) {
    const u32 o = arg<u32>(ctx, 0), local = arg<u32>(ctx, 1);
    const u32 w = load<u32>(kWorld32);
    const u32 rec = EpRec32(w);
    u32 n = load<u32>(o + 24), pl = load<u32>(o + 20);
    u32 total = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 p = pl + 860 * i;
        if (p == 0) continue;
        const u32 s = load<u8>(p + 825);
        if (s == 255) continue;
        total += PopU16_32(rec + 204 + 2 * s);
    }
    u32 sum = 0;
    for (u32 i = 0; i < n; ++i) sum += OwnedCount32(load<u32>(pl + 860 * i + 724));
    if (local != 0) {
        const u32 s = load<u8>(local + 825);
        const u32 pop = s != 255 ? PopU16_32(rec + 204 + 2 * s) : 0u;
        const u32 cnt = OwnedCount32(load<u32>(local + 724));
        if (!(pop < cnt) && cnt != 0) {
            const u32 a = rec + 4 * load<u8>(local + 822) + 496;
            store<u16>(a, static_cast<u16>(load<u16>(a) | 0x40));
        }
    }
    if (!(static_cast<s32>(load<u32>(0x0106846Cu)) > static_cast<s32>(total))) {
        call(ctx, TrophyAward32, {0xA2474384u});
        pl = load<u32>(o + 20);
        n = load<u32>(o + 24);
    }
    if (!(static_cast<s32>(sum) > static_cast<s32>(total)) && static_cast<s32>(sum) > 0 && n != 0) {
        s32 left = static_cast<s32>(n);
        u32 p = pl;
        do {
            --left;
            const u32 w2 = load<u32>(kWorld32);
            const u32 a = EpRec32(w2) + 4 * load<u8>(p + 822) + 496;
            p += 860;
            store<u16>(a, static_cast<u16>(load<u16>(a) | 0x40));
        } while (left > 0);
    }
    ctx.gpr[3] = call(ctx, CountTrophy32, {o});
}

// void Treasure_Register_002E66F4(Session* o, Spawner* sp, Obj* by): for an
// active (+188 bit 0) collector by, the treasure sp->+220 t is recorded for
// the player o->+36 (or the first player holding the world's key): with a
// progress slot s (u8 +825, not 255; s >= 12 prints "Episode Index is out
// of bounds"), the treasure's entry in the owned table (key
// t->+60->+204) sets bit (u8 entry+1) of rec+80+124[s] and bit 0x20 of
// rec+80+348, else "Couldn't find treasure ..." is printed; both recount
// (Mask_Recount_002E6320(o, o->+36)). t's vslot 28(t, 0) always ends it.
// r3 as the original leaves it.
DECOMP(002E66F4, Treasure_Register_002E66F4) {
    const u32 o = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1), by = arg<u32>(ctx, 2);
    if (sp == 0 || by == 0) return;  // r3 = o
    if ((load<u32>(by + 188) & 1u) == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 w = load<u32>(kWorld32);
    const s32 team = static_cast<s8>(load<u8>(by + 172));
    const u32 t = load<u32>(sp + 220);
    const u32 rec = w + load<u8>(w + static_cast<u32>(team) * 0x20C80u + 4208) * 4056u + 80;
    if (t == 0) {
        ret(ctx, w);
        return;
    }
    u32 p = load<u32>(o + 36);
    if (p == 0) p = KeyPlayer32(o, w);
    if (p != 0) {
        const u32 s = load<u8>(p + 825);
        if (s != 255) {
            if (s >= 12) {
                call(ctx, StrFormat32, {0x00B1D308u, s, 12});  // "Episode Index is out of bounds '%i'! ..."
            } else {
                u32 v = 0;
                const u32 d = load<u32>(load<u32>(t + 60) + 204);
                if (d != 0) {
                    Stack frame(ctx, 160);
                    store<u32>(frame.addr() + 112, d);
                    v = load<u32>(static_cast<u32>(call(ctx, MapFindA32, {kOwnedTab32, frame.addr() + 112})));
                }
                if (v != 0) {
                    OrBit32(rec + 2 * s + 124, load<u8>(v + 1));
                    store<u32>(rec + 348, load<u32>(rec + 348) | 0x20);
                } else {
                    const u32 h = static_cast<u32>(call(ctx, HashStr32, {load<u32>(sp + 168)}));
                    const u32 def = load<u32>(t + 60);
                    const u32 nm = static_cast<u32>(VCall(ctx, def, 12, {def}));
                    call(ctx, StrFormat32, {0x00AD8E08u, h, nm});  // "Couldn't find treasure '%s' ..."
                }
                call(ctx, 0x002E6320u, {o, load<u32>(o + 36)});
            }
        }
    }
    ctx.gpr[3] = VCall(ctx, t, 28, {t, 0});
}

// void Mask_Collect_002E58C8(Session* o, Spawner* sp): the mask sp->+220 m
// collected by the player o->+36 (or the first player holding the world's
// key) with mask slot s (u8 +824, not 255), when the hash 0xCE33CC21 is
// registered in *0x00F65BE8 +28: m's entry in the mask info map (key
// m->+60->+204) sets bit (u8 entry+1) of rec+80+100[s], else "Couldn't
// find mask ..." is printed; vslot 28(m, 0). o->+60 = the sum of
// Player_Count2DAC24 over the counted players (u8 +828); trophies
// 0xA35F4F24 (first mask) and 0x5CDF8715 (count reaching *0x010682A4, when
// *0x00F64C28 is set). The first time (bit 0x100 of rec+80+348) the
// first-mask notice is started (Player_StartAction). r3 as the original
// leaves it.
DECOMP(002E58C8, Mask_Collect_002E58C8) {
    const u32 o = arg<u32>(ctx, 0), sp = arg<u32>(ctx, 1);
    Stack frame(ctx, 192);
    const u32 f = frame.addr();
    u32 p = load<u32>(o + 36);
    if (p == 0) p = KeyPlayer32(o, load<u32>(kWorld32));
    const u32 w = load<u32>(kWorld32);
    const u32 ep = w + 4208;
    const u32 rec = EpRec32(w) + 80;
    const u32 s = p != 0 ? load<u8>(p + 824) : 255u;
    store<u32>(f + 132, 0xCE33CC21u);
    const u32 v = load<u32>(static_cast<u32>(call(ctx, SMapFind32, {load<u32>(0x00F65BE8u) + 28, f + 132})));
    if (s == 255 || v == 0 || sp == 0) {
        ret(ctx, v);
        return;
    }
    const u32 m = load<u32>(sp + 220);
    if (m == 0) {
        ctx.gpr[3] = call(ctx, HashStr32, {load<u32>(sp + 168)});
        return;
    }
    u32 e = 0;
    const u32 d = load<u32>(load<u32>(m + 60) + 204);
    if (d != 0) {
        store<u32>(f + 128, d);
        e = load<u32>(static_cast<u32>(call(ctx, MapFindB32, {kMaskTab32, f + 128})));
    }
    if (e != 0) {
        OrBit32(rec + 2 * s + 100, load<u8>(e + 1));
    } else {
        const u32 h = static_cast<u32>(call(ctx, HashStr32, {load<u32>(sp + 168)}));
        const u32 def = load<u32>(m + 60);
        const u32 nm = static_cast<u32>(VCall(ctx, def, 12, {def}));
        call(ctx, StrFormat32, {0x00AD8D30u, h, nm});  // "Couldn't find mask '%s' ..."
    }
    VCall(ctx, m, 28, {m, 0});
    u32 sum = 0;
    u32 n = load<u32>(o + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 q = load<u32>(o + 20) + 860 * i;
        if (load<u8>(q + 828) == 0) continue;
        const u32 w2 = load<u32>(kWorld32);
        sum += static_cast<u32>(call(ctx, PlayerCount32, {q, EpRec32(w2) + 80}));
        n = load<u32>(o + 24);
    }
    store<u32>(o + 60, sum);
    if (sum >= 1) {
        call(ctx, TrophyAward32, {0xA35F4F24u});
        sum = load<u32>(o + 60);
    }
    if (load<u32>(0x00F64C28u) != 0 && !(sum < load<u32>(0x010682A4u))) call(ctx, TrophyAward32, {0x5CDF8715u});
    const u32 flags = load<u32>(rec + 348);
    if ((flags & 0x100u) != 0) {
        ret(ctx, flags);
        return;
    }
    store<u32>(rec + 348, flags | 0x100);
    // "$STR_NOTIFY_FIRST_MASK", "$BUTTON_X $STR_YES $BUTTON_CIRCLE $STR_NO"
    ctx.gpr[3] = call(ctx, StartAction32, {load<u32>(kWorld32), load<u32>(ep + 8), 0x00AD8DC0u, 0x00AD8DD8u, 0x00F35F80u, 0, 0, 0, 1, 1});
}

namespace {
constexpr u32 TextLookup32 = 0x0034A398u;   // Text_Lookup34A398(id) -> Text* or 0
constexpr u32 FindComp32 = 0x0034CC34u;     // Obj_FindComp34CC34(o, key) -> node or 0
constexpr u32 SayNode32 = 0x00350958u;      // Speaker_Say350958(node, text, u8 rev, -, -, -, r9, r10; f1, f2, f3)
constexpr u32 SayNew32 = 0x003514ACu;       // Speaker_Say3514AC(spk, text, u8 rev, -, -, -, r9, r10, voice; f1, f2, f3)
constexpr u32 LastVoice32 = 0x00350860u;    // Obj_SetLastVoice_00350860(node, voice)
constexpr u32 TrackLen32 = 0x00353748u;     // Track_Length353748(node) -> f1
constexpr u32 SetSign32 = 0x003541E0u;      // Obj_SetSign_003541E0(node; f1)
constexpr u32 EmitStop32 = 0x00443424u;     // Emitter_Stop(emitter, 1)
constexpr u32 kFixedRate32 = 0x0105AE90u;   // u8: use the fixed rate constant

// The {emitter +384, key +388} pairs of o (s32 count +448) whose key is
// key (or all with key -1) are stopped (Emitter_Stop(e, 1)) and removed by
// moving the last pair into their place (the vacated pair is zeroed).
void StopVoices32(PPUContext& ctx, u32 o, u32 key) {
    s32 n = static_cast<s32>(load<u32>(o + 448));
    for (s32 i = 0; i < n; ++i) {
        const u32 slot = o + 384 + 8 * i;
        if (!(load<u32>(slot + 4) == key || key == 0xFFFFFFFFu)) continue;
        call(ctx, EmitStop32, {load<u32>(slot), 1});
        u32 c = load<u32>(o + 448) - 1;
        store<u32>(o + 448, c);
        store<u32>(slot, load<u32>(o + 384 + 8 * c));
        c = load<u32>(o + 448);
        store<u32>(slot + 4, load<u32>(o + 388 + 8 * c));
        store<u32>(o + 384 + 8 * c, 0);
        c = load<u32>(o + 448);
        store<u32>(o + 388 + 8 * c, 0);
        n = static_cast<s32>(load<u32>(o + 448));
        --i;
    }
}

void StoreF32_32(u32 ea, f64 v) { store<u32>(ea, std::bit_cast<u32>(static_cast<f32>(v))); }
}  // namespace

// bool Actor_Say_00276EC0(Actor* o, u32 text, u32 key, u8 rev, -, -, -,
// s32 wsel, u32 line (stack), -, u32 voice (stack); f32 weight, f32 blend,
// f32 rate, f32 dur): the voices of key are stopped; the line text (Text_
// Lookup34A398) is said on o->+36's node key (Speaker_Say350958) or on a
// new speaker (Speaker_Say3514AC), with the rate replaced by the constant
// 0x00276EB8 while u8 0x0105AE90 is set, and tagged with voice. With dur >
// that constant the node's sign is (Track_Length - blend) / dur. Without
// o->+36 or the text: Hash_ToString(text), o->+8->vfn[76]() and r3 = 0;
// else r3 = 1.
DECOMP(00276EC0, Actor_Say_00276EC0) {
    const u32 o = arg<u32>(ctx, 0), text = arg<u32>(ctx, 1), key = arg<u32>(ctx, 2);
    const u32 rev = static_cast<u8>(ctx.gpr[6]);
    const u64 wsel = ctx.gpr[10];
    const u32 sp0 = static_cast<u32>(ctx.gpr[1]);
    const u32 line = load<u32>(sp0 + 116), voice = load<u32>(sp0 + 132);
    const f64 weight = ctx.fpr[1], blend = ctx.fpr[2], dur = ctx.fpr[4];
    f64 rate = ctx.fpr[3];
    StopVoices32(ctx, o, key);
    u32 t = 0;
    if (load<u32>(o + 36) != 0) t = static_cast<u32>(call(ctx, TextLookup32, {text}));
    if (load<u32>(o + 36) == 0 || t == 0) {
        call(ctx, HashStr32, {text});
        const u32 x = load<u32>(o + 8);
        VCall(ctx, x, 76, {x});
        ret(ctx, 0u);
        return;
    }
    if (load<u8>(kFixedRate32) != 0) rate = Lf(0x00276EB8u);
    u32 node = static_cast<u32>(call(ctx, FindComp32, {load<u32>(o + 36), key}));
    if (node != 0) {
        callf(ctx, SayNode32, {node, t, rev, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], wsel, line}, {weight, rate, blend});
        call(ctx, LastVoice32, {node, voice});
    } else {
        callf(ctx, SayNew32, {load<u32>(o + 36), t, rev, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], wsel, line, voice}, {weight, rate, blend});
        node = static_cast<u32>(ctx.gpr[3]);
        call(ctx, LastVoice32, {node, voice});
    }
    if (node != 0) ctx.fpr[1] = Lf(0x00276EB8u);  // f1 as the original leaves it
    if (node != 0 && dur > ctx.fpr[1]) {
        const f64 len = callf(ctx, TrackLen32, {node}, {});
        const f64 sign = static_cast<f64>(static_cast<f32>(static_cast<f64>(static_cast<f32>(len - blend)) / dur));
        const u32 o2 = load<u32>(o + 36);
        if (o2 != 0) {
            const u32 n2 = static_cast<u32>(call(ctx, FindComp32, {o2, key}));
            if (n2 != 0) callf(ctx, SetSign32, {n2}, {sign});
        }
    }
    ret(ctx, 1u);
}

// bool Actor_SayPair_00277210(Actor* o, u32 text1, u32 text2, -, -, -,
// u32 key1, u32 key2, u8 rev, -, -, s32 wsel, u32 voice1, u32 voice2
// (stack); f32 sign1, f32 sign2, f32 weight, f32 rate, f32 blend): the
// voices of key1 are stopped; both lines (Text_Lookup34A398) are said on
// o->+36's nodes key1 / key2 (Speaker_Say350958, r10 = 0; +16 / +20 = sign1
// / sign2, tagged voice1 / voice2) and node 2's sign becomes
// Track_Length(2) / (weight * Track_Length(1)); without both nodes line 1
// goes to a new speaker (Speaker_Say3514AC). The rate is the constant
// 0x00277208 while u8 0x0105AE90 is set. r3 = 0 without o->+36 or a text,
// else 1.
DECOMP(00277210, Actor_SayPair_00277210) {
    const u32 o = arg<u32>(ctx, 0), text1 = arg<u32>(ctx, 1), text2 = arg<u32>(ctx, 2);
    const u32 key1 = static_cast<u32>(ctx.gpr[8]), key2 = static_cast<u32>(ctx.gpr[9]);
    const u32 rev = static_cast<u8>(ctx.gpr[10]);
    const u32 sp0 = static_cast<u32>(ctx.gpr[1]);
    const u32 wsel = load<u32>(sp0 + 140), voice1 = load<u32>(sp0 + 148), voice2 = load<u32>(sp0 + 156);
    const f64 sign1 = ctx.fpr[1], sign2 = ctx.fpr[2], weight = ctx.fpr[3], blend = ctx.fpr[5];
    f64 rate = ctx.fpr[4];
    StopVoices32(ctx, o, key1);
    if (load<u32>(o + 36) == 0) return ret(ctx, 0u);
    const u32 t1 = static_cast<u32>(call(ctx, TextLookup32, {text1}));
    if (t1 == 0) return ret(ctx, 0u);
    const u32 t2 = static_cast<u32>(call(ctx, TextLookup32, {text2}));
    if (t2 == 0) return ret(ctx, 0u);
    if (load<u8>(kFixedRate32) != 0) rate = Lf(0x00277208u);
    const u32 n1 = static_cast<u32>(call(ctx, FindComp32, {load<u32>(o + 36), key1}));
    const u32 n2 = n1 != 0 ? static_cast<u32>(call(ctx, FindComp32, {load<u32>(o + 36), key2})) : 0u;
    if (n1 == 0 || n2 == 0) {
        callf(ctx, SayNew32, {load<u32>(o + 36), t1, rev, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], wsel, 0, voice1}, {weight, rate, blend});
        const u32 s = static_cast<u32>(ctx.gpr[3]);
        if (s != 0) call(ctx, LastVoice32, {s, voice1});
        return ret(ctx, 1u);
    }
    callf(ctx, SayNode32, {n1, t1, rev, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], wsel, 0}, {weight, rate, blend});
    StoreF32_32(n1 + 20, sign1);
    StoreF32_32(n1 + 16, sign1);
    call(ctx, LastVoice32, {n1, voice1});
    callf(ctx, SayNode32, {n2, t2, rev, ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], wsel, 0}, {weight, rate, blend});
    StoreF32_32(n2 + 20, sign2);
    StoreF32_32(n2 + 16, sign2);
    call(ctx, LastVoice32, {n2, voice2});
    const f64 len2 = callf(ctx, TrackLen32, {n2}, {});
    const f64 len1 = callf(ctx, TrackLen32, {n1}, {});
    const f64 q = static_cast<f64>(static_cast<f32>(len2 / static_cast<f64>(static_cast<f32>(weight * len1))));
    callf(ctx, SetSign32, {n2}, {q});
    ret(ctx, 1u);
}

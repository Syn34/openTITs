// Game code, part 46: the small-function sweep continued from the remaining
// 99 instruction functions (size order; obj45*.cpp did 97-99). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj46.h"

using namespace sly4;
using namespace sly4::obj46;

// ---------------------------------------------------------------- sweep: 99 instructions
// T* f(T* o, a1, a2, u8* cfg, a4): SubMachine_Construct(o, a1, cfg,
// a2->+72, a4) (r7 passed through: the callee reads it), then the fields:
// bytes +53/+54/+68/+80..83/+87/+97 = 0, +84..86 = 1, +60/+76/+92/+100 = 0,
// +72 = 1.0f, vtable 0x00B208F8, +88 = a2, bit 0x80000000 of +96 cleared;
// PtrVector_Construct(o + 104); u16 +110 = 4; the three rows of the 4x4
// at 0x01164BD0 into +128/+144/+160/+176 (12 bytes each); +192..212 = 0,
// bytes +228/+229/+236..240/+248 = 0, +232/+244 = 0; +28 |= 0x1000.
// r3 = o + 28.
DECOMP(004015D0, Obj_Construct_B208F8_004015D0) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), a2 = arg<u32>(ctx, 2), cfg = arg<u32>(ctx, 3), a4 = arg<u32>(ctx, 4);
    call(ctx, 0x0047C5D8u, {o, a1, cfg, load<u32>(a2 + 72), a4});
    store<u8>(o + 53, 0);
    store<u8>(o + 54, 0);
    store<u32>(o + 60, 0);
    store<u8>(o + 68, 0);
    store<u32>(o + 72, 0x3F800000u);
    store<u32>(o + 76, 0);
    store<u8>(o + 80, 0);
    store<u8>(o + 81, 0);
    store<u8>(o + 82, 0);
    store<u8>(o + 83, 0);
    const u32 w96 = load<u32>(o + 96) & 0x7FFFFFFFu;
    store<u8>(o + 84, 1);
    store<u8>(o + 85, 1);
    store<u8>(o + 86, 1);
    store<u8>(o + 87, 0);
    store<u32>(o, 0x00B208F8u);
    store<u32>(o + 88, a2);
    store<u32>(o + 92, 0);
    store<u32>(o + 96, w96);
    store<u8>(o + 97, 0);
    store<u32>(o + 100, 0);
    call(ctx, 0x005516E0u, {o + 104});
    constexpr u32 g = 0x01164BD0u;
    Cp45(o + 128, g);
    Cp45(o + 132, g + 4);
    Cp45(o + 136, g + 8);
    Cp45(o + 144, g + 16);
    Cp45(o + 148, g + 20);
    Cp45(o + 152, g + 24);
    store<u16>(o + 110, 4);
    Cp45(o + 160, g + 32);
    Cp45(o + 164, g + 36);
    Cp45(o + 168, g + 40);
    Cp45(o + 176, g + 48);
    store<u32>(o + 192, 0);
    Cp45(o + 180, g + 52);
    store<u32>(o + 196, 0);
    Cp45(o + 184, g + 56);
    for (u32 off = 200; off <= 212; off += 4) store<u32>(o + off, 0);
    store<u8>(o + 228, 0);
    store<u8>(o + 229, 0);
    store<u32>(o + 232, 0);
    for (u32 off = 236; off <= 240; ++off) store<u8>(o + off, 0);
    store<u32>(o + 244, 0);
    store<u8>(o + 248, 0);
    store<u32>(o + 28, load<u32>(o + 28) | 0x1000u);
    ctx.fpr[1] = Lfs45(g + 56);  // f1 as the copy leaves it
    ret(ctx, o + 28);
}

// void f(T* o): byte +479 = 0; Handle_Call363A3C(0xBB9F16F3, 0, 1, -, 0,
// 0, 0x01164CA4; -1.0f); o->+92->vt[88](it, 1, 1, 1, 1);
// SetSelect_491000(o, 0x164A530C); the child 0x435538CB of o->+92 (when
// found) ->vt[88](it, 0, 1, 1, 1). Then with c = *(o->vt[76](o) +
// 0x1281E0): c set and Obj_HasWord368_000FC5DC(c, o->+60) == 0 ->
// Camera_PushTarget0FC040(c, o->+60, -, 0; -1.0f). r3 = the last result.
DECOMP(00412D50, Obj_Enter_00412D50) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 k = Lfs45(0x00412D4Cu);  // -1.0f
    store<u8>(o + 479, 0);
    // r6: the caller's, unread by the callee
    callf(ctx, 0x0036735Cu, {0xBB9F16F3u, 0, 1, ctx.gpr[6], 0, 0, 0x01164CA4u}, {k});
    const u32 c92 = load<u32>(o + 92);
    VCall(ctx, c92, 88, {c92, 1, 1, 1, 1});
    call(ctx, 0x00491000u, {o, 0x164A530Cu});
    if (static_cast<u32>(call(ctx, 0x000DB290u, {load<u32>(o + 92), 0x435538CBu})) != 0) {
        const u32 ch = static_cast<u32>(call(ctx, 0x000DB290u, {load<u32>(o + 92), 0x435538CBu}));
        VCall(ctx, ch, 88, {ch, 0, 1, 1, 1});
    }
    const u32 r = static_cast<u32>(VCall(ctx, o, 76, {o}));
    const u32 c = load<u32>(r + 0x001281E0u);
    if (c == 0) {
        ret(ctx, r);
        return;
    }
    if (static_cast<u32>(call(ctx, 0x000FC5DCu, {c, load<u32>(o + 60)})) != 0) return;
    // r5: left by the previous call, unread by the callee
    callf(ctx, 0x000FC040u, {c, load<u32>(o + 60), ctx.gpr[5], 0}, {k});
}

// SpawnNamed46: u32 f(Owner* o, const char* name): name 0 -> r3 = o. Else the def of
// String_HashNoCase(name) in the map *(0x00F8F154)->+16 (PointMap_Find);
// none -> String_Format(0x00AE5284, name, o->+4). Else a record of the
// given size built by ctor(p, {def, o}) (Mem_Alloc(size, 0, 16)); the global list
// 0x00F65BE8 (u16 cap +4, count +6, peak +8, flags +10, data +12; 8-byte
// entries) full at 512 -> String_Format(0x00AE5160, 512). Else (grown to
// twice the capacity by Array_Reserve when full) the entry {rec->+60->+204,
// rec} is appended, the peak raised, flags |= 1 (r3 = the flags).
// Quirk: a failed allocation still reads rec->+60 from address 60.
static void SpawnNamed46(PPUContext& ctx, u32 size, u32 ctor) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1);
    if (name == 0) {
        ret(ctx, o);
        return;
    }
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s + 8, 0);
    store<u32>(s + 4, 0);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710u, {name}));
    store<u32>(s, h);
    const u32 found = static_cast<u32>(call(ctx, 0x0073343Cu, {load<u32>(load<u32>(0x00F8F154u) + 16), s}));
    const u32 def = load<u32>(found);
    if (def == 0) {
        call(ctx, 0x005534D0u, {0x00AE5284u, name, load<u32>(o + 4)});
        return;
    }
    store<u32>(s + 4, def);
    store<u32>(s + 8, o);
    const u32 a = static_cast<u32>(call(ctx, 0x0054ED38u, {size, 0, 16}));
    u32 rec = 0;
    if (a != 0) {
        call(ctx, ctor, {a, s + 4});
        rec = a;
    }
    const u32 g = load<u32>(0x00F65BE8u);
    const u32 n = load<u16>(g + 6);
    if (static_cast<s32>(n) >= 512) {
        call(ctx, 0x005534D0u, {0x00AE5160u, 512});
        return;
    }
    const u32 v = load<u32>(load<u32>(rec + 60) + 204);
    const u32 cap = load<u16>(g + 4);
    if (n == cap) call(ctx, 0x00558F3Cu, {g + 4, (cap << 1) & 0xFFFEu});
    const u32 n2 = load<u16>(g + 6);
    const u32 data = load<u32>(g + 12);
    const u32 n3 = load<u16>(g + 6);
    store<u32>(data + n2 * 8, v);
    const u32 peak = load<u16>(g + 8);
    store<u32>(data + n3 * 8 + 4, rec);
    const u32 cnt = n3 + 1;
    const u32 fl = load<u16>(g + 10) | 1u;
    store<u16>(g + 6, static_cast<u16>(cnt));
    store<u16>(g + 8, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(cnt & 0xFFFFu) ? peak : cnt));
    store<u16>(g + 10, static_cast<u16>(fl));
    ret(ctx, fl);
}

// The 92-byte record of 0x004551C8.
DECOMP(00455468, Obj_SpawnNamed_00455468) { SpawnNamed46(ctx, 92, 0x004551C8u); }

// bool f(s32 id): the 152-byte entry e of the table 0x010BE8BC (count
// 0x010C1A9C) with e->+0 == id, its flag byte b = *(0x00F65CA0)->+80 +
// 3948 + index. Unless (b & 1) == 0 and *(0x00F64C20)->+36 != 0, the
// balance +32 of that block is >= e->+12 and (b & 2) == 0: b |= 3, the
// balance -= e->+12; then with c = the entries whose flag has bit 2:
// c >= 1 -> Trophy_Award0B27B4(0xE5FEEEF1), c >= count ->
// Trophy_Award0B27B4(0x4E8EB622); r3 = 1. Else r3 = 0.
DECOMP(0049E0DC, Unlock_Buy_0049E0DC) {
    const u32 id = arg<u32>(ctx, 0);
    constexpr u32 tbl = 0x010BE8BCu, kcount = 0x010C1A9Cu;
    const u32 base = load<u32>(0x00F65CA0u) + 80;
    const u32 n = load<u32>(kcount);
    u32 i = 0;
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    for (;;) {
        if (load<u32>(tbl + 152 * i) == id) break;
        ++i;
        if (!(n > i)) {
            ret(ctx, 0);
            return;
        }
    }
    const u32 fp = base + i + 3948;
    const u32 b = load<u8>(fp);
    if ((b & 1u) == 0 && load<u32>(load<u32>(0x00F64C20u) + 36) != 0) {
        ret(ctx, 0);
        return;
    }
    const u32 e = tbl + 152 * i;
    if (static_cast<s32>(load<u32>(base + 32)) < static_cast<s32>(load<u32>(e + 12)) || (b & 2u) != 0) {
        ret(ctx, 0);
        return;
    }
    store<u8>(fp, static_cast<u8>(b | 3u));
    store<u32>(base + 32, load<u32>(base + 32) - load<u32>(e + 12));
    const u32 blk = load<u32>(0x00F65CA0u);
    u32 cnt = load<u32>(kcount);
    u32 c = 0;
    if (cnt != 0) {
        for (u32 j = 0; j < cnt; ++j)
            if ((load<u8>(blk + 4028 + j) & 2u) != 0) ++c;
        if (c >= 1) {
            call(ctx, 0x000B27B4u, {0xE5FEEEF1u});
            cnt = load<u32>(kcount);
        }
    }
    if (!(cnt > c)) call(ctx, 0x000B27B4u, {0x4E8EB622u});
    ret(ctx, 1);
}

// u32 f(void* dst, Value* v): v 0 -> r3 = dst. Else by the kind
// v->vt[32](v): 12 and 2 -> *dst = v->vt[24](v) (u32); 1 -> *dst = the f1
// of v->vt[20](v); 4 -> the 4x3 floats v->+32/+48/+64/+80 into dst rows of
// 16 bytes; 3 -> the vector v->+32 into dst. r3 = the last result.
DECOMP(0049E8E4, Value_Get_0049E8E4) {
    const u32 dst = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    if (v == 0) {
        ret(ctx, dst);
        return;
    }
    const s32 t = static_cast<s32>(static_cast<u32>(VCall(ctx, v, 32, {v})));
    if (t == 12 || t == 2) {
        store<u32>(dst, static_cast<u32>(VCall(ctx, v, 24, {v})));
    } else if (t == 4) {
        u32 w[12];
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) w[3 * r + k] = load<u32>(v + 32 + 16 * r + 4 * k);
        for (u32 r = 0; r < 4; ++r)
            for (u32 k = 0; k < 3; ++k) store<u32>(dst + 16 * r + 4 * k, w[3 * r + k]);
        ctx.fpr[1] = Lfs45(v + 32);  // f1 as the copy leaves it
    } else if (t == 3) {
        const u32 x = load<u32>(v + 32), y = load<u32>(v + 36), z = load<u32>(v + 40);
        store<u32>(dst, x);
        store<u32>(dst + 4, y);
        store<u32>(dst + 8, z);
        ctx.fpr[1] = Lfs45(v + 32);  // f1 as the copy leaves it
    } else if (t == 1) {
        VCall(ctx, v, 20, {v});
        store<u32>(dst, W45(ctx.fpr[1]));
    }
}

// u32 f(Def* o, XmlCursor* c): vtable 0x00B22470, u16 +4 = 1, the vectors
// +16 / +48 = 0x01164B30, +12/+32/+64/+68/+72 = 0; then the hash
// 0x8A93973F into +12 (default 0); when absent: the vec3 0x00210FAA into
// +16, the hash 0x075F231D into +32, the vec3 2683 into +48, the hashes
// 0x04C4EAAC / 0xB7EA3D94 (f32) / 0x63D534F3 into +64 / +68 / +72 (the
// defaults being the current values). r3 = the last read's result.
DECOMP(004CB5F8, XmlDef_Read_B22470_004CB5F8) {
    const u32 o = arg<u32>(ctx, 0), c = arg<u32>(ctx, 1);
    constexpr u32 g = 0x01164B30u;
    store<u32>(o + 12, 0);
    store<u16>(o + 4, 1);
    store<u32>(o, 0x00B22470u);
    Cp45(o + 16, g);
    Cp45(o + 20, g + 4);
    store<u32>(o + 32, 0);
    Cp45(o + 24, g + 8);
    Cp45(o + 48, g);
    Cp45(o + 52, g + 4);
    store<u32>(o + 64, 0);
    Cp45(o + 56, g + 8);
    store<u32>(o + 68, 0);
    store<u32>(o + 72, 0);
    ctx.fpr[1] = Lfs45(g + 8);  // f1 as the copy leaves it
    if (static_cast<u32>(call(ctx, 0x004FA14Cu, {c, 0x8A93973Fu, o + 12, 0})) != 0) return;
    call(ctx, 0x004F9E0Cu, {c, 0x00210FAAu, o + 16, g});
    call(ctx, 0x004FA14Cu, {c, 0x075F231Du, o + 32, load<u32>(o + 32)});
    call(ctx, 0x004F9E0Cu, {c, 2683, o + 48, g});
    call(ctx, 0x004FA14Cu, {c, 0x04C4EAACu, o + 64, load<u32>(o + 64)});
    callf(ctx, 0x004F9B20u, {c, 0xB7EA3D94u, o + 68}, {Lfs45(o + 68)});
    call(ctx, 0x004FA14Cu, {c, 0x63D534F3u, o + 72, load<u32>(o + 72)});
}

// ~T() [vtable 0x00B0E888]: the PtrVectors +492/+472/+452, SmallFree of
// the buffer +428 (+444 * 16 bytes), the PtrVectors +408 down to +308 (step
// 20), then the base 0x00B0DC70: NavQuery_Free62D08C(o + 216).
DECOMP(007392D8, VtB0E888_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B0E888u);
    for (u32 off : {492u, 472u, 452u}) VecDtor(ctx, o + off);
    call(ctx, 0x00552420u, {load<u32>(o + 428), load<u32>(o + 444) << 4});
    for (u32 off = 408; off >= 308; off -= 20) VecDtor(ctx, o + off);
    store<u32>(o, 0x00B0DC70u);
    call(ctx, 0x0062D08Cu, {o + 216});
}

// ---------------------------------------------------------------- sweep: 100 instructions
// u32 f(T* o): unless the word o->+48->+172 has bit 0x40 (r3 = the word)
// or 0x20 (r3 = 1): o->vt[236](o); the basis A (+128) = B (+144) x C (+160),
// then B = C x A; Matrix_NormalizeInPlace(A, C, B); the rows A, B, C and
// the position +176 into o->+48 +96/+112/+128/+144. r3 = o->+48.
DECOMP(000131C8, Obj_SyncBasis_000131C8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(load<u32>(o + 48) + 172);
    if (((w >> 6) & 1u) != 0) {
        ret(ctx, w);
        return;
    }
    if (((w >> 5) & 1u) != 0) {
        ret(ctx, 1);
        return;
    }
    VCall(ctx, o, 236, {o});
    const u32 a = o + 128, b = o + 144, c = o + 160;
    f64 t = F45(Lfs45(b + 8) * Lfs45(c + 4));
    store<u32>(a, W45(Fms45(Lfs45(b + 4), Lfs45(c + 8), t)));
    t = F45(Lfs45(b) * Lfs45(c + 8));
    const f64 ay = Fms45(Lfs45(b + 8), Lfs45(c), t);
    store<u32>(a + 4, W45(ay));
    t = F45(Lfs45(b + 4) * Lfs45(c));
    const f64 az = Fms45(Lfs45(b), Lfs45(c + 4), t);
    store<u32>(a + 8, W45(az));
    t = F45(Lfs45(c + 8) * ay);
    store<u32>(b, W45(Fms45(Lfs45(c + 4), az, t)));
    t = F45(Lfs45(c) * Lfs45(a + 8));
    store<u32>(b + 4, W45(Fms45(Lfs45(c + 8), Lfs45(a), t)));
    t = F45(Lfs45(c + 4) * Lfs45(a));
    store<u32>(b + 8, W45(Fms45(Lfs45(c), Lfs45(a + 4), t)));
    call(ctx, 0x0056D374u, {a, c, b});
    const u32 d = load<u32>(o + 48);
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) Cp45(d + 96 + 16 * r + 4 * k, o + 128 + 16 * r + 4 * k);
    ctx.fpr[1] = Lfs45(o + 184);  // f1 as the copy leaves it
    ret(ctx, d);
}

// bool f(-, XmlCursor* c, PtrVector* v): c 0 or v not empty -> 0. Counts
// the children of c for which Tree_FindPath_004B6DA0 succeeds (none -> 0),
// reserves that many, then appends Tree_SeekB_004B6C88(child) of each
// such child when nonzero (doubling the capacity when full, raising the
// peak +10). r3 = 1.
DECOMP(0009810C, Xml_CollectPaths_0009810C) {
    const u32 cur = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    if (cur == 0 || load<u16>(v + 8) != 0) {
        ret(ctx, 0);
        return;
    }
    u32 ch = static_cast<u32>(call(ctx, 0x004F9140u, {cur}));
    u32 n = 0;
    if (ch == 0) {
        ret(ctx, 0);
        return;
    }
    do {
        if (static_cast<u32>(call(ctx, 0x004B6DA0u, {ch})) != 0) ++n;
        ch = static_cast<u32>(call(ctx, 0x004F8D74u, {ch}));
    } while (ch != 0);
    if (n == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x00551718u, {v, n});
    for (ch = static_cast<u32>(call(ctx, 0x004F9140u, {cur})); ch != 0; ch = static_cast<u32>(call(ctx, 0x004F8D74u, {ch}))) {
        if (static_cast<u32>(call(ctx, 0x004B6DA0u, {ch})) == 0) continue;
        const u32 x = static_cast<u32>(call(ctx, 0x004B6C88u, {ch}));
        if (x == 0) continue;
        const u32 cap = load<u16>(v + 4);
        if (load<u16>(v + 8) == cap) call(ctx, 0x00551718u, {v, (cap << 1) & 0x1FFFEu});
        const u32 n1 = load<u16>(v + 8);
        const u32 data = load<u32>(v);
        const u32 n2 = load<u16>(v + 8);
        const u32 peak = load<u16>(v + 10);
        store<u32>(data + ((n1 << 2) & 0x3FFFCu), x);
        const u32 cnt = n2 + 1;
        store<u16>(v + 8, static_cast<u16>(cnt));
        store<u16>(v + 10, static_cast<u16>(static_cast<s32>(peak) > static_cast<s32>(cnt & 0xFFFFu) ? peak : cnt));
    }
    ret(ctx, 1);
}

// Rec* f(-, XmlCursor* c): a 24-byte record (Mem_Alloc(24, 0, 16); 0 ->
// r3 = 0), Obj_InitVt_B21C98(p, c), vtable 0x00B10F88, the hashes
// 0x040EFE5F / 0x03F47A7A into +12 / +16; then by the kind hash
// 0x995789E4: 0x03FE2A3C -> the f32 0x001FE7AA into +20 (default
// 0x0009AF5C), 0x0021B90E -> the hash, 0x00011BCF -> the s32. r3 = p.
DECOMP(0009AF60, XmlRec_New_B10F88_0009AF60) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {24, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p, 0x00B10F88u);
    call(ctx, 0x004FA14Cu, {cur, 0x040EFE5Fu, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x03F47A7Au, p + 16, load<u32>(p + 16)});
    Stack frame(ctx, 16);
    const u32 s = frame.addr();
    store<u32>(s, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x995789E4u, s, 0});
    const u32 kind = load<u32>(s);
    if (kind == 0x03FE2A3Cu)
        callf(ctx, 0x004F9B20u, {cur, 0x001FE7AAu, p + 20}, {Lfs45(0x0009AF5Cu)});
    else if (kind == 0x0021B90Eu)
        call(ctx, 0x004FA14Cu, {cur, 0x001FE7AAu, p + 20, 0});
    else if (kind == 0x00011BCFu)
        call(ctx, 0x004F99BCu, {cur, 0x001FE7AAu, p + 20, 0});
    ret(ctx, p);
}

// Rec* f(-, XmlCursor* c): a 40-byte record (Mem_Alloc(40, 0, 16); 0 ->
// r3 = 0), Obj_InitVt_B21C98(p, c), vtable 0x00B11348, +12..+36 = 0; the
// hashes 0x7A30D7E3 / 0x93116591 / 0xCF0502B9 / 0xF636DCCA into +12..+24
// and the f32s 0xB7EA3D94 / 0xBE36C5F4 / 0x000110AD into +28..+36 (the
// defaults being the current values). r3 = p.
DECOMP(0009EC00, XmlRec_New_B11348_0009EC00) {
    const u32 cur = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38u, {40, 0, 16}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x004BB3B0u, {p, cur});
    store<u32>(p + 12, 0);
    store<u32>(p + 16, 0);
    store<u32>(p + 20, 0);
    store<u32>(p + 24, 0);
    store<u32>(p, 0x00B11348u);
    store<u32>(p + 28, 0);
    store<u32>(p + 32, 0);
    store<u32>(p + 36, 0);
    call(ctx, 0x004FA14Cu, {cur, 0x7A30D7E3u, p + 12, 0});
    call(ctx, 0x004FA14Cu, {cur, 0x93116591u, p + 16, load<u32>(p + 16)});
    call(ctx, 0x004FA14Cu, {cur, 0xCF0502B9u, p + 20, load<u32>(p + 20)});
    call(ctx, 0x004FA14Cu, {cur, 0xF636DCCAu, p + 24, load<u32>(p + 24)});
    callf(ctx, 0x004F9B20u, {cur, 0xB7EA3D94u, p + 28}, {Lfs45(p + 28)});
    callf(ctx, 0x004F9B20u, {cur, 0xBE36C5F4u, p + 32}, {Lfs45(p + 32)});
    callf(ctx, 0x004F9B20u, {cur, 0x000110ADu, p + 36}, {Lfs45(p + 36)});
    ret(ctx, p);
}

// u32 f(Obj* o): o->vt[28](o, 0, 0); every component c (any kind) gets
// c->vt[16](c) and Body_SetMode(c, 1); every kind 21 component
// Call_28EBB8_0(c); +436 / +440 = 0; o->+484 and o->+488 (when set)
// ->vt[40](it, 0, 0). r3 = the last result (or +484 when both are 0).
DECOMP(00122830, Obj_ResetBodies_00122830) {
    const u32 o = arg<u32>(ctx, 0);
    VCall(ctx, o, 28, {o, 0, 0});
    for (u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 0, 0})); c != 0; c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 0, c}))) {
        VCall(ctx, c, 16, {c});
        call(ctx, 0x00261328u, {c, 1});
    }
    for (u32 c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 21, 0})); c != 0; c = static_cast<u32>(call(ctx, 0x000D1288u, {o, 21, c})))
        call(ctx, 0x0028EE68u, {c});
    const u32 x = load<u32>(o + 484);
    store<u32>(o + 436, 0);
    store<u32>(o + 440, 0);
    u32 r = x;
    if (x != 0) r = static_cast<u32>(VCall(ctx, x, 40, {x, 0, 0}));
    const u32 y = load<u32>(o + 488);
    if (y != 0) r = static_cast<u32>(VCall(ctx, y, 40, {y, 0, 0}));
    ret(ctx, r);
}

// u32 f(Out* out, Obj* x): with p = CallK2_000D94F4(x) set: out->+8 =
// x->+728, out->+4 = p->+1232 (t); t > k0 -> t = k1 / t; p->+1224 /
// +1228 = k1 / (t + k2) (k = 0x0016CEC8); p->+1232 = the f1 of
// Obj_RandRange_0025DD60(p); x->+728 = out->+8 + 1 and
// Obj_PlayCue_002971BC(p, that). Then unless x->+348 <= k0 and x->+692 is
// already 0x03DE9A54 (r3 = that, f1 = +348): Anim_Restart2775A8(
// x->vt[60](x), 0xD467C476, 0, 0, 0, -1, 0, 0, -, -, 0, -1; k1, k3, k0,
// k0) and +692 = r3 = 0xD467C476.
DECOMP(0016CED8, Obj_RestartIdle_0016CED8) {
    const u32 out = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    constexpr u32 k = 0x0016CEC8u;
    const u32 p = static_cast<u32>(call(ctx, 0x000D94F4u, {x}));
    if (p != 0) {
        f64 t = Lfs45(p + 1232);
        store<u32>(out + 8, load<u32>(x + 728));
        Cp45(out + 4, p + 1232);
        if (t > Lfs45(k)) t = F45(Lfs45(k + 4) / t);
        t = F45(Lfs45(k + 4) / F45(t + Lfs45(k + 8)));
        store<u32>(p + 1224, W45(t));
        store<u32>(p + 1228, W45(t));
        ctx.fpr[1] = t;  // f1 as the original has it at the call
        call(ctx, 0x0025DD60u, {p});
        store<u32>(p + 1232, W45(ctx.fpr[1]));
        const u32 n = load<u32>(out + 8) + 1;
        store<u32>(x + 728, n);
        call(ctx, 0x002971BCu, {p, n});
    }
    const f64 k0 = Lfs45(k);
    const f64 f = Lfs45(x + 348);
    if (f <= k0 && load<u32>(x + 692) == 0x03DE9A54u) {
        ctx.fpr[1] = f;
        ret(ctx, 0x03DE9A54u);
        return;
    }
    const u32 an = static_cast<u32>(VCall(ctx, x, 60, {x}));
    // Arguments 9 / 10 (r1 +112 / +120) are never written; the callee doesn't read them.
    callf(ctx, 0x002775A8u, {an, 0xD467C476u, 0, 0, 0, ~0ull, 0, 0, 0, 0, 0, ~0ull}, {Lfs45(k + 4), Lfs45(k + 12), k0, k0});
    store<u32>(x + 692, 0xD467C476u);
    ret(ctx, 0xD467C476u);
}

// u32 f(Obj* o): once (byte +592), when o->+216->+240 is 0xA970EC1F (else
// r3 = o->+216): the box of o->+216->vt[12](&box, it) gives the extent d =
// max - min; Shape_Init110EC(&shape, *(0x00F557EC), the frame {identity
// rows of 0x01164BD0, pos (o->+144, o->+148 + d.y * *(0x001B4844),
// o->+152)}, &d, 0x00AD1494, 0xFFFF); Volume_SetShape637D14(o + 588,
// &shape), Shape_Release638498(&shape); +592 = 1. r3 = 1.
DECOMP(001B4848, Obj_BuildVolume_001B4848) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 216);
    if (load<u32>(c + 240) != 0xA970EC1Fu || load<u8>(o + 592) != 0) {
        ret(ctx, c);
        return;
    }
    // sp+112 extent (s), +128 box (s + 16), +160 frame (s + 48), +224 shape (s + 112)
    Stack frame(ctx, 128);
    const u32 s = frame.addr();
    const u32 opd = load<u32>(load<u32>(c) + 12);
    call_ptr(ctx, opd, {s + 16, c, opd, load<u32>(opd)});
    const f64 dx = F45(Lfs45(s + 32) - Lfs45(s + 16));
    const f64 dy = F45(Lfs45(s + 36) - Lfs45(s + 20));
    const f64 dz = F45(Lfs45(s + 40) - Lfs45(s + 24));
    constexpr u32 g = 0x01164BD0u;
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) Cp45(s + 48 + 16 * r + 4 * k, g + 16 * r + 4 * k);
    store<u32>(s, W45(dx));
    store<u32>(s + 4, W45(dy));
    store<u32>(s + 8, W45(dz));
    Cp45(s + 96, o + 144);
    Cp45(s + 104, o + 152);
    const f64 y = Fma45(dy, Lfs45(0x001B4844u), Lfs45(o + 148));
    store<u32>(s + 100, W45(y));
    ctx.fpr[1] = y;  // f1 as the original leaves it
    call(ctx, 0x000110ECu, {s + 112, load<u32>(0x00F557ECu), s + 48, s, 0x00AD1494u, 0xFFFF});
    call(ctx, 0x00637D14u, {o + 588, s + 112});
    call(ctx, 0x00638498u, {s + 112});
    store<u8>(o + 592, 1);
    ret(ctx, 1);
}

// u32 f(Obj* o, bool on): on -> World_ListRemove_0046B7A4(*(0x00F65CA0),
// 3, 0x00F34078, o, o), o->+564 (when set) ->vt[40](it, 0) and +564 = 0;
// o->vt[396](o, 4, 0); the skeleton +416 (when set): every bone released
// (AnimSkel_Release7D4EA8), the properties 0xA734004D / 0x91C7CF5B
// removed, the skeleton released, +416 = 0; +428 = 0. Then r3 =
// Obj_SetRagdollBase(o, on).
DECOMP(001FF4B0, Obj_SetRagdoll_001FF4B0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        call(ctx, 0x0046B7A4u, {load<u32>(0x00F65CA0u), 3, 0x00F34078u, o, o});
        const u32 x = load<u32>(o + 564);
        if (x != 0) {
            VCall(ctx, x, 40, {x, 0});
            store<u32>(o + 564, 0);
        }
        VCall(ctx, o, 396, {o, 4, 0});
        if (load<u32>(o + 416) != 0) {
            for (s32 i = 0;; ++i) {
                const s32 n = static_cast<s32>(static_cast<u32>(call(ctx, 0x0091A2CCu, {load<u32>(o + 416)})));
                if (!(i < n)) break;
                const u32 bone = static_cast<u32>(call(ctx, 0x0091A7D8u, {load<u32>(o + 416), static_cast<u32>(i)}));
                call(ctx, 0x007D4EA8u, {bone});
            }
            Stack frame(ctx, 16);
            const u32 s = frame.addr();
            call(ctx, 0x0093FED8u, {s, load<u32>(o + 416), 0xA734004Du});
            call(ctx, 0x0093FED8u, {s + 8, load<u32>(o + 416), 0x91C7CF5Bu});
            call(ctx, 0x007D4EA8u, {load<u32>(o + 416)});
            store<u32>(o + 416, 0);
        }
        store<u32>(o + 428, 0);
    }
    call(ctx, 0x000E02F8u, {o, on});
}

// T* f(T* o, const Desc* d): Thunk_000C2B90(o, d), vtable 0x00B19378; the
// vector d->+92 into +156, +144 / +148 = d->+80 / +84, +164.. = d->+100..
// (floats +156..+172), bytes +176..178 = d->+112..114, +140 / +152 = 0.
// Then the node keyed d->+88 in the tree of Set_Find(0) (unsigned keys;
// left +12 / right +16): the search path goes into the set's stack (u16
// +100, slots +20), the hit (or the last node) into +104; a hit also sets
// the global 0x011DE288 = node->+8 and +152 = r3 = node->+8; else r3 = 0.
// No set -> r3 = 0 (+152 untouched).
DECOMP(0020A0F0, Obj_Construct_B19378_0020A0F0) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000C2B90u, {o, d});
    const u32 w92 = load<u32>(d + 92), w80 = load<u32>(d + 80);
    store<u32>(o + 156, w92);
    const u32 w96 = load<u32>(d + 96);
    store<u32>(o + 144, w80);
    store<u32>(o + 160, w96);
    store<u32>(o, 0x00B19378u);
    const u32 w100 = load<u32>(d + 100);
    store<u32>(o + 140, 0);
    const u32 w84 = load<u32>(d + 84);
    store<u32>(o + 164, w100);
    store<u32>(o + 148, w84);
    const u8 b112 = load<u8>(d + 112);
    const u32 w104 = load<u32>(d + 104);
    store<u32>(o + 152, 0);
    store<u8>(o + 176, b112);
    store<u32>(o + 168, w104);
    const u8 b113 = load<u8>(d + 113);
    const u32 w108 = load<u32>(d + 108);
    store<u8>(o + 177, b113);
    store<u32>(o + 172, w108);
    store<u8>(o + 178, load<u8>(d + 114));
    ctx.fpr[1] = Lfs45(d + 108);  // f1 as the copy leaves it
    const u32 s = static_cast<u32>(call(ctx, 0x004BCE90u, {0}));
    if (s == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 root = load<u32>(load<u32>(s + 16));
    const u32 key = load<u32>(d + 88);
    if (root == 0) {
        store<u32>(s + 104, 0);
        store<u32>(0x011DE288u, 0);
        store<u32>(o + 152, 0);
        ret(ctx, 0);
        return;
    }
    store<u16>(s + 100, 0);
    for (u32 n = root;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            store<u32>(s + 104, n);
            store<u32>(0x011DE288u, load<u32>(n + 8));
            const u32 hit = load<u32>(s + 104);
            const u32 r = hit != 0 ? load<u32>(hit + 8) : 0;
            store<u32>(o + 152, r);
            ret(ctx, r);
            return;
        }
        const u32 nx = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (nx == 0) {
            store<u32>(s + 104, n);  // n is never 0 here
            store<u32>(o + 152, 0);
            ret(ctx, 0);
            return;
        }
        const s32 i = static_cast<s16>(load<u16>(s + 100));
        store<u16>(s + 100, static_cast<u16>(i + 1));
        store<u32>(s + 20 + static_cast<u32>(i) * 4, n);
        n = nx;
    }
}

// T* f(a0, a1, Desc* d, a3): TriggerDef_ReadKinds214C28(a0, a1, d, a3,
// &def) into a stack def (+0 0x00C00000, +12 d->+12, +16 / +20 the empty
// string 0x00AD366C, +32 1, +36 / +40 / +72 -1.0f, +76 the name 0x00AD3704,
// bytes +80 / +81 = 1, u16 +84 0xFFFF, the rest 0); then a 192-byte object
// (SmallAlloc; 0 -> r3 = 0): Thunk_000C2B90(p, &def), bytes +172..175 =
// def +80..83, vtable 0x00B19510, String_CopyN(p + 140, def.name, 31),
// u16 +176 / +178 = def +84 / +86, +180 = def +88, vtable 0x00B19E28, byte
// +173 = 1. r3 = p.
DECOMP(00214E1C, Trigger_New_00214E1C) {
    const u32 a5 = arg<u32>(ctx, 2);
    Stack frame(ctx, 96);  // the def at sp+112..203
    const u32 s = frame.addr();
    store<u32>(s + 16, 0x00AD366Cu);
    store<u32>(s + 12, load<u32>(a5 + 12));
    store<u32>(s + 20, 0x00AD366Cu);
    for (u32 k = 24; k <= 31; ++k) store<u8>(s + k, 0);
    store<u32>(s + 36, 0xBF800000u);
    store<u32>(s + 40, 0xBF800000u);
    store<u8>(s + 44, 0);
    for (u32 k = 48; k <= 68; k += 4) store<u32>(s + k, 0);
    store<u32>(s + 72, 0xBF800000u);
    store<u32>(s + 4, 0);
    store<u32>(s + 8, 0);
    store<u32>(s + 76, 0x00AD3704u);
    store<u8>(s + 80, 1);
    store<u8>(s + 82, 0);
    store<u8>(s + 83, 0);
    store<u16>(s + 84, 0xFFFF);
    store<u16>(s + 86, 0);
    store<u32>(s + 88, 0);
    store<u32>(s, 0x00C00000u);
    store<u32>(s + 32, 1);
    store<u8>(s + 81, 1);
    call(ctx, 0x00214C28u, {ctx.gpr[3], ctx.gpr[4], a5, ctx.gpr[6], s});
    const u32 p = static_cast<u32>(call(ctx, 0x0055238Cu, {192}));
    if (p == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x000C2B90u, {p, s});
    store<u8>(p + 172, load<u8>(s + 80));
    store<u8>(p + 173, load<u8>(s + 81));
    store<u32>(p, 0x00B19510u);
    store<u8>(p + 174, load<u8>(s + 82));
    store<u8>(p + 175, load<u8>(s + 83));
    call(ctx, 0x00553638u, {p + 140, load<u32>(s + 76), 31});
    store<u16>(p + 176, load<u16>(s + 84));
    store<u32>(p + 180, load<u32>(s + 88));
    store<u32>(p, 0x00B19E28u);
    store<u16>(p + 178, load<u16>(s + 86));
    store<u8>(p + 173, 1);
    ret(ctx, p);
}

// u32 f(-, XmlCursor* c, T* o): the bools 0xBEF25C9F / 0x1EF4F9F5 /
// 0x2D407057 / 0x778A1C92 into the bits 0x20 / 0x10 / 0x08 / 0x02 of the
// byte +184 (defaults: the current bits); then the hash 0x51609AAA: its
// index in the 7-entry table 0x00AD3FE4 (when there) into +188. r3 = the
// hash.
DECOMP(0021D838, XmlDef_ReadFlags_0021D838) {
    const u32 cur = arg<u32>(ctx, 1), o = arg<u32>(ctx, 2);
    Stack frame(ctx, 16);  // sp+112 the hash, sp+116 the bool
    const u32 s = frame.addr();
    store<u8>(s + 4, 0);
    u32 b = load<u8>(o + 184);
    call(ctx, 0x004F986Cu, {cur, 0xBEF25C9Fu, s + 4, (b >> 5) & 1u});
    b = (load<u8>(o + 184) & ~0x20u) | ((static_cast<u32>(load<u8>(s + 4)) << 5) & 0x20u);
    store<u8>(o + 184, static_cast<u8>(b));
    call(ctx, 0x004F986Cu, {cur, 0x1EF4F9F5u, s + 4, (b >> 4) & 1u});
    b = (load<u8>(o + 184) & ~0x10u) | ((static_cast<u32>(load<u8>(s + 4)) << 4) & 0x10u);
    store<u8>(o + 184, static_cast<u8>(b));
    call(ctx, 0x004F986Cu, {cur, 0x2D407057u, s + 4, (b >> 3) & 1u});
    b = (load<u8>(o + 184) & ~0x08u) | ((static_cast<u32>(load<u8>(s + 4)) << 3) & 0x08u);
    store<u8>(o + 184, static_cast<u8>(b));
    call(ctx, 0x004F986Cu, {cur, 0x778A1C92u, s + 4, (b >> 1) & 1u});
    b = (load<u8>(o + 184) & ~0x02u) | ((static_cast<u32>(load<u8>(s + 4)) << 1) & 0x02u);
    store<u8>(o + 184, static_cast<u8>(b));
    call(ctx, 0x004FA14Cu, {cur, 0x51609AAAu, s, 0});
    const u32 h = load<u32>(s);
    for (u32 i = 0; i < 7; ++i)
        if (load<u32>(0x00AD3FE4u + 4 * i) == h) {
            store<u32>(o + 188, i);
            break;
        }
    ret(ctx, h);
}

// u32 f(Obj* o, f32 dt): +1264 = 0; unless bit 0x80000000 of +384 is clear
// (r3 = 0): +1380 += dt (t), o->+400->vt[12](it) = info; ready = bit
// 0x1000 of info->+36 and t >= *(0x0024A2D8). With a target owner +396:
// the first Target_Next271C08(CallK2_000D93D4(o), 0, prev) entry other
// than o goes into +1264; bit 0x40000000 of owner->+1268 = ready;
// Obj_SetTarget_00296758(owner, +1264, 0). Then the effect +1364 (when
// set) gets Effect_SetInstance_004CF330(fx, ready ? +1368 : +1372) unless
// already fx->+4. r3 = the last result, f1 = t.
DECOMP(0024A2E0, Obj_TickAim_0024A2E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    const u32 w = load<u32>(o + 384);
    store<u32>(o + 1264, 0);
    if ((w >> 31) == 0) {
        ret(ctx, 0);
        return;
    }
    const f64 t = F45(Lfs45(o + 1380) + dt);
    const u32 c400 = load<u32>(o + 400);
    store<u32>(o + 1380, W45(t));
    ctx.fpr[1] = t;
    const u32 info = static_cast<u32>(VCall(ctx, c400, 12, {c400}));
    const u32 own = load<u32>(o + 396);
    u32 ready = 0;
    if ((load<u32>(info + 36) & 0x1000u) != 0 && Lfs45(o + 1380) >= Lfs45(0x0024A2D8u)) ready = 1;
    if (own != 0) {
        const u32 p = static_cast<u32>(call(ctx, 0x000D93D4u, {o}));
        if (p != 0) {
            for (u32 r = static_cast<u32>(call(ctx, 0x00271C08u, {p, 0, 0})); r != 0; r = static_cast<u32>(call(ctx, 0x00271C08u, {p, 0, r}))) {
                if (r != o) {
                    store<u32>(o + 1264, r);
                    break;
                }
            }
        }
        const u32 ow = load<u32>(o + 396);
        store<u32>(ow + 1268, (load<u32>(ow + 1268) & ~0x40000000u) | (ready << 30));
        call(ctx, 0x00296758u, {load<u32>(o + 396), load<u32>(o + 1264), 0});
    }
    const u32 fx = load<u32>(o + 1364);
    if (fx == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 inst = ready != 0 ? load<u32>(o + 1368) : load<u32>(o + 1372);
    if (inst == load<u32>(fx + 4)) {
        ret(ctx, fx);
        return;
    }
    call(ctx, 0x004CF330u, {fx, inst});
}

// bool f(Session* s, s32 id): the 860-byte entry of the array +20 (count
// +24) with +724 == id (none -> 0): Session_Stop2DDAB0(s), Set_Stop4BD1CC
// of Set_Find(0x7719A548) when found, +40 / +72 / +48 = 0,
// UserBlock_Reset45C7A8 of each user block (*(0x00F65CA0) + 4208, stride
// 0x20C80, count byte +29); +36 = e, 0x002DB8B0(e); bit 0x80 cleared in the
// u16 *(0x00F65CA0) + 496 + 4056 * byte +4208 + 4 * e->+822; World_StartLevel(
// *(0x00F65CA0), e->+744, 0) when that is set. r3 = 1.
DECOMP(002E142C, Session_StartEntry_002E142C) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1);
    const u32 n = load<u32>(o + 24);
    u32 i = 0;
    for (; i < n; ++i)
        if (load<u32>(load<u32>(o + 20) + 860 * i + 724) == id) break;
    if (n == 0 || i >= n) {
        ret(ctx, 0);
        return;
    }
    const u32 e = load<u32>(o + 20) + i * 860;
    if (e == 0) {
        ret(ctx, 0);
        return;
    }
    call(ctx, 0x002DDAB0u, {o});
    const u32 st = static_cast<u32>(call(ctx, 0x004BCE90u, {0x7719A548u}));
    if (st != 0) call(ctx, 0x004BD1CCu, {st});
    store<u32>(o + 40, 0);
    store<u32>(o + 72, 0);
    store<u32>(o + 48, 0);
    for (u32 k = 0, off = 0; k < load<u8>(load<u32>(0x00F65CA0u) + 29); ++k, off += 0x20C80u)
        call(ctx, 0x0045C7A8u, {load<u32>(0x00F65CA0u) + off + 4208});
    store<u32>(o + 36, e);
    call(ctx, 0x002DB8B0u, {e});
    const u32 g = load<u32>(0x00F65CA0u);
    const u32 lev = load<u32>(e + 744);
    const u32 a = g + load<u8>(g + 4208) * 4056 + load<u8>(e + 822) * 4 + 496;
    store<u16>(a, static_cast<u16>(load<u16>(a) & ~0x80u));
    if (lev != 0) call(ctx, 0x004674C4u, {load<u32>(0x00F65CA0u), lev, 0});
    ret(ctx, 1);
}

// bool f(const Key* k, Obj* o): with c = o->+12 (bit 0x8 of +192 set), x =
// c->vt[60](c) and y = x->+36 all set (else 0):
// Obj_SetByteByHash_0035327C(y, the word cell (k->+4, k->+2, 0), the byte
// cell (k->+4, k->+2, 1)) of the table *(0x00F65038). r3 = 1.
DECOMP(00396250, Obj_ApplyCell_00396250) {
    const u32 k = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const u32 c = load<u32>(o + 12);
    if (c == 0 || (load<u32>(c + 192) & 0x8u) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 x = static_cast<u32>(VCall(ctx, c, 60, {c}));
    if (x == 0 || load<u32>(x + 36) == 0) {
        ret(ctx, 0);
        return;
    }
    const u32 y = load<u32>(x + 36);
    const u32 tbl = load<u32>(0x00F65038u);
    const s32 a = static_cast<s16>(load<u16>(k + 4)), b = static_cast<s16>(load<u16>(k + 2));
    const u32 v1 = load<u32>(TblCell46(tbl, a, b, 0));
    const u32 v2 = load<u8>(TblCell46(tbl, a, b, 1));
    call(ctx, 0x0035327Cu, {y, v1, v2});
    ret(ctx, 1);
}

// u32 f(const Key* k, T* o): the 132-byte slot k->+6 of o (+152, count u16
// +144; out of range or its +0 >= 10 -> none, address 0) gets at +4 + u16
// *(0x0106CF80) the String_HashNoCase of the string cell (k->+4, k->+2)
// of the table *(0x00F65038), or 0 when that cell is 0. r3 = the hash (0).
DECOMP(003976C8, Obj_SetCellHash_003976C8) {
    const u32 k = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const s32 i = static_cast<s16>(load<u16>(k + 6));
    u32 slot = 0;
    if (i < static_cast<s32>(load<u16>(o + 144))) {
        slot = o + 152 + static_cast<u32>(i) * 132u;
        if (!(static_cast<s32>(load<u32>(slot)) < 10)) slot = 0;
    }
    const s32 a = static_cast<s16>(load<u16>(k + 4)), b = static_cast<s16>(load<u16>(k + 2));
    const u32 dst = slot + load<u16>(0x0106CF80u) + 4;
    const u32 tbl = load<u32>(0x00F65038u);
    u32 h = 0;
    if (load<u32>(TblCell46(tbl, a, b, 0)) != 0) h = static_cast<u32>(call(ctx, 0x0056A710u, {load<u32>(TblCell46(tbl, a, b, 0))}));
    store<u32>(dst, h);
    ret(ctx, h);
}

// T* f(T* o, u32 key): the node keyed key in the tree *(o->+160) (search
// stack u16 +244 / slots +164): none or an empty tree -> +248 = the last
// node (0 for an empty tree, which also zeroes the global 0x011DE488), f1
// = *(0x003DE5E8). A hit: +248 = node, 0x011DE488 = node->+8 (f32), then
// the same search again (same results), and f1 = node->+8. r3 = o.
DECOMP(003DE5F0, Tree_FindF32_003DE5F0) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    constexpr u32 kdef = 0x003DE5E8u, kglob = 0x011DE488u;
    for (u32 pass = 0; pass < 2; ++pass) {
        const u32 root = load<u32>(load<u32>(o + 160));
        if (root == 0) {
            store<u32>(o + 248, 0);
            store<u32>(kglob, 0);
            ctx.fpr[1] = Lfs45(kdef);
            return;
        }
        bool hit = false;
        const u32 n = TreeSeek46(root, key, o + 244, o + 164, hit);
        if (!hit) {
            store<u32>(o + 248, n);  // n is never 0 here
            ctx.fpr[1] = Lfs45(kdef);
            return;
        }
        store<u32>(o + 248, n);
        Cp45(kglob, n + 8);
    }
    const u32 n = load<u32>(o + 248);
    ctx.fpr[1] = n != 0 ? Lfs45(n + 8) : Lfs45(kdef);
}

// T* f(T* o, a1, Cfg* cfg, u8* a3, a4): SubMachine_Construct(o, a1, a3,
// cfg->+0, a4) (r7 passed through), then +72 / +112 / +116 = 1.0f, bit
// 0x80000000 of +92 cleared and byte +93 = 0, +96..+104 / +120 / +124 /
// +60 / +76 / +256 / +264 / +268 = 0, bytes +53 / +54 / +68 / +80..83 /
// +87 = 0 and +84..86 = 1, the vector 0x01164B40 into +224, the vec4
// 0x01164C90 into +240, +260 = cfg->+64, vtable 0x00B20850, +88 = cfg;
// PtrVector_Construct(o + 272), u16 +278 = 4, +292..+304 / +356 / +364 =
// 0, bytes +352..355 / +360..363 / +372 = 0; +28 |= 0x200. r3 = o + 28,
// f1 = cfg->+64.
DECOMP(003F834C, Obj_Construct_B20850_003F834C) {
    const u32 o = arg<u32>(ctx, 0), a1 = arg<u32>(ctx, 1), cfg = arg<u32>(ctx, 2), a3 = arg<u32>(ctx, 3), a4 = arg<u32>(ctx, 4);
    call(ctx, 0x0047C5D8u, {o, a1, a3, load<u32>(cfg), a4});
    const u32 w92 = load<u32>(o + 92) & 0x7FFFFFFFu;
    store<u32>(o + 72, 0x3F800000u);
    store<u32>(o + 96, 0);
    store<u32>(o + 92, w92);
    store<u32>(o + 100, 0);
    store<u32>(o + 104, 0);
    store<u32>(o + 112, 0x3F800000u);
    store<u32>(o + 116, 0x3F800000u);
    store<u32>(o + 120, 0);
    store<u32>(o + 124, 0);
    store<u8>(o + 53, 0);
    store<u8>(o + 54, 0);
    constexpr u32 g = 0x01164B40u, h = 0x01164C90u;
    Cp45(o + 224, g);
    store<u32>(o + 60, 0);
    store<u8>(o + 68, 0);
    store<u32>(o + 76, 0);
    store<u8>(o + 80, 0);
    store<u8>(o + 81, 0);
    store<u8>(o + 82, 0);
    Cp45(o + 228, g + 4);
    store<u8>(o + 83, 0);
    store<u8>(o + 84, 1);
    store<u8>(o + 85, 1);
    store<u8>(o + 86, 1);
    store<u8>(o + 87, 0);
    Cp45(o + 232, g + 8);
    store<u32>(o, 0x00B20850u);
    store<u32>(o + 88, cfg);
    store<u8>(o + 93, 0);
    Cp45(o + 240, h);
    Cp45(o + 244, h + 4);
    Cp45(o + 248, h + 8);
    store<u32>(o + 256, 0);
    Cp45(o + 252, h + 12);
    store<u32>(o + 264, 0);
    Cp45(o + 260, cfg + 64);
    store<u32>(o + 268, 0);
    ctx.fpr[1] = Lfs45(cfg + 64);  // f1 as the copy leaves it
    call(ctx, 0x005516E0u, {o + 272});
    store<u32>(o + 292, 0);
    store<u32>(o + 296, 0);
    store<u32>(o + 300, 0);
    store<u32>(o + 304, 0);
    store<u16>(o + 278, 4);
    for (u32 k = 352; k <= 355; ++k) store<u8>(o + k, 0);
    store<u32>(o + 356, 0);
    for (u32 k = 360; k <= 363; ++k) store<u8>(o + k, 0);
    store<u32>(o + 364, 0);
    store<u8>(o + 372, 0);
    store<u32>(o + 28, load<u32>(o + 28) | 0x200u);
    ret(ctx, o + 28);
}

// The 140-byte record of 0x00061810 (see SpawnNamed46).
DECOMP(00455038, Obj_SpawnNamed_00455038) { SpawnNamed46(ctx, 140, 0x00061810u); }

// s32 f(u32 hash): the value kind of a type-name hash (0 for unknown):
// 0x00014B07 1, 0x7A30D7E3 2, 0xEB9EE198 3, 0xB2E59948 4, 0xE89DF217 5,
// 0x044DC6AB 6, 0xAB27550F 7, 0xAB39AA94 8, 0x0021B90E 10, 0x92685411 11,
// 0x03FE2A3C 12, 0x0027BF5A 13, 0x001F32EA 14, 0x0028233F 15, 0x87208BC1 16.
DECOMP(0049EA74, Value_KindOfHash_0049EA74) {
    const s32 h = static_cast<s32>(arg<u32>(ctx, 0));
    static constexpr u32 kTab[][2] = {
        {0x00014B07u, 1},  {0x7A30D7E3u, 2},  {0xEB9EE198u, 3},  {0xB2E59948u, 4},  {0xE89DF217u, 5},
        {0x044DC6ABu, 6},  {0xAB27550Fu, 7},  {0xAB39AA94u, 8},  {0x0021B90Eu, 10}, {0x92685411u, 11},
        {0x03FE2A3Cu, 12}, {0x0027BF5Au, 13}, {0x001F32EAu, 14}, {0x0028233Fu, 15}, {0x87208BC1u, 16},
    };
    u32 r = 0;
    for (const auto& e : kTab)
        if (static_cast<u32>(h) == e[0]) r = e[1];
    ret(ctx, r);
}

// A buffer of the allocator +off+12 (data +off+8, u16 count +off, u16
// stride*2 +off+6): allocator->vt[8](it, data, count * stride) when data
// is set, then Array_Destruct(o + off).
static void ArrFree46(PPUContext& ctx, u32 o, u32 off) {
    const u32 size = load<u16>(o + off) * ((load<u16>(o + off + 6) >> 1) & 0x7FFFu);
    const u32 data = load<u32>(o + off + 8);
    if (data != 0) {
        const u32 al = load<u32>(o + off + 12);
        VCall(ctx, al, 8, {al, data, size});
    }
    call(ctx, 0x0054EC94u, {o + off});
}

// ~T() [vtable 0x00B20558]: the PtrVector +392, the buffer +368, the
// PtrVector +112, the buffers +88 and +64, then Dtor_B219D0_00490B34(o).
DECOMP(0077B384, VtB20558_Dtor) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20558u);
    VecDtor(ctx, o + 392);
    ArrFree46(ctx, o, 368);
    VecDtor(ctx, o + 112);
    ArrFree46(ctx, o, 88);
    ArrFree46(ctx, o, 64);
    call(ctx, 0x00490B34u, {o});
}

// Game code, part 22: the condition-node init, the hit event queue test
// and the continued sweep of small still-recompiled game functions (in
// size order). Most have no known caller context yet, so their names
// describe what they do and carry the address. Each was checked against the
// original in the PPC simulator.
//
// Register notes: where the original leaves a value in r3..r9 or f1 that a
// caller could read, the value is set the same way, as the comments say
// ("r4 left ...").
#include "obj22.h"

using namespace sly4;
using namespace sly4::obj22;

// ---------------------------------------------------------------- condition nodes
// void Cond_Init(Cond* p, s16 id, const char* class name, u32 kind): p->+8
// = id, p->+0 = 0, p->+12 = a new 52-byte binding owner (0x004D2BFC(o,
// name, 0, 0); 0 when the allocation fails), p->+4 = String_HashNoCase(
// name). Then a knowledge-data block is taken from the table at
// *0x00F65038 (its entry: the data cursor, p, type 6; the cursor moves 128
// bytes); with all 0x1800 in use the error is formatted and the index is
// -1. p->+16 = the s16 index, p->+20 = its 128-byte record (0 for -1),
// whose +0 (u16) / +2 / +3 are cleared, the block entry's +12 is set to the
// hash 0xC2A7B944 and the record's +2 gets the kind bits. Quirks kept: for
// index -1 the hash goes to the table's +32 (entry -1) and the kind byte to
// address 2. Finally the bindings "FireOnce" (4, 1) and "DebugBreak" (2,
// 1) are made (their +76 cleared) and "DebugBreak" gets the value names
// "OnEvaluate" / "OnActivate" (100-byte copies appended while below its
// +72 capacity). r3 / r4 as the original leaves them.
DECOMP(0038E5DC, Cond_Init) {
    const u32 p = arg<u32>(ctx, 0), name = arg<u32>(ctx, 2);
    const u64 kind = ctx.gpr[6];
    store<u16>(p + 8, static_cast<u16>(ctx.gpr[4]));
    store<u32>(p, 0);
    const u32 owner = static_cast<u32>(call(ctx, o22fn::Mem_Alloc, {52, 0, 16}));
    if (owner != 0) call(ctx, o22fn::NodeOwner_Init, {owner, name, 0, 0});
    store<u32>(p + 12, owner);
    store<u32>(p + 4, static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {name})));

    // take a block
    const u32 t = load<u32>(kKDataTable);
    const u32 n = load<u32>(t + 28);
    u32 raw;
    if (n < kKDataMax) {
        store<u32>(t + 36 + (n << 4), load<u32>(t + 32));
        store<u32>(t + 40 + (load<u32>(t + 28) << 4), p);
        store<u32>(t + 44 + (load<u32>(t + 28) << 4), 6);
        const u32 c = load<u32>(t + 28), cur = load<u32>(t + 32);
        store<u32>(t + 28, c + 1);
        store<u32>(t + 32, cur + 128);
        raw = c;
    } else {
        call(ctx, o22fn::String_Format, {0x00ADD314, kKDataMax});  // "Run out of KData Block (%d), ..."
        raw = ~0u;
    }
    store<u16>(p + 16, static_cast<u16>(raw));
    const u32 idx = static_cast<u32>(static_cast<s32>(static_cast<s16>(raw)));
    const u32 t1 = load<u32>(kKDataTable);
    const u32 rec = idx == ~0u ? 0u : load<u32>(t1 + 24) + (idx << 7);
    store<u32>(p + 20, rec);
    const u32 t2 = load<u32>(kKDataTable);
    u32 dst = 0;
    if (rec != 0) {
        store<u16>(rec, 0);
        store<u8>(rec + 2, 0);
        store<u8>(rec + 3, 0);
        dst = load<u32>(p + 20);
    }
    store<u32>(t2 + (idx << 4) + 48, 0xC2A7B944u);
    store<u8>(dst + 2, static_cast<u8>(load<u8>(rec + 2) | kind));

    // bindings
    u32 b = static_cast<u32>(call(ctx, o22fn::Node_Bind, {load<u32>(p + 12), 0x00ADD708, 4, 1, ~0ull, 0}));  // "FireOnce"
    store<u32>(b + 76, 0);
    b = static_cast<u32>(call(ctx, o22fn::Node_Bind, {load<u32>(p + 12), 0x00ADD718, 2, 1, ~0ull, 0}));  // "DebugBreak"
    store<u32>(b + 76, 0);
    call(ctx, o22fn::Binding_Reserve, {b, 2});
    const u32 names[2] = {0x00ADD728, 0x00ADD738};  // "OnEvaluate", "OnActivate"
    u32 cnt = load<u32>(b), cap = load<u32>(b + 72);
    for (u32 i = 0; i < 2; ++i) {
        if (cnt >= cap) {
            ctx.gpr[3] = cap;  // r3 / r4 left
            ctx.gpr[4] = cnt;
            return;
        }
        const u32 s = static_cast<u32>(call(ctx, o22fn::Mem_Alloc, {100, 0, 16}));
        store<u32>(load<u32>(b + 68) + (load<u32>(b) << 2), s);
        call(ctx, o22fn::String_CopyN, {load<u32>(load<u32>(b + 68) + (load<u32>(b) << 2)), names[i], 100});
        cnt = load<u32>(b) + 1;
        store<u32>(b, cnt);
        if (i == 0) cap = load<u32>(b + 72);
    }
    ctx.gpr[3] = cnt;  // r3 left as the count
}

// ---------------------------------------------------------------- hit event queue
// bool Obj_TestE0BCC(Obj* o, const HitEvent* ev): with ev->+14 bit 0x200
// and the queue enabled (0x01032148 nonzero), ev is queued for o and 0 is
// returned (0 too when the 512 records are full): the count at 0x01022080
// is taken atomically, the record gets o (+116), the kind (ev->+14 bits
// 13..15, +112), ev's +4 / +8 / +12, the high byte of +14 (bit field by bit
// field) and the kind's fields (0: +16..+52, 1: ..+76, 2: ..+100, 3:
// ..+36, 4: ..+60; kinds 5..7 nothing). Otherwise it returns whether the
// u16 at o->+216->+190 has no bits outside the mask ev->+12. r4..r9 / f1
// as the original leaves them.
DECOMP(000E0BCC, Obj_TestE0BCC) {
    const u32 o = arg<u32>(ctx, 0), ev = arg<u32>(ctx, 1);
    const u32 f14 = load<u16>(ev + 14);
    const u32 mask = load<u16>(ev + 12);
    if ((f14 & 0x200u) == 0 || load<u32>(kHitQueueGate) == 0) {
        const u32 h = load<u16>(load<u32>(o + 216) + 190);
        ctx.gpr[4] = h & mask;  // r4..r6 left
        ctx.gpr[5] = mask;
        ctx.gpr[6] = 0;
        ret(ctx, h == (h & mask) ? 1u : 0u);
        return;
    }
    ctx.gpr[5] = 0;  // r5 left
    const u32 n = load<u32>(kHitQueueCount);
    if (static_cast<s32>(n) >= static_cast<s32>(kHitQueueMax)) {
        ctx.gpr[6] = n;  // r6 left
        ret(ctx, 0u);
        return;
    }
    // lwarx / stwcx. loop in the original: the count is taken atomically
    // (the full test above is not repeated).
    u32 old;
    for (;;) {
        old = mem::r32(kHitQueueCount);
        if (mem::cas32(kHitQueueCount, old, old + 1)) break;
    }
    const u32 r = kHitQueue + (old << 7);
    ctx.gpr[8] = ctx.gpr[3];  // r8 / r9 left
    ctx.gpr[9] = static_cast<u32>(old + 1);
    store<u32>(r + 116, o);
    const u32 kind = (load<u16>(ev + 14) >> 13) & 7u;
    store<u32>(r + 112, kind);
    ret(ctx, 0u);
    if (kind > 4) {
        ctx.gpr[6] = old << 7;  // r6 / r7 left
        ctx.gpr[7] = kind;
        return;
    }
    // the common head: +4, +8, +12, then +14's bits 0xE000, 0x1000 .. 0x100
    CopyW(r, ev, 4);
    const u32 w8 = load<u32>(ev + 8);
    u32 m14 = load<u16>(r + 14);
    store<u32>(r + 8, w8);
    store<u16>(r + 12, load<u16>(ev + 12));
    u32 s14 = 0;
    for (const u32 m : {0xE000u, 0x1000u, 0x800u, 0x400u, 0x200u, 0x100u}) {
        s14 = load<u16>(ev + 14);
        m14 = (m14 & ~m) | (s14 & m);
        store<u16>(r + 14, static_cast<u16>(m14));
    }
    ctx.gpr[6] = s14;  // r6 / r7 left (changed below by some kinds)
    ctx.gpr[7] = m14;
    u32 f = 0;
    for (const u32 off : {16u, 20u, 24u, 32u, 36u}) f = CopyW(r, ev, off);
    switch (kind) {
    case 3:
        break;
    case 0:
        CopyW(r, ev, 40);
        f = CopyW(r, ev, 48);
        ctx.gpr[4] = CopyW(r, ev, 52);
        break;
    case 4:
        for (const u32 off : {40u, 48u, 52u}) f = CopyW(r, ev, off);
        ctx.gpr[6] = CopyW(r, ev, 56);
        ctx.gpr[4] = load<u8>(ev + 60);
        store<u8>(r + 60, static_cast<u8>(ctx.gpr[4]));
        break;
    case 2: {
        for (const u32 off : {40u, 48u, 52u}) CopyW(r, ev, off);
        const u8 b = load<u8>(ev + 56);
        store<u8>(r + 56, b);
        ctx.gpr[6] = b;
        for (const u32 off : {60u, 64u, 80u, 84u, 88u, 96u}) f = CopyW(r, ev, off);
        ctx.gpr[4] = CopyW(r, ev, 100);
        break;
    }
    case 1: {
        for (const u32 off : {40u, 48u, 52u, 56u}) CopyW(r, ev, off);
        const u32 w64 = load<u32>(ev + 64);
        u32 d72 = load<u32>(r + 72);
        store<u32>(r + 64, w64);
        f = CopyW(r, ev, 68);
        d72 = (d72 & 0x7FFFFFFFu) | (load<u32>(ev + 72) & 0x80000000u);
        store<u32>(r + 72, d72);
        const u32 s72 = load<u32>(ev + 72);
        d72 = (d72 & ~0x40000000u) | (s72 & 0x40000000u);
        store<u32>(r + 72, d72);
        ctx.gpr[6] = d72;
        ctx.gpr[7] = s72;
        ctx.gpr[4] = CopyW(r, ev, 76);
        break;
    }
    }
    ctx.fpr[1] = F32Bits(f);  // f1 left as the last float copied
}

// ---------------------------------------------------------------- sweep: 12 instructions
// Obj* f(const Obj* o, s32 key): the first of the list o->+32 (linked by
// +72) whose +76 is key, or 0. r5 left as the last +76 read.
DECOMP(0045666C, List32_Find76_0045666C) {
    u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    while (p != 0) {
        const u32 k = load<u32>(p + 76);
        ctx.gpr[5] = k;
        if (k == arg<u32>(ctx, 1)) break;
        p = load<u32>(p + 72);
    }
    ret(ctx, p);
}
// bool f(): the block at 0x010C5A20: +6 (u16) != 720 && +33 (u8) == 0
DECOMP(0046C7B0, Global_Test10C5A20_0046C7B0) {
    constexpr u32 g = 0x010C5A20;
    const u32 h = load<u16>(g + 6);
    ctx.gpr[5] = h;  // r4 / r5 left
    u32 r = 0;
    if (h != 720 && load<u8>(g + 33) == 0) r = 1;
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// void f(Rec* r, f32 x, f32 y, f32 z, ?, ?, ?, u32 w, bool b): r->+0..+8 =
// x, y, z; +12 = w; +16 = 0; +20: bit 31 = b, bit 30 set
DECOMP(00470560, Rec_Set_00470560) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 w20 = load<u32>(r + 20);
    store<f32>(r, static_cast<f32>(farg(ctx, 0)));
    store<f32>(r + 4, static_cast<f32>(farg(ctx, 1)));
    store<f32>(r + 8, static_cast<f32>(farg(ctx, 2)));
    store<u32>(r + 12, arg<u32>(ctx, 4));
    store<u32>(r + 16, 0);
    const u32 v = (w20 & 0x7FFFFFFFu) | (arg<u32>(ctx, 5) << 31) | 0x40000000u;
    store<u32>(r + 20, v);
    ctx.gpr[4] = 0;  // r4 / r5 left
    ctx.gpr[5] = v;
}
// f(?, u8 a, u8 b): sub_003697E0(a, b)
DECOMP(0047BE60, Call_3697E0_U8_0047BE60) { call(ctx, 0x003697E0, {arg<u32>(ctx, 1) & 0xFFu, arg<u32>(ctx, 2) & 0xFFu}); }
// f(a1..a10): sub_004B0850(a1..a10) (the 9th / 10th from the stack, as words)
DECOMP(004B0C40, Call_4B0850_004B0C40) {
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 a9 = load<u32>(sp + 116), a10 = load<u32>(sp + 124);
    call(ctx, 0x004B0850, {ctx.gpr[3], ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], ctx.gpr[10], a9, a10});
}
// Material* Material_Create(shader): m = Material_Alloc(); m->+16 = shader
DECOMP(004FF43C, Material_Create) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 m = static_cast<u32>(call(ctx, 0x004FF2E8, {s}));
    store<u32>(m + 16, s);
}
// f(Obj* o, const char* s): String_CopyN(o + 72, s, 127)
DECOMP(0073641C, Name72_Copy_0073641C) { call(ctx, o22fn::String_CopyN, {arg<u32>(ctx, 0) + 72, ctx.gpr[4], 127}); }
// bool f(const Obj* o): o->+348 <= *(f32*)0x00737750
DECOMP(00737758, Field348_Le_00737758) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 c = load<f32>(0x00737750), v = load<f32>(o + 348);
    ctx.gpr[4] = 0x00737750;  // r4 / r5 / f1 / f2 left
    ctx.gpr[5] = o;
    ctx.fpr[1] = c;
    ctx.fpr[2] = v;
    ctx.gpr[3] = v <= c ? 1u : 0u;
}
// bool f(?, const Obj* o): o->+4 < *(f32*)0x00738688
DECOMP(00738690, Field4_Lt_00738690) {
    const u32 p = arg<u32>(ctx, 1) + 4;
    const f64 c = load<f32>(0x00738688), v = load<f32>(p);
    ctx.gpr[4] = p;  // r4 / f1 / f2 left
    ctx.fpr[1] = c;
    ctx.fpr[2] = v;
    ctx.gpr[3] = v < c ? 1u : 0u;
}
// bool f(const Obj* o): o->+4 is 4, 5 or 6
DECOMP(00739760, Field4_Is4To6_00739760) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ctx.gpr[4] = static_cast<u32>(v);  // r4 left
    ret(ctx, (v == 4 || v == 5 || v == 6) ? 1u : 0u);
}
// bool f(const Obj* o, u32 i): the high bit of 2-bit entry i of the words at o + 36
DECOMP(0074BC58, Bits2_TestHigh_0074BC58) {
    const u32 i = arg<u32>(ctx, 1);
    const u32 sh = (i << 1) & 0x1Eu, off = (i >> 2) & 0x3FFFFFFCu;
    const u32 m = 2u << sh;
    ctx.gpr[4] = off;  // r4..r6 left
    ctx.gpr[5] = m;
    ctx.gpr[6] = 2;
    ret(ctx, (load<u32>(arg<u32>(ctx, 0) + 36 + off) & m) != 0 ? 1u : 0u);
}
// f32 f(f32 x): the constants at 0x0074C6F0 (f32 a, f64 b, f64 c): t = a - x
// >= 0 ? x : b; t >= 0 ? t : c (fsel)
DECOMP(0074C708, Clamp_0074C708) {
    constexpr u32 k = 0x0074C6F0;
    const f64 x = farg(ctx, 0);
    const f64 a = load<f32>(k), b = std::bit_cast<f64>(load<u64>(k + 8)), c = std::bit_cast<f64>(load<u64>(k + 16));
    f64 t = rt::to_single(a - x);
    t = rt::to_single(t >= 0.0 ? x : b);
    t = rt::to_single(t >= 0.0 ? t : c);
    ctx.fpr[2] = x;  // f2 / f3 / f4 left
    ctx.fpr[3] = c;
    ctx.fpr[4] = b;
    ctx.gpr[3] = k;  // r3 left
    ctx.fpr[1] = t;
}
// u32 f(const Obj* o, u32 i, u32 j): (o->+132)[i * 2]->+48[j * 2 + 1]
DECOMP(0074CB40, Field_Get132_48_0074CB40) {
    const u32 i = arg<u32>(ctx, 1), j = arg<u32>(ctx, 2);
    const u32 p = load<u32>(arg<u32>(ctx, 0) + (i << 3) + 132);
    ctx.gpr[4] = j << 3;  // r4 left
    ret(ctx, load<u32>(load<u32>(p + 48) + (j << 3) + 4));
}
// u8 f(const Obj* o): o->+348 <= *(f32*)0x0074DA70 ? 0 : o->+761
DECOMP(0074DA78, Field348_Byte761_0074DA78) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = load<f32>(o + 348), c = load<f32>(0x0074DA70);
    ctx.gpr[4] = 0x0074DA70;  // r4 / f1 / f2 left
    ctx.fpr[1] = v;
    ctx.fpr[2] = c;
    ret(ctx, v <= c ? 0u : static_cast<u32>(load<u8>(o + 761)));
}
// void f(const Obj* o, const u8 b[4]): o->+0->+12..+15 = b (byte by byte)
DECOMP(0074E2B8, Field_Set0_12b_0074E2B8) {
    const u32 d = load<u32>(arg<u32>(ctx, 0)) + 12, s = arg<u32>(ctx, 1);
    u32 c[4];
    for (u32 k = 0; k < 4; ++k) {
        c[k] = load<u8>(s + k);
        store<u8>(d + k, static_cast<u8>(c[k]));
    }
    ctx.gpr[3] = d;  // r3..r5 left
    ctx.gpr[4] = c[3];
    ctx.gpr[5] = c[2];
}
// The vtables 0x00B13974 / 0x00B1F4E0 / 0x00B1F4C0 at +0 / +8 / +12 of the
// object at this - delta (a secondary-base thunk of the init 0x0076EB78).
static void SetVtB13974(PPUContext& ctx, u32 delta) {
    const u32 o = arg<u32>(ctx, 0) - delta;
    store<u32>(o, 0x00B13974);
    store<u32>(o + 12, 0x00B1F4C0);
    store<u32>(o + 8, 0x00B1F4E0);
    ctx.gpr[3] = o;  // r3..r6 left
    ctx.gpr[4] = 0x00B13974;
    ctx.gpr[5] = 0x00B1F4C0;
    ctx.gpr[6] = 0x00B1F4E0;
}
// void f(Base12* b): the object at b - 12 gets the vtables of 0x0076EB78
DECOMP(00771AEC, Obj_InitVt_B13974_Adj12) { SetVtB13974(ctx, 12); }
// void f(Base8* b): the object at b - 8 gets the vtables of 0x0076EB78
DECOMP(00771B1C, Obj_InitVt_B13974_Adj8) { SetVtB13974(ctx, 8); }
// u32 f(const Arr* a, u32 i): i < a->+6 (u16) ? a->+12[i].+4 (8-byte entries) : 0
DECOMP(0077CD74, Arr8_Get4_0077CD74) {
    const u32 a = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 n = load<u16>(a + 6);
    ctx.gpr[5] = n;  // r4 / r5 left
    if (i >= n) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = i << 3;
    ret(ctx, load<u32>(load<u32>(a + 12) + (i << 3) + 4));
}
// bool f(): *(f32*)0x010854AC <= *(f32*)0x0077ECF8
DECOMP(0077ED00, Global_Le_0077ED00) {
    const f64 c = load<f32>(0x0077ECF8), v = load<f32>(0x010854AC);
    ctx.gpr[4] = 0x01080000;  // r4 / r5 / f1 / f2 left
    ctx.gpr[5] = 0x0077ECF8;
    ctx.fpr[1] = c;
    ctx.fpr[2] = v;
    ctx.gpr[3] = v <= c ? 1u : 0u;
}

// ---------------------------------------------------------------- sweep: 13 instructions
// f(const Obj* o): World_ShowHeadline(*(World**)0x00F65CA0, o->+12 (u8))
DECOMP(00092C78, Call_ShowHeadline_00092C78) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 12);
    call(ctx, 0x00482B20, {load<u32>(0x00F65CA0), b});
}
// f(const Obj* o): sub_000AB954(o->+16, o->+24, o->+28 (u8)) (r6 = the byte too)
DECOMP(00099CC8, Call_AB954_00099CC8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 24), b = load<u8>(o + 28);
    call(ctx, 0x000AB954, {load<u32>(o + 16), a, b, b});
}
// f(const Obj* o): sub_000ABA14(o->+12, o->+20 (u8), o->+21 (u8)) (r6 = the last byte too)
DECOMP(00099F6C, Call_ABA14_00099F6C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u8>(o + 20), b = load<u8>(o + 21);
    call(ctx, 0x000ABA14, {load<u32>(o + 12), a, b, b});
}
// f(const Obj* o): sub_004A783C(o->+160, o->+372, o->+364 (u8)) (r6 = the byte too)
DECOMP(000DF4CC, Call_4A783C_000DF4CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u32>(o + 372), b = load<u8>(o + 364);
    call(ctx, 0x004A783C, {load<u32>(o + 160), a, b, b});
}
// f(a, u8 b, u8 c, u8 d, u8 e): sub_000D9F1C(a, b, c, d, e)
DECOMP(001F7128, Call_D9F1C_U8_001F7128) {
    call(ctx, 0x000D9F1C, {ctx.gpr[3], arg<u32>(ctx, 1) & 0xFFu, arg<u32>(ctx, 2) & 0xFFu, arg<u32>(ctx, 3) & 0xFFu, arg<u32>(ctx, 4) & 0xFFu});
}
// void f(Obj* o, const f32 d[3]): o->+1200..+1208 += d
DECOMP(0024DC5C, Vec1200_Add_0024DC5C) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    const f32 x = static_cast<f32>(static_cast<f64>(load<f32>(o + 1200)) + static_cast<f64>(load<f32>(d)));
    const f64 y0 = load<f32>(o + 1204), z0 = load<f32>(o + 1208);
    store<f32>(o + 1200, x);
    const f32 y = static_cast<f32>(y0 + static_cast<f64>(load<f32>(d + 4)));
    store<f32>(o + 1204, y);
    const f32 z = static_cast<f32>(z0 + static_cast<f64>(load<f32>(d + 8)));
    store<f32>(o + 1208, z);
    ctx.fpr[2] = y0;  // f2 / f3 left
    ctx.fpr[3] = z0;
    ret(ctx, z);  // f1 left as the last sum
}
// void f(const Obj* o, u32* out, ?, s32 i): *out = i == -1 ? 0 : (o->+64 + i * 48)->+40
DECOMP(00264D70, Rec48_Get40_00264D70) {
    const u64 i = ctx.gpr[6];
    u32 v = 0;
    if (static_cast<s32>(i) != -1) {
        const u32 r = load<u32>(arg<u32>(ctx, 0) + 64) + static_cast<u32>(i) * 48u;
        v = load<u32>(r + 40);
        ctx.gpr[3] = r;  // r3 / r6 left
        ctx.gpr[6] = i << 5;
    }
    ctx.gpr[5] = v;  // r5 left
    store<u32>(arg<u32>(ctx, 1), v);
}
// The record of the current slot of *(T**)0x00F65CA0: g + 80 + g->+4208 (u8) * 4056 (r4 / r5 left).
static u32 CurSlotRec22(PPUContext& ctx) {
    const u32 g = load<u32>(0x00F65CA0);
    const u32 off = load<u8>(g + 4208) * 4056u;
    ctx.gpr[4] = g + off;
    ctx.gpr[5] = off;
    return g + off + 80;
}
// u8 f(const Obj* o): the current slot record's byte +464 + o->+502
DECOMP(002D98A4, SlotRec_Byte464_002D98A4) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 502);
    ret(ctx, load<u8>(CurSlotRec22(ctx) + b + 464));
}
// void f(Obj* o): if (o->+12) *(u8*)0x01068841 = 1, *(u8*)0x01068840 = 0; o->+8 (u8) = 1
DECOMP(002F47E8, Field_Set8b_002F47E8) {
    const u32 o = arg<u32>(ctx, 0);
    if (load<u32>(o + 12) != 0) {
        store<u8>(0x01068841, 1);
        store<u8>(0x01068840, 0);
        ctx.gpr[5] = 1;  // r5..r7 left
        ctx.gpr[6] = 0x01070000;
        ctx.gpr[7] = 0;
    }
    store<u8>(o + 8, 1);
    ctx.gpr[4] = 1;  // r4 left
}
// s32 f(const f32* a, const f32* b): *a < *b ? 1 : *a > *b ? -1 : 0 (a descending sort comparator)
DECOMP(00332DF4, CmpF32Desc_00332DF4) {
    const f64 a = load<f32>(arg<u32>(ctx, 0)), b = load<f32>(arg<u32>(ctx, 1));
    ctx.fpr[1] = b;  // f1 / f2 left
    ctx.fpr[2] = a;
    ret(ctx, a < b ? 1 : (a > b ? -1 : 0));
}
// bool f(const Obj* o, u32 out[3]): o->+2212 ? (out = {o->+2212, 5 - o->+2628, 5}, 1) : 0
DECOMP(0033891C, Field_Get2212_0033891C) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    const u32 v = load<u32>(o + 2212);
    ctx.gpr[5] = v;  // r5 / r6 left
    if (v == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(out, v);
    const u32 d = 5u - load<u32>(o + 2628);
    store<u32>(out + 8, 5);
    store<u32>(out + 4, d);
    ctx.gpr[5] = d;
    ctx.gpr[6] = 5;
    ret(ctx, 1u);
}
// void f(const Obj* o, const f32 v[3]): if (o->+116) o->+116->+144..+152 = v
DECOMP(00352DD4, Field_Set116_144v_00352DD4) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 116);
    ctx.gpr[3] = p;  // r3 left (the destination)
    if (p == 0) return;
    const u32 d = p + 144, s = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 k = 0; k < 3; ++k) w = CopyW(d, s, 4 * k);
    ctx.gpr[3] = d;
    ctx.fpr[1] = F32Bits(w);  // f1 left as the last value
}
// f32 f(const Obj* o): o->+36->+40->+12->+12 (f32), or *(f32*)0x00354080 when a link is null
DECOMP(00354088, Field_Get36_40_12f_00354088) {
    u32 p = load<u32>(arg<u32>(ctx, 0) + 36);
    if (p != 0) p = load<u32>(p + 40);
    if (p == 0) {
        ctx.gpr[3] = 0x00354080;  // r3 left
        ret(ctx, load<f32>(0x00354080));
        return;
    }
    p = load<u32>(p + 12);
    ctx.gpr[3] = p;
    ret(ctx, load<f32>(p + 12));
}
// void f(Obj* o, s32 k, u32 v, bool b): k 3: o->+316 = v, o->+396 bit 0x80000 = b; k 4: o->+320 = v
DECOMP(0036C124, Field_SetByKind_0036C124) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    const s32 k = arg<s32>(ctx, 1);
    if (k == 3) {
        const u32 w = load<u32>(o + 396);
        store<u32>(o + 316, v);
        const u32 n = (w & ~0x00080000u) | ((arg<u32>(ctx, 3) << 19) & 0x00080000u);
        store<u32>(o + 396, n);
        ctx.gpr[4] = n;  // r4 left
    } else if (k == 4) {
        store<u32>(o + 320, v);
    }
}
// f(o, u8 on): on ? sub_0036F0D0(o, on) : sub_0036EFC8(o, 0)
DECOMP(0036F1C8, Call_36F0D0Or36EFC8) {
    const u32 on = arg<u32>(ctx, 1) & 0xFFu;
    call(ctx, on != 0 ? 0x0036F0D0u : 0x0036EFC8u, {ctx.gpr[3], on});
}
// bool f(): p = *(T**)0x00F65820: !p || (p->+396 != 3 && p->+396 != 5)
DECOMP(003F2B5C, Global_Test_F65820_003F2B5C) {
    const u32 p = load<u32>(0x00F65820);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    const s32 v = load<s32>(p + 396);
    ret(ctx, (v != 3 && v != 5) ? 1u : 0u);
}
// void f(): the block at 0x010AA1B0: +92 (u16) = 0x8006, +94 (u16) = 771,
// +88 bit 31 cleared and bits 15..30 = 770
DECOMP(00428F00, Global_Init10AA1B0_00428F00) {
    constexpr u32 g = 0x010AA1B0;
    const u32 w = load<u32>(g + 88);
    store<u16>(g + 92, 0x8006);
    store<u16>(g + 94, 771);
    const u32 n = (w & 0x00007FFFu) | (770u << 15);
    store<u32>(g + 88, n);
    ctx.gpr[3] = g;  // r3..r7 left
    ctx.gpr[4] = 0x8006;
    ctx.gpr[5] = 770;
    ctx.gpr[6] = 771;
    ctx.gpr[7] = n;
}
// (*(T**)0x00F65CA0)->+40 bit `m` = o->+9 != 0 (r3..r6 left).
static void SetF65CA0Flag40(PPUContext& ctx, u32 m) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 9);
    const u32 g = load<u32>(0x00F65CA0);
    u32 w = load<u32>(g + 40);
    if (b != 0) {
        w |= m;
    } else {
        w &= ~m;
        ctx.gpr[6] = 0;
    }
    store<u32>(g + 40, w);
    ctx.gpr[3] = g;
    ctx.gpr[4] = w;
    ctx.gpr[5] = b;
}
// void f(const Obj* o): (*(T**)0x00F65CA0)->+40 bit 0x2 = o->+9 != 0
DECOMP(00431FF8, Global_F65CA0_Flag40_2) { SetF65CA0Flag40(ctx, 0x2); }
// void f(const Obj* o): (*(T**)0x00F65CA0)->+40 bit 0x4 = o->+9 != 0
DECOMP(00432100, Global_F65CA0_Flag40_4) { SetF65CA0Flag40(ctx, 0x4); }
// f(const Obj* o): sub_004897AC(*(T**)0x00F65CA0, o->+9 (u8)) (r5 = the byte too)
DECOMP(0043207C, Call_4897AC_0043207C) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 9);
    call(ctx, 0x004897AC, {load<u32>(0x00F65CA0), b, b});
}
// void f(Obj* o, f32 x): if |x - o->+96| >= *(f32*)0x00441D10: o->+96 = x, *(u8*)0x00F65B68 = 1
DECOMP(00441D18, Field96_SetIfMoved_00441D18) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 x = farg(ctx, 0);
    const f64 d = std::fabs(rt::to_single(x - static_cast<f64>(load<f32>(o + 96))));
    const f64 c = load<f32>(0x00441D10);
    ctx.fpr[2] = d;  // f2 / f3 / r4 / r5 left
    ctx.fpr[3] = c;
    ctx.gpr[4] = 0x00441D10;
    if (d < c) return;
    store<f32>(o + 96, static_cast<f32>(x));
    store<u8>(0x00F65B68, 1);
    ctx.gpr[4] = 0x00F60000;
    ctx.gpr[5] = 1;
}
// void f(Rec* r, u32 a, u32 b, u32 c): r = {+0 28, +4 (*(T**)0x00F65CA0)->+12->+0 (f32), +8 b, +12 c, +20 0, +24 a}
DECOMP(00464EA0, Rec_Init28_00464EA0) {
    const u32 r = arg<u32>(ctx, 0);
    store<u32>(r + 20, 0);
    const u32 g = load<u32>(0x00F65CA0);
    store<u32>(r, 28);
    const u32 p = load<u32>(g + 12);
    const u32 f = load<u32>(p);
    store<u32>(r + 8, arg<u32>(ctx, 2));
    store<u32>(r + 4, f);
    store<u32>(r + 12, arg<u32>(ctx, 3));
    store<u32>(r + 24, arg<u32>(ctx, 1));
    ctx.gpr[7] = p;  // r7..r9 / f1 left
    ctx.gpr[8] = 0x00F60000;
    ctx.gpr[9] = 28;
    ctx.fpr[1] = F32Bits(f);
}
// bool f(const World* w): p = w->+0x12847C: p && (p->+164 bit 0x80000000)
DECOMP(00489E78, World_Test12847C_00489E78) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 0x0012847Cu);
    ctx.gpr[4] = 0x0012847C;  // r4 left
    ret(ctx, p != 0 ? load<u32>(p + 164) >> 31 : 0u);
}
// void f(const World* w): *(f32*)0x010C5A5C = w->+0x1281C0 ? w->+0x1281C0->+876 : *(f32*)0x0048C83C
DECOMP(0048C840, World_Set10C5A5C_0048C840) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 0x001281C0u);
    const u32 f = p != 0 ? load<u32>(p + 876) : load<u32>(0x0048C83C);
    store<u32>(0x010C5A5C, f);
    ctx.gpr[3] = 0x010C5A20;  // r3..r5 / f1 left
    ctx.gpr[4] = 0x0048C83C;
    ctx.gpr[5] = 0x001281C0;
    ctx.fpr[1] = F32Bits(f);
}
// f(Obj* o): if (o->+31 (u8)) Obj_SetMode470750(o, 23, 0)
DECOMP(0048CDF8, Call_SetMode23_0048CDF8) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 31);
    ctx.gpr[4] = b;  // r4 left
    if (b != 0) call(ctx, 0x00470750, {ctx.gpr[3], 23, 0});
}
// Rec* f(u32 i): i < 84 ? 0x010BE8BC + i * 152 : 0
DECOMP(0049E704, Table10BE8BC_At_0049E704) {
    const u64 i = ctx.gpr[3];
    if (static_cast<u32>(i) >= 84) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = i << 4;  // r4 / r5 left
    ctx.gpr[5] = 0x010BE8BC;
    ret(ctx, 0x010BE8BCu + static_cast<u32>(i) * 152u);
}
// void* f(u32 n): MemStack_Alloc(*(T**)0x010DD718, n)
DECOMP(004FD1E0, MemStack_Alloc_10DD718) { call(ctx, 0x0055260C, {load<u32>(0x010DD718), ctx.gpr[3]}); }

// ---------------------------------------------------------------- vector helpers
namespace {
enum class VOp { Add, Sub, Mul };
inline f32 VApply(VOp op, f64 a, f64 b) {
    return static_cast<f32>(op == VOp::Add ? a + b : op == VOp::Sub ? a - b : a * b);
}
// out = a op b (3 floats; all loads first, as the original schedules them).
void Vec3Op(PPUContext& ctx, VOp op, u32 out, u32 a, u32 b) {
    const f64 a0 = load<f32>(a), b0 = load<f32>(b), a1 = load<f32>(a + 4), b1 = load<f32>(b + 4);
    const f32 r0 = VApply(op, a0, b0);
    const f64 a2 = load<f32>(a + 8), b2 = load<f32>(b + 8);
    const f32 r1 = VApply(op, a1, b1), r2 = VApply(op, a2, b2);
    store<f32>(out, r0);
    store<f32>(out + 4, r1);
    store<f32>(out + 8, r2);
    ctx.fpr[1] = r0;  // f1..f5 left
    ctx.fpr[2] = r2;
    ctx.fpr[3] = r1;
    ctx.fpr[4] = b1;
    ctx.fpr[5] = b2;
}
// o[0..2] op= d[0..2], component by component (o's +4 / +8 read before the first store).
void Vec3OpInPlace(PPUContext& ctx, VOp op, u32 o, u32 d) {
    const f32 x = VApply(op, load<f32>(o), load<f32>(d));
    const f64 y0 = load<f32>(o + 4), z0 = load<f32>(o + 8);
    store<f32>(o, x);
    const f32 y = VApply(op, y0, load<f32>(d + 4));
    store<f32>(o + 4, y);
    const f32 z = VApply(op, z0, load<f32>(d + 8));
    store<f32>(o + 8, z);
    ctx.fpr[1] = z;  // f1..f3 left
    ctx.fpr[2] = y0;
    ctx.fpr[3] = z0;
}
// A 24-byte record {+0 vtable, +4 a, +8 kind, +12 c, +16 0, +20 value word}.
void InitTypedValue(PPUContext& ctx, u32 vt, u32 kind, u32 value) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 4, arg<u32>(ctx, 1));
    store<u32>(o + 8, kind);
    store<u32>(o + 12, arg<u32>(ctx, 3));
    store<u32>(o + 16, 0);
    store<u32>(o + 20, value);
    store<u32>(o, vt);
}
// s32 compare of two f32s: *a > *b ? gt : *a < *b ? -gt : 0 (f1 / f2 left).
void CmpF32(PPUContext& ctx, s32 gt) {
    const f64 a = load<f32>(arg<u32>(ctx, 0)), b = load<f32>(arg<u32>(ctx, 1));
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    ret(ctx, a > b ? gt : (a < b ? -gt : 0));
}
}  // namespace

// void Vec3_Sub(f32 out[3], const f32 a[3], const f32 b[3]): out = a - b
DECOMP(00734A8C, Vec3_Sub_00734A8C) { Vec3Op(ctx, VOp::Sub, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)); }
// void Vec3_Add(f32 out[3], const f32 a[3], const f32 b[3]): out = a + b
DECOMP(00734E94, Vec3_Add_00734E94) { Vec3Op(ctx, VOp::Add, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)); }
// void Vec3_Mul(f32 out[3], const f32 a[3], const f32 b[3]): out = a * b (per component)
DECOMP(00735CDC, Vec3_Mul_00735CDC) { Vec3Op(ctx, VOp::Mul, arg<u32>(ctx, 0), arg<u32>(ctx, 1), arg<u32>(ctx, 2)); }
// void f(f32 out[3], const f32 box[6]): out = box[3..5] - box[0..2]
DECOMP(00760E20, Box_Size_00760E20) { Vec3Op(ctx, VOp::Sub, arg<u32>(ctx, 0), arg<u32>(ctx, 1) + 16, arg<u32>(ctx, 1)); }
// void Vec3_SubInPlace(f32 v[3], const f32 d[3]): v -= d
DECOMP(00742C74, Vec3_SubInPlace_00742C74) { Vec3OpInPlace(ctx, VOp::Sub, arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }
// void Vec3_AddInPlace(f32 v[3], const f32 d[3]): v += d
DECOMP(0074C600, Vec3_AddInPlace_0074C600) { Vec3OpInPlace(ctx, VOp::Add, arg<u32>(ctx, 0), arg<u32>(ctx, 1)); }
// void f(TypedValue* v, u32 a, f32 x, u32 c): kind 3 (a float), vtable 0x00B0DC20
DECOMP(00736770, TypedValue_InitF32_00736770) {
    InitTypedValue(ctx, 0x00B0DC20, 3, std::bit_cast<u32>(static_cast<f32>(farg(ctx, 0))));
    ctx.gpr[5] = 3;  // r5 / r7..r9 left
    ctx.gpr[7] = 0x00B0DC20;
    ctx.gpr[8] = arg<u32>(ctx, 0) + 20;
    ctx.gpr[9] = 0;
}
// void f(TypedValue* v, u32 a, u32 x, u32 c): kind 2, vtable 0x00B0DC34
DECOMP(007367E8, TypedValue_Init2_007367E8) {
    InitTypedValue(ctx, 0x00B0DC34, 2, arg<u32>(ctx, 2));
    ctx.gpr[7] = 2;  // r7..r10 left
    ctx.gpr[8] = 0x00B0DC34;
    ctx.gpr[9] = arg<u32>(ctx, 0) + 20;
    ctx.gpr[10] = 0;
}
// void f(TypedValue* v, u32 a, u32 x, u32 c): kind 5, vtable 0x00B0DC48
DECOMP(00736860, TypedValue_Init5_00736860) {
    InitTypedValue(ctx, 0x00B0DC48, 5, arg<u32>(ctx, 2));
    ctx.gpr[7] = 5;  // r7..r10 left
    ctx.gpr[8] = 0x00B0DC48;
    ctx.gpr[9] = arg<u32>(ctx, 0) + 20;
    ctx.gpr[10] = 0;
}
// void f(Obj* o, f32 x): if x > *(f32*)0x007386D8 and o->+305 bit 0x10 is clear, o->+305 |= 0x80
DECOMP(007386E0, Flag305_SetIfAbove_007386E0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 c = load<f32>(0x007386D8);
    ctx.fpr[2] = c;  // f2 / r4 / r5 left
    ctx.gpr[4] = 0x007386D8;
    if (!(farg(ctx, 0) > c)) return;
    const u32 b = load<u8>(o + 305);
    ctx.gpr[4] = b;
    ctx.gpr[5] = (b >> 4) & 1u;
    if ((b & 0x10u) != 0) return;
    ctx.gpr[4] = b | 0x80u;
    store<u8>(o + 305, static_cast<u8>(b | 0x80u));
}
// bool f(const Obj* o): o->+4 is not 1, 4, 12 or 13
DECOMP(00738CAC, Field4_Not1_4_12_13_00738CAC) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ctx.gpr[4] = static_cast<u32>(v);  // r4 left
    ret(ctx, (v == 12 || v == 13 || v == 1 || v == 4) ? 0u : 1u);
}
// bool f(const Obj* o): o->+4 is 7, 8, 9 or 10
DECOMP(00032A30, Field4_Is7To10_00032A30) {
    const s32 v = load<s32>(arg<u32>(ctx, 0) + 4);
    ctx.gpr[4] = static_cast<u32>(v);  // r4 left
    ret(ctx, (v == 7 || v == 8 || v == 9 || v == 10) ? 1u : 0u);
}
// void f(u32* w): *w byte-swapped (through a stack word and lfs / stfs)
DECOMP(00742CDC, Word_ByteSwap_00742CDC) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 b3 = load<u8>(p + 3), b0 = load<u8>(p), b1 = load<u8>(p + 1), b2 = load<u8>(p + 2);
    const u32 w = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
    store<u32>(p, w);
    ctx.gpr[4] = b2;  // r4 / r5 / f1 left
    ctx.gpr[5] = b1;
    ctx.fpr[1] = F32Bits(w);
}
// void f(f32 out[3], const Obj* o, const Obj* p): out = p ? p->+48..+56 : o->+144..+152
DECOMP(00743720, Pos_Get_00743720) {
    const u32 out = arg<u32>(ctx, 0), p = arg<u32>(ctx, 2);
    const u32 s = p != 0 ? p + 48 : arg<u32>(ctx, 1) + 144;
    u32 w = 0;
    for (u32 k = 0; k < 3; ++k) w = CopyW(out, s, 4 * k);
    ctx.gpr[4] = s;  // r4 / f1 left
    ctx.fpr[1] = F32Bits(w);
}
// void f(V4 d[3], const V4 s[3]): 48 bytes copied (lvx / stvx, 16-byte aligned)
DECOMP(0074D744, Vec4x3_Copy_0074D744) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    VSt(d, VLd(s));
    VSt(d + 16, VLd(s + 16));
    VSt(d + 32, VLd(s + 32));
    ctx.gpr[4] = s + 32;  // r4..r7 left
    ctx.gpr[5] = s + 16;
    ctx.gpr[6] = 32;
    ctx.gpr[7] = 16;
}
// void f(Obj* o): o->+0 = o->+4 = 0 and the 64 bytes at +16 zeroed (stvx)
DECOMP(0074E3AC, Obj_Clear_0074E3AC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0);
    store<u32>(o + 4, 0);
    const V4 z{};
    for (u32 k = 1; k <= 4; ++k) VSt(o + 16 * k, z);
    ctx.gpr[4] = 0;  // r4..r8 left
    ctx.gpr[5] = 64;
    ctx.gpr[6] = 16;
    ctx.gpr[7] = 32;
    ctx.gpr[8] = 48;
}
// u32 f(const Obj* o): o->+40 bit 31 set and bit 16 clear ? (o->+136->+68 >> 1) & 3 : 3
DECOMP(0074E7D0, Field_Mode136_68_0074E7D0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 40);
    ctx.gpr[5] = 3;  // r4..r6 left
    ctx.gpr[6] = w >> 31;
    ctx.gpr[4] = w;
    if ((w >> 31) != 0) {
        ctx.gpr[4] = (w >> 16) & 1u;
        if ((w & 0x10000u) == 0) ctx.gpr[5] = (load<u32>(load<u32>(o + 136) + 68) >> 1) & 3u;
    }
    ret(ctx, ctx.gpr[5]);
}
// s32 f(const f32* a, const f32* b): *a > *b ? 1 : *a < *b ? -1 : 0
DECOMP(00767A04, CmpF32_00767A04) { CmpF32(ctx, 1); }
// s32 f(const f32* a, const f32* b): *a > *b ? 1 : *a < *b ? -1 : 0
DECOMP(0077D7C4, CmpF32_0077D7C4) { CmpF32(ctx, 1); }
// void f(Obj* d, const Obj* s): d's +4 (u8), +32 / +36 / +40 / +48 (f32), +52 (u8) from s
DECOMP(00788264, Obj_CopyFields_00788264) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 f32w = load<u32>(s + 32);
    const u32 b4 = load<u8>(s + 4);
    store<u32>(d + 32, f32w);
    const u32 f36 = load<u32>(s + 36);
    store<u8>(d + 4, static_cast<u8>(b4));
    store<u32>(d + 36, f36);
    CopyW(d, s, 40);
    const u32 w = CopyW(d, s, 48);
    const u32 b52 = load<u8>(s + 52);
    store<u8>(d + 52, static_cast<u8>(b52));
    ctx.gpr[4] = b52;  // r4 / r5 / f1 left
    ctx.gpr[5] = b4;
    ctx.fpr[1] = F32Bits(w);
}
// f(u32 id): sub_00398DAC(*(T**)0x00F65798, id)
DECOMP(000653CC, Call_398DAC_000653CC) { call(ctx, 0x00398DAC, {load<u32>(0x00F65798), arg<u32>(ctx, 0)}); }
// f(const Obj* o): Handle_Notify36742C(o->+12, o->+16 (u8), o->+17 (u8), 0) (r7 = the last byte too)
DECOMP(00096820, Call_HandleNotify_00096820) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 a = load<u8>(o + 16), b = load<u8>(o + 17);
    call(ctx, 0x0036742C, {load<u32>(o + 12), a, b, 0, b});
}
// f(): the object at `o` gets the vtable 0x00B0D99C, then sub_004FC1A0(o) (its base init / dtor)
static void SetVtB0D99C(PPUContext& ctx, u32 o) {
    store<u32>(o, 0x00B0D99C);
    call(ctx, 0x004FC1A0, {o});
}
// f(): SetVtB0D99C on the global at 0x00F9CD38
DECOMP(00099FA8, Global_VtB0D99C_F9CD38) { SetVtB0D99C(ctx, 0x00F9CD38); }
// f(): SetVtB0D99C on the global at 0x010B16C8
DECOMP(004566D8, Global_VtB0D99C_10B16C8) { SetVtB0D99C(ctx, 0x010B16C8); }
// f(s32 n): if (n != -1) Trophy_Unlock4FB4A0(*(Trophy**)0x00F668E8, n)
DECOMP(000B277C, Trophy_UnlockIf_000B277C) {
    const u32 n = arg<u32>(ctx, 0);
    if (static_cast<s32>(n) == -1) return;
    call(ctx, 0x004FB4A0, {load<u32>(0x00F668E8), ctx.gpr[3]});
}

// ---------------------------------------------------------------- sweep: 14 instructions
// void f(Obj* o, u8 on): o->+172 bit 0 = on (bit 0 of the argument); when
// off, o->+176 (u16) bit 0x10 is cleared first
DECOMP(000DB258, Flag_Set172_1_000DB258) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1);
    const u32 w = load<u32>(o + 172);
    if ((a & 0xFFu) == 0) {
        const u32 h = load<u16>(o + 176) & ~0x10u;
        store<u16>(o + 176, static_cast<u16>(h));
        ctx.gpr[6] = 0;  // r6 / r7 left
        ctx.gpr[7] = h;
    }
    const u32 n = (w & ~1u) | (a & 1u);
    store<u32>(o + 172, n);
    ctx.gpr[4] = n;  // r4 / r5 left
    ctx.gpr[5] = w & ~1u;
}
// void f(Obj* o, u32 v): when v changes, o->+744 (f32) = (v ? 0x01032CCC :
// 0x010324B4)->+72; o->+440 = v
DECOMP(00106700, Field440_Set_00106700) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 old = load<u32>(o + 440);
    ctx.gpr[5] = old;  // r5 / f1 left
    if (old != v) {
        const u32 c = v != 0 ? 0x01032CCCu : 0x010324B4u;
        const u32 f = CopyW(o + 744 - 72, c, 72);
        ctx.gpr[5] = c;
        ctx.fpr[1] = F32Bits(f);
    }
    store<u32>(o + 440, v);
}
// bool f(const Obj* o): o->+188 bit 0x20 && o->+216->+980 == 0x04318BE6
DECOMP(0012999C, Field_Test216_980_0012999C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 188) & 0x20u;
    const u32 p = load<u32>(o + 216);
    ctx.gpr[4] = p;  // r4 / r5 left
    ctx.gpr[5] = b;
    u32 r = 0;
    if (b != 0) {
        const u32 v = load<u32>(p + 980);
        ctx.gpr[4] = v;
        ctx.gpr[5] = 0x04318BE6;
        r = v == 0x04318BE6u ? 1u : 0u;
    }
    ret(ctx, r);
}
// T* f(o): r = CallK2_000D9424(o); if (r) r->+28 = 0xCABDE7BA
DECOMP(0012BD5C, Call_D9424_SetHash_0012BD5C) {
    const u32 r = static_cast<u32>(call(ctx, 0x000D9424, {ctx.gpr[3]}));
    if (r == 0) return;
    store<u32>(r + 28, 0xCABDE7BAu);
    ctx.gpr[4] = 0xCABDE7BAu;  // r4 left
}
// void f(Obj* o): (o && (o->+196 bit 0x02000000) ? o : 0)->+855 |= 0x40
// (quirk kept: writes address 855 otherwise)
DECOMP(001CF5E0, Flag855_Set40_001CF5E0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 p = 0;
    if (o != 0) {
        const u32 b = load<u32>(o + 196) & 0x02000000u;
        ctx.gpr[5] = b;  // r5 left
        if (b != 0) p = o;
    }
    const u32 v = load<u8>(p + 855) | 0x40u;
    store<u8>(p + 855, static_cast<u8>(v));
    ctx.gpr[3] = v;  // r3 / r4 left
    ctx.gpr[4] = p;
}
// bool f(o, f32 x): sub_000E4720(o, x) != 0
DECOMP(001EEE40, Call_E4720_Bool) { ret(ctx, call(ctx, 0x000E4720, {ctx.gpr[3]}) != 0 ? 1u : 0u); }
// bool f(o, f32 x): sub_000E5000(o, x) != 0
DECOMP(001EEE78, Call_E5000_Bool) { ret(ctx, call(ctx, 0x000E5000, {ctx.gpr[3]}) != 0 ? 1u : 0u); }
// f(Obj* o, f32 x): o->+388->+510 (s16) = the high half of (f32)x's bits; sub_000DA25C(o, x)
DECOMP(00207204, Field388_SetHalf_00207204) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 388);
    const s32 hi = static_cast<s32>(std::bit_cast<u32>(static_cast<f32>(farg(ctx, 0)))) >> 16;
    store<u16>(p + 510, static_cast<u16>(hi));
    call(ctx, 0x000DA25C, {o, p, static_cast<u64>(static_cast<s64>(hi))});
}
// f(const Obj* o): if (o->+356) Effect_SetHash4CF988(o->+356, 0x707A34AB)
DECOMP(00229C5C, Effect_SetHash356_00229C5C) {
    const u32 e = load<u32>(arg<u32>(ctx, 0) + 356);
    ctx.gpr[3] = e;  // r3 left
    if (e != 0) call(ctx, 0x004CF988, {e, 0x707A34ABu});
}
// void f(Obj* o, u8 b): o->+600 = 0, o->+604 = b; if o->+592 <= *(f32*)0x002448B8, o->+592 = 1.0f
DECOMP(002448C0, Field592_Reset_002448C0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 c = load<f32>(0x002448B8), v = load<f32>(o + 592);
    store<u32>(o + 600, 0);
    store<u8>(o + 604, static_cast<u8>(arg<u32>(ctx, 1)));
    ctx.gpr[5] = 0x002448B8;  // r4..r6 / f1 / f2 left
    ctx.gpr[6] = 0;
    ctx.fpr[1] = c;
    ctx.fpr[2] = v;
    if (v <= c) {
        store<u32>(o + 592, 0x3F800000u);
        ctx.gpr[4] = 0x3F800000u;
    }
}
// void f(Obj* o, s32 i, ?, bool both, f32 x): entry i (112 bytes): +192 = x, and +188 = x when both
DECOMP(00254108, Entry112_SetF_00254108) {
    const u64 b = ctx.gpr[3] + ((ctx.gpr[4] << 7) - (ctx.gpr[4] << 4));
    const f32 x = static_cast<f32>(farg(ctx, 0));
    const u32 both = arg<u32>(ctx, 3) & 0xFFu;
    store<f32>(static_cast<u32>(b + 192), x);
    ctx.gpr[4] = static_cast<u32>(b + 192);  // r3..r5 left
    ctx.gpr[5] = b + 192;
    ctx.gpr[3] = b;
    if (both != 0) {
        store<f32>(static_cast<u32>(b + 188), x);
        ctx.gpr[3] = static_cast<u32>(b + 188);
    }
}
// T* f(const Obj* o): o->+48 < o->+36 (u16) ? o->+28[o->+48]->+48->+48 : 0
DECOMP(00268120, Field_Cur28_48_48_00268120) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 i = load<u32>(o + 48), n = load<u16>(o + 36);
    ctx.gpr[5] = n;  // r4 / r5 left
    ctx.gpr[4] = i;
    if (i >= n) {
        ret(ctx, 0u);
        return;
    }
    ctx.gpr[4] = i << 2;
    ret(ctx, load<u32>(load<u32>(load<u32>(load<u32>(o + 28) + (i << 2)) + 48) + 48));
}
// f(a, ?, c, f32 x): if (c) sub_002874E8(c, a, a, x)
DECOMP(00287874, Call_2874E8_Swap_00287874) {
    const u64 a = ctx.gpr[3], c = ctx.gpr[5];
    ctx.gpr[4] = c;  // r4 left
    if (static_cast<u32>(c) != 0) call(ctx, 0x002874E8, {c, a, a});
}
// bool f(const Obj* o): p = o->+8->+164: !p || ((p->+40 (u16) >> 5) & 3) == 3
DECOMP(0029394C, Field_Test8_164_0029394C) {
    const u32 p = load<u32>(load<u32>(arg<u32>(ctx, 0) + 8) + 164);
    if (p == 0) {
        ret(ctx, 1u);
        return;
    }
    const u32 v = (load<u16>(p + 40) >> 5) & 3u;
    ctx.gpr[4] = v;  // r4 left
    ret(ctx, v == 3 ? 1u : 0u);
}
// f(o, f32 x): if |x| >= *(f32*)0x00297DFC, sub_00297BA8(o, |x|)
DECOMP(00297E00, Call_297BA8_IfAbs_00297E00) {
    const f64 a = std::fabs(farg(ctx, 0)), c = load<f32>(0x00297DFC);
    ctx.fpr[1] = a;  // f1 / f2 / r4 left
    ctx.fpr[2] = c;
    ctx.gpr[4] = 0x00297DFC;
    if (a < c) return;
    call(ctx, 0x00297BA8, {ctx.gpr[3]});
}
// void f(Obj* o): the counter in o->+28 bits 2..9 (u16) counts down; on reaching 0, o->+72 = 0 and o->+76 (u8) = 0
DECOMP(002D2EC4, Counter28_Dec_002D2EC4) {
    const u32 o = arg<u32>(ctx, 0);
    u32 h = load<u16>(o + 28);
    u32 n = (h >> 2) & 0xFFu;
    ctx.gpr[4] = h;  // r4 / r5 left
    ctx.gpr[5] = n;
    if (n == 0) return;
    n -= 1;
    h = (h & ~0x3FCu) | ((n << 2) & 0x3FCu);
    n &= 0xFFu;
    store<u16>(o + 28, static_cast<u16>(h));
    ctx.gpr[4] = h;
    ctx.gpr[5] = n;
    if (n != 0) return;
    store<u32>(o + 72, 0);
    store<u8>(o + 76, 0);
    ctx.gpr[4] = 0;
}
// u8 f(const Obj* o): the current slot record's byte +418 + o->+822 * 4
DECOMP(002DAFFC, SlotRec_Byte418_002DAFFC) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 822);
    ret(ctx, load<u8>(CurSlotRec22(ctx) + (b << 2) + 418));
}
// f(const Obj* o): UserBlock_Find465F30((*(T**)0x00F65CA0) + 4208, o->+96)
DECOMP(002E20CC, Call_UserBlockFind_002E20CC) {
    const u32 b = load<u32>(0x00F65CA0) + 4208;
    call(ctx, 0x00465F30, {b, load<u32>(arg<u32>(ctx, 0) + 96), b});
}
// bool f(Obj* o): o->+40 && o->+41 (u8) ? 1 : (o->+41 = 1, o->+48 = o->+44 (f32), 0)
DECOMP(002FD660, Field_Latch41_002FD660) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = load<u8>(o + 40);
    if (b != 0) {
        b = load<u8>(o + 41);
        if (b != 0) {
            ctx.gpr[4] = b;  // r4 left
            ret(ctx, 1u);
            return;
        }
    }
    const u32 f = load<u32>(o + 44);
    store<u8>(o + 41, 1);
    store<u32>(o + 48, f);
    ctx.gpr[4] = 1;  // r4 / f1 left
    ctx.fpr[1] = F32Bits(f);
    ret(ctx, 0u);
}
// bool f(const Obj* o): !(o->+160 >> 1) || o->+408 + 1 >= o->+8->+168 (signed)
DECOMP(003814A8, Field_Test408_003814A8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 160), v = load<u32>(o + 408);
    ctx.gpr[4] = v + 1;  // r4..r6 left
    ctx.gpr[5] = v;
    ctx.gpr[6] = w >> 1;
    if ((w >> 1) == 0) {
        ret(ctx, 1u);
        return;
    }
    ret(ctx, static_cast<s32>(v + 1) < load<s32>(load<u32>(o + 8) + 168) ? 0u : 1u);
}
// bool f(const Obj* o): o->+428 > *(f32*)0x00419C70 && o->+432 (u8)
DECOMP(00419C78, Field428_Test_00419C78) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = load<f32>(o + 428), c = load<f32>(0x00419C70);
    ctx.gpr[4] = 0x00419C70;  // r4 / f1 / f2 left
    ctx.fpr[1] = v;
    ctx.fpr[2] = c;
    ret(ctx, (v > c && load<u8>(o + 432) != 0) ? 1u : 0u);
}
// T* f(u32 i): the array at 0x010AA694 (+0 data, +8 u16 count): i < count ? data[i] : 0
DECOMP(0042BCD4, Array10AA694_At_0042BCD4) {
    constexpr u32 a = 0x010AA694;
    const u32 i = arg<u32>(ctx, 0), n = load<u16>(a + 8);
    ctx.gpr[4] = n;  // r4 left
    if (i >= n) {
        ret(ctx, 0u);
        return;
    }
    const u32 d = load<u32>(a);
    ctx.gpr[4] = d;
    ret(ctx, load<u32>(d + (i << 2)));
}
// void f(Obj* o): Flag_Set8_40000000(o), o->+32 = 0
DECOMP(00434AB8, Obj_Flag8Clear32_00434AB8) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0042CA60, {o});
    store<u32>(o + 32, 0);
    ret(ctx, 0u);  // r3 left as 0
}
// bool f(T** pp): pp && *pp && (*pp)->+88 > 0 (signed)
DECOMP(00443168, Ptr_Test88_00443168) {
    const u32 pp = arg<u32>(ctx, 0);
    u32 p = 0;
    if (pp != 0) p = load<u32>(pp);
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const s32 v = load<s32>(p + 88);
    ctx.gpr[4] = static_cast<u64>(static_cast<s64>(v >> 31));  // r4 left
    ret(ctx, v > 0 ? 1u : 0u);
}
// f(a, s8 b): if (*(u8*)0x010AEC60) sub_006D5038(a, b)
DECOMP(0044CEB0, Call_6D5038_If_0044CEB0) {
    const u64 b = static_cast<u64>(static_cast<s64>(static_cast<s8>(arg<u32>(ctx, 1))));
    const u32 f = load<u8>(0x010AEC60);
    ctx.gpr[4] = b;  // r4 / r5 left
    ctx.gpr[5] = f;
    if (f != 0) call(ctx, 0x006D5038, {ctx.gpr[3], b, f});
}
// The type index (the global type record's +16->+64) used to index an object's per-type table.
static u32 TypeIndex(u32 rec) { return load<u32>(load<u32>(rec + 16) + 64); }
// T* f(const Obj* o): o->+48 ? o->+48->+12[type index of 0x010AB310] : 0
DECOMP(004562B8, Field48_TypeSlot_004562B8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 48);
    ctx.gpr[4] = p;  // r4 left
    if (p == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 t = load<u32>(p + 12);
    ctx.gpr[4] = t;
    ret(ctx, load<u32>(t + (TypeIndex(0x010AB310) << 2)));
}
// bool f(const Obj* o): o->+308->+12[type index of rec] == 0 (r4 / r5 left)
static void TypeSlotIsNull(PPUContext& ctx, u32 rec) {
    const u32 t = load<u32>(load<u32>(arg<u32>(ctx, 0) + 308) + 12);
    const u32 v = load<u32>(t + (TypeIndex(rec) << 2));
    ctx.gpr[4] = v;
    ctx.gpr[5] = t;
    ret(ctx, v == 0 ? 1u : 0u);
}
// bool f(const Obj* o): o's slot for the type 0x00F94A54 is empty
DECOMP(00738A54, Obj_TypeSlotNull_F94A54) { TypeSlotIsNull(ctx, 0x00F94A54); }
// bool f(const Obj* o): o's slot for the type 0x00F94CAC is empty
DECOMP(00738FEC, Obj_TypeSlotNull_F94CAC) { TypeSlotIsNull(ctx, 0x00F94CAC); }
// The 28-byte message records of 0x00464EA0: {+0 kind, +4 time (f32 of
// (*(T**)0x00F65CA0)->+12->+0), +8 / +12 / +16, +20, +24}.
static u32 MsgTime(PPUContext& ctx, u32 reg) {
    const u32 p = load<u32>(load<u32>(0x00F65CA0) + 12);
    ctx.gpr[reg] = p;  // left
    const u32 f = load<u32>(p);
    ctx.fpr[1] = F32Bits(f);
    return f;
}
// void f(Msg* m, u32 a, u32 b): m = {7, time, 0, 0, 0, a, b}
DECOMP(004651D8, Msg_Init7_004651D8) {
    const u32 m = arg<u32>(ctx, 0);
    store<u32>(m + 20, arg<u32>(ctx, 1));
    store<u32>(m, 7);
    const u32 f = MsgTime(ctx, 4);
    store<u32>(m + 24, arg<u32>(ctx, 2));
    store<u32>(m + 4, f);
    for (u32 k = 8; k <= 16; k += 4) store<u32>(m + k, 0);
    ctx.gpr[6] = 0x00F60000;  // r6..r8 left
    ctx.gpr[7] = 7;
    ctx.gpr[8] = 0;
}
// void f(Msg* m, u32 b): m = {kind, time, 0, 0, 0, 0, b} (r5..r7 left)
static void MsgInitK(PPUContext& ctx, u32 kind) {
    const u32 m = arg<u32>(ctx, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, kind);
    const u32 f = MsgTime(ctx, 6);
    store<u32>(m + 24, arg<u32>(ctx, 1));
    store<u32>(m + 4, f);
    for (u32 k = 8; k <= 16; k += 4) store<u32>(m + k, 0);
    ctx.gpr[5] = 0;
    ctx.gpr[7] = kind;
}
// void f(Msg* m, u32 b): kind 13
DECOMP(00465384, Msg_Init13_00465384) { MsgInitK(ctx, 13); }
// void f(Msg* m, u32 b): kind 14
DECOMP(004653BC, Msg_Init14_004653BC) { MsgInitK(ctx, 14); }
// f(): Handle_Notify36742C(*(u32*)0x00F64D08, 1, 0, 0)
DECOMP(004902B8, Call_HandleNotify_F64D08) { call(ctx, 0x0036742C, {load<u32>(0x00F64D08), 1, 0, 0}); }
// void f(s32 m): if m is 1 or 2, *(u32*)0x010BB850 = m
DECOMP(00491180, Global_Set10BB850_00491180) {
    const s32 m = arg<s32>(ctx, 0);
    if (m != 1 && m != 2) return;
    store<u32>(0x010BB850, static_cast<u32>(m));
    ctx.gpr[3] = 0x010C0000;  // r3 / r4 left
    ctx.gpr[4] = static_cast<u32>(m);
}
// bool f(Obj* o, const s32* g): if (o) WorldObj_SetGroup(o, *g); 1
DECOMP(004B00EC, WorldObj_SetGroupFrom_004B00EC) {
    const u32 o = arg<u32>(ctx, 0);
    if (o != 0) call(ctx, 0x00583E54, {o, static_cast<u64>(static_cast<s64>(load<s32>(arg<u32>(ctx, 1))))});
    ret(ctx, 1u);
}
// void f(Obj* o, const char* s): o->+72 = String_HashNoCase(s)
DECOMP(00736560, Field72_SetHash_00736560) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 72, static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {ctx.gpr[4]})));
}
// void f(TypedValue* v, u32 a, u8 hi, u32 c, u32 lo): kind 4, vtable 0x00B0DC0C,
// value (hi << 24) | (lo & 0xFFFFFF)
DECOMP(007366F4, TypedValue_Init4_007366F4) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 4, arg<u32>(ctx, 1));
    store<u32>(o + 12, arg<u32>(ctx, 3));
    const u32 v = (arg<u32>(ctx, 4) & 0x00FFFFFFu) | (arg<u32>(ctx, 2) << 24);
    store<u32>(o + 8, 4);
    store<u32>(o + 16, 0);
    store<u32>(o + 20, v);
    store<u32>(o, 0x00B0DC0C);
    ctx.gpr[4] = o + 20;  // r4..r8 left
    ctx.gpr[5] = 0x00B0DC0C;
    ctx.gpr[6] = 0;
    ctx.gpr[7] = v;
    ctx.gpr[8] = 4;
}
// void f(f32 out[3], const f32 v[3], f32 s): out = v * (*(f32*)0x0074B5B8 / s)
DECOMP(0074B5C0, Vec3_DivScale_0074B5C0) {
    const u32 out = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const f64 k = static_cast<f32>(static_cast<f64>(load<f32>(0x0074B5B8)) / farg(ctx, 0));
    const f32 x = static_cast<f32>(static_cast<f64>(load<f32>(v)) * k);
    const f32 y = static_cast<f32>(static_cast<f64>(load<f32>(v + 4)) * k);
    const f32 z = static_cast<f32>(static_cast<f64>(load<f32>(v + 8)) * k);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[2] = x;  // f2..f4 left
    ctx.fpr[3] = y;
    ctx.fpr[4] = load<f32>(v + 8);
    ctx.gpr[5] = 0x0074B5B8;  // r5 left
    ctx.fpr[1] = z;
}
// void f(Obj* o): the 80 bytes at o zeroed (stvx), then o->+52 = 0.1f
DECOMP(0074CDE0, Obj_Clear80_0074CDE0) {
    const u32 o = arg<u32>(ctx, 0);
    const V4 z{};
    for (u32 k = 0; k < 5; ++k) VSt(o + 16 * k, z);
    store<u32>(o + 52, 0x3DCCCCCDu);
    ctx.gpr[4] = 16;  // r4..r8 left
    ctx.gpr[5] = 64;
    ctx.gpr[6] = 32;
    ctx.gpr[7] = 48;
    ctx.gpr[8] = 0x3DCCCCCDu;
}
// f(Obj* o): if (o) o->+0 = 0x00B1F5D0 (vtable), sub_007D1CE4(o)
DECOMP(00771D08, Obj_DtorVtB1F5D0_00771D08) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1F5D0);
    call(ctx, 0x007D1CE4, {o, 0x00B1F5D0});
}

// ---------------------------------------------------------------- sweep: 15 instructions
// void f(Obj* o, u32 v): append v to the 8-entry array at o + 3700 (count +20) when not full
DECOMP(0001B238, Arr3700_Push_0001B238) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 n0 = load<s32>(o + 20);
    ctx.gpr[5] = static_cast<u32>(n0);  // r3 / r5..r7 left
    if (n0 >= 8) return;
    const u32 n = load<u32>(o + 20);
    store<u32>(o + 20, n + 1);
    const u32 e = o + (n << 2) + 3700;
    store<u32>(e, arg<u32>(ctx, 1));
    ctx.gpr[3] = e;
    ctx.gpr[5] = o + 20;
    ctx.gpr[6] = n + 1;
    ctx.gpr[7] = n << 2;
}
// bool f(Obj* o): o->+72 < *(f32*)0x0001BE4C ? (o->+72 = (f32) 0x00F9CB78->+72, 1) : 0
DECOMP(0001BE50, Field72_Reset_0001BE50) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 v = load<f32>(o + 72), c = load<f32>(0x0001BE4C);
    ctx.fpr[1] = v;  // r4 / f1 / f2 left
    ctx.fpr[2] = c;
    ctx.gpr[4] = 0x0001BE4C;
    if (!(v < c)) {
        ret(ctx, 0u);
        return;
    }
    const u32 f = load<u32>(0x00F9CB78 + 72);
    store<u32>(o + 72, f);
    ctx.gpr[4] = 0x00F9CB78;
    ctx.fpr[1] = F32Bits(f);
    ret(ctx, 1u);
}
// f(Obj* o, a): o->vfn[220 / 4](o, a)
DECOMP(0002BED8, Obj_VCall220_0002BED8) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 220, {o, ctx.gpr[4]});
}
// f(const Obj* o): World_SetExposure(*(World**)0x00F65CA0, o->+12, o->+16, o->+20)
DECOMP(0008054C, World_SetExposureFrom_0008054C) {
    const u32 o = arg<u32>(ctx, 0), w = load<u32>(0x00F65CA0);
    callf(ctx, 0x00484488, {w, w}, {load<f32>(o + 12), load<f32>(o + 16), load<f32>(o + 20)});
}
// f(const Obj* o): if (o->+12) SaveSlot_MarkSeen(*(T**)0x00F64C20, o->+12)
DECOMP(00099808, SaveSlot_MarkSeen12_00099808) {
    const u32 h = load<u32>(arg<u32>(ctx, 0) + 12);
    ctx.gpr[3] = h;  // r3 left
    if (h != 0) call(ctx, 0x002E2874, {load<u32>(0x00F64C20), h, 0x00F60000});
}
// f(Obj* o, const f32 v[3]): o->+736..+744 = v; sub_000F498C(o, 1)
DECOMP(000F5E28, Field736_SetV_000F5E28) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 k = 0; k < 3; ++k) {
        w = load<u32>(v + 4 * k);
        store<u32>(o + 736 + 4 * k, w);
    }
    ctx.fpr[1] = F32Bits(w);
    call(ctx, 0x000F498C, {o, 1});
}
// f(const Obj* o): sub_00443368(o->+348)
DECOMP(000F60DC, Call_443368_348_000F60DC) { call(ctx, 0x00443368, {load<u32>(arg<u32>(ctx, 0) + 348)}); }
// f(Obj* o): if (o->+196) Mem_FreeIfSet(o->+196) (r4 = o->+192, r5 = o)
DECOMP(0011A5CC, Field196_Free_0011A5CC) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 196);
    ctx.gpr[4] = p;  // r4 left
    if (p != 0) call(ctx, 0x00555CC0, {p, load<u32>(o + 192), o});
}
// void f(f32 out[3], const Obj* o): out = o->+216 ? o->+216->+320..+328 : the 3 floats at 0x01164B40
DECOMP(001E1EE4, Pos216_Get_001E1EE4) {
    const u32 out = arg<u32>(ctx, 0), p = load<u32>(arg<u32>(ctx, 1) + 216);
    const u32 s = p != 0 ? p + 320 : 0x01164B40u;
    u32 w = 0;
    for (u32 k = 0; k < 3; ++k) w = CopyW(out, s, 4 * k);
    ctx.gpr[4] = s;  // r4 / f1 left
    ctx.fpr[1] = F32Bits(w);
}
// void f(Obj* o, const Ev* e): e->+8 == 0x69988172 && o->+856 && e->+0 == o->+856: o->+980 |= 0x20000
DECOMP(001EEEB0, Obj_OnEvent69988172_001EEEB0) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 k = load<u32>(e + 8);
    ctx.gpr[6] = k;  // r4..r6 left
    ctx.gpr[5] = 0x69988172;
    if (k != 0x69988172u) return;
    const u32 t = load<u32>(o + 856);
    ctx.gpr[5] = t;
    if (t == 0) return;
    const u32 v = load<u32>(e);
    ctx.gpr[4] = v;
    if (v != t) return;
    const u32 w = load<u32>(o + 980) | 0x20000u;
    store<u32>(o + 980, w);
    ctx.gpr[4] = w;
}
// f(Obj* o, u8 b): if (!b && o->+856) sub_001EE680(o, 1)
DECOMP(001F509C, Call_1EE680_001F509C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[4] = b;  // r4 left
    if (b != 0) return;
    const u32 t = load<u32>(o + 856);
    ctx.gpr[4] = t;
    if (t != 0) call(ctx, 0x001EE680, {o, 1});
}
// Body* f(const Obj* o): o->+32 ? Hk_GetBody9D19C8(o->+32) : 0
DECOMP(00252940, Obj_GetBody_00252940) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    u32 r = 0;
    if (p != 0) r = static_cast<u32>(call(ctx, 0x009D19C8, {p}));
    ctx.gpr[4] = r;  // r4 left
    ret(ctx, r);
}
// f(Obj* o): if (o->+84) sub_000250A0(o->+84, o + 32)
DECOMP(0027CDEC, Call_250A0_0027CDEC) {
    const u64 a = ctx.gpr[3] + 32;
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 84);
    ctx.gpr[4] = p;  // r4 left
    if (p != 0) call(ctx, 0x000250A0, {p, static_cast<u32>(a), a});
}
// o->+off = x; and o->+2292 = x when o->+2276->+4->+8 is `hash` (r4 / r5 left).
static void SetF2292If(PPUContext& ctx, u32 off, u32 hash) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 x = static_cast<f32>(farg(ctx, 0));
    const u32 p = load<u32>(o + 2276);
    store<f32>(o + off, x);
    ctx.gpr[4] = p;
    if (p == 0) return;
    const u32 q = load<u32>(p + 4);
    const u32 h = q != 0 ? load<u32>(q + 8) : 0u;
    ctx.gpr[4] = hash;
    ctx.gpr[5] = h;
    if (h == hash) store<f32>(o + 2292, x);
}
// void f(Obj* o, f32 x): o->+2284 = x (and +2292 for the 0x9F6559A5 kind)
DECOMP(0029F6E8, Field2284_Set_0029F6E8) { SetF2292If(ctx, 2284, 0x9F6559A5u); }
// void f(Obj* o, f32 x): o->+2288 = x (and +2292 for the 0xD3380BFC kind)
DECOMP(0029F724, Field2288_Set_0029F724) { SetF2292If(ctx, 2288, 0xD3380BFCu); }
// u32 f(const Obj* o): the current slot record's u16 +416 + o->+822 * 4, low 3 bits
DECOMP(002DA990, SlotRec_Bits416_002DA990) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 822);
    ret(ctx, load<u16>(CurSlotRec22(ctx) + (b << 2) + 416) & 7u);
}
// void f(?, u8 v): the current slot record's byte +329 = v == 1
DECOMP(002E6D30, SlotRec_Set329_002E6D30) {
    const u32 v = (arg<u32>(ctx, 1) & 0xFFu) == 1 ? 1u : 0u;
    const u32 r = CurSlotRec22(ctx);
    store<u8>(r + 329, static_cast<u8>(v));
    ctx.gpr[4] = v;  // r3..r5 left
    ret(ctx, r);
}
// f(Obj* o, Cursor* c): XmlCursor_ReadF32(c, 0x00274ACD, &o->+4, default o->+4)
DECOMP(003032EC, Xml_ReadF32_4_003032EC) {
    const u64 p = ctx.gpr[3] + 4;
    const f64 d = load<f32>(static_cast<u32>(p));
    callf(ctx, 0x004F9B20, {ctx.gpr[4], 0x00274ACD, p, 0x00270000}, {d});
}
// f(Obj* o): o->+80 = (f32) 0x01069AF0->+72; sub_00308F5C(o->+32->+872, 0)
DECOMP(003131E0, Field80_Reset_003131E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 q = load<u32>(o + 32);
    const u32 f = load<u32>(0x01069AF0 + 72);
    store<u32>(o + 80, f);
    ctx.fpr[1] = F32Bits(f);
    call(ctx, 0x00308F5C, {load<u32>(q + 872), 0, 0x01069AF0, q});
}
// f(Obj* o): sub_003274D0(o, 0); sub_00334114(o)
DECOMP(0033521C, Call_3274D0_334114) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x003274D0, {o, 0});
    call(ctx, 0x00334114, {o});
}
// f(Obj* o, u32 i): if (o->+264[i]) Obj_Attach364408(o, o->+264[i])
DECOMP(0036E714, Obj_Attach264_0036E714) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(static_cast<u32>(ctx.gpr[3] + (ctx.gpr[4] << 2) + 264));
    ctx.gpr[4] = p;  // r4 left
    if (p != 0) call(ctx, 0x00364408, {o, p});
}
// f(Obj* o): if (o->+4) o->+4->+24 |= 0x20000000; Flag_Set8_40000000(o)
DECOMP(0042EA30, Obj_Flag24Flag8_0042EA30) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 p = load<u32>(o + 4);
    if (p != 0) store<u32>(p + 24, load<u32>(p + 24) | 0x20000000u);
    call(ctx, 0x0042CA60, {o});
}
// f(const Obj* o): World_SetExposure(*(World**)0x00F65CA0, o->+12, o->+16, o->+20)
DECOMP(0042FA90, World_SetExposureFrom_0042FA90) {
    const u32 o = arg<u32>(ctx, 0), w = load<u32>(0x00F65CA0);
    callf(ctx, 0x00484488, {w, w}, {load<f32>(o + 12), load<f32>(o + 16), load<f32>(o + 20)});
}
// (*(T**)0x00F65CA0)->+off bit `bit` = o->+9 bit 0; then Flag_Set8_40000000(o).
static void SetF65CA0Bit(PPUContext& ctx, u32 off, u32 bit) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 g = load<u32>(0x00F65CA0);
    const u32 b = load<u8>(o + 9);
    const u32 v = (load<u8>(g + off) & ~(1u << bit)) | ((b & 1u) << bit);
    store<u8>(g + off, static_cast<u8>(v));
    call(ctx, 0x0042CA60, {o, g, b, v});
}
// f(Obj* o): (*(T**)0x00F65CA0)->+45 bit 0x20 = o->+9; Flag_Set8_40000000(o)
DECOMP(00438558, Global_F65CA0_Bit45_20) { SetF65CA0Bit(ctx, 45, 5); }
// f(Obj* o): (*(T**)0x00F65CA0)->+46 bit 0x80 = o->+9; Flag_Set8_40000000(o)
DECOMP(004385EC, Global_F65CA0_Bit46_80) { SetF65CA0Bit(ctx, 46, 7); }
// f(Obj* o): (*(T**)0x00F65CA0)->+46 bit 0x40 = o->+9; Flag_Set8_40000000(o)
DECOMP(00438680, Global_F65CA0_Bit46_40) { SetF65CA0Bit(ctx, 46, 6); }
// f(Obj* o): o->+0 = 0x00B217E8 (vtable), *(u32*)0x00F658D4 = 0, Thunk_004F41AC(o)
DECOMP(0043DA08, Obj_DtorVtB217E8_0043DA08) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B217E8);
    store<u32>(0x00F658D4, 0);
    call(ctx, 0x004F41AC, {o, 0x00B217E8, 0x00F60000, 0});
}
// f(a, b, c): sub_00446198(a, 0, b, c, 1)
DECOMP(00446348, Call_446198_00446348) { call(ctx, 0x00446198, {ctx.gpr[3], 0, ctx.gpr[4], ctx.gpr[5], 1}); }
// void f(Obj* o, u32 a, u32 b): every node of the list o->+32 (linked by +72) gets +64 = a, +68 = b
DECOMP(0045669C, List32_Set64_0045669C) {
    const u32 a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    u32 p = load<u32>(arg<u32>(ctx, 0) + 32);
    while (p != 0) {
        store<u32>(p + 64, a);
        store<u32>(p + 68, b);
        p = load<u32>(p + 72);
        if (p == 0) break;
        store<u32>(p + 64, a);
        const u32 n = load<u32>(p + 72);
        store<u32>(p + 68, b);
        ctx.gpr[6] = n;  // r6 left
        p = n;
    }
    ret(ctx, 0u);
}
// void f(Msg* m, const Obj* o): m = {kind, time, 0, o->+off->+204, 0, 0}
static void MsgInitRef(PPUContext& ctx, u32 kind, u32 off) {
    const u32 m = arg<u32>(ctx, 0);
    store<u32>(m + 20, 0);
    store<u32>(m, kind);
    const u32 f = MsgTime(ctx, 6);
    store<u32>(m + 8, 0);
    store<u32>(m + 4, f);
    const u32 v = load<u32>(load<u32>(arg<u32>(ctx, 1) + off) + 204);
    store<u32>(m + 16, 0);
    store<u32>(m + 12, v);
    ctx.gpr[4] = v;  // r4 / r5 / r7 left
    ctx.gpr[5] = 0;
    ctx.gpr[7] = kind;
}
// void f(Msg* m, const Obj* o): kind 17 with o->+60->+204
DECOMP(004655D4, Msg_Init17_004655D4) { MsgInitRef(ctx, 17, 60); }
// void f(Msg* m, const Obj* o): kind 18 with o->+40->+204
DECOMP(00465610, Msg_Init18_00465610) { MsgInitRef(ctx, 18, 40); }
// void f(Obj* o, u32 v): o->+68 = v, o->+72 / +73 (u8) = 0, the first 68 bytes zeroed (memset)
static void Clear68(PPUContext& ctx, u32 v) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 68, v);
    store<u8>(o + 72, 0);
    store<u8>(o + 73, 0);
    call(ctx, 0x00726E54, {o, 0, 68, 0});
}
// void f(Obj* o): o->+68 = 0, +72 / +73 = 0, the first 68 bytes zeroed
DECOMP(0049E860, Obj_Clear68_0049E860) { Clear68(ctx, 0); }
// void f(Obj* o, u32 v): o->+68 = v, +72 / +73 = 0, the first 68 bytes zeroed
DECOMP(0049E8A8, Obj_Clear68Set_0049E8A8) { Clear68(ctx, arg<u32>(ctx, 1)); }
// f(Obj* o, const f32 v[3]): o->+64..+72 = v; WorldObj_MarkDirty(o)
DECOMP(00735C1C, WorldObj_SetPos64_00735C1C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    u32 w = 0;
    for (u32 k = 0; k < 3; ++k) {
        w = load<u32>(v + 4 * k);
        store<u32>(o + 64 + 4 * k, w);
    }
    ctx.fpr[1] = F32Bits(w);
    call(ctx, 0x0057D000, {o, v});
}
// void f(f32 out[3], const Obj* o): out = o->+144..+152 + o->+464..+472
DECOMP(00738308, Obj_PosPlus464_00738308) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    const f64 a0 = load<f32>(o + 464), a1 = load<f32>(o + 468), p0 = load<f32>(o + 144), p1 = load<f32>(o + 148);
    const f32 x = static_cast<f32>(p0 + a0);
    const f64 p2 = load<f32>(o + 152), a2 = load<f32>(o + 472);
    const f32 y = static_cast<f32>(p1 + a1), z = static_cast<f32>(p2 + a2);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.gpr[5] = o + 144;  // r5 / f1..f5 left
    ctx.fpr[1] = x;
    ctx.fpr[2] = y;
    ctx.fpr[3] = z;
    ctx.fpr[4] = p1;
    ctx.fpr[5] = a2;
}
// bool f(const Obj* o): o->+60 && o->+60->+348 <= *(f32*)0x00738520
DECOMP(00738528, Field60_348Le_00738528) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 60);
    u32 r = 0;
    if (p != 0) {
        const f64 v = load<f32>(p + 348), c = load<f32>(0x00738520);
        ctx.gpr[5] = 0x00738520;  // r5 / f1 / f2 left
        ctx.fpr[1] = v;
        ctx.fpr[2] = c;
        if (v <= c) r = 1;
    }
    ctx.gpr[4] = r;  // r4 left
    ret(ctx, r);
}
// f(Obj* o): o->+380 = 0x00B1200C, o->+0 = 0x00B126B8 (vtables); VtB12550_Dtor_000DF934(o)
DECOMP(00744358, Obj_DtorVtB126B8_00744358) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 380, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    call(ctx, 0x000DF934, {o, 0x00B1200C, 0x00B126B8});
}
// f(Obj* o, ?, a): o->vfn[16 / 4](o, <the OPD>, a) (r4 = the OPD, r6 = its entry, as the original leaves them)
DECOMP(0074948C, Obj_VCall16_0074948C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(load<u32>(o) + 16);
    call_ptr(ctx, opd, {o, opd, ctx.gpr[5], load<u32>(opd)});
}
// bool f(const Obj* o): o->+968 (u8) has any of 0x40 / 0x20 / 0x10
DECOMP(0074BB5C, Flag968_Any70_0074BB5C) {
    const u32 b = load<u8>(arg<u32>(ctx, 0) + 968);
    u32 r = 0;
    ctx.gpr[5] = (b >> 6) & 1u;  // r4 / r5 left
    if ((b & 0x40u) != 0) r = 1;
    else {
        ctx.gpr[5] = (b >> 5) & 1u;
        if ((b & 0x20u) != 0) r = 1;
        else if ((b & 0x10u) != 0) r = 1;
    }
    ctx.gpr[4] = r;
    ret(ctx, r);
}
// void f(Rec* d, const f32 a[3], ?, u8 k, const f32 c[3], f32 x): d = {+0 a, +16 c, +32 x, +36 k}
DECOMP(0074E8A0, Rec_Set_0074E8A0) {
    const u32 d = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), c = arg<u32>(ctx, 4);
    for (u32 k = 0; k < 3; ++k) CopyW(d, a, 4 * k);
    store<u32>(d + 16, load<u32>(c));
    const u32 c1 = load<u32>(c + 4);
    store<u8>(d + 36, static_cast<u8>(arg<u32>(ctx, 3)));
    store<u32>(d + 20, c1);
    const u32 c2 = load<u32>(c + 8);
    store<f32>(d + 32, static_cast<f32>(farg(ctx, 0)));
    store<u32>(d + 24, c2);
    ctx.fpr[2] = F32Bits(c2);  // f2 left
}
// f(PtrVector* v, u8 free): free ? PtrVector_Clear(v) : v->+8 (u16) = 0
static void VecClearOrReset(PPUContext& ctx) {
    const u32 f = arg<u32>(ctx, 1) & 0xFFu;
    if (f != 0) {
        call(ctx, 0x0054EC1C, {ctx.gpr[3], f});
        return;
    }
    store<u16>(arg<u32>(ctx, 0) + 8, 0);
    ctx.gpr[4] = 0;  // r4 left
}
// f(PtrVector* v, u8 free): free ? PtrVector_Clear(v) : count = 0
DECOMP(007609A0, PtrVector_Reset_007609A0) { VecClearOrReset(ctx); }
// f(PtrVector* v, u8 free): free ? PtrVector_Clear(v) : count = 0
DECOMP(007616B0, PtrVector_Reset_007616B0) { VecClearOrReset(ctx); }
// f(Obj* o, a): o->vfn[60 / 4](o, a)
DECOMP(00769AA4, Obj_VCall60_00769AA4) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 60, {o, ctx.gpr[4]});
}
// void f(?, Obj* o): o->+64 = 0, o->+8 = o->+40->+12 ? (f32) o->+40->+12->+12 : (f32) *k
static void ResetFromRef(PPUContext& ctx, u32 k) {
    const u32 o = arg<u32>(ctx, 1);
    u32 p = load<u32>(o + 40);
    store<u32>(o + 64, 0);
    ctx.gpr[5] = 0;  // r3 / r5 / f1 left
    if (p != 0) p = load<u32>(p + 12);
    const u32 s = p != 0 ? p + 12 : k;
    const u32 f = load<u32>(s);
    ctx.gpr[3] = p != 0 ? p : k;
    ctx.fpr[1] = F32Bits(f);
    store<u32>(o + 8, f);
}
// void f(?, Obj* o): o->+64 = 0, o->+8 from o->+40->+12->+12 or *(f32*)0x007718A8
DECOMP(007718B0, Obj_Reset8_007718B0) { ResetFromRef(ctx, 0x007718A8); }
// void f(?, Obj* o): o->+64 = 0, o->+8 from o->+40->+12->+12 or *(f32*)0x007718F0
DECOMP(007718F8, Obj_Reset8_007718F8) { ResetFromRef(ctx, 0x007718F0); }
// f(Obj* o, u8 b): o->+656 bit 0x01000000 = b bit 0; if b == 1, sub_00175CF0(o)
DECOMP(0077D09C, Flag656_Set_0077D09C) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 w = (load<u32>(o + 656) & ~0x01000000u) | ((b & 1u) << 24);
    store<u32>(o + 656, w);
    ctx.gpr[6] = w;  // r6 left
    if ((b & 0xFFu) == 1) call(ctx, 0x00175CF0, {o, b, ctx.gpr[5], w});
}

// ---------------------------------------------------------------- sweep: 16 instructions
// f(const Obj* o, a): o->+24->vfn[8 / 4](o->+24, a)
DECOMP(0001815C, Obj_VCall24_8_0001815C) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 24);
    sly4::obj19::VCall(ctx, p, 8, {p, ctx.gpr[4]});
}
// f(Obj* o, f32 x): w = o->+48->+172; unless bit 0x40 or 0x20, sub_00012E70(o, x)
DECOMP(0003C1BC, Call_12E70_0003C1BC) {
    const u32 w = load<u32>(load<u32>(arg<u32>(ctx, 0) + 48) + 172);
    ctx.gpr[5] = (w >> 6) & 1u;  // r4 / r5 left
    ctx.gpr[4] = w;
    if ((w & 0x40u) != 0) return;
    ctx.gpr[4] = (w >> 5) & 1u;
    if ((w & 0x20u) != 0) return;
    call(ctx, 0x00012E70, {ctx.gpr[3], 0, 0});
}
// f(const Obj* o): sub_0048C6EC(*(T**)0x00F65CA0, o->+12 (u8), o->+16, o->+20, o->+24)
DECOMP(00072DF8, Call_48C6EC_00072DF8) {
    const u32 o = arg<u32>(ctx, 0), g = load<u32>(0x00F65CA0);
    callf(ctx, 0x0048C6EC, {g, load<u8>(o + 12), g}, {load<f32>(o + 16), load<f32>(o + 20), load<f32>(o + 24)});
}
// void f(s32* res, const T* a, u32 out[3]): a->+4 == 0 ? *res = -2 :
// (out = {*(u32*)0x00F64584 + 7232, 0, 1}, *res = 0)
DECOMP(000B16E0, Get_F64584_000B16E0) {
    const u32 res = arg<u32>(ctx, 0), out = arg<u32>(ctx, 2);
    const u32 n = load<u32>(arg<u32>(ctx, 1) + 4);
    if (n < 1) {
        store<u32>(res, static_cast<u32>(-2));
        ctx.gpr[4] = static_cast<u64>(-2);  // r4 left
        return;
    }
    const u32 b = load<u32>(0x00F64584) + 7232;
    store<u32>(out + 4, 0);
    store<u32>(out + 8, 1);
    store<u32>(res, 0);
    store<u32>(out, b);
    ctx.gpr[4] = b;  // r4 / r6 / r7 left
    ctx.gpr[6] = 0;
    ctx.gpr[7] = 1;
}
// f(?, Obj* o): o->vfn[48 / 4](o)
DECOMP(000DD778, Obj_VCall48_000DD778) {
    const u32 o = arg<u32>(ctx, 1);
    sly4::obj19::VCall(ctx, o, 48, {o});
}
// f32 f(const Obj* o): o->+400 ? (World->+0x1281D4)->+84 * o->+192 : *(f32*)0x000EB03C
DECOMP(000EB040, Obj_Scale192_000EB040) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(o + 400);
    ctx.gpr[4] = t;  // r3..r5 left
    if (t == 0) {
        ctx.gpr[3] = 0x000EB03C;
        ret(ctx, load<f32>(0x000EB03C));
        return;
    }
    const u32 g = load<u32>(0x00F65CA0);
    const f64 s = load<f32>(o + 192);
    const u32 p = load<u32>(g + 0x001281D4u);
    ctx.gpr[3] = p;
    ctx.gpr[4] = g;
    ctx.gpr[5] = 0x001281D4;
    ctx.fpr[2] = load<f32>(p + 84);
    ret(ctx, static_cast<f32>(ctx.fpr[2] * s));
}
// f(Obj* o): o->vfn[32 / 4](o, 0)
DECOMP(001882B8, Obj_VCall32_0_001882B8) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 32, {o, 0});
}
// f(Obj* o): o->vfn[32 / 4](o, 0)
DECOMP(00194714, Obj_VCall32_0_00194714) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 32, {o, 0});
}
// void f(): p = World->+0x1281F4; if (p && (p->+28 & 0x100)) p->+309 = 1
DECOMP(001882F8, World_Flag1281F4_001882F8) {
    const u32 p = load<u32>(load<u32>(0x00F65CA0) + 0x001281F4u);
    ctx.gpr[3] = p;  // r3 / r4 left
    ctx.gpr[4] = 0x001281F4;
    if (p == 0) return;
    const u32 b = load<u32>(p + 28) & 0x100u;
    ctx.gpr[4] = b;
    if (b == 0) return;
    store<u8>(p + 309, 1);
    ctx.gpr[4] = 1;
}
// f(?, Obj* o): o->vfn[48 / 4](o)
DECOMP(001BD614, Obj_VCall48_001BD614) {
    const u32 o = arg<u32>(ctx, 1);
    sly4::obj19::VCall(ctx, o, 48, {o});
}
// f(Object* t): d = Target_GetDef0D951C(t); d->+124 bit 0x100000 cleared; Obj_SetStateFx266E6C(d, 2)
DECOMP(001E9164, Target_SetState2_001E9164) {
    const u32 d = static_cast<u32>(call(ctx, 0x000D951C, {ctx.gpr[3]}));
    const u32 w = load<u32>(d + 124) & ~0x00100000u;
    store<u32>(d + 124, w);
    call(ctx, 0x00266E6C, {d, 2, 0, w});
}
// u32 f(const Obj* o, s32 i): (i != -1 ? (o->+64 + i * 48)->+32 : 0), or o->+48 when 0
DECOMP(00264DA4, Rec48_Get32Or48_00264DA4) {
    const u32 o = arg<u32>(ctx, 0);
    u32 v = 0;
    if (arg<s32>(ctx, 1) != -1) {
        const u32 r = load<u32>(o + 64) + arg<u32>(ctx, 1) * 48u;
        ctx.gpr[4] = r;  // r4 left
        v = load<u32>(r + 32);
    }
    if (v == 0) v = load<u32>(o + 48);
    ctx.gpr[5] = v;  // r5 left
    ret(ctx, v);
}
// f(Obj* o): if (o->+92 && (o->+92->+164 & 0x8000)) sub_000D6614(o, 1)
DECOMP(0027BDC8, Call_D6614_0027BDC8) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 92);
    ctx.gpr[4] = p;  // r4 left
    if (p == 0) return;
    const u32 b = (load<u32>(p + 164) >> 15) & 1u;
    ctx.gpr[4] = b;
    if (b != 0) call(ctx, 0x000D6614, {ctx.gpr[3], b});
}
// f(Obj* o): World_AddTimedCallback(World, 2, cb, o, o->+8)
static void AddCallback2(PPUContext& ctx, u32 cb) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0046B5F8, {load<u32>(0x00F65CA0), 2, cb, o, load<u32>(o + 8)});
}
// f(Obj* o): World_AddTimedCallback(World, 2, 0x00F35300, o, o->+8)
DECOMP(00286B54, World_AddCallback_00286B54) { AddCallback2(ctx, 0x00F35300); }
// f(Obj* o): World_AddTimedCallback(World, 2, 0x00F353B0, o, o->+8)
DECOMP(0028EE94, World_AddCallback_0028EE94) { AddCallback2(ctx, 0x00F353B0); }
// Rec* f(Obj* o): the next of the 16 128-byte records at o + 112 (count +2160), or 0 when full
DECOMP(0029F760, Rec128_Alloc_0029F760) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 n0 = load<s32>(o + 2160);
    ctx.gpr[5] = static_cast<u32>(n0);  // r4..r6 left
    ctx.gpr[4] = 0;
    if (n0 >= 16) {
        ret(ctx, 0u);
        return;
    }
    const u32 n = load<u32>(o + 2160);
    store<u32>(o + 2160, n + 1);
    const u32 r = o + (n << 7) + 112;
    ctx.gpr[4] = r;
    ctx.gpr[5] = n + 1;
    ctx.gpr[6] = n << 7;
    ret(ctx, r);
}
// void f(Obj* o, u32 h): o->+2564 = the index (0 / 1) of h in the 2 words at 0x00F64A70 (h != 0)
DECOMP(002ABD08, Field2564_FindF64A70_002ABD08) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (h == 0) return;
    u32 w = 0;
    for (u32 i = 0; i < 2; ++i) {
        w = load<u32>(0x00F64A70 + 4 * i);
        if (w == h) store<u32>(o + 2564, i);
    }
    ctx.gpr[5] = 0x00F64A70;  // r5..r8 left
    ctx.gpr[6] = 8;
    ctx.gpr[7] = 2;
    ctx.gpr[8] = w;
}
// void f(Base24* b): Counter28_Dec on the object at b - 24 (r3 left as the halfword, or 0)
DECOMP(002D2EFC, Counter28_Dec_Adj24) {
    const u32 o = arg<u32>(ctx, 0) - 24;
    u32 h = load<u16>(o + 28);
    u32 n = (h >> 2) & 0xFFu;
    ctx.gpr[4] = o;  // r3..r5 left
    ctx.gpr[5] = n;
    ctx.gpr[3] = h;
    if (n == 0) return;
    n -= 1;
    h = (h & ~0x3FCu) | ((n << 2) & 0x3FCu);
    n &= 0xFFu;
    store<u16>(o + 28, static_cast<u16>(h));
    ctx.gpr[3] = h;
    ctx.gpr[5] = n;
    if (n != 0) return;
    store<u32>(o + 72, 0);
    store<u8>(o + 76, 0);
    ctx.gpr[3] = 0;
}
// f32 f(const Obj* o, u32 i): ((o->+48->+8->+0)[o->+52 * 2]->+0)[i * 2 + 1] (f32)
DECOMP(0031F8AC, Table_GetF_0031F8AC) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 a = load<u32>(load<u32>(load<u32>(o + 48) + 8));
    const u32 b = load<u32>(load<u32>(a + (load<u32>(o + 52) << 3)));
    ctx.gpr[3] = b + (i << 3);  // r3..r5 left
    ctx.gpr[4] = i << 3;
    ctx.gpr[5] = a;
    ret(ctx, load<f32>(b + (i << 3) + 4));
}
// bool f(Reader* r, ?, ...): *(u32*)a = *(u32*)(a + 4) = 0; Iff_ReadChunks(r, 0); 1
static void IffReadReset(PPUContext& ctx, u32 a) {
    store<u32>(a, 0);
    store<u32>(a + 4, 0);
    call(ctx, 0x00569388, {ctx.gpr[3], 0, 0x01070000, 0, 0x01070000});
    ret(ctx, 1u);
}
// bool f(Reader* r): the words at 0x0106BFA4 / 0x0106BFA8 = 0; Iff_ReadChunks(r, 0); 1
DECOMP(0034B844, Iff_ReadReset_0034B844) { IffReadReset(ctx, 0x0106BFA4); }
// bool f(Reader* r): the words at 0x0106BFB4 / 0x0106BFB8 = 0; Iff_ReadChunks(r, 0); 1
DECOMP(0034BB18, Iff_ReadReset_0034BB18) { IffReadReset(ctx, 0x0106BFB4); }
// f(Treasure* t, u32 h): if (t->+164 bit 31 && t->+132 == h) Treasure_Release425BA8(t, 1)
DECOMP(0042681C, Treasure_ReleaseIf_0042681C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 b = load<u32>(o + 164) >> 31;
    ctx.gpr[5] = b;  // r5 left
    if (b == 0) return;
    const u32 k = load<u32>(o + 132);
    ctx.gpr[5] = k;
    if (k == arg<u32>(ctx, 1)) call(ctx, 0x00425BA8, {o, 1, k});
}
// Obj_InitVt_B20CC8(o, on), then o->+0 = vt (r3 left as vt).
static void InitVtOver(PPUContext& ctx, u64 on, u32 vt) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x0042C9D0, {o, on});
    store<u32>(o, vt);
    ctx.gpr[3] = vt;
}
// void f(Obj* o): Obj_InitVt_B20CC8(o, 1), vtable 0x00B20E48
DECOMP(0042E9F0, Obj_InitVt_B20E48) { InitVtOver(ctx, 1, 0x00B20E48); }
// void f(Obj* o, u8 on): Obj_InitVt_B20CC8(o, on), vtable 0x00B21088
DECOMP(00433424, Obj_InitVt_B21088) { InitVtOver(ctx, arg<u32>(ctx, 1) & 0xFFu, 0x00B21088); }
// void f(Obj* o): Obj_InitVt_B20CC8(o, 1), vtable 0x00B21370
DECOMP(00439DC4, Obj_InitVt_B21370) { InitVtOver(ctx, 1, 0x00B21370); }
// f(Obj* o): Emitter_StopGlobal(o->+9 (u8)); Flag_Set8_40000000(o)
DECOMP(0043A80C, Emitter_Stop9_0043A80C) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x00449E10, {load<u8>(o + 9)});
    call(ctx, 0x0042CA60, {o});
}
// void* f(u32 n): *(u32*)0x010AC934 += n; Mem_Alloc(n, 0, 16)
DECOMP(0044DC80, Mem_AllocCounted_0044DC80) {
    const u32 n = arg<u32>(ctx, 0);
    const u32 t = load<u32>(0x010AC934) + n;
    store<u32>(0x010AC934, t);
    call(ctx, o22fn::Mem_Alloc, {n, 0, 16, 0x010B0000, t});
}
// void f(Msg* m, u32 a, f32 x): m = {10 (6 first), time, 0, 0, 0, a, x}
DECOMP(0046524C, Msg_Init10_0046524C) {
    const u32 m = arg<u32>(ctx, 0);
    store<u32>(m + 20, arg<u32>(ctx, 1));
    store<u32>(m, 6);
    const u32 p = load<u32>(load<u32>(0x00F65CA0) + 12);
    const u32 f = load<u32>(p);
    store<f32>(m + 24, static_cast<f32>(farg(ctx, 0)));
    store<u32>(m + 4, f);
    for (u32 k = 8; k <= 16; k += 4) store<u32>(m + k, 0);
    store<u32>(m, 10);
    ctx.gpr[4] = p;  // r4..r7 / f2 left
    ctx.gpr[5] = 10;
    ctx.gpr[6] = 6;
    ctx.gpr[7] = 0;
    ctx.fpr[2] = F32Bits(f);
}
// bool f(const u8* slot, u32 i): 2-bit entry i of the words at the save slot's +4016 (World + 80 + 3936) is 1
DECOMP(00465E60, SaveSlot_Bits2Is1_00465E60) {
    const u32 i = arg<u32>(ctx, 1);
    const u32 g = load<u32>(0x00F65CA0);
    const u32 r = g + load<u8>(arg<u32>(ctx, 0)) * 4056u + 4016;
    const u32 sh = (i << 1) & 0x1Eu, off = (i >> 2) & 0x3FFFFFFCu;
    ctx.gpr[4] = off;  // r4..r6 left
    ctx.gpr[5] = g;
    ctx.gpr[6] = sh;
    ret(ctx, ((load<u32>(r + off) >> sh) & 3u) == 1 ? 1u : 0u);
}
// u32 f(): q = (World->+0x1281D4)->+4: q ? ++q->+116->+48 : 0
DECOMP(004A2560, World_Counter_004A2560) {
    const u32 q = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x001281D4u) + 4);
    ctx.gpr[4] = 0x001281D4;  // r4 / r5 left
    if (q == 0) {
        ret(ctx, 0u);
        return;
    }
    const u32 p = load<u32>(q + 116);
    const u64 v = static_cast<u64>(load<u32>(p + 48)) + 1;
    store<u32>(p + 48, static_cast<u32>(v));
    ctx.gpr[4] = p;
    ctx.gpr[5] = v;
    ret(ctx, static_cast<u32>(v));
}
// f(Cursor* c, const char* name): XmlCursor_FirstChildRaw(c, String_HashNoCase(name))
DECOMP(004F8C30, XmlCursor_FirstChildNamed) {
    const u32 c = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {ctx.gpr[4]}));
    call(ctx, 0x004F8ACC, {c, h});
}
// Rec* f(Ring* r): the next of the 2048 128-byte records at r->+512, the
// cursor r->+532 advanced atomically (wrapping 2047 -> 0)
DECOMP(0056E048, Ring_Next_0056E048) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 c = r + 532;
    u32 old, nw;
    for (;;) {  // lwarx / stwcx. loop in the original
        old = mem::r32(c);
        nw = old == 2047 ? 0u : old + 1;
        if (mem::cas32(c, old, nw)) break;
    }
    ctx.gpr[4] = old << 7;  // r4..r7 left
    ctx.gpr[5] = c;
    ctx.gpr[6] = nw;
    ctx.gpr[7] = old;
    ret(ctx, load<u32>(r + 512) + (old << 7));
}
// f(Obj* o, u8 on): o->+172 bit 0x8 = on bit 0; if on, sub_000CFEC4(o, 0)
DECOMP(00737350, Flag172_8_00737350) {
    const u32 o = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    const u32 w = (load<u32>(o + 172) & ~0x8u) | ((b & 1u) << 3);
    store<u32>(o + 172, w);
    ctx.gpr[6] = w;  // r6 left
    if ((b & 0xFFu) != 0) call(ctx, 0x000CFEC4, {o, 0, ctx.gpr[5], w});
}
// void f(Rec* r, u32 a, ?, ?, const f32 c[2], f32 x, f32 y, f32 z): r = {+0 a,
// +4 x, +8 min(y, *(f32*)0x0074D038), +12 c[0], +16 c[1], +20 z}
DECOMP(0074D040, Rec_Set_0074D040) {
    const u32 r = arg<u32>(ctx, 0), c = arg<u32>(ctx, 4);
    store<f32>(r + 4, static_cast<f32>(farg(ctx, 0)));
    const f64 k = load<f32>(0x0074D038), y = farg(ctx, 1);
    store<u32>(r, arg<u32>(ctx, 1));
    store<f32>(r + 8, static_cast<f32>(y < k ? y : k));
    CopyW(r + 12, c, 0);
    const u32 c1 = load<u32>(c + 4);
    store<f32>(r + 20, static_cast<f32>(farg(ctx, 2)));
    store<u32>(r + 16, c1);
    ctx.gpr[5] = 0x0074D038;  // r5 / f1 left
    ctx.fpr[1] = F32Bits(c1);
}
// void Vec3_Cross(f32 out[3], const f32 a[3], const f32 b[3]) (fused)
DECOMP(00755DAC, Vec3_Cross_00755DAC) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 a1 = load<f32>(a + 4), b2 = load<f32>(b + 8), a2 = load<f32>(a + 8), b1 = load<f32>(b + 4);
    const f64 b0 = load<f32>(b), a0 = load<f32>(a);
    const f64 t7 = static_cast<f32>(a2 * b1), t8 = static_cast<f32>(a0 * b2), t9 = static_cast<f32>(a1 * b0);
    const f32 x = static_cast<f32>(std::fma(a1, b2, -t7));
    const f32 y = static_cast<f32>(std::fma(a2, b0, -t8));
    const f32 z = static_cast<f32>(std::fma(a0, b1, -t9));
    store<f32>(out, x);
    store<f32>(out + 4, y);
    store<f32>(out + 8, z);
    ctx.fpr[1] = x;  // f1..f9 left
    ctx.fpr[2] = y;
    ctx.fpr[3] = z;
    ctx.fpr[4] = b1;
    ctx.fpr[5] = b0;
    ctx.fpr[6] = a0;
    ctx.fpr[7] = t7;
    ctx.fpr[8] = t8;
    ctx.fpr[9] = t9;
}
// u32 f(const Obj* o, u32 i): *SortedMap_Find749A14(0x0105752C, &o->+128[i])
DECOMP(00760858, Map_FindValue_00760858) {
    const u32 k = arg<u32>(ctx, 0) + (arg<u32>(ctx, 1) << 2) + 128;
    const u32 p = static_cast<u32>(call(ctx, 0x00749A14, {0x0105752C, k, 0x01050000}));
    ret(ctx, load<u32>(p));
}
// The vtables of a 5-base object: +20 0x00B1F23C, +8 0x00B1F228, +0 0x00B13974, +16 0x00B1F26C, +12 0x00B1F250.
static void SetVt5B13974(PPUContext& ctx) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o + 20, 0x00B1F23C);
    store<u32>(o + 8, 0x00B1F228);
    store<u32>(o, 0x00B13974);
    store<u32>(o + 16, 0x00B1F26C);
    store<u32>(o + 12, 0x00B1F250);
    ctx.gpr[4] = 0x00B1F26C;  // r4..r8 left
    ctx.gpr[5] = 0x00B1F228;
    ctx.gpr[6] = 0x00B1F250;
    ctx.gpr[7] = 0x00B13974;
    ctx.gpr[8] = 0x00B20000;
}
// void f(Obj* o): the five vtables of 0x00B13974's derived class
DECOMP(0076BE3C, Obj_InitVt5_0076BE3C) { SetVt5B13974(ctx); }
// void f(Obj* o): the five vtables of 0x00B13974's derived class
DECOMP(0076EAEC, Obj_InitVt5_0076EAEC) { SetVt5B13974(ctx); }
// f(Obj* o, u32 a): o->vfn[72 / 4](o, a)
DECOMP(00795A34, Obj_VCall72_00795A34) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 72, {o, arg<u32>(ctx, 1)});
}
// f(Obj* o, u32 a): o->vfn[80 / 4](o, a)
DECOMP(00795A74, Obj_VCall80_00795A74) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 80, {o, arg<u32>(ctx, 1)});
}
// f(Obj* o, a): o->vfn[188 / 4](o, a, 0)
DECOMP(00795E7C, Obj_VCall188_00795E7C) {
    const u32 o = arg<u32>(ctx, 0);
    sly4::obj19::VCall(ctx, o, 188, {o, ctx.gpr[4], 0});
}
// bool f(const Obj* o): o's byte for the type 0x00F95410 is set and o->+304 bit 0 is clear
DECOMP(0003D000, Obj_TypeByte_F95410_0003D000) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = load<u32>(load<u32>(o + 308) + 12);
    const u32 b = load<u8>(t + (TypeIndex(0x00F95410) << 2));
    ctx.gpr[4] = b;  // r4 / r5 left
    ctx.gpr[5] = t;
    ret(ctx, b != 0 && (load<u8>(o + 304) & 1u) == 0 ? 1u : 0u);
}
// f(Obj* o): o->+0 = 0x00B0EF30 (vtable), *(u32*)0x00F557F8 = 0, Thunk_004B1308(o + 4)
DECOMP(00064FFC, Obj_DtorVtB0EF30_00064FFC) {
    const u64 b = ctx.gpr[3] + 4;
    store<u32>(arg<u32>(ctx, 0), 0x00B0EF30);
    store<u32>(0x00F557F8, 0);
    call(ctx, 0x004B1308, {static_cast<u32>(b), 0x00B0EF30, 0x00F50000, b, 0});
}
// f(const Obj* o): if ((World + 4208)->+8) Overlay_Clear2AA9C8(o->+12)
DECOMP(000877AC, Overlay_Clear12_000877AC) {
    const u32 v = load<u32>(load<u32>(0x00F65CA0) + 4208 + 8);
    ctx.gpr[4] = v;  // r4 left
    if (v != 0) call(ctx, 0x002AA9C8, {load<u32>(arg<u32>(ctx, 0) + 12), v});
}
// f(): PtrVector_Clear(0x00F9CDA8); Nop(0x00F9CDA8)
DECOMP(000ABB30, PtrVector_ClearF9CDA8) {
    call(ctx, 0x0054EC1C, {0x00F9CDA8});
    call(ctx, 0x0054EC18, {0x00F9CDA8});
}
// bool f(Obj* o, const s32* v): *v 1: o->+184 (u64) bit 0x80 cleared; 2: set; 1
DECOMP(00140B9C, Flag184_80_ByMode_00140B9C) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 v = load<s32>(arg<u32>(ctx, 1));
    ctx.gpr[4] = static_cast<u32>(v);  // r4 / r5 left
    if (v == 1) {
        const u64 d = load<u64>(o + 184) & ~0x80ull;
        store<u64>(o + 184, d);
        ctx.gpr[4] = 0;
        ctx.gpr[5] = d;
    } else if (v == 2) {
        const u64 d = load<u64>(o + 184) | 0x80ull;
        store<u64>(o + 184, d);
        ctx.gpr[4] = d;
    }
    ret(ctx, 1u);
}
// f(Obj* o, Obj* d): sub_000DFBFC(o, d); d->+200 = o->+236
DECOMP(0016CCD8, Call_DFBFC_Copy236_0016CCD8) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {o, d});
    const u32 v = load<u32>(o + 236);
    store<u32>(d + 200, v);
    ret(ctx, v);  // r3 left as the value
}
// f(const Obj* o, x): sub_00296840(CallK2_000D94F4(x), (s32) o->+4)
DECOMP(0016D850, Call_296840_0016D850) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 r = static_cast<u32>(call(ctx, 0x000D94F4, {ctx.gpr[4]}));
    call(ctx, 0x00296840, {r, static_cast<u64>(static_cast<s64>(load<s32>(o + 4)))});
}
// f(Obj* o, Obj* d): sub_000CDB28(o, d); d->+144 = o->+180
DECOMP(001A7D48, Call_CDB28_Copy180_001A7D48) {
    const u32 o = arg<u32>(ctx, 0), d = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {o, d});
    const u32 v = load<u32>(o + 180);
    store<u32>(d + 144, v);
    ret(ctx, v);  // r3 left as the value
}
// f(Obj* o): o->+0 = 0x00B1C030 (vtable); SortedMap_Remove75DE78(0x01057544, o + 36)
DECOMP(00274144, Obj_DtorVtB1C030_00274144) {
    const u64 k = ctx.gpr[3] + 36;
    store<u32>(arg<u32>(ctx, 0), 0x00B1C030);
    call(ctx, 0x0075DE78, {0x01057544, static_cast<u32>(k), 0x01057544, k});
}
// u32 f(Player* p): Player_Count2DAC24(p, the current slot record)
DECOMP(002DAE5C, Player_CountCur_002DAE5C) {
    const u64 p = ctx.gpr[3];
    const u32 r = CurSlotRec22(ctx);
    call(ctx, 0x002DAC24, {p, r, ctx.gpr[5]});
}
// u32 f(Player* p): sub_002DB040(p, the current slot record)
DECOMP(002DB5D4, Player_CallCur_002DB5D4) {
    const u64 p = ctx.gpr[3];
    const u32 r = CurSlotRec22(ctx);
    call(ctx, 0x002DB040, {p, r, ctx.gpr[5]});
}
// f(Obj* o): World_AddTimedCallback(World, 3, 0x00F36160, o, 0)
DECOMP(002F4998, World_AddCallback3_002F4998) {
    const u32 w = load<u32>(0x00F65CA0);
    call(ctx, 0x0046B5F8, {w, 3, 0x00F36160, ctx.gpr[3], 0, w});
}
// f(Obj* o, u32 h, u32 k): if (h == o->+8) sub_002FC600(o, k == 0x82485B95 ? 2 : 1, k)
DECOMP(002FCA20, Call_2FC600_002FCA20) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = load<u32>(o + 8);
    ctx.gpr[6] = v;  // r6 left
    if (arg<u32>(ctx, 1) != v) return;
    const u32 m = arg<u32>(ctx, 2) == 0x82485B95u ? 2u : 1u;
    call(ctx, 0x002FC600, {o, m, ctx.gpr[5], 0x82485B95u});
}
// f(Obj* o, Cursor* c): XmlCursor_ReadF32(c, 0x00274ACD, &o->+4, default (f32) 0x01069AF0->+72)
DECOMP(00302F6C, Xml_ReadF32_4_00302F6C) {
    const u64 p = ctx.gpr[3] + 4;
    callf(ctx, 0x004F9B20, {ctx.gpr[4], 0x00274ACD, static_cast<u32>(p), 0x01069AF0, 0x00270000}, {load<f32>(0x01069B38)});
}
// void f(Obj* o, const char* s): *(u32*)0x0106C270 = String_HashNoCase(s), *(Obj**)0x00F64CB4 = o
DECOMP(00369D14, Global_SetHashObj_00369D14) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = static_cast<u32>(call(ctx, o22fn::String_HashNoCase, {ctx.gpr[4]}));
    store<u32>(0x0106C270, h);
    store<u32>(0x00F64CB4, o);
    ctx.gpr[4] = 0x01070000;  // r4 / r5 left
    ctx.gpr[5] = 0x00F60000;
}
// void f(Obj* o): for (o->+160 >> 1) 2 / 3: o->+340 = o->+336 >= *(f32*)0x0036C0D8 ? o->+336 : o->+332
DECOMP(0036C0E0, Field340_Pick_0036C0E0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 k = load<u32>(o + 160) >> 1;
    ctx.gpr[4] = k;  // r4 / f1 / f2 left
    if (k != 2 && k != 3) return;
    u32 w = load<u32>(o + 336);
    const f64 c = load<f32>(0x0036C0D8);
    ctx.gpr[4] = 0x0036C0D8;
    ctx.fpr[2] = c;
    if (!(F32Bits(w) >= c)) w = load<u32>(o + 332);
    store<u32>(o + 340, w);
    ctx.fpr[1] = F32Bits(w);
}
// f(Obj* o, u8 on): on ? (o->+216 bit 0x40000000 clear: Obj_StartMode368360(o)) : Obj_ShopModeOff3684B8(o)
DECOMP(00376954, Obj_Mode_00376954) {
    const u32 o = arg<u32>(ctx, 0);
    if ((arg<u32>(ctx, 1) & 0xFFu) == 0) {
        call(ctx, 0x003684B8, {o, 0});
        return;
    }
    const u32 b = (load<u32>(o + 216) >> 30) & 1u;
    ctx.gpr[4] = b;  // r4 left
    if (b == 0) call(ctx, 0x00368360, {o, 0});
}
// bool f(Obj* o, const s32* v): if (*v == 0) the +120 / +124 words and the
// +152 / +153 / +157 / +158 bytes are cleared and +161 = 1; 1
DECOMP(003AAF38, Obj_ResetIf0_003AAF38) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 v = load<s32>(arg<u32>(ctx, 1));
    ctx.gpr[4] = static_cast<u32>(v);  // r4 / r5 left
    if (v == 0) {
        store<u8>(o + 153, 0);
        store<u8>(o + 152, 0);
        store<u8>(o + 157, 0);
        store<u32>(o + 120, 0);
        store<u32>(o + 124, 0);
        store<u8>(o + 158, 0);
        store<u8>(o + 161, 1);
        ctx.gpr[4] = 0;
        ctx.gpr[5] = 1;
    }
    ret(ctx, 1u);
}
// f32 f(const Obj* o): p = o->+104: p && (p->+192 bit 0x2) ? p->+348 / p->+344 : *(f32*)0x003ABE80
DECOMP(003ABE88, Field104_Ratio_003ABE88) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 104);
    if (p != 0) {
        const u32 b = load<u32>(p + 192) & 2u;
        ctx.gpr[4] = b;  // r3 / r4 / f2 left
        if (b != 0) {
            ctx.gpr[3] = p;
            ctx.fpr[2] = load<f32>(p + 344);
            ret(ctx, static_cast<f32>(static_cast<f64>(load<f32>(p + 348)) / ctx.fpr[2]));
            return;
        }
    }
    ctx.gpr[3] = 0x003ABE80;
    ret(ctx, load<f32>(0x003ABE80));
}
// f(): if (*(u8*)0x010B9F38) sub_00466208(World); sub_00429C04()
DECOMP(00466574, Call_466208_429C04) {
    if (load<u8>(0x010B9EF0 + 72) != 0) call(ctx, 0x00466208, {load<u32>(0x00F65CA0)});
    call(ctx, 0x00429C04);
}
// void f(Timer* t, u8 k, f32 d): t->+0 == k ? t->+4 = 0 : (t->+4 += d; t->+4 >= t->+8: t->+0 = k)
DECOMP(004A08C8, Timer_Step_004A08C8) {
    const u32 t = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1) & 0xFFu;
    const u32 s = load<u8>(t);
    ctx.gpr[4] = k;  // r4 / r5 / f1 / f2 left
    ctx.gpr[5] = s;
    if (s == k) {
        store<u32>(t + 4, 0);
        ctx.gpr[4] = 0;
        return;
    }
    const f32 v = static_cast<f32>(static_cast<f64>(load<f32>(t + 4)) + farg(ctx, 0));
    const f64 e = load<f32>(t + 8);
    store<f32>(t + 4, v);
    ctx.fpr[1] = v;
    ctx.fpr[2] = e;
    if (static_cast<f64>(v) >= e) store<u8>(t, static_cast<u8>(k));
}
// void f(Obj* o): o->+0..+8 and +16..+24 = the 3 floats at 0x01164B40, o->+32 = 0
DECOMP(004A109C, Obj_ResetPos_004A109C) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 s = 0x01164B40;
    for (u32 k = 0; k < 3; ++k) store<u32>(o + 4 * k, load<u32>(s + 4 * k));
    store<u32>(o + 16, load<u32>(s));
    store<u32>(o + 20, load<u32>(s + 4));
    const u32 w = load<u32>(s + 8);
    store<u32>(o + 32, 0);
    store<u32>(o + 24, w);
    ctx.gpr[4] = 0x01160000;  // r4..r6 / f1 left
    ctx.gpr[5] = s;
    ctx.gpr[6] = 0;
    ctx.fpr[1] = F32Bits(w);
}
// f32 f(f32 t): the curve with the constants at 0x004A5788 {c0, c1, c2}: t < c0
// ? c1 * t * t : c2 - c1 * (c2 - t)^2 (fused)
DECOMP(004A5798, Curve_004A5798) {
    constexpr u32 k = 0x004A5788;
    const f64 t = farg(ctx, 0);
    const f64 c0 = load<f32>(k), c1 = load<f32>(k + 4);
    ctx.gpr[3] = k;  // r3 / f2 / f3 left
    if (t < c0) {
        const f64 a = static_cast<f32>(t * c1);
        ctx.fpr[2] = a;
        ctx.fpr[3] = c0;
        ret(ctx, static_cast<f32>(a * t));
        return;
    }
    const f64 c2 = load<f32>(k + 8);
    const f64 u = static_cast<f32>(c2 - t);
    const f64 a = static_cast<f32>(u * c1);
    ctx.fpr[2] = a;
    ctx.fpr[3] = c2;
    ret(ctx, static_cast<f32>(-std::fma(a, u, -c2)));
}
// u32 f(): the cached Save_Profile0C9FC8(key) in the word at `slot`
static void CachedProfile(PPUContext& ctx, u32 slot, u32 key) {
    u32 v = load<u32>(slot);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, 0x000C9FC8, {key}));
        store<u32>(slot, v);
        ctx.gpr[4] = 0x00F60000;  // r4 left
    }
    ret(ctx, v);
}
// u32 f(): *(u32*)0x00F65F78, set from Save_Profile0C9FC8(0x42361175) when 0
DECOMP(004AE3D0, Profile_Cached_F65F78) { CachedProfile(ctx, 0x00F65F78, 0x42361175u); }
// u32 f(): *(u32*)0x00F65F7C, set from Save_Profile0C9FC8(0x02D45643) when 0
DECOMP(004AE414, Profile_Cached_F65F7C) { CachedProfile(ctx, 0x00F65F7C, 0x02D45643u); }
// f(Obj* o): sub_004D3378(o, name, k); sub_004B2DA0(o)
static void Call4D3378Then4B2DA0(PPUContext& ctx, u32 name, u32 k) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004D3378, {o, name, k});
    call(ctx, 0x004B2DA0, {o});
}
// f(Obj* o): sub_004D3378(o, 0x00AEE340, 0); sub_004B2DA0(o)
DECOMP(004B40CC, Call_4D3378_0_004B40CC) { Call4D3378Then4B2DA0(ctx, 0x00AEE340, 0); }
// f(Obj* o): sub_004D3378(o, 0x00AEE348, 1); sub_004B2DA0(o)
DECOMP(004B499C, Call_4D3378_1_004B499C) { Call4D3378Then4B2DA0(ctx, 0x00AEE348, 1); }

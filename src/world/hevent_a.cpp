// World_HandleEvent callees, 13..24 instructions: accessors, bit-field
// helpers, callback thunks. Most callers are still recompiled, so each
// function leaves the volatile registers the way the original does where that
// is cheap (noted per function).
#include "hevent.h"

using namespace sly4;
using namespace sly4::hevent;

// void Globals_F65F74_Reset(): clears the words 0x00F65F74..0x00F65F80 and the
// count of the vector at 0x010C2480 (name?). Leaves r3/r6/r7 = 0x00F60000,
// r4 = the vector, r5 = 0.
DECOMP(004AE458, Globals_F65F74_Reset) {
    store<u32>(kReset_F65F74, 0);
    store<u16>(kList10C2480 + 8, 0);
    store<u32>(kReset_F65F74 + 4, 0);
    store<u32>(kReset_F65F74 + 8, 0);
    store<u32>(kReset_F65F74 + 12, 0);
    ctx.gpr[3] = ctx.gpr[6] = ctx.gpr[7] = 0x00F60000u;
    ctx.gpr[4] = kList10C2480;
    ctx.gpr[5] = 0;
}

// void Mask_Set(u32* p, u32 n, u8 on): *p |= 1 << n when on == 1, else &= ~(1 << n).
// Leaves r4 = the new word, r5 = the old word, r6 = 1.
DECOMP(0077B13C, Mask_Set) {
    const u32 p = arg<u32>(ctx, 0);
    const u32 n = arg<u32>(ctx, 1) & 63;
    const u32 m = n < 32 ? (1u << n) : 0u;  // slw
    const u32 old = load<u32>(p);
    const u32 v = (arg<u32>(ctx, 2) & 0xFF) == 1 ? (old | m) : (old & ~m);
    store<u32>(p, v);
    ctx.gpr[4] = v;
    ctx.gpr[5] = old;
    ctx.gpr[6] = 1;
}

// bool Obj_State160Is2to4(Object* o): (o->+160 >> 1) is 2, 3 or 4. Leaves r4 = the state.
DECOMP(007875D0, Obj_State160Is2to4) {
    const u32 s = load<u32>(arg<u32>(ctx, 0) + 160) >> 1;
    ctx.gpr[4] = s;
    ret(ctx, (s == 2 || s == 3 || s == 4) ? 1u : 0u);
}

// void Emitter_StopGlobal(u8 fade): Emitter_Stop(*0x010AFEA4, fade). The
// handle is passed by value through a stack slot of the original's frame,
// which nothing else reads.
DECOMP(00449E10, Emitter_StopGlobal) {
    const u32 fade = arg<u32>(ctx, 0) & 0xFF;
    call(ctx, hfn::Emitter_Stop, {load<u32>(kEmitterHandle10AFEA4), fade});
}

// bool Obj_Byte656On(Object* o, s32 i): byte o[656 + i] has 0x80 set and 0x40 clear.
// Leaves r4 = the result, r5 = the 0x80 bit.
DECOMP(0074BB98, Obj_Byte656On) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 i = static_cast<s32>(arg<u32>(ctx, 1));
    const u32 b = ref<u8>(o + 656 + static_cast<u32>(i));
    const u32 hi = (b >> 7) & 1;
    const u32 r = (hi != 0 && ((b >> 6) & 1) == 0) ? 1u : 0u;
    ctx.gpr[5] = hi;
    ctx.gpr[4] = r;
    ret(ctx, r);
}

// bool Obj_Ptr24Bit164_15(Object* o): o->p24 && (o->p24->+164 & 0x8000). Leaves r4 = the result.
DECOMP(0074D8C0, Obj_Ptr24Bit164_15) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 24);
    const u32 r = p != 0 ? (load<u32>(p + 164) >> 15) & 1 : 0u;
    ctx.gpr[4] = r;
    ret(ctx, r);
}

// u32 Global_2DDD24_Get744(): sub_002DDD24() ? its +744 : 0. Leaves r4 = the object.
DECOMP(00787534, Global_2DDD24_Get744) {
    const u32 p = static_cast<u32>(call(ctx, hfn::sub_002DDD24));
    ctx.gpr[4] = p;
    ret(ctx, p != 0 ? load<u32>(p + 744) : 0u);
}

// void PtrVector_ClearOrEmpty(PtrVector* v, u8 free): PtrVector_Clear(v) when
// free, else count (+8) = 0 with r4 = 0.
DECOMP(00787D74, PtrVector_ClearOrEmpty) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 f = arg<u32>(ctx, 1) & 0xFF;
    if (f != 0) {
        call(ctx, hfn::PtrVector_Clear, {v, f});
        return;
    }
    ctx.gpr[4] = 0;
    store<u16>(v + 8, 0);
}

// s32 Obj_ScaledValue4168(Object* o): (s32)(o->f4168 * tuning 0x010AECFC * 32768).
// Leaves r4/r5 = the constant / tuning addresses, f2/f3 = their values, f1 = the fctiwz bits.
DECOMP(0048CEA0, Obj_ScaledValue4168) {
    const u32 o = arg<u32>(ctx, 0);
    const f32 k = load<f32>(kScaleConst48CE98);
    const f32 t = load<f32>(kTune10AECFC + 72);
    const f32 v = static_cast<f32>(static_cast<f64>(static_cast<f32>(
                      static_cast<f64>(load<f32>(o + 4168)) * t)) * k);
    const u64 bits = rt::fctiw(v, true);
    ctx.gpr[4] = kScaleConst48CE98;
    ctx.gpr[5] = kTune10AECFC;
    ctx.fpr[2] = k;
    ctx.fpr[3] = t;
    ctx.set_fpr_bits(1, bits);
    ret(ctx, static_cast<s32>(static_cast<u32>(bits)));
}

// void Obj_Forward20(Object* o, u32 v): sub_004CF988(o->p20, v) when o->p20 is set.
// Without one, r3 = v, r4 = 0, r5 = o (the original's moves).
DECOMP(0036A4C0, Obj_Forward20) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const u32 p = load<u32>(o + 20);
    if (p == 0) {
        ctx.gpr[5] = o;
        ctx.gpr[4] = 0;
        ctx.gpr[3] = v;
        return;
    }
    call(ctx, hfn::sub_004CF988, {p, v, v});  // r5 = v as in the original
}

// void World_SaveSlotCall(World* w, s32 user): sub_000AE454(&w->save + user * 4056, (u8*)w + 4137 + user * 2).
DECOMP(00787964, World_SaveSlotCall) {
    const u32 w = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1);
    const u32 a = w + kSaveData + i * kSaveSlotSize;
    const u32 b = w + ((i << 1) & 0xFFFFFFFEu) + 4137;
    call(ctx, hfn::sub_000AE454, {a, b, i * kSaveSlotSize});
}

// void Obj_UpdateFlags2336(Object* o, u8 reset): byte +2336: reset clears 0x80;
// then if 0x80 is clear and 0x20 set, 0x10 is cleared and 0x08 set.
// Leaves r4 = the byte, r5 = its 0x80 bit.
DECOMP(0029D838, Obj_UpdateFlags2336) {
    const u32 o = arg<u32>(ctx, 0);
    u32 b = ref<u8>(o + 2336);
    if ((arg<u32>(ctx, 1) & 0xFF) != 0) {
        b &= ~0x80u;
        ref<u8>(o + 2336) = static_cast<u8>(b);
    }
    const u32 hi = (b >> 7) & 1;
    if (hi == 0 && ((b >> 5) & 1) != 0) {
        b = (b & ~0x10u) | 0x08u;
        ref<u8>(o + 2336) = static_cast<u8>(b);
    }
    ctx.gpr[4] = b;
    ctx.gpr[5] = hi;
}

// void HandleMap_ClearField20(): every object in the handle map gets +20 = 0.
// Leaves r3 = the end of the entries, r4 = the count, r5 = 0, r6 = the index, r7 = the last object.
DECOMP(0035C028, HandleMap_ClearField20) {
    const u32 n = load<u16>(kHandleMap + 2);
    u32 e = load<u32>(kHandleMap + 8);
    u32 i = 0;
    while (static_cast<s32>(i) < static_cast<s32>(n)) {
        const u32 obj = load<u32>(e + 4);
        store<u32>(obj + 20, 0);
        ctx.gpr[7] = obj;
        ++i;
        e += 8;
    }
    ctx.gpr[3] = e;
    ctx.gpr[4] = n;
    ctx.gpr[5] = 0;
    ctx.gpr[6] = i;
}

// void HashedName_Set(u8 keep, const char* name): unless keep, *0x0106C278 =
// String_HashNoCase(name) and byte 0x0106C274 = 1 (name?). Leaves r3 = the
// hash, r4/r5 = 0x01070000, r6 = 1; when keep, r3 = name and r5 = keep.
DECOMP(0036960C, HashedName_Set) {
    const u32 keep = arg<u32>(ctx, 0) & 0xFF;
    const u32 name = arg<u32>(ctx, 1);
    ctx.gpr[5] = keep;
    if (keep != 0) {
        ctx.gpr[3] = name;
        return;
    }
    const u32 h = static_cast<u32>(call(ctx, hfn::String_HashNoCase, {name}));
    store<u32>(kHashedName, h);
    ref<u8>(kHashedNameSet) = 1;
    ctx.gpr[4] = ctx.gpr[5] = 0x01070000u;
    ctx.gpr[6] = 1;
    ret(ctx, h);
}

// u32 Player_Get340(): Player_Get() ? Player_Get()->+340 : 0 (the getter is called twice).
DECOMP(0046E760, Player_Get340) {
    u32 v = 0;
    if (call(ctx, hfn::Player_Get) != 0) v = load<u32>(static_cast<u32>(call(ctx, hfn::Player_Get)) + 340);
    ret(ctx, v);
}

// bool Obj_List2240Blocked(Object* o): 1 unless o->+2240 -> list -> u16 count (+8)
// is non-zero; then the 0x00800000 bit of +2244. Leaves r4 = o + 2240.
DECOMP(00325F7C, Obj_List2240Blocked) {
    const u32 p = arg<u32>(ctx, 0) + 2240;
    ctx.gpr[4] = p;
    const u32 a = load<u32>(p);
    if (a == 0) return ret(ctx, 1u);
    const u32 b = load<u32>(a);
    if (b == 0) return ret(ctx, 1u);
    if (load<u16>(b + 8) == 0) return ret(ctx, 1u);
    ret(ctx, (load<u32>(p + 4) >> 23) & 1);
}

// void Obj_Child28_Vfn32(Object* o): o->p28 && p28->vfunc+32(p28, 0). Leaves r3 = 0 without one.
DECOMP(004644E0, Obj_Child28_Vfn32) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 28);
    if (p == 0) return ret(ctx, 0u);
    vcall(ctx, p, 32, {p, 0});
}

// void Obj_Child28_Vfn36(Object* o, u8 b): o->p28 && p28->vfunc+36(p28, b).
DECOMP(0046452C, Obj_Child28_Vfn36) {
    const u32 p = load<u32>(arg<u32>(ctx, 0) + 28);
    const u32 b = arg<u32>(ctx, 1) & 0xFF;
    ctx.gpr[4] = b;
    if (p == 0) return ret(ctx, 0u);
    vcall(ctx, p, 36, {p, b});
}

// void Busy105CB80_Wait(): sleeps 300 us at a time while *0x0105CB80 != 0 (name?)
DECOMP(00286300, Busy105CB80_Wait) {
    u32 v = load<u32>(kBusyFlag105CB80);
    while (v != 0) {
        ctx.gpr[11] = kSysTimerUsleep;
        syscall(ctx, kSysTimerUsleep, {300});
        v = load<u32>(kBusyFlag105CB80);
    }
    ret(ctx, v);
}

// void Obj_RunCallback400(Object* o) / Obj_RunCallback404: the OPD at +400
// (+404) called with (+408 (+412), +416) when set. r5 = the OPD, r6 = the
// second argument, r7 = the entry, as in the original; r4 = 0 without one.
DECOMP(00787874, Obj_RunCallback400) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(o + 400);
    ctx.gpr[4] = opd;
    if (opd == 0) return;
    const u32 b = load<u32>(o + 416);
    call_ptr(ctx, opd, {load<u32>(o + 408), b, opd, b, load<u32>(opd)});
}
DECOMP(007878C4, Obj_RunCallback404) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 opd = load<u32>(o + 404);
    ctx.gpr[4] = opd;
    if (opd == 0) return;
    const u32 b = load<u32>(o + 416);
    call_ptr(ctx, opd, {load<u32>(o + 412), b, opd, b, load<u32>(opd)});
}

// void Obj_QueueIfState3(Object* o): when bits 20..27 of +244 are 3,
// World_AddTimedCallback(current world, 11, OPD 0x00F35510, o, 0). Leaves r4 = the state otherwise.
DECOMP(002ABE5C, Obj_QueueIfState3) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = (load<u32>(o + 244) >> 20) & 0xFF;
    ctx.gpr[4] = s;
    if (s != 3) return;
    call(ctx, hfn::World_AddTimedCallback, {load<u32>(kCurrentWorld), 11, kCallbackOpdF35510, o, 0});
}

// void World_UserAspectCall(?, u32 user): sub_003697B0((u8)user, block->f108 / block->f100)
// for the current world's user block (r3 is not read). r4 = the block, r5 = world + user * size.
DECOMP(0047BBEC, World_UserAspectCall) {
    const u32 i = arg<u32>(ctx, 1);
    const u32 base = load<u32>(kCurrentWorld) + i * kUserBlockSize;
    const u32 blk = base + kUserBlocks;
    const f32 b = load<f32>(blk + 100);
    const f32 q = static_cast<f32>(static_cast<f64>(load<f32>(blk + 108)) / static_cast<f64>(b));
    ctx.fpr[2] = b;
    callf(ctx, hfn::sub_003697B0, {i & 0xFF, blk, base}, {q});
}

// Material* Material_Clone(const Material* src): Material_Alloc(src), keeping src's +16, then
// Material_CopyFrom(new, src) (name?)
DECOMP(0050956C, Material_Clone) {
    const u32 src = arg<u32>(ctx, 0);
    const u32 keep = load<u32>(src + 16);
    const u32 m = static_cast<u32>(call(ctx, hfn::Material_Alloc, {src}));
    store<u32>(m + 16, keep);
    call(ctx, hfn::Material_CopyFrom, {m, src});
    ret(ctx, m);
}

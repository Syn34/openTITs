// Game code, part 31: the small-function sweep continued from the last 53
// instruction functions (size order; obj30*.cpp did 49-53). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj31.h"

using namespace sly4;
using namespace sly4::obj31;

// ---------------------------------------------------------------- sweep: 53 instructions
// void f(u8* out, T* o, a, b): *out = the u8 0x00885750 writes for (o, a,
// b) when the predicate OPD o->+256 is null or fp(a, b) is true, else 0
// (r3 = *out)
DECOMP(0034284C, Obj_EvalPredicate_0034284C) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    Stack frame(ctx, 16);
    const u32 buf = frame.addr();
    const u32 fp = load<u32>(o + 256);
    u32 at = buf + 1;
    if (fp != 0) {
        if (static_cast<u32>(call_ptr(ctx, fp, {a, b, fp, load<u32>(fp)})) == 0) {
            store<u8>(out, 0);
            ctx.gpr[3] = 0;
            return;
        }
        at = buf;
    }
    call(ctx, 0x00885750, {at, o, a, b});
    const u8 v = load<u8>(at);
    store<u8>(out, v);
    ctx.gpr[3] = v;
}

// bool f(Reader* r): the 65-byte table 0x0106C010 (zeroed on first use,
// flag u8 0x0106C008) gets a u8 count (into a scratch byte) and the 64
// bytes 0x0106C011..; both 0x00F64CA4 and 0x00F64CA8 point at them. r3 = 1.
DECOMP(00346EA8, Iff_ReadTable64_00346EA8) {
    const u32 r = arg<u32>(ctx, 0);
    Stack frame(ctx, 16);
    if (load<u8>(0x0106C008) == 0) {
        call(ctx, 0x00726E54, {0x0106C010u, 0, 65});
        store<u8>(0x0106C008, 1);
    }
    call(ctx, 0x005643C4, {r, frame.addr(), 0});
    for (u32 k = 0; k < 64; ++k) call(ctx, 0x0056998C, {r, 0x0106C011u + k, 0});
    store<u32>(0x00F64CA4, 0x0106C011);
    store<u32>(0x00F64CA8, 0x0106C011);
    ctx.gpr[3] = 1;
}

// void f(T* o, s32 id, a, b): the first of the u16 +8 entries (224 bytes,
// array o->+96->+0) whose s32 +0 is id gets the Vec3 +208 =
// Listener_LocalDirection(a, b). r3 = the entry, or b when none.
DECOMP(00352E08, Listener_SetDirById_00352E08) {
    const u32 o = arg<u32>(ctx, 0), id = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 list = load<u32>(o + 96);
    ctx.gpr[3] = b;
    for (u32 i = 0;; ++i) {
        const u32 n = list != 0 ? load<u16>(list + 8) : 0u;
        if (i >= n) return;
        if (load<u32>(load<u32>(list) + i * 224) != id) continue;
        Stack frame(ctx, 16);
        const u32 v = frame.addr();
        call(ctx, 0x005532E4, {v, a, b});
        const u32 e = load<u32>(load<u32>(o + 96)) + i * 224;
        CopyW31(e + 208, v);
        CopyW31(e + 212, v + 4);
        ctx.fpr[1] = LfsAt(v + 8);
        CopyW31(e + 216, v + 8);
        ctx.gpr[3] = e;
        return;
    }
}

// void f(a, T* n, u8 on): 0x004A6CF8(n->+160, on) and, for on, the same
// for each child (240-byte entries of n->+340, u16 count +8; child +192),
// recursively. r3 = a for a null n, else as left.
DECOMP(00370A0C, Node_SetOnRec_00370A0C) {
    const u32 a = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1), on = arg<u32>(ctx, 2) & 0xFFu;
    if (n == 0) return;
    ctx.gpr[3] = call(ctx, 0x004A6CF8, {load<u32>(n + 160), on});
    if (on == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 l = load<u32>(n + 340);
        ctx.gpr[3] = l;
        const u32 cnt = l != 0 ? load<u16>(l + 8) : 0u;
        if (i >= cnt) return;
        call(ctx, 0x00370A0C, {a, load<u32>(load<u32>(l) + i * 240 + 192), on});
    }
}

// void f(T* o, u32 h): Effect_Release(o->+340, h) when set; then, for h,
// each non-null o->+20[k] (u16 count +28) whose vt[272]() is set gets
// Effect_SetHash4CF988(that, h)
DECOMP(0039FF34, Obj_SetEffectHash_0039FF34) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    const u32 e0 = load<u32>(o + 340);
    ctx.gpr[3] = e0;
    if (e0 != 0) ctx.gpr[3] = call(ctx, 0x004CF3C4, {e0, h});
    if (h == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 28);
        ctx.gpr[3] = n;
        if (i >= n) return;
        const u32 e = load<u32>(load<u32>(o + 20) + 4 * i);
        if (e == 0) continue;
        const u32 r = static_cast<u32>(VCall(ctx, e, 272, {e}));
        if (r != 0) call(ctx, 0x004CF988, {r, h});
    }
}

// void f(T* o): o->+12 = Obj_FindByHandle(o->+16, 0); when found with bit
// 8 of +192, its +380 = c = its +392 and, when set, (c + 80)->vt[28](c +
// 80, 0, 4), c->+100->+0 = o->+20 and its u8 +12 = !(o->+8 bit 31); then
// Flag_Set8(o)
DECOMP(0042D92C, Cmd_BindTarget_0042D92C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 t = static_cast<u32>(call(ctx, o31fn::Obj_FindByHandle, {load<u32>(o + 16), 0}));
    store<u32>(o + 12, t);
    if (t != 0 && (load<u32>(t + 192) & 8u) != 0) {
        const u32 c = load<u32>(t + 392);
        store<u32>(t + 380, c);
        if (c != 0) {
            VCall(ctx, c + 80, 28, {c + 80, 0, 4});
            const u32 p = load<u32>(c + 100);
            store<u32>(p, load<u32>(o + 20));
            store<u8>(p + 12, (load<u32>(o + 8) >> 31) == 0 ? 1 : 0);
        }
    }
    ctx.gpr[3] = call(ctx, o31fn::Flag_Set8, {o});
}

// T::T(T* o, const u8 a[4], const u8 b[4], ?, bool bit, u8 c; f32 v): the
// base 0x0042C9D0(o, c), vtable 0x00B210E8, bytes +9..+12 = a and +13..+16
// = b, f32 +20 = v, bit 31 of +24 = bit (r3 = the new +24)
DECOMP(004340D8, Cmd_Ctor_004340D8) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const u32 bit = arg<u32>(ctx, 4), c = arg<u32>(ctx, 5) & 0xFFu;
    const f64 v = farg(ctx, 0);
    call(ctx, 0x0042C9D0, {o, c});
    store<u32>(o, 0x00B210E8);
    CopyBytes30(o + 9, a, 4);
    CopyBytes30(o + 13, b, 4);
    const u32 w = load<u32>(o + 24);
    store<f32>(o + 20, static_cast<f32>(v));
    const u32 nw = (w & 0x7FFFFFFFu) | (bit << 31);
    store<u32>(o + 24, nw);
    ctx.gpr[3] = nw;
}

// void f(T* o): for the object found by handle o->+12, Mesh_SetParam2(c,
// o->+16) with c its kind-16 component when that component's +12 is 16
// (else null); then Flag_Set8(o)
DECOMP(00438900, Cmd_SetMeshParam_00438900) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 h = load<u32>(o + 12);
    if (h != 0) {
        const u32 t = static_cast<u32>(call(ctx, o31fn::Obj_FindByHandle, {h, 0}));
        if (t != 0) {
            u32 c = 0;
            if (static_cast<u32>(call(ctx, o31fn::Obj_FindComponent, {t, 16, 0})) != 0 &&
                load<u32>(static_cast<u32>(call(ctx, o31fn::Obj_FindComponent, {t, 16, 0})) + 12) == 16)
                c = static_cast<u32>(call(ctx, o31fn::Obj_FindComponent, {t, 16, 0}));
            call(ctx, 0x00287260, {c, load<u32>(o + 16)});
        }
    }
    ctx.gpr[3] = call(ctx, o31fn::Flag_Set8, {o});
}

// bool f(T* o): false when some entry e of the array 0x010AA694 (u16 count
// +8, Array10AA694_At) with a nonzero u16 +12 has its +0 among the u16
// o->+20 words of o->+12
DECOMP(0043ACB8, Cmd_NoneListed_0043ACB8) {
    const u32 o = arg<u32>(ctx, 0);
    u32 res = 1;
    for (u32 i = 0; i < load<u16>(0x010AA69C); ++i) {
        const u32 e = static_cast<u32>(call(ctx, 0x0042BCD4, {i}));
        const u32 n = load<u16>(o + 20);
        u32 p = load<u32>(o + 12);
        for (u32 j = 0; j < n; ++j, p += 4)
            if (load<u32>(e) == load<u32>(p) && load<u16>(e + 12) != 0) res = 0;
    }
    ctx.gpr[3] = res;
}

// s32 f(?, ?, s32 a, s32 b): the pending fios op p in the slot table
// 0x010B0BD0 (72-byte rows a, word b): 1 when none, 0 while p->+152 is
// clear, else freed (slot = 0) with 1 when fios_GetResult(p) equals its
// u64 +168, else -4
DECOMP(004504AC, Fios_PollSlot_004504AC) {
    const u32 a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    const u32 slot = 0x010B0BC8u + a * 72 + b * 4 + 8;
    const u32 p = load<u32>(slot);
    if (p == 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (load<u32>(p + 152) == 0) {  // read before an isync
        ctx.gpr[3] = 0;
        return;
    }
    const u64 want = load<u64>(p + 168);
    const u64 got = call(ctx, 0x005F8AF4, {p, p});
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C));
    call(ctx, 0x005FE784, {s, load<u32>(slot)});
    store<u32>(slot, 0);
    ctx.gpr[3] = got == want ? 1u : static_cast<u64>(static_cast<s64>(-4));
}

// static init: the tuning vars 0x010BE820 (name 0x00AEB470, +72 = 17) and
// 0x010BE870 (0x00AEB498, 42), vtable 0x00B0D968, and the 84 152-byte
// records from 0x010BE8BC cleared (words +0..+12, +144; bytes +148, +16,
// +80). r3 = the end of the records.
DECOMP(0049E78C, StaticInit_0049E78C) {
    call(ctx, 0x004FBFA8, {0x010BE820u, 0x00AEB470u});
    store<u32>(0x010BE820, 0x00B0D968);
    store<u32>(0x010BE820 + 72, 17);
    call(ctx, 0x004FBFA8, {0x010BE870u, 0x00AEB498u});
    store<u32>(0x010BE870, 0x00B0D968);
    store<u32>(0x010BE870 + 72, 42);
    u32 p = 0x010BE8BC;
    for (u32 k = 0; k < 84; ++k, p += 152) {
        store<u32>(p, 0);
        store<u32>(p + 4, 0);
        store<u32>(p + 8, 0);
        store<u32>(p + 12, 0);
        store<u32>(p + 144, 0);
        store<u8>(p + 148, 0);
        store<u8>(p + 16, 0);
        store<u8>(p + 80, 0);
    }
    ctx.gpr[3] = p;
}

// void f(T* o; f32): 0x004A0990(o; f1 as passed), then the Vec3 +64 =
// lerp(+32, +48, t) with t = f32 +12 (the k - t form, k = *0x004A0FC4),
// copied to *o->+80 when set (r3 = o->+80; f1 = the last value moved)
DECOMP(004A0FC8, Obj_LerpPos_004A0FC8) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x004A0990, {o}, {farg(ctx, 0)});  // reads the f1 the caller passed
    const f64 k = LfsAt(0x004A0FC4), t = LfsAt(o + 12);
    const f64 s = Sub28(k, t);
    const f64 a = Mul28(t, LfsAt(o + 48)), b = Mul28(t, LfsAt(o + 52)), c = Mul28(t, LfsAt(o + 56));
    const f32 x = Madd28(s, LfsAt(o + 32), a);
    const f32 y = Madd28(s, LfsAt(o + 36), b);
    const f32 z = Madd28(s, LfsAt(o + 40), c);
    store<f32>(o + 64, x);
    store<f32>(o + 68, y);
    store<f32>(o + 72, z);
    const u32 p = load<u32>(o + 80);
    ctx.gpr[3] = p;
    ctx.fpr[1] = x;
    if (p == 0) return;
    store<f32>(p, x);
    CopyW31(p + 4, o + 68);
    ctx.fpr[1] = LfsAt(o + 72);
    CopyW31(p + 8, o + 72);
}

// T::~T(T* o): nothing for null; vtable 0x00B21BC0, bit 0 of the high
// word of c = o->+28's u64 +184 cleared and c->vt[40](c, 0),
// AnimSkel_Release7D4EA8(o->+32), the member vtables +84 / +48 =
// 0x00B21B58, 0x009000A0(o), then the pool (r13 - 28332)->+44 takes it
// back: pool->vt[12](pool, o, u16 o->+4)
DECOMP(004A9524, Dtor_B21BC0_004A9524) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B21BC0);
    const u32 c = load<u32>(o + 28);
    store<u64>(c + 184, load<u64>(c + 184) & ~(1ull << 32));
    VCall(ctx, c, 40, {c, 0});
    call(ctx, 0x007D4EA8, {load<u32>(o + 32)});
    store<u32>(o + 84, 0x00B21B58);
    store<u32>(o + 48, 0x00B21B58);
    call(ctx, 0x009000A0, {o});
    const u32 pool = load<u32>(Sda(ctx, -28332) + 44);
    ctx.gpr[3] = VCall(ctx, pool, 12, {pool, o, load<u16>(o + 4)});
}

// bool f(T* o): finds key = (u8 +24 bit 0x80 ? o->+20 : 0) in the binary
// tree 0x010C2820->+12->+0 (node key +4, value +8, +12 taken when the
// node's key is greater (unsigned), else +16), recording the path in the
// s16-counted stack 0x010C2830[] (count u16 0x010C2880). Found: 0x010C2884
// = the node, 0x011DE4A0 = its value, r3 = 1. Else 0x010C2884 = the last
// node visited (null, with 0x011DE4A0 = 0, for an empty tree), r3 = 0.
DECOMP(004B6DA0, Tree_FindPath_004B6DA0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 G = 0x010C2820;
    const u32 key = (load<u8>(o + 24) & 0x80u) != 0 ? load<u32>(o + 20) : 0u;
    u32 node = load<u32>(load<u32>(G + 12));
    ctx.gpr[3] = 0;
    if (node == 0) {
        store<u32>(0x011DE4A0, 0);
        store<u32>(G + 100, 0);
        return;
    }
    store<u16>(G + 96, 0);
    u32 cur;
    for (;;) {
        cur = node;
        const u32 k = load<u32>(cur + 4);
        if (k == key) {
            store<u32>(G + 100, cur);
            store<u32>(0x011DE4A0, load<u32>(cur + 8));
            ctx.gpr[3] = 1;
            return;
        }
        node = k > key ? load<u32>(cur + 12) : load<u32>(cur + 16);
        if (node == 0) break;
        const s32 n = static_cast<s16>(load<u16>(G + 96));
        store<u16>(G + 96, static_cast<u16>(n + 1));
        store<u32>(G + 16 + 4 * static_cast<u32>(n), cur);
    }
    if (cur == 0) store<u16>(G + 96, 0);  // dead: cur is a visited node
    store<u32>(G + 100, cur);
}

// void f(Vec* v, const T* e): appends e's 48-byte record (words +0..+8,
// +16..+32 copied; growing to twice the u16 capacity +4 when full); u16
// count +8, high-water +10 (r3 = the old +10, f1 = e->+28)
DECOMP(00735530, Vec48_Push_00735530) {
    const u32 v = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1);
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, o31fn::PtrVector_Reserve, {v, (cap << 1) & 0x1FFFEu});
    const u32 d = load<u32>(v) + load<u16>(v + 8) * 48;
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u}) CopyW31(d + off, e + off);
    ctx.fpr[1] = LfsAt(e + 28);
    CopyW31(d + 28, e + 28);
    CopyW31(d + 32, e + 32);
    const u32 n = load<u16>(v + 8);
    BumpCount31(ctx, v, n, load<u16>(v + 10));
}

// bool f(T* o, u32 x): pushes x on the word vector o->+252 (created on
// first use: SmallAlloc(20), PtrVector_Construct, u16 +6 = 4; a failed
// allocation still carries on with a null vector, as the original does).
// r3 = 1.
DECOMP(007373D4, Obj_PushWord252_007373D4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 v = load<u32>(o + 252);
    if (v == 0) {
        v = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {20}));
        store<u32>(o + 252, v);
        if (v != 0) {
            call(ctx, o31fn::PtrVector_Construct, {v});
            store<u16>(v + 6, 4);
            v = load<u32>(o + 252);
        }
    }
    u32 n = load<u16>(v + 8);
    const u32 cap = load<u16>(v + 4);
    if (n == cap) {
        call(ctx, o31fn::PtrVector_Reserve, {v, (cap << 1) & 0xFFFFFFFEu});
        n = load<u16>(v + 8);
    }
    const u32 base = load<u32>(v), n2 = load<u16>(v + 8), hi = load<u16>(v + 10);
    store<u32>(base + (n << 2), x);
    BumpCount31(ctx, v, n2, hi);
    ctx.gpr[3] = 1;
}

// void f(T* o): the values (+72) of the 76-byte tuning vars from
// 0x01038480: f32 #0..#8 -> +4..+36, u8 #9 -> +40, f32 #11 -> +44, u8 #10
// -> +41, f32 #12 -> +48 (f1 = #12)
DECOMP(00738040, Tuning_Load_00738040) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 B = 0x01038480 + 72;
    for (u32 i = 0; i < 9; ++i) CopyW31(o + 4 + 4 * i, B + 76 * i);
    store<u8>(o + 40, load<u8>(B + 76 * 9));
    CopyW31(o + 44, B + 76 * 11);
    store<u8>(o + 41, load<u8>(B + 76 * 10));
    ctx.fpr[1] = LfsAt(B + 76 * 12);
    CopyW31(o + 48, B + 76 * 12);
}

// void f(a, b, const Mat* m): the component c = Effect_FindComponent(a, b)
// of kind 4 (c->vt[32]()) gets the 3x4 rows of m (12 words at +32..+88,
// skipping each row's w). r3 = c, or the kind when not 4.
DECOMP(00755E4C, Effect_SetMatrix_00755E4C) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {a, b}));
    ctx.gpr[3] = c;
    if (c == 0) return;
    ctx.gpr[3] = VCall(ctx, c, 32, {c});
    if (static_cast<u32>(ctx.gpr[3]) != 4) return;
    for (u32 off : {0u, 4u, 8u, 16u, 20u, 24u, 32u, 36u, 40u, 48u, 52u}) CopyW31(c + 32 + off, m + off);
    ctx.fpr[1] = LfsAt(m + 56);
    CopyW31(c + 88, m + 56);
}

// void f(T* o, x): t = o->+96 gets t->vt[84](t, M) for the matrix M =
// Matrix_FromRows(rows, x) of the three Vec3 rows t->+96 / +112 / +128
// (copied to the stack), then WorldObj_MarkDirty(o->+96->+160)
DECOMP(0077ADF8, Obj_ApplyRows_0077ADF8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 t = load<u32>(o + 96);
    Stack frame(ctx, 112);
    const u32 rows = frame.addr(), m = rows + 48;
    for (u32 r = 0; r < 3; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW31(rows + 16 * r + 4 * k, t + 96 + 16 * r + 4 * k);
    ctx.fpr[1] = LfsAt(t + 136);  // the last value moved, as the original leaves f1
    call(ctx, 0x00553B4C, {m, rows, x});
    VCall(ctx, t, 84, {t, m});
    ctx.gpr[3] = call(ctx, 0x0057D000, {load<u32>(load<u32>(o + 96) + 160)});
}

// T::~T(T* o): vtables 0x00B20AF0 / +108 0x00B20AE0; the u16 +176 of
// o->+112 gets bit 0x1000 when its +172 has bit 0x00040000; +112 = 0, the
// vectors +268 and +248 cleared, base vtable 0x00B207A8 and
// Dtor_B219D0_00490B34(o)
DECOMP(0077D484, Dtor_B20AF0_0077D484) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20AF0);
    const u32 c = load<u32>(o + 112);
    store<u32>(o + 108, 0x00B20AE0);
    if (c != 0 && (load<u32>(c + 172) & 0x00040000u) != 0) store<u16>(c + 176, static_cast<u16>(load<u16>(c + 176) | 0x1000u));
    store<u32>(o + 112, 0);
    for (u32 off : {268u, 248u}) {
        call(ctx, o31fn::PtrVector_Clear, {o + off});
        call(ctx, o31fn::Nop_54EC18, {o + off});
    }
    store<u32>(o, 0x00B207A8);
    ctx.gpr[3] = call(ctx, 0x00490B34, {o});
}

// T::~T() of the 0x00F18D98 class (two identical copies)
DECOMP(00786F88, Dtor_F18D98_00786F88) { DtorF18D98_31(ctx); }

DECOMP(007876C8, Dtor_F18D98_007876C8) { DtorF18D98_31(ctx); }

// ---------------------------------------------------------------- sweep: 54 instructions
// void f(T* o, x): the object t = (e = Effect_Bind4BB7E0(x, o->+12, 0))'s
// +280 when e's +276 is 0x7A30D7E3, else Obj_FindById(o->+12); when t has
// bit 8 of +200: 0x001F4460(t) for u8 o->+16, Obj_FlushEntries(t) for u8
// o->+17
DECOMP(00085750, Obj_FlushBound_00085750) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(o + 12), 0}));
    u32 t = 0;
    if (e != 0 && load<u32>(e + 276) == 0x7A30D7E3u) t = load<u32>(e + 280);
    ctx.gpr[3] = 0;
    if (t == 0) {
        t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(o + 12)}));
        ctx.gpr[3] = 0;
        if (t == 0) return;
    }
    if ((load<u32>(t + 200) & 8u) == 0) return;
    if (load<u8>(o + 16) != 0) call(ctx, 0x001F4460, {t});
    ctx.gpr[3] = load<u8>(o + 17);
    if (load<u8>(o + 17) != 0) ctx.gpr[3] = call(ctx, 0x001ED9D4, {t});
}

// void f(T* o): when u8 +21 or +23 is set, the index i of hash o->+16 in
// level o->+12's table (Table_IndexOf; Hash_ToString(o->+16) when absent)
// gets Level_SetBit2(lvl, i, u8 +20) for +21, else Level_SetBit1(lvl, i,
// u8 +22) for +23; lvl = *0x00F65CA0 + o->+12 * 0x20C80 + 4208
DECOMP(0008DC70, Level_ApplyBits_0008DC70) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 n = load<u32>(o + 12), h = load<u32>(o + 16);
    ctx.gpr[3] = h;
    if (load<u8>(o + 21) == 0 && load<u8>(o + 23) == 0) return;
    const u32 base = load<u32>(0x00F65CA0);
    const u32 lv = base + n * 0x20C80u;
    const u32 lvl = lv + 4208;
    const u32 i = static_cast<u32>(call(ctx, 0x00465E10, {h, lv, base}));
    ctx.gpr[3] = i;
    if (static_cast<s32>(i) == -1) {
        ctx.gpr[3] = call(ctx, 0x0056A6E0, {load<u32>(o + 16)});
        return;
    }
    if (load<u8>(o + 21) != 0) {
        ctx.gpr[3] = call(ctx, 0x00465EA0, {lvl, i, load<u8>(o + 20)});
        return;
    }
    if (load<u8>(o + 23) != 0) ctx.gpr[3] = call(ctx, 0x00465D88, {lvl, i, load<u8>(o + 22)});
}

// void f(u32* mode, T* s, u32 out[2]): see UiSetupName31 (no +7324 = 36)
DECOMP(000B1440, Ui_SetupName_000B1440) { UiSetupName31(ctx, false); }

// void f(): frees the pool 0x00F64624 (its buffer +8 through the allocator
// +12: vt[8](alloc, buf, u16 +0 * (u16 +6 >> 1)); Array_Destruct,
// Mem_FreeFrom(p, 0)) and the block 0x00F64620, clearing both and
// 0x01013790. r3 = 0x01010000.
DECOMP(000B2814, Pool_Shutdown_000B2814) {
    const u32 p = load<u32>(0x00F64624);
    if (p != 0) {
        const u32 sz = load<u16>(p) * ((load<u16>(p + 6) >> 1) & 0x7FFFu);
        const u32 q = load<u32>(p + 8);
        if (q != 0) {
            const u32 a = load<u32>(p + 12);
            VCall(ctx, a, 8, {a, q, sz});
        }
        call(ctx, 0x0054EC94, {p});
        call(ctx, 0x0054EE6C, {p, 0});
        store<u32>(0x00F64624, 0);
    }
    const u32 b = load<u32>(0x00F64620);
    if (b != 0) {
        call(ctx, 0x0054EE6C, {b, 0});
        store<u32>(0x00F64620, 0);
    }
    store<u32>(0x01013790, 0);
    ctx.gpr[3] = 0x01010000;
}

// void f(T* o, bool on): for on, each of the (o->+172 >> 19) & 7 emitter
// slots (12 bytes from +256) is stopped (Emitter_Stop(+0, 1)) and reset to
// {0, -1, 0}; then bit 0x10 of u16 +176 = on (r3 = the new u16)
DECOMP(000DB180, Obj_StopEmitters_000DB180) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1);
    if ((on & 0xFFu) != 0 && ((load<u32>(o + 172) >> 19) & 7u) != 0) {
        for (u32 i = 0;;) {
            const u32 s = o + 12 * i + 256;
            // the original passes +0 through a stack doubleword (stw / ld / rldicl)
            call(ctx, 0x00443424, {load<u32>(s), 1});
            store<u32>(s, 0);
            ++i;
            store<u32>(s + 8, 0);
            store<u32>(s + 4, 0xFFFFFFFFu);
            if (i >= ((load<u32>(o + 172) >> 19) & 7u)) break;
        }
    }
    const u32 h = (load<u16>(o + 176) & ~0x10u) | ((on & 1u) << 4);
    store<u16>(o + 176, static_cast<u16>(h));
    ctx.gpr[3] = h;
}

// void f(const T* s, ?, T* d): the 4x3 transform s+16.. to d, then from the
// tuning vars (+72 of 0x01034314.. 76 bytes apart): d->+64 = #0 when
// >= *0x0010B6A4, +80 = #2, u8 +72 = #1, +76 = #4, +84 = 1, +68 = #3
// (r3 = &#3, f1 = #3)
DECOMP(0010B6A8, Cam_LoadTuning_0010B6A8) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 2);
    CopyRows4x3_31(ctx, d, s);
    constexpr u32 V = 0x01034314 + 72;
    const f64 v0 = LfsAt(V);
    ctx.fpr[1] = v0;
    if (v0 >= LfsAt(0x0010B6A4)) CopyW31(d + 64, V);
    CopyW31(d + 80, V + 76 * 2);
    store<u8>(d + 72, load<u8>(V + 76));
    CopyW31(d + 76, V + 76 * 4);
    store<u32>(d + 84, 1);
    ctx.fpr[1] = LfsAt(V + 76 * 3);
    CopyW31(d + 68, V + 76 * 3);
    ctx.gpr[3] = 0x01034314 + 76 * 3;
}

// void f(T* s, ?, T* d): when s->+84 is set, s->+64 = s->+84 Vec3 +144 +
// s->+96 and the 4x3 transform s+16.. goes to d; then d->+64 = s->+80 when
// >= *0x001137FC (f1 = s->+80)
DECOMP(00113800, Cam_LoadXform_00113800) {
    const u32 s = arg<u32>(ctx, 0), d = arg<u32>(ctx, 2);
    const u32 t = load<u32>(s + 84);
    if (t != 0) {
        const u32 v = t + 144;
        const f32 x = Add28(LfsAt(v), LfsAt(s + 96));
        const f32 y = Add28(LfsAt(v + 4), LfsAt(s + 100));
        const f32 z = Add28(LfsAt(v + 8), LfsAt(s + 104));
        store<f32>(s + 64, x);
        store<f32>(s + 68, y);
        store<f32>(s + 72, z);
        CopyRows4x3_31(ctx, d, s);
    }
    const f64 v = LfsAt(s + 80);
    ctx.fpr[1] = v;
    if (v >= LfsAt(0x001137FC)) CopyW31(d + 64, s + 80);
}

// const char* f(T* o): String_Format(0x00AC6E3C, name, f32 +96, +100,
// +104, +120 * k, +124 * k, bit 31 of +148) with name = o->+136->vt[76]()
// or 0x00AC6DA4 and k = *0x00114128 (the doubles go in both FPRs and
// GPRs, as for varargs)
DECOMP(00114130, Obj_Describe_00114130) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 136);
    const u32 name = c != 0 ? static_cast<u32>(VCall(ctx, c, 76, {c})) : 0x00AC6DA4u;
    const f64 k = LfsAt(0x00114128);
    const f64 a = Mul28(LfsAt(o + 120), k), b3 = LfsAt(o + 124);
    const f64 b = Mul28(b3, k);
    const f64 x = LfsAt(o + 96), y = LfsAt(o + 100), z = LfsAt(o + 104);
    const u32 bit = load<u32>(o + 148) >> 31;
    auto bits = [](f64 v) { return std::bit_cast<u64>(v); };
    callf(ctx, 0x005534D0, {0x00AC6E3Cu, name, bits(x), bits(y), bits(z), bits(a), bits(b), bit}, {a, b, b3, x, y, z});
}

// void f(T* o): bit 0x20000000 of o->+580 = some level l (count u8
// *0x00F65CA0->+29, 0x20C80 bytes apart) has a player l->+4216 within
// sqrt(o->+576) of o's position +144 (only when +576 > *0x001C7264);
// f1 = the position's z
DECOMP(001C7268, Obj_NearPlayer_001C7268) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 r2 = LfsAt(o + 576);
    ctx.fpr[1] = LfsAt(0x001C7264);
    if (!(r2 > ctx.fpr[1])) return;
    const u32 w = load<u32>(o + 580);
    store<u32>(o + 580, w & ~0x20000000u);
    u32 l = load<u32>(0x00F65CA0);
    const f64 x = LfsAt(o + 144), y = LfsAt(o + 148), z = LfsAt(o + 152);
    ctx.fpr[1] = z;
    const u32 n = load<u8>(l + 29);
    if (n == 0) return;
    for (u32 i = 0;;) {
        const u32 q = load<u32>(l + 4208 + 8);
        if (q != 0) {
            const u32 v = q + 144;
            const f64 dy = Sub28(LfsAt(v + 4), y), dx = Sub28(LfsAt(v), x), dz = Sub28(LfsAt(v + 8), z);
            f64 d = Mul28(dy, dy);
            d = Madd28(dx, dx, d);
            d = Madd28(dz, dz, d);
            if (d <= r2) {
                store<u32>(o + 580, w | 0x20000000u);
                return;
            }
        }
        ++i;
        if (i >= n) return;
        l += 0x20C80;
    }
}

// void f(T* o, u8 v): when o->+856->vt[272]() is set, the effect
// Effect_Bind4BB7E0(that (asked again), 0x03BA7BE9, 0) gets vt[40](e, v, 0)
DECOMP(001EE5A4, Obj_EffectSet_001EE5A4) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[3] = 0;
    const u32 c = load<u32>(o + 856);
    if (c == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 272, {c})) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 c2 = load<u32>(o + 856);
    const u32 r = static_cast<u32>(VCall(ctx, c2, 272, {c2}));
    const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {r, 0x03BA7BE9u, 0}));
    ctx.gpr[3] = 0;
    if (e == 0) return;
    ctx.gpr[3] = VCall(ctx, e, 40, {e, v, 0});
}

// bool f(T* o): when bit 0x20000 of +980 is set it is cleared (false);
// else true when o->+856 (with bit 0x40000 of +172) has vt[32](c, 0) == 0
// and vt[32](c, 1) == 6
DECOMP(001EF728, Obj_EffectIdle_001EF728) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 w = load<u32>(o + 980);
    ctx.gpr[3] = 0;
    if ((w & 0x00020000u) != 0) {
        store<u32>(o + 980, w & ~0x00020000u);
        return;
    }
    const u32 c = load<u32>(o + 856);
    if (c == 0 || (load<u32>(c + 172) & 0x00040000u) == 0) return;
    if (static_cast<u32>(VCall(ctx, c, 32, {c, 0})) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 c2 = load<u32>(o + 856);
    ctx.gpr[3] = static_cast<u32>(VCall(ctx, c2, 32, {c2, 1})) == 6 ? 1 : 0;
}

// T* f(a, b, c): p = SmallAlloc(192) built by 0x00746148(p, c, 0, 0, 1, 0,
// 1, 0, 0, 0) with +188 = -1, vtable 0x00B1A5D0 and bit 0x10 of +8; then
// 0x0021D838(a, b, p). r3 = p or 0.
DECOMP(0021D9C8, Obj_NewB1A5D0_0021D9C8) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {192}));
    if (p != 0) {
        call(ctx, 0x00746148, {p, c, 0, 0, 1, 0, 1, 0, 0, 0});
        const u32 f = load<u32>(p + 8) | 0x10u;
        store<u32>(p + 188, 0xFFFFFFFFu);
        store<u32>(p, 0x00B1A5D0);
        store<u32>(p + 8, f);
    }
    call(ctx, 0x0021D838, {a, b, p});
    ctx.gpr[3] = p;
}

// void f(T* o, u32 i, const T* src, bool notify): slot o->+372[i]'s +12
// points at o + 84 + 24i (no src) or at o + 228 + 24i holding a copy of
// src's six words; then for notify o->vt[24](o; *0x00271CAC)
DECOMP(00271CB0, Obj_SetSlotSrc_00271CB0) {
    const u32 o = arg<u32>(ctx, 0), i = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2), nt = arg<u32>(ctx, 3);
    const u32 slot = o + (i << 2) + 372;
    if (src == 0) {
        store<u32>(load<u32>(slot) + 12, o + i * 24 + 84);
    } else {
        const u32 d = o + i * 24 + 228;
        const u32 w0 = load<u32>(src);
        CopyW31(d + 4, src + 4);
        store<u32>(d, w0);
        for (u32 off : {8u, 12u, 16u}) CopyW31(d + off, src + off);
        ctx.fpr[1] = LfsAt(src + 20);
        CopyW31(d + 20, src + 20);
        store<u32>(load<u32>(slot) + 12, d);
    }
    ctx.gpr[3] = o;
    if ((nt & 0xFFu) == 0) return;
    ctx.fpr[1] = LfsAt(0x00271CAC);
    ctx.gpr[3] = VCall(ctx, o, 24, {o});
}

// void f(T* o, u8 on): unless bit 0x40 of u8 +28 (r3 = that u8), sets
// 0x80 / clears 0x40 there, wakes o->+20 (vt[12]) when its u8 +28 is 0,
// then o->vt[8](o, opd, on; *0x002E8BC0) until bit 0x40 is set (once when
// !on); r3 = bit 0x40
DECOMP(002E8BC8, Task_RunUntilDone_002E8BC8) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 b = load<u8>(o + 28);
    ctx.gpr[3] = b;
    if ((b & 0x40u) != 0) return;
    const u32 c = load<u32>(o + 20);
    store<u8>(o + 28, static_cast<u8>((b | 0x80u) & ~0x40u));
    if (c != 0 && load<u8>(c + 28) == 0) VCall(ctx, c, 12, {c});
    u32 bit;
    do {
        ctx.fpr[1] = LfsAt(0x002E8BC0);
        VCallOpdR4E6_30(ctx, o, 8, on);
        bit = (load<u8>(o + 28) >> 6) & 1u;
    } while (bit == 0 && on != 0);
    ctx.gpr[3] = bit;
}

// void f(T* o, u8 v): each set o->+2596[48k] (k < s32 o->+3360, re-read)
// gets vt[40](c, 0) and is cleared; then Recs_Reset325A30(o, v)
DECOMP(0032E760, Recs_ReleaseAll_0032E760) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    s32 n = static_cast<s32>(load<u32>(o + 3360));
    for (s32 i = 0; i < n; ++i) {
        const u32 s = o + 48 * static_cast<u32>(i) + 2592 + 4;
        const u32 c = load<u32>(s);
        if (c == 0) continue;
        VCall(ctx, c, 40, {c, 0});
        store<u32>(s, 0);
        n = static_cast<s32>(load<u32>(o + 3360));
    }
    ctx.gpr[3] = call(ctx, 0x00325A30, {o, v});
}

// void f(T* o): unless state (+160 >> 1) is 0 or 3: o->+24->vt[40](c, 1,
// 1) when set, state = 3, Speaker_SayId4A5C90(o->+28, o->+200, 0;
// *0x0036E208) and Effect_SetHash4CF988(o->+20, 0x048129EB) when set
DECOMP(0036E210, Talk_Finish_0036E210) {
    const u32 o = arg<u32>(ctx, 0);
    u32 w = load<u32>(o + 160);
    ctx.gpr[3] = w;
    const u32 s = w >> 1;
    if (s == 3 || s == 0) return;
    const u32 c = load<u32>(o + 24);
    if (c != 0) {
        VCall(ctx, c, 40, {c, 1, 1});
        w = load<u32>(o + 160);
    }
    store<u32>(o + 160, (w & 1u) | 6u);
    callf(ctx, 0x004A5C90, {load<u32>(o + 28), load<u32>(o + 200), 0}, {LfsAt(0x0036E208)});
    const u32 t = load<u32>(o + 20);
    if (t != 0) ctx.gpr[3] = call(ctx, 0x004CF988, {t, 0x048129EBu});
}

// void f(T* o): the cycle index +116 wraps 3 -> 0; 0x003AB0B8(o, h1, h2,
// 0, 0x003AB80C, 1; *0x003AB80C) with h1 / h2 the no-case hashes of
// String_Format(0x00ADF148, o + 28, names[i]) / (0x00ADF158, o + 60, ...)
// over the name table 0x00ADF2C0
DECOMP(003AB810, Anim_PlayCycle_003AB810) {
    const u32 o = arg<u32>(ctx, 0);
    u32 i = load<u32>(o + 116);
    if (i == 3) {
        i = 0;
        store<u32>(o + 116, 0);
    }
    constexpr u32 T = 0x00ADF2C0;
    u32 r = static_cast<u32>(call(ctx, 0x005534D0, {0x00ADF148u, o + 28, load<u32>(T + (i << 2))}));
    const u32 h1 = static_cast<u32>(call(ctx, 0x0056A710, {r}));
    const u32 i2 = load<u32>(o + 116);
    r = static_cast<u32>(call(ctx, 0x005534D0, {0x00ADF158u, o + 60, load<u32>(T + (i2 << 2))}));
    const u32 h2 = static_cast<u32>(call(ctx, 0x0056A710, {r}));
    callf(ctx, 0x003AB0B8, {o, h1, h2, 0, 0x003AB80Cu, 1}, {LfsAt(0x003AB80C)});
}

// s32 f(const char* path, u32* out): fios_OpenFile(sched, attr = {u64 1,
// u8 127, 0...}, the path copied (255 chars), 1, &h) then 0x005F8A38(r,
// 0): 0 and *out = h when it succeeded with a handle, else -2
DECOMP(0044C10C, File_Open_0044C10C) {
    const u32 path = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    Stack frame(ctx, 304);
    const u32 at = frame.addr(), hp = at + 40, buf = at + 48;
    store<u64>(at, 1);
    store<u8>(at + 8, 127);
    for (u32 off : {12u, 16u, 20u, 28u, 24u}) store<u32>(at + off, 0);
    call(ctx, 0x00553638, {buf, path, 256});
    store<u8>(buf + 255, 0);
    const u32 s = static_cast<u32>(call(ctx, 0x005FC90C));
    const u32 r = static_cast<u32>(call(ctx, 0x005FEF14, {s, at, buf, 1, hp}));
    ctx.gpr[3] = static_cast<u64>(static_cast<s64>(-2));
    if (static_cast<u32>(call(ctx, 0x005F8A38, {r, 0})) != 0) {
        ctx.gpr[3] = static_cast<u64>(static_cast<s64>(-2));
        return;
    }
    const u32 h = load<u32>(hp);
    if (h == 0) {
        ctx.gpr[3] = static_cast<u64>(static_cast<s64>(-2));
        return;
    }
    store<u32>(out, h);
    ctx.gpr[3] = 0;
}

// void f(Vec2* out, const Vec2* a, const Vec2* b; f32 t, f32 u): out = a +
// (b - a) * (1 - w) with w = 1 - t, replaced (for 0.01 <= w <= 0.998) by
// the curve table 0x00AEB6D4[6 * (s32)(w * 400) + i], i = clamp((s32)(u *
// 60 + 0.5) - 1 after max 1, .., 5) (constants at 0x004A848C)
DECOMP(004A84A8, Curve_Lerp2_004A84A8) {
    const u32 out = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2);
    const f64 t = farg(ctx, 0), u = farg(ctx, 1);
    constexpr u32 K = 0x004A848C;
    const f64 k0 = LfsAt(K);
    f64 w = Sub28(k0, t);
    const f64 a0 = LfsAt(a), a1 = LfsAt(a + 4);
    const f64 dx = Sub28(LfsAt(b), a0), dy = Sub28(LfsAt(b + 4), a1);
    if (!(w < LfsAt(K + 4)) && !(w > LfsAt(K + 8))) {
        s32 i = static_cast<s32>(rt::fctiw(Madd28(u, LfsAt(K + 12), LfsAt(K + 16)), true));
        if (i < 1) i = 1;
        i -= 1;
        if (i >= 6) i = 5;
        const s32 j = static_cast<s32>(rt::fctiw(Mul28(w, LfsAt(K + 20)), true));
        const u32 idx = static_cast<u32>(j) * 6u + static_cast<u32>(i);
        w = LfsAt(0x00AEB6D4u + (idx << 2));
    }
    const f64 m = Sub28(k0, w);
    const f32 x = Madd28(dx, m, a0), y = Madd28(dy, m, a1);
    store<f32>(out, x);
    store<f32>(out + 4, y);
    ctx.fpr[1] = y;
    ctx.gpr[3] = out;
}

// T* f(?, XmlCursor* x): SmallAlloc(24): u16 +4 = 1, vtable 0x00B21D78,
// hash +8 (0x0024728B) and the f32 +20 read twice (0x00014B01, then
// 0x04DA9951, each defaulting to the current value). r3 = p or 0.
DECOMP(004BEE5C, Xml_NewB21D78_004BEE5C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {24}));
    ctx.gpr[3] = p;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B21D78);
    store<u32>(p + 20, 0);
    call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x0024728Bu, p + 8, 0});
    callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x00014B01u, p + 20}, {LfsAt(p + 20)});
    callf(ctx, o28fn::XmlCursor_ReadF32, {x, 0x04DA9951u, p + 20}, {LfsAt(p + 20)});
    ctx.gpr[3] = p;
}

// void f(Effect* e, u32 h): for bit 0x100 of e->+136: 0x004BCC90(e->+4, e,
// h) when e->+4 is set, the same for each non-null child e->+112[k] (u16
// count +120), recursively, then Effect_Release(e, e->+132) and +132 = 0
// when set
DECOMP(004CF548, Effect_StopRec_004CF548) {
    const u32 e = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    ctx.gpr[3] = 0;
    if ((load<u32>(e + 136) & 0x100u) == 0) return;
    const u32 p = load<u32>(e + 4);
    if (p != 0) call(ctx, 0x004BCC90, {p, e, h});
    for (u32 i = 0; i < load<u16>(e + 120); ++i) {
        const u32 c = load<u32>(load<u32>(e + 112) + 4 * i);
        if (c != 0) call(ctx, 0x004CF548, {c, h});
    }
    const u32 t = load<u32>(e + 132);
    ctx.gpr[3] = t;
    if (t == 0) return;
    ctx.gpr[3] = call(ctx, 0x004CF3C4, {e, t});
    store<u32>(e + 132, 0);
}

// T::~T(T* o): vtable 0x00B20A48; o->+332 and o->+336 each get bit 0x1000
// of u16 +176 when their +172 has bit 0x00040000, and are cleared; the
// vector +164 cleared; base vtable 0x00B207A8 and Dtor_B219D0_00490B34(o)
DECOMP(0077D3AC, Dtor_B20A48_0077D3AC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B20A48);
    for (u32 off : {332u, 336u}) {
        const u32 c = load<u32>(o + off);
        if (c != 0 && (load<u32>(c + 172) & 0x00040000u) != 0) store<u16>(c + 176, static_cast<u16>(load<u16>(c + 176) | 0x1000u));
        store<u32>(o + off, 0);
    }
    call(ctx, o31fn::PtrVector_Clear, {o + 164});
    call(ctx, o31fn::Nop_54EC18, {o + 164});
    store<u32>(o, 0x00B207A8);
    ctx.gpr[3] = call(ctx, 0x00490B34, {o});
}

// ---------------------------------------------------------------- sweep: 55 instructions
// bool f(SplineInstance* o): for the modes 2, 3, 7, 10 and 12 with bit
// 0x8000 of +164: true when there is no parent +260 or it is followed,
// else when the parent has bit 0x8000 of +164 and its kind ((+4 >> 10) &
// 63) is not 13
DECOMP(000239E4, Spline_IsActive_000239E4) {
    const u32 o = arg<u32>(ctx, 0);
    const s32 m = static_cast<s32>(call(ctx, 0x005A8BA8, {o}));
    ctx.gpr[3] = 0;
    if (!(m == 2 || m == 3 || m == 7 || m == 10 || m == 12)) return;
    if ((load<u32>(o + 164) & 0x8000u) == 0) return;
    if (load<u32>(o + 260) == 0 || static_cast<u32>(call(ctx, 0x005A8BE4, {o})) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    ctx.gpr[3] = 0;
    const u32 p = load<u32>(o + 260);
    if ((load<u32>(p + 164) & 0x8000u) == 0) return;
    if (((load<u32>(p + 4) >> 10) & 0x3Fu) == 13) return;
    ctx.gpr[3] = 1;
}

// void f(Vec3* out, T* s): out = s->+384, or s->+400 past the end (s32 i =
// +372 > s->+368->+36 = n), or, for 0 < i, 0x00025F00(&v, s, e) with e =
// the key s->+368->+12[i - 1] (or [n - i] for bit 0x80 of u8 +416) when
// set. r3 = s or the call's result; f1 = the last value moved.
DECOMP(000261BC, Spline_PointAt_000261BC) {
    const u32 out = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 3; ++k) CopyW31(out + 4 * k, s + 384 + 4 * k);
    ctx.fpr[1] = LfsAt(s + 392);
    const u32 t = load<u32>(s + 368);
    const s32 i = static_cast<s32>(load<u32>(s + 372));
    const s32 n = static_cast<s32>(load<u32>(t + 36));
    ctx.gpr[3] = s;
    if (i > n) {
        for (u32 k = 0; k < 3; ++k) CopyW31(out + 4 * k, s + 400 + 4 * k);
        ctx.fpr[1] = LfsAt(s + 408);
        return;
    }
    if (!(i > 0)) return;
    const u32 k = (load<u8>(s + 416) & 0x80u) != 0 ? static_cast<u32>(n - i) : static_cast<u32>(i - 1);
    const u32 e = load<u32>(t + 4 * k + 12);
    if (e == 0) return;
    Stack frame(ctx, 16);
    const u32 v = frame.addr();
    ctx.gpr[3] = call(ctx, 0x00025F00, {v, s, e});
    for (u32 j = 0; j < 3; ++j) CopyW31(out + 4 * j, v + 4 * j);
    ctx.fpr[1] = LfsAt(v + 8);
}

// f32 f(T* o): with t = (k0 - phase) * length of track o->+48
// (Anim_TrackLength164 / Obj_AnimPhase, k0 = *0x00048F3C) and the info
// block o->+48->vt[208](a, &info, 0; t): (t > *0x00048F40 ? info+56 / t :
// that constant) * Anim_Track36Get(o->+64, 0) * f32 o->+76
DECOMP(00048F48, Anim_RateScale_00048F48) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x000DC418, {load<u32>(o + 48), 0});
    const f64 len = ctx.fpr[1];
    call(ctx, 0x000DC6A0, {load<u32>(o + 48), 0});
    const f64 t = Mul28(Sub28(LfsAt(0x00048F3C), ctx.fpr[1]), len);
    Stack frame(ctx, 64);
    const u32 info = frame.addr();
    const u32 a = load<u32>(o + 48);
    const u32 opd = load<u32>(load<u32>(a) + 208);
    ctx.fpr[1] = t;
    call_ptr(ctx, opd, {a, info, opd, 0, load<u32>(opd)});
    f64 f = LfsAt(0x00048F40);
    const u32 c = load<u32>(o + 64);
    if (t > f) f = static_cast<f32>(LfsAt(info + 56) / t);
    call(ctx, 0x00276AD8, {c, 0});
    ctx.fpr[1] = Mul28(Mul28(f, ctx.fpr[1]), LfsAt(o + 76));
}

// void f(T* o): from the name table o->+48->+12: +136 = names[idx A], and
// the u16 +116 = the +8 of the entry (20 bytes, *0x00F557F4->+12) whose
// no-case hash of names[idx B] SortedMap_Find734750 finds (s16 index, -1
// none); then Field_Reset88_00455B7C(o). idx A / B = the +64 of
// 0x010AB3F8 / 0x010AB3E0's targets.
DECOMP(00061BB8, Obj_BindNames_00061BB8) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 tbl = load<u32>(load<u32>(o + 48) + 12);
    store<u32>(o + 136, load<u32>(tbl + (load<u32>(load<u32>(0x010AB3F8) + 64) << 2)));
    const u32 ib = load<u32>(load<u32>(0x010AB3E0) + 64);
    const u32 g = load<u32>(0x00F557F4);
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {load<u32>(tbl + (ib << 2))}));
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), h);
    const u32 r = static_cast<u32>(call(ctx, 0x00734750, {g + 16, frame.addr()}));
    const s32 k = static_cast<s16>(load<u16>(r));
    if (k != -1) {
        const u32 e = load<u32>(g + 12) + static_cast<u32>(k) * 20;
        if (e != 0) store<u16>(o + 116, load<u16>(e + 8));
    }
    ctx.gpr[3] = call(ctx, 0x00455B7C, {o});
}

// void f(T* o, x): t = Obj_FindById(o->+12), else the +280 of e =
// Effect_Bind4BB7E0(x, o->+16, 0) when its +276 is 0x02235C33; then
// Obj_Detach0D72C0(t->+336, t) when set
DECOMP(00071FE4, Obj_DetachTarget_00071FE4) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = 0;
    const u32 id = load<u32>(o + 12);
    ctx.gpr[3] = 0;
    if (id != 0) t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    if (t == 0) {
        ctx.gpr[3] = 0;
        const u32 h = load<u32>(o + 16);
        if (h != 0) {
            const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, h, 0}));
            ctx.gpr[3] = 0;
            if (e != 0) {
                ctx.gpr[3] = 0x02235C33;  // as left by the compare
                if (load<u32>(e + 276) == 0x02235C33u) t = load<u32>(e + 280);
            }
        }
        if (t == 0) return;
    }
    const u32 c = load<u32>(t + 336);
    ctx.gpr[3] = c;
    if (c != 0) ctx.gpr[3] = call(ctx, 0x000D72C0, {c, t});
}

// void f(T* o, x): see TargetCall31 (0x000D99D8)
DECOMP(000972CC, Obj_TargetCallD99D8_000972CC) { TargetCall31(ctx, 0x000D99D8); }

// void f(T* o, x): see TargetCall31 (0x000D9C88)
DECOMP(000974EC, Obj_TargetCallD9C88_000974EC) { TargetCall31(ctx, 0x000D9C88); }

// void f(T* o, x): see TargetCall31 (0x000D9B30)
DECOMP(00097708, Obj_TargetCallD9B30_00097708) { TargetCall31(ctx, 0x000D9B30); }

// void f(T* o, x): for the effect e = Effect_Bind4BB7E0(x, o->+12, 0)
// (retried with 1): CmdList_Push(p) with p = SmallAlloc(24) built by
// VtB21208_Ctor(p, e, o->+16, u8 +20, u8 +21) (or null)
DECOMP(000A41E0, Cmd_PushEffect_000A41E0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 h = load<u32>(o + 12);
    if (h == 0) return;  // r3 = o
    u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, h, 0}));
    if (e == 0) e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(o + 12), 1}));
    ctx.gpr[3] = 0;
    if (e == 0) return;
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {24}));
    u32 q = 0;
    if (p != 0) {
        call(ctx, 0x00438348, {p, e, load<u32>(o + 16), load<u8>(o + 20), load<u8>(o + 21)});
        q = p;
    }
    ctx.gpr[3] = call(ctx, o30fn::CmdList_Push, {q});
}

// T* f(?, XmlCursor* x): SmallAlloc(88): u16 +4 = 1, vtable 0x00B11B88,
// hash +8 (0x0024728B) and the string +20 (64 bytes) from attribute
// 0x00014B01 when present and non-empty. r3 = p or 0.
DECOMP(000A79D0, Xml_NewB11B88_000A79D0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {88}));
    ctx.gpr[3] = p;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p, 0x00B11B88);
    store<u8>(p + 20, 0);
    call(ctx, o28fn::XmlCursor_ReadHash, {x, 0x0024728Bu, p + 8, 0});
    const u32 s = static_cast<u32>(call(ctx, 0x004F9534, {x, 0x00014B01u}));
    if (s != 0 && load<u8>(s) != 0) call(ctx, 0x00553684, {p + 20, s, 64});
    ctx.gpr[3] = p;
}

// void f(u32* mode, T* s, u32 out[2]): see UiSetupName31 (with +7324 = 36)
DECOMP(000B1518, Ui_SetupName_000B1518) { UiSetupName31(ctx, true); }

// void f(T* o, Mat* out, u32 i, const Mat* def): 0x000E3534(o, out, the
// 48-byte entry i of o->+388, def) when that list is non-empty, else out =
// def's 3x4 rows (f1 = the last value moved; r3 = def)
DECOMP(000E7CE8, Obj_PoseOrDefault_000E7CE8) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1), i = arg<u32>(ctx, 2), def = arg<u32>(ctx, 3);
    const u32 l = load<u32>(o + 388);
    if (l != 0 && load<u16>(l + 8) != 0) {
        ctx.gpr[3] = call(ctx, 0x000E3534, {o, out, load<u32>(l) + i * 48, def});
        return;
    }
    for (u32 r = 0; r < 4; ++r)
        for (u32 k = 0; k < 3; ++k) CopyW31(out + 16 * r + 4 * k, def + 16 * r + 4 * k);
    ctx.fpr[1] = LfsAt(def + 56);
    ctx.gpr[3] = def;
}

// void f(T* o, u8 on): for on, when bit 31 of +344 is set: 0x000DC314(o, 0,
// 0), o->+364->vt[8]() and +364 = 0 when its s16 +4 is positive; then bit
// 31 of +344 and u8 +345 cleared. Then 0x000D74C0(o, on).
DECOMP(000FA4F4, Obj_Release364_000FA4F4) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    if (on != 0) {
        if ((load<u32>(o + 344) >> 31) != 0) {
            call(ctx, 0x000DC314, {o, 0, 0});
            const u32 c = load<u32>(o + 364);
            VCall(ctx, c, 8, {c});
            const s32 s = static_cast<s16>(load<u16>(load<u32>(o + 364) + 4));
            if ((static_cast<u32>(s | (s - 1)) >> 31) == 0) store<u32>(o + 364, 0);
        }
        store<u32>(o + 344, load<u32>(o + 344) & 0x7FFFFFFFu);
        store<u8>(o + 345, 0);
    }
    ctx.gpr[3] = call(ctx, 0x000D74C0, {o, on});
}

// void f(T* o, const char* s, a, b): for b, the no-case hash of s sets bit
// 31 of +468 (0x5564C0C9, 0x1B12498F) or clears it (0xE9B65CFB,
// 0xD49B5F41; r3 = the new word), else 0x000DD368(o, s, a, b). r3 = o
// for !b.
DECOMP(000FC88C, Obj_SetAttr_000FC88C) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2), b = arg<u32>(ctx, 3);
    if (b == 0) return;
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {s}));
    if (h == 0x5564C0C9u || h == 0x1B12498Fu) {
        const u32 v = load<u32>(o + 468) | 0x80000000u;
        store<u32>(o + 468, v);
        ctx.gpr[3] = v;
        return;
    }
    if (h == 0xE9B65CFBu || h == 0xD49B5F41u) {
        const u32 v = load<u32>(o + 468) & 0x7FFFFFFFu;
        store<u32>(o + 468, v);
        ctx.gpr[3] = v;
        return;
    }
    ctx.gpr[3] = call(ctx, 0x000DD368, {o, s, a, b});
}

// void f(T* o): 0x0034A1FC(o->+164, 0) when set; then w = o->+160's
// matrix +160 (unless it is the shared identity 0x01193010) = the 3x4
// rows of 0x01164BD0 and WorldObj_MarkDirty(w)
DECOMP(000FCD1C, Obj_ResetMatrix_000FCD1C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 c = load<u32>(o + 164);
    ctx.gpr[3] = 0;
    if (c != 0) call(ctx, 0x0034A1FC, {c, 0});
    const u32 w = load<u32>(o + 160);
    if (w == 0) return;
    const u32 m = load<u32>(w + 160);
    ctx.gpr[3] = m;
    if (m == 0x01193010u) return;
    static constexpr u32 off[12] = {0, 4, 8, 16, 20, 24, 32, 36, 40, 48, 52, 56};
    u32 v[12];
    for (u32 k = 0; k < 12; ++k) v[k] = load<u32>(0x01164BD0u + off[k]);
    ctx.fpr[1] = LfsAt(0x01164BD0);
    for (u32 k = 0; k < 12; ++k) store<u32>(m + off[k], v[k]);
    ctx.gpr[3] = call(ctx, 0x0057D000, {w});
}

// bool f(T* o, ?, x; f32 v): 0x00115900(o + 96, o + 160; v), then
// o->vt[16] / [20] / [24](o, opd, x; v). r3 = 1.
DECOMP(00115EB0, Obj_Update3_00115EB0) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 2);
    const f64 v = farg(ctx, 0);
    callf(ctx, 0x00115900, {o + 96, o + 160}, {v});
    for (u32 slot : {16u, 20u, 24u}) {
        ctx.fpr[1] = v;
        VCallOpdR4E6_30(ctx, o, slot, x);
    }
    ctx.fpr[1] = v;
    ctx.gpr[3] = 1;
}

// void f(T* o, x): the old effect +412 (unless its +4's +8 is 0x00010CBB:
// Effect_Release(e, 0x00010CBB) and Effect_SetTime(e; *0x00129CB0)) gets
// bit 0x200 of +136; +412 = Obj_EffectDCD20(o, x, 0, 1) with that bit
// cleared. r3 = 0.
DECOMP(00129CB8, Obj_SwapEffect_00129CB8) {
    const u32 o = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 e = load<u32>(o + 412);
    if (e != 0) {
        const u32 p = load<u32>(e + 4);
        if (p != 0 && load<u32>(p + 8) != 0x00010CBBu) {
            call(ctx, 0x004CF3C4, {e, 0x00010CBBu});
            const u32 e2 = load<u32>(o + 412);
            callf(ctx, 0x004CF710, {e2, e2}, {LfsAt(0x00129CB0)});
            e = load<u32>(o + 412);
        }
        store<u32>(e + 136, load<u32>(e + 136) | 0x200u);
        store<u32>(o + 412, 0);
    }
    const u32 r = static_cast<u32>(call(ctx, 0x000DCD20, {o, x, 0, 1}));
    store<u32>(o + 412, r);
    if (r != 0) store<u32>(r + 136, load<u32>(r + 136) & ~0x200u);
    ctx.gpr[3] = 0;
}

// void f(const T* a, T* b): Obj_CopyQueryEx(a, b), then the fields a+240..
// +324 to b+208.. / +200 (in the original's order; r3 = a->+324, f1 =
// a->+300)
DECOMP(0017F318, Obj_CopyQueryAll_0017F318) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000DFBFC, {a, b});
    static constexpr u16 ops[][2] = {{208, 240}, {212, 244}, {216, 248}, {224, 256}, {228, 260}, {232, 264}, {240, 272},
                                     {244, 276}, {248, 280}, {264, 296}, {252, 284}, {256, 288}, {260, 292}, {276, 308}};
    for (auto& p : ops) CopyW31(b + p[0], a + p[1]);
    store<u8>(b + 272, load<u8>(a + 304));
    ctx.fpr[1] = LfsAt(a + 300);
    CopyW31(b + 268, a + 300);
    CopyW31(b + 280, a + 312);
    store<u8>(b + 284, load<u8>(a + 316));
    CopyW31(b + 288, a + 320);
    const u32 w = load<u32>(a + 324);
    store<u32>(b + 200, w);
    ctx.gpr[3] = w;
}

// void f(T* o, const T* s): the 96-byte sample i = min((u32)(s->+36 * n),
// n - 1) of o (+416, u16 count n at +1890) gets the Vec3 s->+16 added;
// then 0x000DB94C(&tmp, s->+0, o->+216->+172, 0, 0, 0, 0, 0, 0) (f1..f3 =
// the new z, y, x)
DECOMP(001A97D8, Obj_AddSample_001A97D8) {
    const u32 o = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    const u32 n = load<u16>(o + 1890);
    const f64 sc = Mul28(LfsAt(s + 36), static_cast<f32>(static_cast<f64>(n)));
    const u32 j = static_cast<u32>(rt::fctid(sc, true));
    u32 idx = n - 1;
    if (j < idx) idx = j;
    const u32 e = o + idx * 96 + 416;
    const f32 x = Add28(LfsAt(e), LfsAt(s + 16));
    const f32 y = Add28(LfsAt(e + 4), LfsAt(s + 20));
    const f32 z = Add28(LfsAt(e + 8), LfsAt(s + 24));
    store<f32>(e, x);
    store<f32>(e + 4, y);
    store<f32>(e + 8, z);
    Stack frame(ctx, 32);
    ctx.fpr[1] = z;
    ctx.fpr[2] = y;
    ctx.fpr[3] = x;
    ctx.gpr[3] = call(ctx, 0x000DB94C, {frame.addr(), load<u32>(s), load<u32>(load<u32>(o + 216) + 172), 0, 0, 0, 0, 0, 0});
}

// bool f(T* o; f32): unless Obj_SyncBodyShapes(o; f1 as passed) fails
// (r3 = 0), each set shape o->+560[16k] (k < 6) gets vt[304](c, o->vt[32](o,
// 0) == 4). r3 = 1.
DECOMP(001C9B3C, Obj_SyncShapes_001C9B3C) {
    const u32 o = arg<u32>(ctx, 0);
    callf(ctx, 0x000E5000, {o}, {farg(ctx, 0)});  // the callee reads the f1 passed in
    if (static_cast<u32>(ctx.gpr[3]) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 fl = static_cast<u32>(VCall(ctx, o, 32, {o, 0})) == 4 ? 1u : 0u;
    for (u32 i = 0; i < 6; ++i) {
        const u32 c = load<u32>(o + 560 + 16 * i);
        if (c != 0) VCall(ctx, c, 304, {c, fl});
    }
    ctx.gpr[3] = 1;
}

// void f(T* o, u8 a, u8 b, r6): unless bit 0x04000000 of +980, for c =
// o->+848: a ? Obj_SetAnim(c, 0, b, r6, 0, 0, 0x01164CA4; *0x001EF978) :
// Obj_Dispatch362(c, 1, b, 0) and, for b, c->+24->vt[40](d, 0, 1)
DECOMP(001EF980, Obj_PlayIdle_001EF980) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2) & 0xFFu, r6 = arg<u32>(ctx, 3);
    const u32 w = load<u32>(o + 980);
    ctx.gpr[3] = (w >> 26) & 1u;
    if ((w & 0x04000000u) != 0) return;
    const u32 c = load<u32>(o + 848);
    ctx.gpr[3] = c;
    if (c == 0) return;
    if ((a & 0xFFu) != 0) {
        callf(ctx, 0x00363A3C, {c, 0, b, r6, 0, 0, 0x01164CA4u}, {LfsAt(0x001EF978)});  // r6 passed through, unread
        return;
    }
    call(ctx, 0x0036325C, {c, 1, b, 0});
    if (b == 0) return;
    const u32 d = load<u32>(load<u32>(o + 848) + 24);
    ctx.gpr[3] = VCall(ctx, d, 40, {d, 0, 1});
}

// void f(const T* a, T* b): Obj_CopyQuery(a, b), b->+144 = a->+180, the
// bits 0x80 / 0x40 / 0x20 / 0x10 / 0x08 of u8 a->+184 as the bytes
// b->+148..+152, b->+156 = a->+188, bit 0x02 as u8 b->+153 (r3 = it)
DECOMP(0021DAA0, Obj_CopyQueryFlags_0021DAA0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1);
    call(ctx, 0x000CDB28, {a, b});
    store<u32>(b + 144, load<u32>(a + 180));
    for (u32 k = 0; k < 5; ++k) store<u8>(b + 148 + k, static_cast<u8>((load<u8>(a + 184) >> (7 - k)) & 1u));
    store<u32>(b + 156, load<u32>(a + 188));
    const u32 v = (load<u8>(a + 184) >> 1) & 1u;
    store<u8>(b + 153, static_cast<u8>(v));
    ctx.gpr[3] = v;
}

// bool f(T* o, T* t, u32 k, r6): 0x000DF960(o, t, 0, r6) for k == 0 when
// o's f32 +348 is above *0x00229A40 and t is not a bit-0x400 (+200) object
// of the same team ((+172 >> 22) & 3); else 0 (f1 = o->+348 once read)
DECOMP(00229A48, Obj_CanTarget_00229A48) {
    const u32 o = arg<u32>(ctx, 0), t = arg<u32>(ctx, 1), k = arg<u32>(ctx, 2), r6 = arg<u32>(ctx, 3);
    ctx.gpr[3] = 0;
    if (k > 1) return;
    ctx.fpr[1] = LfsAt(o + 348);
    if (ctx.fpr[1] <= LfsAt(0x00229A40)) return;
    if (t != 0 && (load<u32>(t + 200) & 0x400u) != 0 && ((load<u32>(o + 172) >> 22) & 3u) == ((load<u32>(t + 172) >> 22) & 3u)) return;
    if (k != 0) return;
    ctx.gpr[3] = call(ctx, 0x000DF960, {o, t, k, r6});
}

// T* f(a, T* n): the first entry e (+4 of the 8-byte pairs n->+208->+132
// ->+48, u16 count +52) whose u8 +100 has bit 0x20; else the same search
// in each child c (list n->+272, next +268) of kind 8 ((+4 >> 10) & 63),
// recursively; else null
DECOMP(00257828, Node_FindFlagged_00257828) {
    const u32 a = arg<u32>(ctx, 0), n = arg<u32>(ctx, 1);
    const u32 s = load<u32>(load<u32>(n + 208) + 132);
    const u32 cnt = load<u16>(s + 52);
    if (cnt != 0) {
        u32 p = load<u32>(s + 48);
        for (u32 i = 0; i < cnt; ++i, p += 8) {
            const u32 e = load<u32>(p + 4);
            if ((load<u8>(e + 100) & 0x20u) != 0) {
                ctx.gpr[3] = e;
                return;
            }
        }
    }
    for (u32 c = load<u32>(n + 272); c != 0;) {
        const u32 nx = load<u32>(c + 268);
        if (((load<u32>(c + 4) >> 10) & 0x3Fu) == 8) {
            const u32 r = static_cast<u32>(call(ctx, 0x00257828, {a, c}));
            if (r != 0) {
                ctx.gpr[3] = r;
                return;
            }
        }
        c = nx;
    }
    ctx.gpr[3] = 0;
}

// void f(T* o): for each of the o->+28 material records (220 bytes at
// o->+24) with a material +12, its 32 f32 params +92.. go to
// Material_ParamSlot(mat, i). r3 = the count.
DECOMP(002878AC, Mat_ApplyParams_002878AC) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = load<u32>(o + 28);
    if (ctx.gpr[3] == 0) return;
    for (u32 j = 0; j < load<u32>(o + 28); ++j) {
        for (u32 i = 0; i < 32; ++i) {
            const u32 r = load<u32>(o + 24) + 220 * j;
            const u32 m = load<u32>(r + 12);
            if (m == 0) continue;
            const u32 v = load<u32>(r + 4 * i + 92);
            store<u32>(static_cast<u32>(call(ctx, 0x0050C018, {m, i})), v);
        }
    }
    ctx.gpr[3] = load<u32>(o + 28);
}

// void f(T* o; f32 dt): a held o->+36 without bit 0x40000 (+172) is let go
// (vt[40](c, 0); +36 = 0, timer +40 = +20, u8 +45 = 1); then, with none
// held and u8 +24 set, the timer +40 counts down by dt, setting u8 +44 at
// <= *0x002B43C8
DECOMP(002B43D0, Grab_Tick_002B43D0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 dt = farg(ctx, 0);
    u32 r = load<u32>(o + 36);
    if (r != 0 && (load<u32>(r + 172) & 0x00040000u) == 0) {
        VCall(ctx, r, 40, {r, 0});
        r = 0;
        store<u32>(o + 36, 0);
        ctx.fpr[1] = LfsAt(o + 20);
        CopyW31(o + 40, o + 20);
        store<u8>(o + 45, 1);
    }
    ctx.gpr[3] = r;
    if (r != 0) return;
    ctx.gpr[3] = load<u8>(o + 24);
    if (load<u8>(o + 24) == 0) return;
    const f32 v = Sub28(LfsAt(o + 40), dt);
    store<f32>(o + 40, v);
    ctx.fpr[1] = v;
    ctx.gpr[3] = 0x002B43C8;
    if (v <= LfsAt(0x002B43C8)) {
        store<u8>(o + 44, 1);
        ctx.gpr[3] = 1;
    }
}

// bool f(T* o): o->+8->vt[252]() when set; Obj_Reset6(t, o->+8) for t =
// o->+64 with bit 8 of +192; t->vt[40](t, 0); +64 = 0 and
// Obj_SetStateFx266E6C(o, 0). r3 = 1.
DECOMP(002B7704, Obj_DropHeld_002B7704) {
    const u32 o = arg<u32>(ctx, 0);
    u32 c = load<u32>(o + 8);
    if (c != 0) {
        VCall(ctx, c, 252, {c});
        c = load<u32>(o + 8);
    }
    u32 t = load<u32>(o + 64);
    if (t != 0 && (load<u32>(t + 192) & 8u) != 0) {
        call(ctx, 0x0012D290, {t, c});
        t = load<u32>(o + 64);
    }
    if (t != 0) VCall(ctx, t, 40, {t, 0});
    store<u32>(o + 64, 0);
    call(ctx, 0x00266E6C, {o, 0});
    ctx.gpr[3] = 1;
}

// T* f(T* o, u32 h): the effect +108 replaced by a new one (SmallAlloc(140),
// Effect_Construct(p, EffectDef_Find(h, 0, 1)), Effect_Start) with bit
// 0x200 of +136 cleared; the old one Effect_Delete'd. r3 = the new +108
// (o for h == 0, 0 when the def is missing).
DECOMP(002B78F0, Obj_SetEffect108_002B78F0) {
    const u32 o = arg<u32>(ctx, 0), h = arg<u32>(ctx, 1);
    if (h == 0) return;
    const u32 d = static_cast<u32>(call(ctx, 0x004BDEDC, {h, 0, 1}));
    ctx.gpr[3] = 0;
    if (d == 0) return;
    const u32 old = load<u32>(o + 108);
    if (old != 0) call(ctx, 0x004D0280, {old});
    const u32 p = static_cast<u32>(call(ctx, o31fn::SmallAlloc, {140}));
    u32 e = 0;
    if (p != 0) {
        call(ctx, 0x004C7A68, {p, d});
        e = p;
    }
    store<u32>(o + 108, e);
    call(ctx, 0x004C753C, {e});
    const u32 e2 = load<u32>(o + 108);
    store<u32>(e2 + 136, load<u32>(e2 + 136) & ~0x200u);
    ctx.gpr[3] = e2;
}

// void f(T* o, u8 on): when on differs from u8 +376, the body b of
// o->+8->vt[64]()->+32 (Hk_GetBody9D19C8) gets 0x0091A82C(b, o + 24) and
// u16 +166 = 0 (on) or 0x0091A228(b, o + 24) (off); +376 = on
DECOMP(002CC884, Phys_SetActive_002CC884) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[3] = load<u8>(o + 376);
    if (on == load<u8>(o + 376)) return;
    const u32 c = load<u32>(o + 8);
    const u32 r = static_cast<u32>(VCall(ctx, c, 64, {c}));
    const u32 h = load<u32>(r + 32);
    ctx.gpr[3] = r;
    if (h == 0) return;
    const u32 b = static_cast<u32>(call(ctx, 0x009D19C8, {h}));
    if (b == 0) return;
    if (on != 0) {
        call(ctx, 0x0091A82C, {b, o + 24});
        store<u16>(b + 166, 0);
    } else {
        call(ctx, 0x0091A228, {b, o + 24});
    }
    store<u8>(o + 376, static_cast<u8>(on));
}

// void f(T* o; f32 a, f32 b): when both exceed *0x002CF49C and o->+8 has
// bit 8 of +192 and bit 0 of its u64 +184: Obj_Call563000(the player of
// its level (s8 +172): 0x010B19E0 + 11968 * u8 lvl->+4209 + 176; a, b)
DECOMP(002CF4A0, Obj_CallPlayer_002CF4A0) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = farg(ctx, 0), b = farg(ctx, 1);
    ctx.fpr[1] = b;
    ctx.fpr[2] = a;
    const f64 k = LfsAt(0x002CF49C);
    if (a <= k || b <= k) return;
    const u32 t = load<u32>(o + 8);
    ctx.gpr[3] = t;
    if (t == 0) return;
    if ((load<u32>(t + 192) & 8u) == 0) return;
    if ((load<u64>(t + 184) & 1u) == 0) return;
    const s32 lv = static_cast<s8>(load<u8>(t + 172));
    const u32 l = load<u32>(0x00F65CA0) + static_cast<u32>(lv) * 0x20C80u + 4208;
    const u32 p = 0x010B19E0u + load<u8>(l + 1) * 11968u + 176;
    callf(ctx, 0x00464488, {p}, {a, b});
}

// T::~T() deleting (size 124): vtable 0x00B1DFF0, vector +64 cleared,
// vtable 0x00B1DCA8, the effect +40 destroyed (Effect_Destruct, SmallFree
// 140) and cleared, vector +44 cleared, Dtor_B219D0_00464AD0, SmallFree(o,
// 124)
DECOMP(0031873C, Dtor_B1DFF0_0031873C) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    store<u32>(o, 0x00B1DFF0);
    call(ctx, o31fn::PtrVector_Clear, {o + 64});
    call(ctx, o31fn::Nop_54EC18, {o + 64});
    const u32 e = load<u32>(o + 40);
    store<u32>(o, 0x00B1DCA8);
    if (e != 0) {
        call(ctx, 0x004C8E80, {e});
        call(ctx, o19fn::SmallFree, {e, 140});
        store<u32>(o + 40, 0);
    }
    call(ctx, o31fn::PtrVector_Clear, {o + 44});
    call(ctx, o31fn::Nop_54EC18, {o + 44});
    call(ctx, 0x00464AD0, {o});
    ctx.gpr[3] = call(ctx, o19fn::SmallFree, {o, 124});
}

// T* f(Section* o, u32 k): in mode 4 (u8 +18), the first child c (list
// +2576, next +2564) in mode 4 whose vt[76](c, k) is set gives that (asked
// again); else o->+2292[k]
DECOMP(0032801C, Section_Lookup_0032801C) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1);
    if (load<u8>(o + 18) == 4) {
        for (u32 c = load<u32>(o + 2576); c != 0; c = load<u32>(c + 2564)) {
            if (load<u8>(c + 18) != 4) continue;
            if (static_cast<u32>(VCall(ctx, c, 76, {c, k})) != 0) {
                ctx.gpr[3] = VCall(ctx, c, 76, {c, k});
                return;
            }
        }
    }
    ctx.gpr[3] = load<u32>(o + 2292 + (k << 2));
}

// u32 f(Section* o, f32* out): the head child o->+2576's vt[84](head, out),
// asked once per child (the original re-reads the head each time); else,
// while the cursor +2540 < +2536 in the 8-byte table +2544: the word at
// the cursor (out = f32 +2552 when given); else 0
DECOMP(0032820C, Section_NextKey_0032820C) {
    const u32 o = arg<u32>(ctx, 0), out = arg<u32>(ctx, 1);
    u32 head = load<u32>(o + 2576);
    for (u32 c = head; c != 0;) {
        const u32 r = static_cast<u32>(VCall(ctx, head, 84, {head, out}));
        if (r != 0) {
            ctx.gpr[3] = r;
            return;
        }
        c = load<u32>(c + 2564);
        head = load<u32>(o + 2576);
    }
    ctx.gpr[3] = 0;
    u32 t = load<u32>(o + 2544);
    if (t == 0) return;
    u32 i = load<u32>(o + 2540);
    if (!(static_cast<s32>(i) < static_cast<s32>(load<u32>(o + 2536)))) return;
    if (out != 0) {
        ctx.fpr[1] = LfsAt(o + 2552);
        CopyW31(out, o + 2552);
        t = load<u32>(o + 2544);
        i = load<u32>(o + 2540);
    }
    ctx.gpr[3] = load<u32>(t + (i << 3));
}

// void f(Section* o, u32 m, u8 force): unless m is the current mode (u8
// +18) and !force: leaving mode 4 Obj_Flag13Off(o, 0); entering 4
// 0x0048FA54(o, o->+2632, 0) when set, else o->vt[28](o, 0); then
// Section_SetModeBase(o, m, force)
DECOMP(0033FF5C, Section_SetMode_0033FF5C) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), f = arg<u32>(ctx, 2) & 0xFFu;
    if (!(f == 0 && m == load<u8>(o + 18))) {
        if (load<u8>(o + 18) == 4) call(ctx, 0x0048F298, {o, 0});
        if (m == 4) {
            const u32 x = load<u32>(o + 2632);
            if (x != 0) call(ctx, 0x0048FA54, {o, x, 0});
        } else {
            VCall(ctx, o, 28, {o, 0});
        }
    }
    ctx.gpr[3] = call(ctx, 0x003260F0, {o, m, f});
}

// void f(T* o): unless state (+160 >> 1) is set: o->+24->vt[40](c, 1, 1)
// when set, Obj_Attach364408(o, o->+256) when set, state = 1,
// Speaker_SayId4A5C90(o->+28, o->+192, 0; *0x003645AC) and
// Effect_SetHash4CF988(o->+20, 0x04296E4C) when set
DECOMP(003645B0, Talk_Start_003645B0) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 s = load<u32>(o + 160) >> 1;
    ctx.gpr[3] = s;
    if (s != 0) return;
    const u32 c = load<u32>(o + 24);
    if (c != 0) VCall(ctx, c, 40, {c, 1, 1});
    const u32 a = load<u32>(o + 256);
    if (a != 0) call(ctx, 0x00364408, {o, a});
    const u32 w = (load<u32>(o + 160) & 1u) | 2u;
    const u32 sp = load<u32>(o + 28);
    store<u32>(o + 160, w);
    callf(ctx, 0x004A5C90, {sp, load<u32>(o + 192), 0}, {LfsAt(0x003645AC)});
    const u32 t = load<u32>(o + 20);
    if (t != 0) ctx.gpr[3] = call(ctx, 0x004CF988, {t, 0x04296E4Cu});
}

// void f(?, x): 0x0045717C(*0x00F65CA0, x), or, when 0x01089D34's u8 +16
// has bit 0x80, for its three objects (+4 / +8 / +12, each's +672):
// 0x0045717C for the first two, then Call_00548508(c), Flush_00429C04(),
// Sync_00548550(c, 0) for the third
DECOMP(003DD974, Net_Send_003DD974) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 s = load<u32>(0x01089D34);
    if (s == 0 || (load<u8>(s + 16) & 0x80u) == 0) {
        ctx.gpr[3] = call(ctx, 0x0045717C, {load<u32>(0x00F65CA0), x});
        return;
    }
    const u32 a = load<u32>(s + 4), b = load<u32>(s + 8), c = load<u32>(s + 12);
    const u32 b6 = load<u32>(b + 672), a6 = load<u32>(a + 672);
    const u32 g = load<u32>(0x00F65CA0);
    const u32 c6 = load<u32>(c + 672);
    call(ctx, 0x0045717C, {g, a6});
    call(ctx, 0x0045717C, {load<u32>(0x00F65CA0), b6});
    call(ctx, 0x00548508, {c6});
    call(ctx, 0x00429C04);
    ctx.gpr[3] = call(ctx, 0x00548550, {c6, 0});
}

// void f(T* o): for each child e = o->+112[i] (u16 count +120) with u8
// +188 set, 0x003EE1F0 on each word of its list +164 (u16 count +172).
// r3 = the count +120.
DECOMP(003F0448, Obj_UpdateChildLists_003F0448) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 120);
        ctx.gpr[3] = n;
        if (i >= n) return;
        if (load<u8>(load<u32>(load<u32>(o + 112) + 4 * i) + 188) == 0) continue;
        for (u32 j = 0;; ++j) {
            const u32 l = load<u32>(load<u32>(o + 112) + 4 * i) + 164;
            if (j >= load<u16>(l + 8)) break;
            call(ctx, 0x003EE1F0, {load<u32>(load<u32>(l) + 4 * j)});
        }
    }
}

// bool f(a, b, T* n): the kind-5 component c = Effect_FindComponent(a, b)
// (c->vt[32]()) has its +28 replaced by n (the old one gets vt[8](), n's
// u16 +4 is counted up). r3 = 1, else 0.
DECOMP(004A65F0, Effect_SetRef_004A65F0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), n = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (a == 0) return;
    const u32 c = static_cast<u32>(call(ctx, 0x004BB4B4, {a, b}));
    ctx.gpr[3] = 0;
    if (c == 0 || static_cast<u32>(VCall(ctx, c, 32, {c})) != 5) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 old = load<u32>(c + 28);
    if (old != 0) VCall(ctx, old, 8, {old});
    store<u32>(c + 28, n);
    if (n != 0) store<u16>(n + 4, static_cast<u16>(load<u16>(n + 4) + 1));
    ctx.gpr[3] = 1;
}

// u32 f(T* o, u32 key): finds key in the binary tree o->+16->+0 (node key
// +4, value +8, +12 for keys below the node's, unsigned), recording the
// path in the s16-counted stack o->+20[] (count u16 +100); +104 = the node
// (or the last visited, null for an empty tree), 0x011DE288 = its value
// (found / empty only). r3 = the value, else 0.
DECOMP(0074BA80, Tree_Find_0074BA80) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1);
    u32 node = load<u32>(load<u32>(o + 16));
    ctx.gpr[3] = 0;
    if (node == 0) {
        store<u32>(o + 104, 0);
        store<u32>(0x011DE288, 0);
        return;
    }
    store<u16>(o + 100, 0);
    u32 cur;
    for (;;) {
        cur = node;
        const u32 k = load<u32>(cur + 4);
        if (k == key) {
            store<u32>(o + 104, cur);
            store<u32>(0x011DE288, load<u32>(cur + 8));
            const u32 t = load<u32>(o + 104);
            if (t != 0) ctx.gpr[3] = load<u32>(t + 8);
            return;
        }
        node = k > key ? load<u32>(cur + 12) : load<u32>(cur + 16);
        if (node == 0) break;
        const s32 n = static_cast<s16>(load<u16>(o + 100));
        store<u16>(o + 100, static_cast<u16>(n + 1));
        store<u32>(o + 20 + 4 * static_cast<u32>(n), cur);
    }
    if (cur == 0) store<u16>(o + 100, 0);  // dead: cur is a visited node
    store<u32>(o + 104, cur);
}

// T::~T(T* o): vtable 0x00B1F860; the blocks +4 of the o->+152 16-byte
// records at +148 and the array +148 itself Mem_FreeFrom'd and cleared;
// base vtable 0x00B1F668 (r3 = it)
DECOMP(00772D9C, Dtor_B1F860_00772D9C) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1F860);
    u32 n = load<u32>(o + 152);
    for (u32 i = 0; i < n; ++i) {
        const u32 p = load<u32>(o + 148) + 16 * i;
        const u32 v = load<u32>(p + 4);
        if (v == 0) continue;
        call(ctx, 0x0054EE6C, {v, 0});
        store<u32>(p + 4, 0);
        n = load<u32>(o + 152);
    }
    const u32 a = load<u32>(o + 148);
    if (a != 0) {
        call(ctx, 0x0054EE6C, {a, 0});
        store<u32>(o + 148, 0);
    }
    store<u32>(o, 0x00B1F668);
    ctx.gpr[3] = 0x00B1F668;
}

// void f(T* o, u8 on): u8 +54 = on; with o->+296: c->vt[88](c, !on, 1, 1,
// 1) and, for u8 +355, World_AddTimedCallback(*0x00F65CA0, 0, 0x00F38648,
// o, 0)
DECOMP(0077EF68, Obj_SetVisible_0077EF68) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu;
    const u32 c = load<u32>(o + 296);
    store<u8>(o + 54, static_cast<u8>(on));
    ctx.gpr[3] = 0;
    if (c == 0) return;
    VCall(ctx, c, 88, {c, on == 0 ? 1u : 0u, 1, 1, 1});
    ctx.gpr[3] = load<u8>(o + 355);
    if (load<u8>(o + 355) == 0) return;
    ctx.gpr[3] = call(ctx, 0x0046B5F8, {load<u32>(0x00F65CA0), 0, 0x00F38648u, o, 0});
}

// T::~T() deleting: vtable 0x00B220D8, the buffer +40 (u16 +32 * (u16 +38
// >> 1) bytes) freed through the allocator +44 (vt[8]), Array_Destruct(o +
// 32), vector +12 cleared, vtable 0x00B11B20, Mem_FreeFrom(o, 0)
DECOMP(00794324, Dtor_B220D8_00794324) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) return;
    const u32 sz = load<u16>(o + 32) * ((load<u16>(o + 38) >> 1) & 0x7FFFu);
    store<u32>(o, 0x00B220D8);
    const u32 buf = load<u32>(o + 40);
    if (buf != 0) {
        const u32 a = load<u32>(o + 44);
        VCall(ctx, a, 8, {a, buf, sz});
    }
    call(ctx, 0x0054EC94, {o + 32});
    call(ctx, o31fn::PtrVector_Clear, {o + 12});
    call(ctx, o31fn::Nop_54EC18, {o + 12});
    store<u32>(o, 0x00B11B20);
    ctx.gpr[3] = call(ctx, 0x0054EE6C, {o, 0});
}

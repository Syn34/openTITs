// The renderer range's other short functions: two-call sequences, flag
// setters, clamps into the render globals, a 12-byte table search and a
// few slot wrappers. Names carry the address where the role is unclear.
#include <bit>

#include "gfx7.h"

using namespace sly4;

namespace {

constexpr u32 kConfig = 0x010C5A20;     // sly4::g::config
constexpr u32 kMemStats = 0x010F0E20;   // render::g::mem_stats: +168 / +172 f32 clamps, +184 u32 mode
constexpr u32 kGcmState = 0x010F10A0;   // fx::g::state (+0 the gcm context)
constexpr u32 kPool = 0x010DF8B0;       // layers3::g::pool (+312 shade intensity)
constexpr u32 kName010DDB40 = 0x010DDB40;  // char[64] (name?)
constexpr u32 kFlag010DDB84 = 0x010DDB84;  // u8: 00546EF4's once-flag (name?)
constexpr u32 kGlobal00F66FBC = 0x00F66FBC;  // u32 (name?)
constexpr u32 kGlobal00F66FD0 = 0x00F66FD0;  // u32: the fur shaders' light record (shader6_draw.cpp kFurLight)
constexpr u32 kFlag00F671DC = 0x00F671DC;    // u8 (name?)
constexpr u32 kFlag01163F80 = 0x01163F80;    // u8 (name?)
constexpr u32 kGlobals01164060 = 0x01164060; // u32 words +0 / +4 / +16 / +20 / +28 copied by 00543C04 (name?)
constexpr u32 kMode011640E4 = 0x011640E4;    // u32 (name?)
constexpr u32 kArg011640E8 = 0x011640E8;     // u32 (pool::query::g::report_base)
constexpr u32 kSkinMatrices = 0x01151A80;    // draw3::g::skin_matrices
constexpr u32 kAngle010DFA00 = 0x010DFA00;   // f32 (name?)

constexpr u32 kfn_String_Copy = 0x00553684;
constexpr u32 kfn_sys_ppu_thread_get_id = 0x00ABF1E4;
constexpr u32 kfn_memalign = 0x0072A88C;
constexpr u32 kfn_Shader_LoadGroup = 0x0050F0F0;
constexpr u32 kfn_Math_Cosf = 0x007344E0;
constexpr u32 kfn_Iff_RegisterChunk = 0x0055902C;
constexpr u32 kfn_MaterialAnim_UploadUV = 0x0050D124;
constexpr u32 kfn_Ctor_00548D14 = 0x00548D14;
constexpr u32 kfn_Copy416To512 = 0x0052AA48;
constexpr u32 kfn_True_0053B650 = 0x0053B650;
constexpr u32 kfn_Nop_0051FF18 = 0x0051FF18;

f64 lfs(u32 a) { return static_cast<f64>(load<f32>(a)); }

// f1 clamped to [lo, hi] (k: f32 lo, hi) the way the clamps branch: lo
// when lo > v, else hi when hi < v, else v (NaN stays).
f64 clamp_branch(f64 v, u32 k) {
    const f64 lo = lfs(k), hi = lfs(k + 4);
    if (lo > v) return lo;
    if (hi < v) return hi;
    return v;
}

}  // namespace

// void (this): this->u8 +48 ? 005170B8(this) : 00518E78(this)
DECOMP(0051A6A0, Call_0051A6A0) {
    const u32 flag = load<u8>(arg<u32>(ctx, 0) + 48);
    ctx.gpr[4] = flag;  // still in r4 at either call
    call(ctx, flag != 0 ? 0x005170B8u : 0x00518E78u);
}

// void (const char* name): String_Copy(0x010DDB40, name, 64)
DECOMP(0051E7F0, SetName_0051E7F0) { call(ctx, kfn_String_Copy, {kName010DDB40, ctx.gpr[3], 64}); }

// void (): 005325D8(0), then 0053B668(*(u32*)0x00F66FBC, 1)
DECOMP(0053BBE0, Call_0053BBE0) {
    call(ctx, 0x005325D8u, {0});
    call(ctx, 0x0053B668u, {load<u32>(kGlobal00F66FBC), 1});
}

// u64 (): sys_ppu_thread_get_id into a zeroed stack u64, returned
DECOMP(005DB070, ThreadId_005DB070) {
    Stack id(ctx, 8);  // 112(r1) of a 128-byte frame
    store<u64>(id.addr(), 0);
    ctx.gpr[4] = 0;
    call(ctx, kfn_sys_ppu_thread_get_id, {id.addr()});
    ctx.gpr[3] = load<u64>(id.addr());
}

// void (this, const f32 v[3]): this->+48..+56 = v * 0.5 (k at 0x0079F808)
DECOMP(0079F810, SetHalf_0079F810) {
    const u32 self = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const f64 k = lfs(0x0079F808u), vz = lfs(v + 8);
    const f32 x = static_cast<f32>(lfs(v) * k), y = static_cast<f32>(lfs(v + 4) * k), z = static_cast<f32>(vz * k);
    store<f32>(self + 48, x);
    store<f32>(self + 52, y);
    store<f32>(self + 56, z);
    ctx.gpr[5] = 0x0079F808u;
    ctx.fpr[1] = x;
    ctx.fpr[2] = z;
    ctx.fpr[3] = y;
    ctx.fpr[4] = vz;
}

// void (this, const f32 v[3]): this->+48..+56 = v * 0.5 (k at 0x0079FE20)
DECOMP(0079FE28, SetHalf_0079FE28) {
    const u32 self = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    const f64 k = lfs(0x0079FE20u), vz = lfs(v + 8);
    const f32 x = static_cast<f32>(lfs(v) * k), y = static_cast<f32>(lfs(v + 4) * k), z = static_cast<f32>(vz * k);
    store<f32>(self + 48, x);
    store<f32>(self + 52, y);
    store<f32>(self + 56, z);
    ctx.gpr[5] = 0x0079FE20u;
    ctx.fpr[1] = x;
    ctx.fpr[2] = z;
    ctx.fpr[3] = y;
    ctx.fpr[4] = vz;
}

// void* FiosAllocator_Alloc(this, u32 size, u32 align_log2): memalign(1 << (align_log2 & 31), size)
DECOMP(007A5F20, FiosAllocator_Alloc) {
    const u32 align = 1u << (ctx.gpr[5] & 31);  // rlwinm 0,27,31 / slw
    ctx.gpr[5] = 1;
    call(ctx, kfn_memalign, {align});  // r4 (the size) passed through
}

// void (s32 which): Shader_LoadGroup(which == 1 ? 4 : 3)
DECOMP(00523AA8, LoadShaders_00523AA8) { call(ctx, kfn_Shader_LoadGroup, {arg<s32>(ctx, 0) == 1 ? 4u : 3u}); }

// u32 (bool on): the u8 at 0x00F671DC = on && config +35 (returned)
DECOMP(00536958, SetFlag_00536958) {
    u32 r = 0;
    if ((arg<u32>(ctx, 0) & 0xFF) != 0 && load<u8>(kConfig + 35) != 0) r = 1;
    store<u8>(kFlag00F671DC, static_cast<u8>(r));
    ctx.gpr[4] = 0x00F60000u;
    ret(ctx, r);
}

// void (): 00527D34(), then 005E31B8(gcm context, 0)
DECOMP(00548900, Call_00548900) {
    call(ctx, 0x00527D34u);
    call(ctx, 0x005E31B8u, {load<u32>(kGcmState), 0});
}

// void (): 00527DAC(mode) unless mem_stats +184 (mode) == 2
DECOMP(00548B88, Call_00548B88) {
    const u32 mode = load<u32>(kMemStats + 184);
    ctx.gpr[3] = mode;
    if (static_cast<s32>(mode) == 2) return;
    call(ctx, 0x00527DACu, {mode});
}

// void (this): 0052BB60(this), then 00549410(this)
DECOMP(00549594, Call_00549594) {
    const u64 self = ctx.gpr[3];
    call(ctx, 0x0052BB60u, {self});
    call(ctx, 0x00549410u, {self});
}

// void (this): 0060DDF0(this->+11976, this + 128, this->+11980)
DECOMP(007A6840, Call_007A6840) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 b = load<u32>(self + 11980);
    ctx.gpr[6] = b;
    call(ctx, 0x0060DDF0u, {load<u32>(self + 11976), static_cast<u32>(ctx.gpr[3] + 128), b});
}

// u32 (): ++config +72, True_0053B650(), returns 1
DECOMP(0050FDC8, Count_0050FDC8) {
    const u64 n = static_cast<u64>(load<u32>(kConfig + 72)) + 1;
    store<u32>(kConfig + 72, static_cast<u32>(n));
    ctx.gpr[3] = kConfig;
    ctx.gpr[4] = n;
    call(ctx, kfn_True_0053B650);
    ret(ctx, 1u);
}

// void (f32 degrees): *(f32*)0x010DFA00 = Math_Cosf(degrees * pi/180)
DECOMP(005275C0, SetCos_005275C0) {
    const f64 rad = static_cast<f64>(static_cast<f32>(farg(ctx, 0) * lfs(0x005275BCu)));  // fmuls
    ctx.gpr[3] = 0x005275BCu;
    ctx.fpr[2] = lfs(0x005275BCu);
    const f64 c = callf(ctx, kfn_Math_Cosf, {}, {rad});
    store<f32>(kAngle010DFA00, static_cast<f32>(c));
    ctx.gpr[3] = 0x010E0000u;
}

// Entry* (Entry* table, u32 key, u32 kind): the first 12-byte entry {+0
// key, +4 nonzero (0 ends the table), +9 u8 kind} matching both, or 0
DECOMP(00537B7C, FindEntry_00537B7C) {
    u64 p = ctx.gpr[3];
    const u32 key = arg<u32>(ctx, 1), kind = arg<u32>(ctx, 2);
    for (;;) {
        const u32 e = static_cast<u32>(p);
        u32 r6 = load<u32>(e + 4);
        if (r6 == 0) {
            ctx.gpr[6] = 0;
            return ret(ctx, 0u);
        }
        r6 = load<u32>(e);
        if (r6 == key) {  // cmpw
            r6 = load<u8>(e + 9);
            if (r6 == kind) {
                ctx.gpr[6] = r6;
                ctx.gpr[3] = p;
                return;
            }
        }
        ctx.gpr[6] = r6;
        p = static_cast<u32>(e + 12);
    }
}

// void (this): 00527ACC(this), then 00527A70(this)
DECOMP(00548938, Call_00548938) {
    const u64 self = ctx.gpr[3];
    call(ctx, 0x00527ACCu, {self});
    call(ctx, 0x00527A70u, {self});
}

// void (this): 0060DC8C(this->+11972, this->+11984, this + 10752, 512)
DECOMP(007A6804, Call_007A6804) {
    const u32 self = arg<u32>(ctx, 0);
    const u32 b = load<u32>(self + 11984);
    ctx.gpr[7] = b;
    call(ctx, 0x0060DC8Cu, {load<u32>(self + 11972), b, static_cast<u32>(ctx.gpr[3] + 10752), 512});
}

// bool (): flag 0x01163F80 set, config +34 clear and config u16 +6 <= 720
DECOMP(005319CC, Check_005319CC) {
    u32 r = 0;
    if (load<u8>(kFlag01163F80) != 0) {
        const u32 c34 = load<u8>(kConfig + 34);
        ctx.gpr[5] = c34;
        if (c34 == 0 && static_cast<s32>(load<u16>(kConfig + 6)) <= 720) r = 1;  // cmpwi
    }
    ctx.gpr[4] = r;
    ret(ctx, r);
}

// void (this): this->+152 / +144 / +156 / +148 / +140 = the words at
// 0x01164060 +0 / +4 / +16 / +20 / +28
DECOMP(00543C04, CopyGlobals_00543C04) {
    const u32 self = arg<u32>(ctx, 0);
    store<u32>(self + 152, load<u32>(kGlobals01164060));
    store<u32>(self + 144, load<u32>(kGlobals01164060 + 4));
    store<u32>(self + 156, load<u32>(kGlobals01164060 + 16));
    store<u32>(self + 148, load<u32>(kGlobals01164060 + 20));
    const u32 last = load<u32>(kGlobals01164060 + 28);
    store<u32>(self + 140, last);
    ctx.gpr[4] = last;
    for (int r = 5; r <= 8; ++r) ctx.gpr[r] = 0x01160000u;
}

// void (): Iff_RegisterChunk('GDEM', OPD 0x00F3BEB0, 0, 0, 0)
DECOMP(00548BC0, RegisterGDEM) { call(ctx, kfn_Iff_RegisterChunk, {0x4744454Du, 0x00F3BEB0u, 0, 0, 0}); }

// void (): Iff_RegisterChunk('KLBM', OPD 0x00F3BE70, 0, 0, 0)
DECOMP(00548CD4, RegisterKLBM) { call(ctx, kfn_Iff_RegisterChunk, {0x4B4C424Du, 0x00F3BE70u, 0, 0, 0}); }

// void (): config +81 = 1 around 0053C4A0()
DECOMP(0051096C, Call_0051096C) {
    store<u8>(kConfig + 81, 1);
    ctx.gpr[3] = 0x010C0000u;
    ctx.gpr[4] = 1;
    call(ctx, 0x0053C4A0u);
    store<u8>(kConfig + 81, 0);
    ret(ctx, 0u);
}

// void (this, x, const f32 v[2]): 00522838(this, x, {v[0], v[1], 0, this->+60})
DECOMP(0052296C, Call_0052296C) {
    const u32 self = arg<u32>(ctx, 0), v = arg<u32>(ctx, 2);
    Stack q(ctx, 16);  // 112(r1) of a 128-byte frame
    const u32 x = load<u32>(v), y = load<u32>(v + 4), w = load<u32>(self + 60);  // lfs / stfs: raw words
    store<u32>(q.addr(), x);
    store<u32>(q.addr() + 4, y);
    store<u32>(q.addr() + 8, 0);
    store<u32>(q.addr() + 12, w);
    ctx.gpr[6] = 0;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(x));
    ctx.fpr[2] = static_cast<f64>(std::bit_cast<f32>(y));
    ctx.fpr[3] = static_cast<f64>(std::bit_cast<f32>(w));
    call(ctx, 0x00522838u, {ctx.gpr[3], ctx.gpr[4], q.addr()});
}

// void (Light* l): *(u32*)0x00F66FD0 = l, then l's virtual slot 3 (+12)
DECOMP(00527668, SetLight_00527668) {
    const u32 l = arg<u32>(ctx, 0);
    store<u32>(kGlobal00F66FD0, l);
    fx::vcall(ctx, l, 12, {ctx.gpr[3]});
}

// void (this): Ctor_00548D14(this), +1536 = 1.0f, vtable 0x00B23D28
DECOMP(0052D08C, Ctor_0052D08C) {
    const u32 self = arg<u32>(ctx, 0);
    call(ctx, kfn_Ctor_00548D14, {ctx.gpr[3]});
    store<u32>(self + 1536, 0x3F800000u);
    store<u32>(self, 0x00B23D28u);
    ctx.gpr[3] = 0x00B23D28u;
    ctx.gpr[4] = 0x3F800000u;
}

// void (f32 v): mem_stats +168 = clamp(v, 0.1, 150)
DECOMP(00536998, SetClamp168_00536998) {
    const f64 v = clamp_branch(farg(ctx, 0), 0x00536990u);
    store<f32>(kMemStats + 168, static_cast<f32>(v));
    ctx.fpr[1] = v;
    ctx.fpr[2] = lfs(0x00536994u);
    ctx.fpr[3] = lfs(0x00536990u);
    ctx.gpr[3] = kMemStats;
}

// void (f32 v): mem_stats +172 = clamp(v, 0.01, 0.1)
DECOMP(005369E8, SetClamp172_005369E8) {
    const f64 v = clamp_branch(farg(ctx, 0), 0x005369DCu);
    store<f32>(kMemStats + 172, static_cast<f32>(v));
    ctx.fpr[1] = v;
    ctx.fpr[2] = lfs(0x005369E0u);
    ctx.fpr[3] = lfs(0x005369DCu);
    ctx.gpr[3] = kMemStats;
}

// void (this): x = this->+672; 00521D20(x), then 005380D0(skin_matrices, x)
DECOMP(00548508, Call_00548508) {
    const u32 x = load<u32>(arg<u32>(ctx, 0) + 672);
    call(ctx, 0x00521D20u, {x});
    call(ctx, 0x005380D0u, {kSkinMatrices, x});
}

// void (): 00527BA8(), then 0056C8A4(*(u32*)0x011640E8) when *(u32*)0x011640E4 == 1
DECOMP(005489CC, Call_005489CC) {
    call(ctx, 0x00527BA8u);
    const u32 mode = load<u32>(kMode011640E4);
    ctx.gpr[3] = mode;
    if (static_cast<s32>(mode) != 1) return;
    call(ctx, 0x0056C8A4u, {load<u32>(kArg011640E8)});
}

// void (this): Copy416To512_0052AA48(this), then u32 +768 / +896 / +1024 / +1152 = 0
DECOMP(005493B0, Reset_005493B0) {
    const u32 self = arg<u32>(ctx, 0);
    call(ctx, kfn_Copy416To512, {ctx.gpr[3]});
    for (u32 off = 768; off <= 1152; off += 128) store<u32>(self + off, 0);
    ret(ctx, 0u);
}

// void (this, Material* m): MaterialAnim_UploadUV(m->+128, 4, 1.0, 1.0) when m->+128 is set
DECOMP(005D6578, UploadUV_005D6578) {
    const u32 anim = load<u32>(arg<u32>(ctx, 1) + 128);
    ctx.gpr[3] = anim;
    if (anim == 0) return;
    const f64 one = lfs(0x005D6570u);
    ctx.gpr[5] = 0x005D6570u;
    callf(ctx, kfn_MaterialAnim_UploadUV, {anim, 4}, {one, one});
}

// void (this): when set, its virtual slot 1 (+4)
DECOMP(00508054, VDelete_00508054) {
    const u64 self = ctx.gpr[3];
    ctx.gpr[4] = self;
    if (static_cast<u32>(self) == 0) return;
    fx::vcall(ctx, static_cast<u32>(self), 4, {self});
}

// void (f32 v): pool +312 (shade intensity) = clamp(v, 0, 1)
DECOMP(00527608, SetShadeIntensity_00527608) {
    const f64 v = clamp_branch(farg(ctx, 0), 0x00527600u);
    store<f32>(kPool + 312, static_cast<f32>(v));
    ctx.fpr[1] = lfs(0x00527604u);
    ctx.fpr[2] = v;
    ctx.fpr[3] = farg(ctx, 0);
    ctx.gpr[3] = kPool;
}

// void (): once (flag 0x010DDB84): Nop_0051FF18(), 005467DC(0), flag = 1
DECOMP(00546EF4, Render_Init546EF4) {
    const u32 done = load<u8>(kFlag010DDB84);
    ctx.gpr[3] = done;
    if (done != 0) return;
    call(ctx, kfn_Nop_0051FF18);
    call(ctx, 0x005467DCu, {0});
    store<u8>(kFlag010DDB84, 1);
    ctx.gpr[3] = 0x010E0000u;
    ctx.gpr[4] = 1;
}

// u32 (this, Item* item, Mat* m): item->+16->+12[m->+28->u8 +100 bit 7 && m->+44 ? 1 : 0]
DECOMP(005D7D9C, Pick_005D7D9C) {
    const u32 item = arg<u32>(ctx, 1), m = arg<u32>(ctx, 2);
    const u32 flag = (load<u8>(load<u32>(m + 28) + 100) >> 7) & 1;  // rlwinm 25,31,31
    const u32 table = load<u32>(load<u32>(item + 16) + 12);
    u32 k = 0;
    if (flag != 0) {
        const u32 e = load<u32>(m + 44);
        ctx.gpr[5] = e;
        if (e != 0) k = 1;
    }
    ctx.gpr[4] = k << 2;
    ctx.gpr[6] = flag;
    ret(ctx, load<u32>(table + (k << 2)));
}

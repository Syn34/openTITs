// The renderer range's other short functions: two-call sequences, flag
// setters, clamps into the render globals, a 12-byte table search and a
// few slot wrappers. Names carry the address where the role is unclear.
#include <bit>

#include "gfx7.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

constexpr u32 kPool = 0x010DF8B0;          // layers3::g::pool (+312 shade intensity)
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

constexpr u32 kfn_sys_ppu_thread_get_id = 0x00ABF1E4;
constexpr u32 kfn_memalign = 0x0072A88C;
constexpr u32 kfn_Math_Cosf = 0x007344E0;
constexpr u32 kfn_Iff_RegisterChunk = 0x0055902C;
constexpr u32 kfn_Ctor_00548D14 = 0x00548D14;
constexpr u32 kfn_Copy416To512 = 0x0052AA48;
constexpr u32 kfn_True_0053B650 = 0x0053B650;
constexpr u32 kfn_Nop_0051FF18 = 0x0051FF18;

be_u32& word(const void* p, u32 off) { return *at<be_u32>(p, off); }
u8* cfg() { return reinterpret_cast<u8*>(&config()); }

// v clamped to [lo, hi] (k: f32 lo, hi) the way the clamps branch: lo
// when lo > v, else hi when hi < v, else v (NaN stays).
f32 clamp_branch(f32 v, u32 k) {
    const f32 lo = global<be_f32>(k), hi = global<be_f32>(k + 4);
    if (lo > v) return lo;
    if (hi < v) return hi;
    return v;
}

// this->+48..+56 = v * k (k: the f32 at `k`).
void set_half(u8* self, const be_f32* v, u32 k) {
    const f32 h = global<be_f32>(k);
    const f32 x = v[0] * h, y = v[1] * h, z = v[2] * h;
    *at<be_f32>(self, 48) = x;
    *at<be_f32>(self, 52) = y;
    *at<be_f32>(self, 56) = z;
}

}  // namespace

// void (this): this->u8 +48 ? 005170B8(this) : 00518E78(this)
void Call_0051A6A0(u8* self) { call(*at<u8>(self, 48) != 0 ? 0x005170B8u : 0x00518E78u, self); }

// void (const char* name): String_Copy(0x010DDB40, name, 64)
void SetName_0051E7F0(const char* name) { call(sly4::fn::String_Copy, kName010DDB40, name, 64u); }

// void (): 005325D8(0), then 0053B668(*(u32*)0x00F66FBC, 1)
void Call_0053BBE0() {
    call(0x005325D8u, 0u);
    call(0x0053B668u, global<be_u32>(kGlobal00F66FBC).get(), 1u);
}

// u64 (): sys_ppu_thread_get_id into a zeroed stack u64, returned
u64 ThreadId_005DB070() {
    GuestFrame id(8);  // 112(r1) of a 128-byte frame
    be_u64* v = id.as<be_u64>();
    *v = u64{0};
    call(kfn_sys_ppu_thread_get_id, v);
    return *v;
}

// void (this, const f32 v[3]): this->+48..+56 = v * 0.5 (k at 0x0079F808)
void SetHalf_0079F810(u8* self, const be_f32* v) { set_half(self, v, 0x0079F808u); }

// void (this, const f32 v[3]): this->+48..+56 = v * 0.5 (k at 0x0079FE20)
void SetHalf_0079FE28(u8* self, const be_f32* v) { set_half(self, v, 0x0079FE20u); }

// void* FiosAllocator_Alloc(this, u32 size, u32 align_log2): memalign(1 << (align_log2 & 31), size)
u32 FiosAllocator_Alloc(void*, u32 size, u32 align_log2) {
    return call<u32>(kfn_memalign, 1u << (align_log2 & 31), size);  // rlwinm 0,27,31 / slw
}

// Material* (s32 which): Shader_LoadGroup(which == 1 ? 4 : 3)
Material* LoadShaders_00523AA8(s32 which) { return Shader_LoadGroup(which == 1 ? 4 : 3); }

// u32 (bool on): the u8 at 0x00F671DC = on && config +35 (returned)
u32 SetFlag_00536958(u32 on) {
    const u32 r = (on & 0xFF) != 0 && config().stereo_ok != 0 ? 1u : 0u;
    global<u8>(kFlag00F671DC) = static_cast<u8>(r);
    return r;
}

// void (): 00527D34(), then 005E31B8(gcm context, 0)
void Call_00548900() {
    call(0x00527D34u);
    call(0x005E31B8u, gcm(), 0u);
}

// void (): 00527DAC(mode) unless mem_stats +184 (mode) == 2
void Call_00548B88() {
    const u32 mode = mem_stats().buffer_select;
    if (static_cast<s32>(mode) == 2) return;
    call(0x00527DACu, mode);
}

// void (this): 0052BB60(this), then 00549410(this)
void Call_00549594(void* self) {
    call(0x0052BB60u, self);
    call(0x00549410u, self);
}

// void (this): 0060DDF0(this->+11976, this + 128, this->+11980)
void Call_007A6840(u8* self) {
    const u32 b = word(self, 11980);
    call(0x0060DDF0u, word(self, 11976).get(), at<u8>(self, 128), b);
}

// u32 (): ++config +72, True_0053B650(), returns 1
u32 Count_0050FDC8() {
    word(cfg(), 72) = word(cfg(), 72) + 1u;
    call(kfn_True_0053B650);
    return 1;
}

// void (f32 degrees): *(f32*)0x010DFA00 = Math_Cosf(degrees * pi/180)
void SetCos_005275C0(f32 degrees) {
    const f32 rad = degrees * global<be_f32>(0x005275BCu);  // fmuls
    global<be_f32>(kAngle010DFA00) = static_cast<f32>(call<f64>(kfn_Math_Cosf, rad));
}

// Entry* (Entry* table, u32 key, u32 kind): the first 12-byte entry {+0
// key, +4 nonzero (0 ends the table), +9 u8 kind} matching both, or 0
const u8* FindEntry_00537B7C(const u8* table, u32 key, u32 kind) {
    for (u32 e = guest(table);; e += 12) {
        const u8* p = at<u8>(nullptr, e);
        if (word(p, 4) == 0) return nullptr;
        if (word(p, 0) == key && p[9] == kind) return p;  // cmpw
    }
}

// void (this): 00527ACC(this), then 00527A70(this)
void Call_00548938(void* self) {
    call(0x00527ACCu, self);
    call(0x00527A70u, self);
}

// void (this): 0060DC8C(this->+11972, this->+11984, this + 10752, 512)
void Call_007A6804(u8* self) {
    const u32 b = word(self, 11984);
    call(0x0060DC8Cu, word(self, 11972).get(), b, at<u8>(self, 10752), 512u);
}

// bool (): flag 0x01163F80 set, config +34 clear and config u16 +6 <= 720
u32 Check_005319CC() {
    if (global<u8>(kFlag01163F80) == 0) return 0;
    return config().stereo == 0 && static_cast<s32>(config().height.get()) <= 720 ? 1u : 0u;  // cmpwi
}

// void (this): this->+152 / +144 / +156 / +148 / +140 = the words at
// 0x01164060 +0 / +4 / +16 / +20 / +28
void CopyGlobals_00543C04(u8* self) {
    const be_u32* g = host<be_u32>(kGlobals01164060);
    word(self, 152) = g[0].get();
    word(self, 144) = g[1].get();
    word(self, 156) = g[4].get();
    word(self, 148) = g[5].get();
    word(self, 140) = g[7].get();
}

// void (): Iff_RegisterChunk('GDEM', OPD 0x00F3BEB0, 0, 0, 0)
void RegisterGDEM() { call(kfn_Iff_RegisterChunk, 0x4744454Du, 0x00F3BEB0u, 0u, 0u, 0u); }

// void (): Iff_RegisterChunk('KLBM', OPD 0x00F3BE70, 0, 0, 0)
void RegisterKLBM() { call(kfn_Iff_RegisterChunk, 0x4B4C424Du, 0x00F3BE70u, 0u, 0u, 0u); }

// void (): config +81 = 1 around 0053C4A0()
void Call_0051096C() {
    cfg()[81] = 1;
    call(0x0053C4A0u);
    cfg()[81] = 0;
}

// void (this, x, const f32 v[2]): 00522838(this, x, {v[0], v[1], 0, this->+60})
void Call_0052296C(u8* self, u32 x, const be_u32* v) {
    GuestFrame q(16);  // 112(r1) of a 128-byte frame
    be_u32* w = q.as<be_u32>();
    const u32 a = v[0], b = v[1], c = word(self, 60);  // lfs / stfs: raw words
    w[0] = a;
    w[1] = b;
    w[2] = 0u;
    w[3] = c;
    call(0x00522838u, self, x, w);
}

// void (Light* l): *(u32*)0x00F66FD0 = l, then l's virtual slot 3 (+12)
void SetLight_00527668(u8* l) {
    global<gp<u8>>(kGlobal00F66FD0) = l;
    vcall(l, 12);
}

// void (this): Ctor_00548D14(this), +1536 = 1.0f, vtable 0x00B23D28
void Ctor_0052D08C(u8* self) {
    call(kfn_Ctor_00548D14, self);
    word(self, 1536) = 0x3F800000u;
    word(self, 0) = 0x00B23D28u;
}

// void (f32 v): mem_stats +168 = clamp(v, 0.1, 150)
void SetClamp168_00536998(f32 v) { *at<be_f32>(&mem_stats(), 168) = clamp_branch(v, 0x00536990u); }

// void (f32 v): mem_stats +172 = clamp(v, 0.01, 0.1)
void SetClamp172_005369E8(f32 v) { *at<be_f32>(&mem_stats(), 172) = clamp_branch(v, 0x005369DCu); }

// void (this): x = this->+672; View_BeginFrame(x), then 005380D0(skin_matrices, x)
// (00057178 reads the r7 this leaves: 005380D0's, which the bridge keeps
// since that guest call is the last thing done.)
void Call_00548508(u8* self) {
    const u32 x = word(self, 672);
    View_BeginFrame(host<u8>(x));
    call(0x005380D0u, kSkinMatrices, x);
}

// void (): 00527BA8(), then 0056C8A4(*(u32*)0x011640E8) when *(u32*)0x011640E4 == 1
void Call_005489CC() {
    call(0x00527BA8u);
    if (static_cast<s32>(global<be_u32>(kMode011640E4).get()) != 1) return;
    call(0x0056C8A4u, global<be_u32>(kArg011640E8).get());
}

// void (this): Copy416To512_0052AA48(this), then u32 +768 / +896 / +1024 / +1152 = 0
void Reset_005493B0(u8* self) {
    call(kfn_Copy416To512, self);
    for (u32 off = 768; off <= 1152; off += 128) word(self, off) = 0u;
}

// void (this, Material* m): MaterialAnim_UploadUV(m->+128, 4, 1.0, 1.0) when m->+128 is set
void UploadUV_005D6578(void*, Material* m) {
    const u32 anim = word(m, 128);
    if (anim == 0) return;
    const f32 one = global<be_f32>(0x005D6570u);
    MaterialAnim_UploadUV(anim, 4, one, one);
}

// void (this): when set, its virtual slot 1 (+4)
void VDelete_00508054(u8* self) {
    if (self) vcall(self, 4);
}

// void (f32 v): pool +312 (shade intensity) = clamp(v, 0, 1)
void SetShadeIntensity_00527608(f32 v) { global<be_f32>(kPool + 312) = clamp_branch(v, 0x00527600u); }

// void (): once (flag 0x010DDB84): Nop_0051FF18(), 005467DC(0), flag = 1
void Render_Init546EF4() {
    if (global<u8>(kFlag010DDB84) != 0) return;
    call(kfn_Nop_0051FF18);
    call(0x005467DCu, 0u);
    global<u8>(kFlag010DDB84) = 1;
}

// u32 (this, Item* item, Mat* m): item->+16->+12[m->+28->u8 +100 bit 7 && m->+44 ? 1 : 0]
u32 Pick_005D7D9C(void*, u8* item, u8* m) {
    const u32 flag = (*at<u8>(*at<gp<u8>>(m, 28), 100) >> 7) & 1;  // rlwinm 25,31,31
    const u32 table = word(*at<gp<u8>>(item, 16), 12);
    const u32 k = flag != 0 && word(m, 44) != 0 ? 1u : 0u;
    return *at<be_u32>(nullptr, table + (k << 2));
}

}  // namespace sly4::gfx

NATIVE(0051A6A0, sly4::gfx::Call_0051A6A0)
NATIVE(0051E7F0, sly4::gfx::SetName_0051E7F0)
NATIVE(0053BBE0, sly4::gfx::Call_0053BBE0)
NATIVE(005DB070, sly4::gfx::ThreadId_005DB070)
NATIVE(0079F810, sly4::gfx::SetHalf_0079F810)
NATIVE(0079FE28, sly4::gfx::SetHalf_0079FE28)
NATIVE(007A5F20, sly4::gfx::FiosAllocator_Alloc)
NATIVE(00523AA8, sly4::gfx::LoadShaders_00523AA8)
NATIVE(00536958, sly4::gfx::SetFlag_00536958)
NATIVE(00548900, sly4::gfx::Call_00548900)
NATIVE(00548B88, sly4::gfx::Call_00548B88)
NATIVE(00549594, sly4::gfx::Call_00549594)
NATIVE(007A6840, sly4::gfx::Call_007A6840)
NATIVE(0050FDC8, sly4::gfx::Count_0050FDC8)
NATIVE(005275C0, sly4::gfx::SetCos_005275C0)
NATIVE(00537B7C, sly4::gfx::FindEntry_00537B7C)
NATIVE(00548938, sly4::gfx::Call_00548938)
NATIVE(007A6804, sly4::gfx::Call_007A6804)
NATIVE(005319CC, sly4::gfx::Check_005319CC)
NATIVE(00543C04, sly4::gfx::CopyGlobals_00543C04)
NATIVE(00548BC0, sly4::gfx::RegisterGDEM)
NATIVE(00548CD4, sly4::gfx::RegisterKLBM)
NATIVE(0051096C, sly4::gfx::Call_0051096C)
NATIVE(0052296C, sly4::gfx::Call_0052296C)
NATIVE(00527668, sly4::gfx::SetLight_00527668)
NATIVE(0052D08C, sly4::gfx::Ctor_0052D08C)
NATIVE(00536998, sly4::gfx::SetClamp168_00536998)
NATIVE(005369E8, sly4::gfx::SetClamp172_005369E8)
NATIVE(00548508, sly4::gfx::Call_00548508)
NATIVE(005489CC, sly4::gfx::Call_005489CC)
NATIVE(005493B0, sly4::gfx::Reset_005493B0)
NATIVE(005D6578, sly4::gfx::UploadUV_005D6578)
NATIVE(00508054, sly4::gfx::VDelete_00508054)
NATIVE(00527608, sly4::gfx::SetShadeIntensity_00527608)
NATIVE(00546EF4, sly4::gfx::Render_Init546EF4)
NATIVE(005D7D9C, sly4::gfx::Pick_005D7D9C)

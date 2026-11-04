// Member constructors called by World_Construct (0x0045CC18): the save
// data, the per-user blocks, the post-effect / fade / time-scale state,
// the frame ring, plus the small copy helpers it uses. See construct.h for
// the member map.
#include "construct.h"

using namespace sly4;
using namespace sly4::wcons;

namespace {

constexpr u32 kSaveSize = 4056;           // bytes cleared by SaveData_Reset (+1 flag byte each at 4056 / 4057)
constexpr u32 kSaveRegion = 0x010B1804;   // u8 copied to +0 (name?)
constexpr u32 kSaveLang = 0x00F65BF4;     // u8 copied to +1, the next one widened to u16 at +2 (name?)
constexpr u32 kSaveTable = 0x00F64494;    // u32[41] copied to +3424, stride 8 (name?)
constexpr u32 kScoreTables = 608, kScoreTableCount = 64, kScoreTableSize = 44;
// Default high-score names, 4 letters each, scores 5000, 4000, ... 1000.
constexpr char kScoreNames[5][4] = {{'B', 'T', 'L', 'Y'}, {'S', 'L', 'Y', 'C'}, {'M', 'U', 'R', 'R'},
                                    {'C', 'A', 'R', 'M'}, {'D', 'I', 'M', 'I'}};
// Tables 6..11 have their own default scores.
constexpr u32 kScoreOverrides[6][5] = {{25000, 20000, 18000, 15000, 10000}, {20000, 18000, 15000, 12000, 10000},
                                       {4500, 4000, 3500, 3000, 2000},     {4000, 3700, 3500, 3000, 2000},
                                       {1000, 800, 700, 600, 500},         {4500, 4000, 3500, 3000, 2000}};
// Words at +352..+392 (stride 8): magic numbers of the save format (name?)
constexpr u32 kSaveMagic[6] = {0x0021C5E0u, 0x001F31A2u, 0x041555C4u, 0x936FF31Du, 0x03C72BA1u, 0x03D69F9Cu};

constexpr u32 kFadeDefaultColor = 0x01164CA8;  // u8[4]

}  // namespace

// void SaveData_Reset(SaveData* s)
// Clears the 4056-byte save image and fills in the defaults: a few system
// bytes, the format words, a 41-entry table and 64 high-score tables of
// five {score, 4-letter name} entries.
DECOMP(000ADE04, SaveData_Reset) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, kfn::Memset, {s, 0, kSaveSize});
    ref<u8>(s) = ref<u8>(kSaveRegion);
    ref<u8>(s + 1) = ref<u8>(kSaveLang);
    store<u16>(s + 2, ref<u8>(kSaveLang + 1));
    store<u32>(s + 32, 0);
    store<u32>(s + 308, 0);
    store<u64>(s + 312, 0);
    store<u16>(s + 320, 0);
    store<u16>(s + 322, 0);
    for (u32 k = 324; k < 328; ++k) ref<u8>(s + k) = 0;
    ref<u8>(s + 344) = 0;
    for (u32 k = 0; k < 6; ++k) store<u32>(s + 352 + k * 8, kSaveMagic[k]);
    for (u32 i = 0; i < 41; ++i) store<u32>(s + 3424 + i * 8, load<u32>(kSaveTable + i * 4));
    for (u32 j = 0; j < kScoreTableCount; ++j) {
        const u32 t = s + kScoreTables + j * kScoreTableSize;
        for (u32 k = 0; k < 5; ++k) {
            store<u32>(t + k * 4, 5000 - 1000 * k);
            for (u32 c = 0; c < 4; ++c) ref<u8>(t + 20 + k * 4 + c) = static_cast<u8>(kScoreNames[k][c]);
        }
        if (j >= 6 && j <= 11)
            for (u32 k = 0; k < 5; ++k) store<u32>(t + k * 4, kScoreOverrides[j - 6][k]);
        ref<u8>(t + 40) = 0;
    }
}

// void Byte_Clear(u8* p): *p = 0 (SaveData +4056). Leaves r4 = 0.
DECOMP(000ADDE8, Byte_Clear) {
    ref<u8>(arg<u32>(ctx, 0)) = 0;
    ctx.gpr[4] = 0;
}

// void Bytes2_Clear(u8* p): p[0] = p[1] = 0 (SaveData +4057). Leaves r4 = 0.
DECOMP(000ADDF4, Bytes2_Clear) {
    const u32 p = arg<u32>(ctx, 0);
    ref<u8>(p) = 0;
    ref<u8>(p + 1) = 0;
    ctx.gpr[4] = 0;
}

// void SaveData_Construct(SaveData* s): World +80.
DECOMP(007865FC, SaveData_Construct) {
    const u32 s = arg<u32>(ctx, 0);
    call(ctx, kfn::SaveData_Reset, {s});
    call(ctx, kfn::Byte_Clear, {s + 4056});
    call(ctx, kfn::Bytes2_Clear, {s + 4057});
}

// void PtrVector_ConstructInit(PtrVector* v, u32 a, u32 b, u8 c)
// Constructs the vector with 4-byte elements and initialises it.
DECOMP(00786658, PtrVector_ConstructInit) {
    const u32 v = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), c = arg<u32>(ctx, 3) & 0xFF;
    call(ctx, kfn::PtrVector_Construct, {v});
    store<u16>(v + 6, 4);
    call(ctx, kfn::PtrVector_Init, {v, a, b, c});
}

// void Fade_Construct(Fade* f): World +0x128290. From = the default
// colour, to = 0, duration and time 0, not running.
DECOMP(007866CC, Fade_Construct) {
    const u32 f = arg<u32>(ctx, 0);
    for (u32 k = 0; k < 4; ++k) ref<u8>(f + k) = ref<u8>(kFadeDefaultColor + k);
    for (u32 k = 4; k < 8; ++k) ref<u8>(f + k) = 0;
    store<u32>(f + 8, 0);
    store<u32>(f + 12, 0);
    store<u32>(f + 16, load<u32>(f + 16) & 0x3FFFFFFFu);  // clear hide-when-done and running
}

// void ScaleBlend_Construct(ScaleBlend* s): World +0x1282A4 (time) and
// +0x1282C0 (audio): {1.0, 0.25, current 1.0, 0, 0, 0}, flag 0x80000000 off.
DECOMP(00786724, ScaleBlend_Construct) {
    const u32 s = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(s + 24) & 0x7FFFFFFFu;
    store<u32>(s, 0x3F800000u);       // 1.0
    store<u32>(s + 4, 0x3E800000u);   // 0.25
    store<u32>(s + 8, 0x3F800000u);   // 1.0
    store<u32>(s + 12, 0);
    store<u32>(s + 16, 0);
    store<u32>(s + 20, 0);
    store<u32>(s + 24, flags);
}

// void Dof_Construct(Dof* d): World +0x1282DC (PostFx_UpdateDof).
DECOMP(00786758, Dof_Construct) {
    const u32 d = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(d + 40) & 0x4FFFFFFFu;  // clear bits 0x80000000, 0x20000000, 0x10000000
    store<u32>(d, 0);
    store<u32>(d + 4, 0);
    store<u32>(d + 8, 0x3F800000u);   // 1.0
    store<u32>(d + 12, 0x41200000u);  // 10.0
    store<u32>(d + 16, 0x41A00000u);  // 20.0
    store<u32>(d + 20, 0x41200000u);  // 10.0
    store<u32>(d + 24, 0x42200000u);  // 40.0
    store<u32>(d + 28, 0x3F800000u);  // 1.0
    store<u32>(d + 32, 1);
    store<u32>(d + 36, 0);
    store<u32>(d + 40, flags);
}

// void Clear16_Construct(void* p): three words and a byte = 0 (World +0x128324).
DECOMP(007867B0, Clear16_Construct) {
    const u32 p = arg<u32>(ctx, 0);
    store<u32>(p, 0);
    store<u32>(p + 4, 0);
    store<u32>(p + 8, 0);
    ref<u8>(p + 12) = 0;
}

// void Filter_Construct(Filter* f): World +0x1283A8 (PostFx_UpdateFilter).
DECOMP(007867C8, Filter_Construct) {
    const u32 f = arg<u32>(ctx, 0);
    const u32 flags = load<u32>(f + 20) & 0x3FFFFFFFu;
    store<u32>(f, 0);
    store<u32>(f + 4, 0x3F800000u);  // 1.0
    store<u32>(f + 8, 0x3F800000u);  // 1.0
    store<u32>(f + 12, 1);
    store<u32>(f + 16, 0);
    store<u32>(f + 20, flags);
}

// void Obj1283C0_Construct(void* o): World +0x1283C0, 188 bytes with a
// vector of 12-byte entries at +132 (name?)
DECOMP(007867FC, Obj1283C0_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 v = o + 132;
    call(ctx, kfn::PtrVector_Construct, {v});
    store<u16>(o + 138, 12);
    call(ctx, kfn::PtrVector_Init, {v, 6, 0, 1});
    const u32 flags = load<u32>(o + 180) & 0x1FFFFFFFu;
    ref<u8>(o) = 0;
    ref<u8>(o + 65) = 0;
    store<u16>(o + 140, 0);
    for (u32 k = 152; k <= 176; k += 4) store<u32>(o + k, 0);
    store<u32>(o + 180, flags);
    store<u32>(o + 184, 0);
}

// void Pair24_Construct(void* o, u32 first): +0 = first, +4..+20 = 0 (World +0x1285CC).
DECOMP(007868A4, Pair24_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, arg<u32>(ctx, 1));
    for (u32 k = 4; k <= 20; k += 4) store<u32>(o + k, 0);
    ctx.gpr[5] = 0;
}

// void View_SetFlag10(View* v, bool on): View +1776 bit 0x10.
DECOMP(007868C4, View_SetFlag10) {
    const u32 v = arg<u32>(ctx, 0);
    const u32 b = (ref<u8>(v + 1776) & ~0x10u) | ((arg<u32>(ctx, 1) << 4) & 0x10u);
    ref<u8>(v + 1776) = static_cast<u8>(b);
    ctx.gpr[5] = b;
}

// void FrameRing_Construct(FrameRing* r, u32 size, u32 size2)
// Three 224-byte frame buffers in +0..+8 (current = +12). A failed
// allocation stores the previous buffer again (0 for the first), as the
// original does.
DECOMP(0078691C, FrameRing_Construct) {
    const u32 r = arg<u32>(ctx, 0), size = arg<u32>(ctx, 1), size2 = arg<u32>(ctx, 2);
    u32 last = 0;
    for (u32 i = 0; i < 3; ++i) {
        const u32 mem = static_cast<u32>(call(ctx, kfn::Mem_Alloc, {224, 0, 16}));
        if (mem != 0) {
            call(ctx, kfn::RenderBuffer_Construct, {mem, size, size2, 1, 1});
            last = mem;
        }
        store<u32>(r + i * 4, last);
    }
    ref<u8>(r + 21) = 0;
    store<u32>(r + 12, load<u32>(r));
    store<u32>(r + 16, 0);
    ref<u8>(r + 20) = 0;
}

// void UserBlock_Construct(UserBlock* b): one of the World's 9 per-user
// blocks (World +4208, 0x20C80 bytes each).
DECOMP(0045C868, UserBlock_Construct) {
    const u32 b = arg<u32>(ctx, 0);
    // Bytes 0..2 = 255, byte 3 loses bits 0x80 and 0x40; +4 keeps its low 20 bits.
    ref<u8>(b) = 255;
    ref<u8>(b + 1) = 255;
    ref<u8>(b + 2) = 255;
    const u32 w0 = 0xFFFFFF00u | (load<u32>(b) & 0x3Fu);
    const u32 w1 = load<u32>(b + 4) & 0x000FFFFFu;
    store<u32>(b, w0);
    store<u32>(b + 4, w1);
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 3; ++c)
            store<u32>(b + kBlockMatrix + row * 16 + c * 4, load<u32>(kIdentity43 + row * 16 + c * 4));
    store<u32>(b + 8, 0);
    store<u32>(b + 12, 0);
    call(ctx, kfn::PtrVector_Construct, {b + kBlockVector});
    store<u16>(b + kBlockVector + 6, 4);
    // Record headers (both lists are cleared again by the memsets below).
    for (u32 i = 0; i < 2048; ++i)
        for (u32 half : {0u, 32u})
            for (u32 k = 0; k < 24; k += 4) store<u32>(b + kBlockRecords + i * 64 + half + k, 0);
    for (u32 i = 0; i < 32; ++i)
        for (u32 half : {0u, 32u})
            for (u32 k = 0; k < 24; k += 4) store<u32>(b + kBlockRecords2 + i * 64 + half + k, 0);
    store<u32>(b + 100, 0);
    store<u32>(b + 104, 0);
    store<u32>(b + 108, 0);
    const u32 a = load<u32>(kTuneBlockA + kTuning);
    store<u32>(b + 100, a);
    store<u32>(b + 108, a);
    store<u32>(b + 104, load<u32>(kTuneBlockB + kTuning));
    store<u32>(b, load<u32>(b) | 0x40u);
    call(ctx, kfn::Memset, {b + kBlockRecords, 0, 0x20404});
    call(ctx, kfn::Memset, {b + kBlockRecords2, 0, 2052});
    store<u32>(b, load<u32>(b) & ~0x80u);
}

// void Vec3_Copy(f32 dst[3], const f32 src[3]). Leaves f1 = src[2].
DECOMP(00734BA0, Vec3_Copy) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 k = 0; k < 12; k += 4) store<u32>(d + k, load<u32>(s + k));
    ctx.fpr[1] = static_cast<f64>(load<f32>(s + 8));
}

// void Mat43_Copy(f32 dst[16], const f32 src[16]): the first three columns
// of the four rows (the w column is left alone). Leaves f1 = src[14].
DECOMP(00734CC8, Mat43_Copy) {
    const u32 d = arg<u32>(ctx, 0), s = arg<u32>(ctx, 1);
    for (u32 row = 0; row < 4; ++row)
        for (u32 c = 0; c < 3; ++c) store<u32>(d + row * 16 + c * 4, load<u32>(s + row * 16 + c * 4));
    ctx.fpr[1] = static_cast<f64>(load<f32>(s + 56));
}

// void AnimScript_RegisterIffChunks(): the 'FRID', 'HRID' and 'DRID' chunk handlers.
DECOMP(004BCE08, AnimScript_RegisterIffChunks) {
    call(ctx, kfn::Iff_RegisterChunk, {0x46524944u, 0x00F3A080u, 0, 0, 0});  // 'FRID'
    call(ctx, kfn::Iff_RegisterChunk, {0x48524944u, 0x00F3A088u, 0, 0, 0});  // 'HRID'
    call(ctx, kfn::Iff_RegisterChunk, {0x44524944u, 0x00F3A090u, 0, 0, 0});  // 'DRID'
}

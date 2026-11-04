// Global singletons and Iff chunk handlers set up near the end of
// World_Construct (0x0045CC18); see construct.h.
#include "construct.h"

using namespace sly4;
using namespace sly4::wcons;

namespace {

constexpr u32 kSub0028BDB0 = 0x0028BDB0;
constexpr u32 kSub004F3C88 = 0x004F3C88;  // (mem, opd table, 18, 0, 0, 15360, u8, 1) (name?)
constexpr u32 kSub00767484 = 0x00767484;  // (mem, 128) (name?)
constexpr u32 kSub0051A79C = 0x0051A79C;  // (name, OPD, 0, 0, 0) (name?)

}  // namespace

// bool Globals_ResetCE0A8(): clears a few globals and calls sub_0028BDB0; returns 1 (name?)
DECOMP(000CE0A8, Globals_ResetCE0A8) {
    store<u16>(0x01013B34u + 8, 0);
    store<u32>(0x00F64730u, 1);
    store<u32>(0x00F64734u, 0xBF800000u);  // -1.0
    store<u16>(0x01056F1Cu + 2, 0);
    call(ctx, kSub0028BDB0);
    ret(ctx, 1u);
}

// Object* Singleton658D4_Construct(Object* o, u8 flag): 156 bytes; base
// constructor, vtable 0x00B217E8, and g 0x00F658D4 = o (name?)
DECOMP(0043D9A4, Singleton658D4_Construct) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1) & 0xFF;
    call(ctx, kSub004F3C88, {o, 0x00F6588Cu, 18, 0, 0, 15360, flag, 1});
    store<u32>(o, 0x00B217E8u);
    store<u32>(0x00F658D4u, o);
}

// void TsaChunks_Register(): 'FTSA', 'HTSA', 'DTSA' Iff chunk handlers.
DECOMP(002E8D74, TsaChunks_Register) {
    call(ctx, kfn::Iff_RegisterChunk, {0x46545341u, 0x00F36028u, 0, 0, 0});
    call(ctx, kfn::Iff_RegisterChunk, {0x48545341u, 0x00F36030u, 0, 0, 0});
    call(ctx, kfn::Iff_RegisterChunk, {0x44545341u, 0x00F36038u, 0, 0, 0});
}

// void KvhChunks_Register(): 'FKVH', 'HKVH', 'DKVH' and 'DEBG' Iff chunk handlers.
DECOMP(00346BB0, KvhChunks_Register) {
    call(ctx, kfn::Iff_RegisterChunk, {0x464B5648u, 0x00F372F8u, 0, 0, 0});
    call(ctx, kfn::Iff_RegisterChunk, {0x484B5648u, 0x00F37300u, 0, 0, 0});
    call(ctx, kfn::Iff_RegisterChunk, {0x444B5648u, 0x00F37308u, 0, 0, 0});
    call(ctx, kfn::Iff_RegisterChunk, {0x44454247u, 0x00F37310u, 0, 0, 0});
}

// void Effects_Construct(Effects* e): the 60-byte effects manager (g 0x00F64C38,
// destroyed by World_Destruct): a 128-entry table, a vector of 96-byte
// entries at +36, and the "Effects1" registration (name?)
DECOMP(002EE304, Effects_Construct) {
    const u32 e = arg<u32>(ctx, 0);
    call(ctx, kSub00767484, {e, 128});
    call(ctx, kfn::PtrVector_Construct, {e + 36});
    store<u16>(e + 42, 96);
    call(ctx, kfn::PtrVector_Init, {e + 36, 16, 0, 0});
    store<u32>(e + 56, 0);
    store<u32>(0x00F64C38u, e);
    ref<u8>(e + 32) = 0;
    call(ctx, kSub0051A79C, {0x00AD96A4u /* "Effects1" */, 0x00F36100u, 0, 0, 0});
    if (ref<u8>(0x01068B75u) == 0) ref<u8>(0x01068B75u) = 1;
}

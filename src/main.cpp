// main(): start-up, the main loop and shutdown.
#include <stdexcept>

#include "sly4.h"

using namespace sly4;

namespace {

// Main_Poll
// After every start-up step main() pumps the cellSysutil callbacks and maps
// the flags they set onto g::main_state (an inlined helper in the
// original). Returns false when the game should stop.
bool Main_Poll(PPUContext& ctx) {
    call(ctx, 0x0056C184);  // Sysutil_CheckCallback
    const u32 flags = load<u32>(g::sysutil_flags);
    auto& state = ref<be_u32>(g::main_state);
    if (flags & kSysutilExitRequested) {
        state = kMainQuit;
        return false;
    }
    if (flags & (kSysutilMenuOpen | kSysutilPause80 | kSysutilPadLost))
        state = kMainPaused;
    else if (flags & kSysutilResume)
        state = kMainResumed;
    else if (state == kMainQuit)
        return false;
    return true;
}

// Defaults written into g::config before Main_ParseCommandLine parses the command
// line. Offsets are into the config block. Most of these are pool sizes and
// counts whose meaning hasn't been worked out yet.
void Main_SetDefaultConfig(u32 build_id) {
    const u32 c = g::config;
    field<u32>(c, 0x000) = 0x00AE820C;  // "Sly4"
    field<u8>(c, 0x024) = 0;
    field<u8>(c, 0x025) = 0;
    field<f32>(c, 0x038) = 0.25f;
    field<u8>(c, 0x05C) = 1;
    field<f32>(c, 0x088) = 300.0f;
    field<f32>(c, 0x08C) = 50.0f;
    field<u8>(c, 0x099) = 4;
    field<u8>(c, 0x09A) = 4;
    field<u8>(c, 0x09B) = 16;
    field<u8>(c, 0x09C) = 1;
    field<u8>(c, 0x09D) = 1;
    field<u8>(c, 0x0A1) = 0;
    field<u8>(c, 0x0A2) = 0;
    field<u8>(c, 0x0A3) = 1;
    field<u32>(c, 0x0A4) = 0x300000;
    field<u32>(c, 0x0A8) = 20;
    field<f32>(c, 0x0AC) = 0.5f;
    field<u32>(c, 0x144) = 0x1F4000;
    field<u32>(c, 0x148) = 0;
    field<u32>(c, 0x14C) = 0x1000;
    field<u32>(c, 0x150) = 0x2000;
    field<u32>(c, 0x154) = 0x4C00;
    field<u32>(c, 0x158) = 0x6000;
    field<u16>(c, 0x16E) = 1280;
    field<u16>(c, 0x178) = 1024;
    field<u16>(c, 0x17C) = 1280;
    field<u16>(c, 0x18E) = 4096;
    field<u32>(c, 0x1A4) = 0x20000;
    field<u32>(c, 0x1A8) = 0x60000;
    field<u16>(c, 0x1AC) = 4000;
    field<u16>(c, 0x1AE) = 1000;
    field<u16>(c, 0x1B0) = 1000;
    field<u16>(c, 0x1B4) = 1000;
    field<u16>(c, 0x1B6) = 4256;
    field<u16>(c, 0x1BA) = 400;
    field<u16>(c, 0x1BC) = 400;
    field<u16>(c, 0x1BE) = 1000;
    field<u16>(c, 0x1C0) = 1000;
    field<u16>(c, 0x1C2) = 1000;
    field<u16>(c, 0x1C4) = 1000;
    field<u16>(c, 0x1C6) = 50;
    field<u16>(c, 0x1C8) = 50;
    field<u32>(c, 0x1CC) = 0x700000;
    field<u32>(c, 0x1D0) = 0x1900000;
    field<u32>(c, 0x1D4) = 0xC8000;
    field<u32>(c, 0x1D8) = 0x800000;
    field<u32>(c, 0x1DC) = 0xC0000;
    field<u32>(c, 0x1E0) = 0x400000;
    field<u32>(c, 0x1E4) = 0x12C000;
    field<u32>(c, 0x1E8) = 0;
    field<u32>(c, 0x1EC) = 0x9800;
    field<u32>(c, 0x1F0) = 0x400;
    field<u16>(c, 0x1F4) = 40;
    field<u32>(c, 0x1F8) = 400;
    field<u32>(c, 0x1FC) = 0x400000;
    field<u32>(c, 0x200) = 0x400000;
    field<u32>(c, 0x208) = 0x00AE8214;  // "BCES01284"
    field<u64>(c, 0x210) = load<u64>(build_id);
    field<u8>(g::mem_stats, 0x088) = 0;
}

constexpr u32 kGameArchive = 0x00AE8220;       // "Sly4.psarc"
constexpr u32 kShaderArchive = 0x00AE8484;     // "System\Shaders.Zip"
constexpr u32 kRamDiskName = 0x00AE849C;       // "RamDisk"
constexpr u32 kBuildId = 0x00463650;           // 8 bytes just before main
constexpr u32 kSysutilOnEvent = 0x00F39E68;    // OPD of Sysutil_OnEvent (0x0049AED8)
constexpr u32 kFrameCallbacks = 0x00F39DC8;    // OPD handed to sub_005129BC (name?)
constexpr u32 kNpPoolSize = 0x20000;

// Function-local statics of the original, initialised on first use.
constexpr u32 kStaticB = 0x011DE300, kStaticBGuard = 0x011DE480;
constexpr u32 kRandomState = 0x01164890;  // u32: random generator state, seeded with the time base
constexpr u32 kFrameFlagF9CDE2 = 0x00F9CDE2, kFirstFrameDone = 0x00F65DF0;

// Rotate the three-entry ring the renderer keeps at *(*(Game) + 0x1285E4).
void Main_RotateFrameRing() {
    const u32 ring = load<u32>(load<u32>(g::game) + 0x1285E4);
    auto& index = ref<u8>(ring + 21);
    const u32 next = (index + 1u) % 3u;
    field<u32>(ring, 16) = field<u32>(ring, 12);
    index = static_cast<u8>(next);
    field<u32>(ring, 12) = load<u32>(ring + next * 4);
}

[[noreturn]] void Main_Shutdown(PPUContext& ctx) {
    call(ctx, fn::Game_Shutdown, {g::game});
    call(ctx, fn::sub_004A8AC4);
    call(ctx, fn::_exit, {0});
    throw std::logic_error("_exit returned");
}

}  // namespace

// int main(int argc, char** argv)
DECOMP(00463658, main) {
    const u64 argc = ctx.gpr[3], argv = ctx.gpr[4];
    Stack frame(ctx, 32);
    const u32 locals = frame.addr();

    Main_SetDefaultConfig(kBuildId);
    // r5 still holds the build id here; Main_ParseCommandLine doesn't use it.
    call(ctx, fn::Main_ParseCommandLine, {argc, argv, load<u64>(kBuildId)});
    call(ctx, 0x0056C1E8, {0, kSysutilOnEvent, 0});  // Sysutil_RegisterCallback
    call(ctx, fn::Main_InitSystems, {1});
    if (!Main_Poll(ctx)) return ret(ctx, 0);

    call(ctx, 0x0056C0B4, {kNpPoolSize});  // Np_Init
    if (call(ctx, 0x0055D78C, {kGameArchive}) != 0) {  // File_Exists
        // MountOptions, passed by value in r4-r6: { 0, 1, 0, 0, 0, 0 }
        for (u32 k = 0; k < 6; ++k) store<u32>(locals + k * 4, k == 1 ? 1 : 0);
        call(ctx, fn::Archive_Mount,
             {kGameArchive, load<u64>(locals), load<u64>(locals + 8), load<u64>(locals + 16)});
    }
    if (!Main_Poll(ctx)) return ret(ctx, 0);

    // The shaders archive is read into memory and mounted as "RamDisk".
    const u32 shader_size = locals + 24;
    store<u32>(shader_size, 0);
    const u32 shaders = static_cast<u32>(call(ctx, fn::File_LoadWhole, {kShaderArchive, shader_size}));
    if (!Main_Poll(ctx)) return ret(ctx, 0);
    field<u32>(g::config, 0x54) =
        static_cast<u32>(call(ctx, fn::Archive_OpenZip, {shaders, load<u32>(shader_size), kRamDiskName}));
    call(ctx, fn::Main_InitSystems, {static_cast<u64>(-2)});
    if (!Main_Poll(ctx)) return ret(ctx, 0);

    // Done with the shader archive: virtual slot 2 on it, then free the data.
    const u32 ramdisk = load<u32>(g::config + 0x54);
    call_ptr(ctx, load<u32>(load<u32>(ramdisk) + 8), {ramdisk});
    field<u32>(g::config, 0x54) = 0;
    call(ctx, fn::Mem_Free, {shaders});
    call(ctx, fn::sub_005129BC, {kFrameCallbacks, 0});

    if (field<u8>(g::game, 11) != 0) {
        if (ref<u8>(kStaticBGuard) == 0) {
            call(ctx, fn::sub_004FD214, {kStaticB});
            ref<u8>(kStaticBGuard) = 1;
        }
        ref<u8>(kStaticB) = 0;
    }
    system_state::get(ctx);
    ref<u8>(system_state::object + 32) |= 0x80;
    call(ctx, fn::sub_004A8AC0);
    call(ctx, fn::sub_000AC0F0, {1});
    if (!Main_Poll(ctx)) return ret(ctx, 0);

    call(ctx, fn::Game_Load, {g::game});
    if (!Main_Poll(ctx)) Main_Shutdown(ctx);

    call(ctx, fn::sub_000AC478);
    u32 start;
    do start = static_cast<u32>(read_timebase());
    while (start == 0);
    auto& state = ref<be_u32>(g::main_state);
    state = kMainStarting;
    store<u32>(kRandomState, start);
    Main_Poll(ctx);  // a quit request here ends the loop below before it starts

    // ------------------------------------------------------------ main loop
    if (state == kMainStarting || state == kMainResumed || state == kMainPaused) {
        do {
            if (ref<u8>(kFrameFlagF9CDE2) != 0) call(ctx, fn::sub_000AC0F0, {0});
            const bool first_frame = ref<u8>(kFirstFrameDone) == 0;
            ref<u8>(kFrameFlagF9CDE2) = 0;

            bool full_frame = false;  // also runs sub_0051096C at the end
            if (first_frame) {
                ref<u8>(kFirstFrameDone) = 1;
                state = static_cast<u32>(call(ctx, fn::Game_Update, {g::game}));
            } else if (field<u8>(g::game, 0x5E40) & 0x80) {
                state = static_cast<u32>(call(ctx, fn::Game_Update, {g::game}));
            } else if (field<u8>(g::config, 0x22) != 0) {
                call(ctx, fn::sub_00512BE0);
                call(ctx, fn::sub_0051265C);
                state = static_cast<u32>(call(ctx, fn::Game_Update, {g::game}));
                full_frame = true;
            } else {
                call(ctx, fn::sub_00512BE0);
                state = static_cast<u32>(call(ctx, fn::Game_Update, {g::game}));
                call(ctx, fn::sub_0051265C);
                full_frame = true;
            }

            // A countdown at Game+0x5E3C; when it runs out, sub_0051299C
            // handles whatever Game+0x5E38 holds.
            auto& countdown = ref<u8>(g::game + 0x5E3C);
            if (countdown > 1) {
                --countdown;
            } else if (field<u32>(g::game, 0x5E38) != 0) {
                call(ctx, fn::sub_0051299C);
                field<u32>(g::game, 0x5E38) = 0;
                countdown = 0;
            }

            call(ctx, fn::sub_00458BAC, {load<u32>(g::game), 2});
            Main_RotateFrameRing();
            call(ctx, fn::sub_00596A8C, {2});
            call(ctx, fn::sub_005124F4, {2});
            if (full_frame) call(ctx, fn::sub_0051096C);
        } while (state == kMainResumed || state == kMainPaused);
    }
    Main_Shutdown(ctx);
}

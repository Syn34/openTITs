// Input devices: the pads Game_Load makes for each player.
#include "load.h"

using namespace sly4;
using namespace sly4::gameload;

namespace {
constexpr u32 kDeviceSize = 296;      // ApeInputDevice elements in the free list
constexpr u32 kDeviceFreeListCount = 4;
constexpr u32 kInputMgrDeviceCount = 80;  // u32 in lg::kInputMgr: next port number
constexpr s32 CELL_OK = 0;
}  // namespace

// ApeInputDevice* InputMgr_CreatePad()
// Allocates an input device from ApeInputMgr's free list (made on first use:
// the inlined ApeInputMgr::GetAllocator()) and constructs it as a pad.
// Game_Load leaves a player pointer in r3; it isn't used.
DECOMP(0056324C, InputMgr_CreatePad) {
    u32 list = load<u32>(lg::kInputMgr);
    if (list == 0) {
        const u32 mem = call32(ctx, lfn::Mem_New, {kFreeListSize});
        if (mem != 0) {
            FreeList_Construct(ctx, mem, lg::kInputDeviceFreeListVtbl, lg::kInputDeviceFreeListName, kDeviceSize,
                               kDeviceFreeListCount);
            list = mem;
        }
        store<u32>(lg::kInputMgr, list);  // a failed allocation crashes below, as in the original
    }
    const u32 dev = static_cast<u32>(call_ptr(ctx, vfunc(list, kFreeListVfnAlloc), {list}));
    if (dev != 0) call(ctx, lfn::PadDevice_Construct, {dev});
    ret(ctx, dev);
}

// void InputDevice_Construct(InputDevice* dev)
// Base constructor: clears the state and takes the next port number.
DECOMP(00562DDC, InputDevice_Construct) {
    const u32 dev = arg<u32>(ctx, 0);
    field<u32>(dev, 0) = lg::kInputDeviceVtbl;
    field<u32>(dev, 16) = 0;
    field<u32>(dev, 20) = 0;
    field<u32>(dev, 24) = 0;
    call(ctx, fn::memset, {dev + 32, 0, 96});
    field<u32>(dev, 132) = 0;
    field<u32>(dev, 140) = 0;
    field<u32>(dev, 136) = 0;
    field<u32>(dev, 144) = 0;
    const u16 flags = field<u16>(dev, kDevFlags);
    field<u32>(dev, 128) = 0;
    field<u32>(dev, 8) = 0;
    ref<u8>(dev + 148) = 1;
    field<u16>(dev, kDevFlags) = static_cast<u16>(flags | kDevFlag4000 | kDevConnected);
    auto& count = field<u32>(lg::kInputMgr, kInputMgrDeviceCount);
    const u32 port = count;
    count = port + 1;
    ref<u8>(dev + kDevPort) = static_cast<u8>(port);
}

// void PadDevice_Construct(InputDevice* dev)
// Pad constructor: turns on pressure and motion sensors for the device's
// port and records whether a pad is connected there (flag 0x8000).
DECOMP(0056FAC8, PadDevice_Construct) {
    const u32 dev = arg<u32>(ctx, 0);
    call(ctx, lfn::InputDevice_Construct, {dev});

    const u16 flags = field<u16>(dev, kDevFlags);
    const u8 port = ref<u8>(dev + kDevPort);
    field<u32>(dev, 0) = lg::kPadDeviceVtbl;
    field<u16>(dev, kDevFlags) = static_cast<u16>(flags | kDevConnected);
    call(ctx, limp::cellPadSetPortSetting, {port, CELL_PAD_SETTING_PRESS_ON | CELL_PAD_SETTING_SENSOR_ON});
    const s32 rc = static_cast<s32>(call32(ctx, limp::cellPadGetInfo2, {lg::kPadInfo}));

    u16 now = field<u16>(dev, kDevFlags);
    if (rc == CELL_OK &&
        (load<u32>(lg::kPadInfo + kPadInfoPortStatus + ref<u8>(dev + kDevPort) * 4u) & CELL_PAD_STATUS_CONNECTED) != 0)
        now = static_cast<u16>(now | kDevConnected);
    else
        now = static_cast<u16>(now & ~kDevConnected);
    field<u16>(dev, kDevFlags) = now;
}

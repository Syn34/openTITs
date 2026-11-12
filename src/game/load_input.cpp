// Input devices: the pads Game_Load makes for each player.
#include "load.h"

#include "../memory/mem.h"

namespace sly4::gameload {
using namespace opentit::native;

namespace {
constexpr u32 kDeviceSize = 296;      // ApeInputDevice elements in the free list
constexpr u32 kDeviceFreeListCount = 4;
constexpr s32 CELL_OK = 0;
}  // namespace

// Allocates an input device from ApeInputMgr's free list (made on first use:
// the inlined ApeInputMgr::GetAllocator()) and constructs it as a pad.
InputDevice* InputMgr_CreatePad() {
    InputMgr& mgr = global<InputMgr>(lg::kInputMgr);
    container::FreeList* list = mgr.devices;
    if (!list) {
        auto* mem = static_cast<container::FreeList*>(memory::Mem_New(kFreeListSize));
        if (mem) {
            FreeList_Construct(mem, lg::kInputDeviceFreeListVtbl, lg::kInputDeviceFreeListName, kDeviceSize,
                               kDeviceFreeListCount);
            list = mem;
        }
        mgr.devices = list;  // a failed allocation crashes below, as in the original
    }
    InputDevice* dev = vcall<InputDevice*>(list, kFreeListVfnAlloc);
    if (dev) PadDevice_Construct(dev);
    return dev;
}

// Base constructor: clears the state and takes the next port number.
void InputDevice_Construct(InputDevice* dev) {
    dev->vtable = lg::kInputDeviceVtbl;
    dev->w16 = 0u;
    dev->w20 = 0u;
    dev->w24 = 0u;
    call(fn::memset, dev->state, 0u, static_cast<u32>(sizeof(dev->state)));
    dev->w132 = 0u;
    dev->w140 = 0u;
    dev->w136 = 0u;
    dev->w144 = 0u;
    const u16 flags = dev->flags;
    dev->w128 = 0u;
    dev->w8 = 0u;
    dev->b148 = 1;
    dev->flags = static_cast<u16>(flags | kDevFlag4000 | kDevConnected);
    be_u32& count = global<InputMgr>(lg::kInputMgr).device_count;
    const u32 port = count;
    count = port + 1;
    dev->port = static_cast<u8>(port);
}

// Pad constructor: turns on pressure and motion sensors for the device's
// port and records whether a pad is connected there (flag 0x8000).
void PadDevice_Construct(InputDevice* dev) {
    InputDevice_Construct(dev);

    const u16 flags = dev->flags;
    const u8 port = dev->port;
    dev->vtable = lg::kPadDeviceVtbl;
    dev->flags = static_cast<u16>(flags | kDevConnected);
    call(limp::cellPadSetPortSetting, static_cast<u32>(port), CELL_PAD_SETTING_PRESS_ON | CELL_PAD_SETTING_SENSOR_ON);
    const s32 rc = call<s32>(limp::cellPadGetInfo2, lg::kPadInfo);

    u16 now = dev->flags;
    const be_u32* port_status = host<be_u32>(lg::kPadInfo + kPadInfoPortStatus);
    if (rc == CELL_OK && (port_status[dev->port] & CELL_PAD_STATUS_CONNECTED) != 0)
        now = static_cast<u16>(now | kDevConnected);
    else
        now = static_cast<u16>(now & ~kDevConnected);
    dev->flags = now;
}

}  // namespace sly4::gameload

NATIVE(0056324C, sly4::gameload::InputMgr_CreatePad)
NATIVE(00562DDC, sly4::gameload::InputDevice_Construct)
NATIVE(0056FAC8, sly4::gameload::PadDevice_Construct)

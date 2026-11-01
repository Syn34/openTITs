// UI start-up (the "RenderTarget_GUI" render target), UI element lookup by
// dotted path, and the AglRenderTarget functions they use.
#include "load.h"

using namespace sly4;
using namespace sly4::gameload;

namespace {

constexpr u32 kGuiTargetType = 2;
constexpr u32 kRtNameSize = 32;
constexpr u32 kPathBufferSize = 64;  // the original's buffer, not bounds-checked

// Render target constants set by RenderTarget_Create.
constexpr f32 kRtF1824Value = 4.0f, kRtF1828Value = 0.01f;
constexpr u32 kRtParamA = 2044, kRtParamB = 2046, kRtParamC = 240;
constexpr f32 kRtParamF1 = 16.0f, kRtParamF2 = 10.0f;  // the floats at 0x00522FB0

// First child of `parent` (or first top-level element) whose name hashes to `hash`.
u32 Ui_FindChild(u32 parent, u32 hash) {
    u32 e = parent != 0 ? field<u32>(parent, kElemFirstChild).get() : load<u32>(lg::kUiRoot);
    for (; e != 0; e = field<u32>(e, kElemNext))
        if (field<u32>(e, kElemNameHash) == hash) return e;
    return 0;
}

}  // namespace

// bool Ui_Init(const ViewRect* viewport)
// Creates the UI render target with two (empty) draw callbacks and sets its
// viewport to `viewport`, or to the whole screen (config +4/+6) when it is 0.
DECOMP(004D35AC, Ui_Init) {
    const u32 viewport = arg<u32>(ctx, 0);

    const u32 target = call32(ctx, lfn::RenderTarget_Create, {kGuiTargetType, 0});
    store<u32>(lg::kMainView, target);
    call(ctx, fn::String_Copy, {target + kRtName, lg::kRenderTargetGuiName, kRtNameSize});
    call(ctx, lfn::RenderTarget_SetByte49, {load<u32>(lg::kMainView), 0});
    call(ctx, lfn::RenderTarget_AddCallback, {load<u32>(lg::kMainView), 0, lg::kUiDrawOpd0, 0, 0});
    call(ctx, lfn::RenderTarget_AddCallback, {load<u32>(lg::kMainView), 1, lg::kUiDrawOpd1, 0, 0});

    auto& rect = ref<ViewRect>(lg::kUiViewRect);
    const u16 width = field<u16>(g::config, 4), height = field<u16>(g::config, 6);
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    const u32 view = load<u32>(lg::kMainView);
    if (viewport != 0) {
        const auto& src = ref<ViewRect>(viewport);
        rect.x = src.x;
        rect.y = src.y;
        rect.width = src.width;
        rect.height = src.height;
    }
    call(ctx, lfn::RenderTarget_SetViewport, {view, lg::kUiViewRect});

    store<u32>(lg::kUiClear0, 0);
    store<u32>(lg::kUiClear1, 0);
    store<u32>(lg::kUiClear2, 0);
    store<u32>(lg::kUiClear0 + 4, 0);
    store<u32>(lg::kUiClear0 + 8, 0);
    store<u32>(lg::kUiClear0 + 12, 0);
    ret(ctx, 1u);
}

// Element* Ui_Find(const char* path, Element* parent)
// Finds an element by a dotted path such as "Common.Cursors.01", starting
// at `parent`'s children (or the top-level elements when parent is 0). A
// leading "." starts from the top level; each extra leading dot after the
// first ("..", "...") goes up one parent. Names compare by
// String_HashNoCase. An empty path returns `parent`.
DECOMP(004D5544, Ui_Find) {
    const u32 path = arg<u32>(ctx, 0);
    u32 parent = arg<u32>(ctx, 1);
    if (path == 0 || load<u8>(path) == 0) return ret(ctx, parent);

    Stack frame(ctx, kPathBufferSize);
    const u32 name = frame.addr();
    u32 out = name;
    u32 p = path;
    do {
        if (load<u8>(p) != '.') {
            store<u8>(out++, load<u8>(p++));
        } else if (p != path) {
            // End of a segment: look it up, then the rest of the path below it.
            store<u8>(out, 0);
            const u32 e = Ui_FindChild(parent, call32(ctx, fn::String_HashNoCase, {name}));
            if (e == 0) return ret(ctx, 0u);
            return ret(ctx, call32(ctx, lfn::Ui_Find, {p + 1, e}));
        } else if (load<u8>(p + 1) != '.') {
            ++p;  // ".name": from the top level
            parent = 0;
        } else {
            ++p;  // "..name": one level up per dot after the first
            while (load<u8>(p) == '.') {
                parent = parent != 0 ? field<u32>(parent, kElemParent).get() : 0u;
                ++p;
            }
        }
    } while (load<u8>(p) != 0);

    store<u8>(out, 0);
    ret(ctx, Ui_FindChild(parent, call32(ctx, fn::String_HashNoCase, {name})));
}

// AglRenderTarget* RenderTarget_Create(u32 type, bool flag)
// Allocates a render target from its free list (made on first use, the
// inlined ApeTListAllocator<AglRenderTarget>::GetAllocator()) and constructs it.
DECOMP(00522FB8, RenderTarget_Create) {
    const u32 type = arg<u32>(ctx, 0);
    const u32 flag = arg<u32>(ctx, 1);

    const u32 pool = load<u32>(lg::kRenderTargetPool);
    u32 list = load<u32>(pool);
    if (list == 0) {
        const u32 mem = call32(ctx, fn::Mem_Alloc, {kFreeListSize, 0, 16});
        if (mem != 0) {
            FreeList_Construct(ctx, mem, lg::kRenderTargetFreeListVtbl, lg::kRenderTargetFreeListName, kRtSize,
                               load<u32>(pool + 4));
            list = mem;
        }
        store<u32>(pool, list);  // a failed allocation crashes below, as in the original
    }

    const u32 rt = static_cast<u32>(call_ptr(ctx, vfunc(list, kFreeListVfnAlloc), {list}));
    if (rt == 0) return ret(ctx, 0u);

    call(ctx, lfn::RenderTarget_Construct, {rt, type});
    const u8 flags = ref<u8>(rt + kRtFlags2);
    field<u32>(rt, 0) = lg::kRenderTargetVtbl;
    ref<u8>(rt + kRtByte1816) = 0;
    field<u64>(rt, kRtU64_1808) = 0;
    field<f32>(rt, kRtF1824) = kRtF1824Value;
    ref<u8>(rt + kRtFlags2) = static_cast<u8>((flags & 0x0F) | ((flag & 1) << 7));
    field<f32>(rt, kRtF1828) = kRtF1828Value;
    // r6/r7 hold leftovers the callee doesn't read.
    callf(ctx, lfn::RenderTarget_SetParams, {rt + kRtParams, kRtParamA, kRtParamB, 0, 0, kRtParamC},
          {kRtParamF1, kRtParamF2});
    ret(ctx, rt);
}

// void RenderTarget_SetByte49(AglRenderTarget* rt, u8 value)  (name?)
DECOMP(0052038C, RenderTarget_SetByte49) {
    ref<u8>(arg<u32>(ctx, 0) + kRtByte49) = arg<u8>(ctx, 1);
}

// void RenderTarget_AddCallback(AglRenderTarget* rt, u32 list, OPD* fn, u32 user, s8 priority)
// Inserts a callback into one of the target's lists, which are kept sorted
// by priority, highest first; a new entry goes after those of equal
// priority. There is no check for a full list (4 entries).
DECOMP(00522500, RenderTarget_AddCallback) {
    const u32 rt = arg<u32>(ctx, 0);
    const u32 list = arg<u32>(ctx, 1);
    const u32 opd = arg<u32>(ctx, 2);
    const u32 user = arg<u32>(ctx, 3);
    const u8 priority_byte = arg<u8>(ctx, 4);
    const s32 priority = static_cast<s8>(priority_byte);

    const u32 tag = ref<u8>(rt + kRtTag);
    const u32 base = rt + list * kRtCallbackListSize + kRtCallbacks;
    s32 i = static_cast<s8>(static_cast<u8>(ref<u8>(base + kRtCallbackCount) - 1));
    while (i >= 0) {
        const u32 e = base + static_cast<u32>(i) * 16;
        if (static_cast<s8>(ref<u8>(e + 12)) <= priority) break;
        call(ctx, lfn::memmove, {e + 16, e, 13});  // shift it down one slot
        i = static_cast<s8>(i - 1);
    }
    const u32 slot = base + static_cast<u32>(i) * 16 + 16;
    ref<u8>(base + kRtCallbackCount) = static_cast<u8>(ref<u8>(base + kRtCallbackCount) + 1);
    auto& entry = ref<RtCallback>(slot);
    entry.opd = opd;
    entry.user = user;
    entry.tag = tag;
    entry.priority = static_cast<s8>(priority_byte);
}

// void RenderTarget_SetViewport(AglRenderTarget* rt, const ViewRect* rect)
// Copies the rectangle into the target twice (+96 and +104), clamping the
// size to the surface's, and sets flag 0x40 when it covers the whole screen.
DECOMP(005200AC, RenderTarget_SetViewport) {
    const u32 rt = arg<u32>(ctx, 0);
    const u32 rect = arg<u32>(ctx, 1);
    auto& vp = ref<ViewRect>(rt + kRtViewport);
    auto& vp2 = ref<ViewRect>(rt + kRtViewport2);

    const u32 surface = field<u32>(rt, kRtSurface);
    const u32 x = field<u16>(rect, 0);
    vp.x = static_cast<u16>(x);
    const u32 y = field<u16>(rect, 2);
    vp.y = static_cast<u16>(y);
    u32 w = field<u16>(rect, 4);
    s32 limit = static_cast<s32>(field<u32>(surface, 56).get());
    vp.width = static_cast<u16>(w);
    u32 h = field<u16>(rect, 6);
    vp.height = static_cast<u16>(h);
    u8 flags = ref<u8>(rt + kRtFlags);

    if (!(static_cast<s32>(w) < limit)) w = static_cast<u32>(limit);
    vp.width = static_cast<u16>(w);
    limit = static_cast<s32>(field<u32>(surface, 60).get());
    if (!(static_cast<s32>(h) < limit)) h = static_cast<u32>(limit);
    vp.height = static_cast<u16>(h);

    vp2.x = static_cast<u16>(x);
    vp2.y = static_cast<u16>(y);
    const u32 w16 = w & 0xFFFF;
    vp2.width = static_cast<u16>(w);
    vp2.height = static_cast<u16>(h);
    limit = static_cast<s32>(field<u32>(surface, 56).get());
    vp2.width = static_cast<u16>(static_cast<s32>(w16) < limit ? w : static_cast<u32>(limit));
    const u32 h16 = h & 0xFFFF;
    limit = static_cast<s32>(field<u32>(surface, 60).get());
    vp2.height = static_cast<u16>(static_cast<s32>(h16) < limit ? h : static_cast<u32>(limit));

    // Full screen: at the origin with the (once-clamped) size equal to config +4/+6.
    if (x == 0 && y == 0 && w16 == field<u16>(g::config, 4) && h16 == field<u16>(g::config, 6))
        flags |= 0x40;
    else
        flags &= static_cast<u8>(~0x40);
    ref<u8>(rt + kRtFlags) = flags;
}

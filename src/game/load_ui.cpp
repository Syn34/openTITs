// UI start-up (the "RenderTarget_GUI" render target), UI element lookup by
// dotted path, and the AglRenderTarget functions they use.
#include "load.h"

#include "../memory/mem.h"
#include "../string/str.h"

namespace sly4::gameload {
using namespace opentit::native;

namespace {

constexpr u32 kGuiTargetType = 2;
constexpr u32 kRtNameSize = 32;
constexpr u32 kPathBufferSize = 64;  // the original's buffer, not bounds-checked

// Render target constants set by RenderTarget_Create.
constexpr f32 kRtF1824Value = 4.0f, kRtF1828Value = 0.01f;
constexpr u32 kRtParamA = 2044, kRtParamB = 2046, kRtParamC = 240;
constexpr f32 kRtParamF1 = 16.0f, kRtParamF2 = 10.0f;  // the floats at 0x00522FB0

// First child of `parent` (or first top-level element) whose name hashes to `hash`.
FindElem* Ui_FindChild(FindElem* parent, u32 hash) {
    FindElem* e = parent ? parent->first_child.get() : host<FindElem>(global<be_u32>(lg::kUiRoot));
    for (; e; e = e->next)
        if (e->name_hash == hash) return e;
    return nullptr;
}

}  // namespace

// Creates the UI render target with two (empty) draw callbacks and sets its
// viewport to `viewport`, or to the whole screen (config +4/+6) when it is 0.
u32 Ui_Init(const ViewRect* viewport) {
    RenderTarget* target = RenderTarget_Create(kGuiTargetType, 0);
    global<be_u32>(lg::kMainView) = guest(target);
    // No null check, as in the original (a failed create copies to guest address 16).
    str::String_Copy(gref<RenderTarget>(guest(target)).name, host<const char>(lg::kRenderTargetGuiName), kRtNameSize);
    auto main_view = [] { return host<RenderTarget>(global<be_u32>(lg::kMainView)); };
    RenderTarget_SetByte49(main_view(), 0);
    RenderTarget_AddCallback(main_view(), 0, lg::kUiDrawOpd0, 0, 0);
    RenderTarget_AddCallback(main_view(), 1, lg::kUiDrawOpd1, 0, 0);

    ViewRect& rect = global<ViewRect>(lg::kUiViewRect);
    const u16 width = gref<be_u16>(g::config + 4), height = gref<be_u16>(g::config + 6);
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    RenderTarget* view = main_view();
    if (viewport) {
        rect.x = viewport->x.get();
        rect.y = viewport->y.get();
        rect.width = viewport->width.get();
        rect.height = viewport->height.get();
    }
    RenderTarget_SetViewport(view, &rect);

    global<be_u32>(lg::kUiClear0) = 0u;
    global<be_u32>(lg::kUiClear1) = 0u;
    global<be_u32>(lg::kUiClear2) = 0u;
    global<be_u32>(lg::kUiClear0 + 4) = 0u;
    global<be_u32>(lg::kUiClear0 + 8) = 0u;
    global<be_u32>(lg::kUiClear0 + 12) = 0u;
    return 1;
}

// Finds an element by a dotted path such as "Common.Cursors.01", starting
// at `parent`'s children (or the top-level elements when parent is 0). A
// leading "." starts from the top level; each extra leading dot after the
// first ("..", "...") goes up one parent. Names compare by
// String_HashNoCase. An empty path returns `parent`.
FindElem* Ui_Find(const char* path_p, FindElem* parent) {
    const u32 path = guest(path_p);
    if (path == 0 || gref<u8>(path) == 0) return parent;

    GuestFrame frame(kPathBufferSize);
    char* name = frame.as<char>();
    char* out = name;
    u32 p = path;
    do {
        if (gref<u8>(p) != '.') {
            *out++ = static_cast<char>(gref<u8>(p++));
        } else if (p != path) {
            // End of a segment: look it up, then the rest of the path below it.
            *out = 0;
            FindElem* e = Ui_FindChild(parent, str::String_HashNoCase(name));
            if (!e) return nullptr;
            return Ui_Find(host<const char>(p + 1), e);
        } else if (gref<u8>(p + 1) != '.') {
            ++p;  // ".name": from the top level
            parent = nullptr;
        } else {
            ++p;  // "..name": one level up per dot after the first
            while (gref<u8>(p) == '.') {
                parent = parent ? parent->parent.get() : nullptr;
                ++p;
            }
        }
    } while (gref<u8>(p) != 0);

    *out = 0;
    return Ui_FindChild(parent, str::String_HashNoCase(name));
}

// Allocates a render target from its free list (made on first use, the
// inlined ApeTListAllocator<AglRenderTarget>::GetAllocator()) and constructs it.
RenderTarget* RenderTarget_Create(u32 type, u32 flag) {
    be_u32* pool = host<be_u32>(global<be_u32>(lg::kRenderTargetPool));
    u32 list = pool[0];
    if (list == 0) {
        const u32 mem = guest(memory::Mem_Alloc(kFreeListSize, 0, 16));
        if (mem != 0) {
            be_u32* w = host<be_u32>(mem);
            for (u32 off = 4; off <= 48; off += 4) w[off / 4] = 0u;
            be_u16* h = host<be_u16>(mem);
            h[52 / 2] = 0;
            h[kFreeListFlags / 2] = static_cast<u16>(h[kFreeListFlags / 2] & ~0x8000);
            w[0] = lg::kRenderTargetFreeListVtbl;
            w[kFreeListName / 4] = lg::kRenderTargetFreeListName;
            FreeList_Init(host<container::FreeList>(mem), kRtSize, pool[1], nullptr, 0);
            list = mem;
        }
        pool[0] = list;  // a failed allocation crashes below, as in the original
    }

    RenderTarget* rt = vcall<RenderTarget*>(host<void>(list), kFreeListVfnAlloc);
    if (!rt) return nullptr;

    call(lfn::RenderTarget_Construct, rt, type);
    const u8 flags = rt->flags2;
    rt->vtable = lg::kRenderTargetVtbl;
    rt->b1816 = 0;
    rt->u1808 = 0u;
    rt->f1824 = kRtF1824Value;
    rt->flags2 = static_cast<u8>((flags & 0x0F) | ((flag & 1) << 7));
    rt->f1828 = kRtF1828Value;
    // Native (common/misc.cpp) but not declared in a header: reached through its address.
    call(lfn::RenderTarget_SetParams, rt->params, kRtParamA, kRtParamB, kRtParamF1, kRtParamF2, kRtParamC);
    return rt;
}

// (name?)
void RenderTarget_SetByte49(RenderTarget* rt, u8 value) { gref<RenderTarget>(guest(rt)).byte49 = value; }

// Inserts a callback into one of the target's lists, which are kept sorted
// by priority, highest first; a new entry goes after those of equal
// priority. There is no check for a full list (4 entries).
void RenderTarget_AddCallback(RenderTarget* rt, u32 list, u32 opd, u32 user, u8 priority_byte) {
    const s32 priority = static_cast<s8>(priority_byte);
    const u32 tag = gref<RenderTarget>(guest(rt)).tag;
    const u32 base = guest(rt) + list * kRtCallbackListSize + kRtCallbacks;
    u8& count = gref<u8>(base + kRtCallbackCount);
    s32 i = static_cast<s8>(static_cast<u8>(count - 1));
    while (i >= 0) {
        const u32 e = base + static_cast<u32>(i) * 16;
        if (static_cast<s8>(gref<u8>(e + 12)) <= priority) break;
        call(lfn::memmove, e + 16, e, 13u);  // shift it down one slot
        i = static_cast<s8>(i - 1);
    }
    const u32 slot = base + static_cast<u32>(i) * 16 + 16;
    count = static_cast<u8>(count + 1);
    RtCallback& entry = gref<RtCallback>(slot);
    entry.opd = opd;
    entry.user = user;
    entry.tag = tag;
    entry.priority = static_cast<s8>(priority_byte);
}

// Copies the rectangle into the target twice (+96 and +104), clamping the
// size to the surface's, and sets flag 0x40 when it covers the whole screen.
void RenderTarget_SetViewport(RenderTarget* rt_p, const ViewRect* rect) {
    RenderTarget* rt = &gref<RenderTarget>(guest(rt_p));  // unchecked, as in the original
    ViewRect& vp = rt->viewport;
    ViewRect& vp2 = rt->viewport2;
    const RtSurface& surface = gref(rt->surface);
    const u32 x = rect->x;
    vp.x = static_cast<u16>(x);
    const u32 y = rect->y;
    vp.y = static_cast<u16>(y);
    u32 w = rect->width;
    s32 limit = surface.width;
    vp.width = static_cast<u16>(w);
    u32 h = rect->height;
    vp.height = static_cast<u16>(h);
    u8 flags = rt->flags;

    if (!(static_cast<s32>(w) < limit)) w = static_cast<u32>(limit);
    vp.width = static_cast<u16>(w);
    limit = surface.height;
    if (!(static_cast<s32>(h) < limit)) h = static_cast<u32>(limit);
    vp.height = static_cast<u16>(h);

    vp2.x = static_cast<u16>(x);
    vp2.y = static_cast<u16>(y);
    const u32 w16 = w & 0xFFFF;
    vp2.width = static_cast<u16>(w);
    vp2.height = static_cast<u16>(h);
    limit = surface.width;
    vp2.width = static_cast<u16>(static_cast<s32>(w16) < limit ? w : static_cast<u32>(limit));
    const u32 h16 = h & 0xFFFF;
    limit = surface.height;
    vp2.height = static_cast<u16>(static_cast<s32>(h16) < limit ? h : static_cast<u32>(limit));

    // Full screen: at the origin with the (once-clamped) size equal to config +4/+6.
    if (x == 0 && y == 0 && w16 == gref<be_u16>(g::config + 4) && h16 == gref<be_u16>(g::config + 6))
        flags |= 0x40;
    else
        flags &= static_cast<u8>(~0x40);
    rt->flags = flags;
}

}  // namespace sly4::gameload

NATIVE(004D35AC, sly4::gameload::Ui_Init)
NATIVE(004D5544, sly4::gameload::Ui_Find)
NATIVE(00522FB8, sly4::gameload::RenderTarget_Create)
NATIVE(0052038C, sly4::gameload::RenderTarget_SetByte49)
NATIVE(00522500, sly4::gameload::RenderTarget_AddCallback)
NATIVE(005200AC, sly4::gameload::RenderTarget_SetViewport)

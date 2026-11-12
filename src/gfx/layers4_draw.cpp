// The derived render layers' item passes: ZTranslucent's depth pre-pass
// (ZLayer_DrawBody) and WaterResolve / ScreenWarp's two passes, plus the
// scene-copy helpers the second resolve pass uses.
#include "layers4.h"
#include "rsx.h"

namespace sly4::gfx {

namespace {

namespace l4 = sly4::render::layers4;
namespace l2 = sly4::render::layer2;
constexpr u32 kBinding = 0x010DC840;          // DrawBinding
constexpr u32 kCurrentSurface = 0x010DDA74;   // object whose +672 is the current view
constexpr u32 kViewMatrix = 0x010DD840;       // f32[4][4] (layers3::g::view_matrix)

// The ShaderPrograms for (sg, item): the group's picker (+16) slot 16, as
// DrawItem_Draw picks it (no view override here).
ShaderPrograms* pick_shader(Material* sg, DrawItem* item) {
    u8* picker = host<u8>(or_zero_page(sg)->w16);
    return vcall<ShaderPrograms*>(picker, l2::kPickShader, sg, item);
}

// Binds the shader's vertex (+4) and fragment (+8) programs as each draw
// inlines it.
void bind_programs(ShaderPrograms* shader) {
    shader = or_zero_page(shader);  // (unchecked by the original)
    bind_vertex(shader->vprogram);
    bind_fragment(shader->fprogram);
}

LayerEntry* entry_at(LayerEntry* e, u32 i) { return at<LayerEntry>(e, 8 * i); }

}  // namespace

// void ZLayer_DrawBody(RenderLayer* l)   (ZTranslucent's item pass; name?)
// For each run of entries with the same key: a depth-only pre-pass
// (colour writes off, depth test as the render state has it, LEQUAL,
// depth write iff the state's depth bit, cull on, CCW front face, alpha
// test and blending off) drawing every item with shader group 2's shader
// for it, its matrix in vertex constants 0..3 and DrawItem_DrawDepth;
// then colour writes back on, the render state cache invalidated and the
// run drawn normally (DrawItem_Draw), the bound shader ended.
void ZLayer_DrawBody(RenderLayer* l) {
    RenderState& st = rs();
    LayerEntry* group = or_zero_page(l->base.get());
    if (!group->item) return;
    for (;;) {
        const u32 key = group->key;
        LayerEntry* end = group;
        do end = entry_at(end, 1);
        while (end->item && end->key == key);

        emit({kColorMask, 0});
        emit({kDepthTestEnable, st.dirty_b[1]});
        emit({kDepthFunc, kGlLequal});
        emit({kDepthMask, (st.dirty_b[1] & 1) != 0 ? 1u : 0u});
        emit({kCullFaceEnable, 1});
        emit({kFrontFace, kGlCcw});
        emit({kAlphaTestEnable, 0});
        emit({kBlendEnable, 0});
        Material* sg = Shader_LoadGroup(2);
        for (LayerEntry* e = group; guest(e) < guest(end); e = entry_at(e, 1)) {
            DrawItem* item = e->item;
            DrawMesh* mesh = or_zero_page(item)->mesh;
            bind_programs(pick_shader(sg, item));
            upload_item_matrix(item);
            DrawItem_DrawDepth(mesh, item);
        }
        emit({kColorMask, 0x01010101});
        Draw_InvalidateState();
        for (LayerEntry* e = group; guest(e) < guest(end); e = entry_at(e, 1)) DrawItem_Draw(e->item);

        DrawBinding& b = global<DrawBinding>(kBinding);
        if (ShaderPrograms* shader = b.shader) {
            vcall(shader, l2::kShaderEnd, 0u);
            b.shader = nullptr;
            b.w8 = 0u;
            b.material = nullptr;
        }
        group = end;
        if (!group->item) break;
    }
}

// void ResolveLayer_Pass1(RenderLayer* l)   (name?)
// Clears alpha (Draw_Clear(CLEAR_A, 0, 1.0, 0, 255)), then with only
// alpha writes on, blending and depth writes off and the layer's four
// floats (+176) in vertex constant 4, draws every item with shader group
// 0's shader for it, its matrix in vertex constants 0..3 and the item's
// mesh (+12) slot 16. Colour writes back on, state invalidated.
void ResolveLayer_Pass1(RenderLayer* l) {
    constexpr u32 kClearA = 0x80;
    Draw_Clear(kClearA, 0, global<be_f32>(l4::kPass1ClearDepth), 0, 255);
    emit({kColorMask, 0x01000000});
    emit({kBlendEnable, 0});
    emit({kDepthMask, 0});
    call(fn::cellGcmSetVertexProgramConstants, gcm(), 4u, 4u, at<u8>(l, 176));
    Material* sg = Shader_LoadGroup(0);
    for (LayerEntry* e = or_zero_page(l->base.get()); e->item; e = entry_at(e, 1)) {
        DrawItem* item = e->item;
        DrawMesh* mesh = item->mesh;
        bind_programs(pick_shader(sg, item));
        upload_item_matrix(item);
        vcall(or_zero_page(mesh), 16, item);
    }
    emit({kColorMask, 0x01010101});
    Draw_InvalidateState();
}

// void ResolveLayer_Pass2(RenderLayer* l)   (name?)
// Counts one more layer (g::config +76), gathers up to 96 non-empty screen
// rects of the items (ResolveLayer_ItemRect; a 97th non-empty rect stops
// the gathering) and their total area. When the area is
// more than 0.9 of the screen (g::config +4 * +6), the whole colour target
// is copied (Draw_CopyScreen, once per layer count); otherwise each rect
// is copied from the scene's render target into the resolve texture
// (Draw_CopyRect). Then the items are drawn normally and the layer ended.
void ResolveLayer_Pass2(RenderLayer* l) {
    struct Rect {
        u32 x0, y0, x1, y1;
    };
    Config& cfg = config();
    be_f32 out[4];
    Rect rects[l4::kMaxRects] = {};

    LayerEntry* first = or_zero_page(l->base.get());
    cfg.layer_count = cfg.layer_count + 1;
    Texture* dst = Stats_ResolveTexture(&mem_stats());
    Target* src = Display_SelectTarget(&mem_stats());
    const be_f32* consts = host<be_f32>(l4::kPass2Consts);  // f32 0.0, f64 0.9 at +8
    f32 area = consts[0];
    u32 n = 0;
    if (first->item) {
        for (LayerEntry* e = first; e->item; e = entry_at(e, 1)) {
            ResolveLayer_ItemRect(l, e->item, &out[0], &out[1], &out[2], &out[3]);
            const f32 x0 = out[0], y0 = out[1], x1 = out[2], y1 = out[3];
            if (!(x1 > x0) || !(y1 > y0)) continue;
            if (n == l4::kMaxRects) break;
            Rect& r = rects[n];
            const f32 w = x1 - x0, h = y1 - y0;
            ++n;
            r.x0 = static_cast<u32>(fctidz(x0));
            area = fmadds(w, h, area);
            r.y0 = static_cast<u32>(fctidz(y0));
            r.x1 = static_cast<u32>(fctidz(x1));
            r.y1 = static_cast<u32>(fctidz(y1));
        }
    }

    const u32 pixels = static_cast<u32>(cfg.width) * cfg.height;  // mullw
    const s64 pixels64 = static_cast<s32>(pixels);                // extsw
    const f64 limit = static_cast<f64>(pixels64) * reinterpret_cast<const be_f64*>(consts + 2)->get();
    if (static_cast<f64>(area) > limit) {
        const u32 stamp = cfg.layer_count;
        be_u32& copied = global<be_u32>(l4::g::copy_stamp);
        if (copied != stamp) {
            copied = stamp;
            global<gp<Texture>>(l4::g::copy_texture) = Draw_CopyScreen();
        }
    } else {
        for (u32 i = 0; i < n; ++i) {
            const Rect& r = rects[i];
            Draw_CopyRect(dst, src, r.x0, r.y0, r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0);
        }
    }

    for (LayerEntry* e = or_zero_page(l->base.get()); e->item; e = entry_at(e, 1)) DrawItem_Draw(e->item);
    RenderLayer_End();
}

// Texture* Stats_ResolveTexture(MemStats* s)   (name?)
// s +100 when g::config +34 is set (per-eye buffers), else s +84.
Texture* Stats_ResolveTexture(MemStats* s) { return config().stereo != 0 ? s->wide : s->resolve; }

// Target* Display_SelectTarget(Texture* const* table)   (name?)
// The +40 render target of the table entry Display_SelectBuffer picks
// (+128 for the alternate buffer set, else +92 / +112 / +120 per eye).
Target* Display_SelectTarget(const void* table) {
    const Display* disp = or_zero_page(display());
    u32 off;
    if (alternate(disp)) off = 128;
    else if (config().stereo != 0) off = eye_offset(disp) + (mem_stats().buffer_select == 1 ? 112 : 120);
    else off = eye_offset(disp) + 92;
    return *at<gp<Target>>(at<gp<Texture>>(table, off)->get(), 40);
}

// void Draw_CopyRect(Texture* dst, Target* src, u32 sx, u32 sy, u32 dx, u32 dy, u32 w, u32 h)   (name?)
// cellGcmSetTransferImage (local to local) of a w x h rect from src (+4
// offset << 1 | location, +32 pitch, +42 first visible row) at (sx, sy)
// to dst (+32 pitch, +36 offset, +44 format) at (dx, dy).
void Draw_CopyRect(Texture* dst, Target* src, u32 sx, u32 sy, u32 dx, u32 dy, u32 w, u32 h) {
    const s32 bpp = format_desc(dst->format_index)->bpp;
    const u32 bytes = static_cast<u32>(bpp / 8) & 0xFF;  // srawi + addze
    const u32 pitch = src->pitch;
    const u32 offset = (src->offset_loc >> 1) + pitch * src->y;
    call(fn::cellGcmSetTransferImage, gcm(), 0u, dst->gcm.offset.get(), dst->gcm.pitch.get(), dx, dy, offset, pitch, sx,
         sy, w, h, bytes);
}

// Texture* Draw_CopyScreen()   (name?)
// Copies the whole colour target into Stats_ResolveTexture's texture and
// returns that texture.
Texture* Draw_CopyScreen() {
    MemStats& ms = mem_stats();
    Texture* tex = config().stereo != 0 ? ms.wide : ms.resolve;
    copy_colour_to(tex);
    return tex;
}

// void ResolveLayer_ItemRect(RenderLayer* l, DrawItem* item, f32* x0, f32* y0, f32* x1, f32* y1)   (name?)
// The item's screen rect in pixels: the mesh's bounding sphere (+92:
// centre, radius) through the item matrix (vmaddfp per lane, + row 3),
// the radius scaled by the largest of the three scales DrawItem_GetScale
// writes; a zero radius gives an empty rect (all 0). Otherwise
// View_ProjectSphere(sphere, view_matrix, view +1008, outs) and the
// result scaled by the screen size (g::config +4 / +6).
void ResolveLayer_ItemRect(RenderLayer*, DrawItem* item, be_f32* px0, be_f32* py0, be_f32* px1, be_f32* py1) {
    const be_f32* bound = at<be_f32>(item->mesh.get(), 92);
    const f32 bx = bound[0], by = bound[1], bz = bound[2], br = bound[3];
    const be_f32* rows = aligned16(item);  // lvx
    f32 c[4];
    for (u32 k = 0; k < 4; ++k) {
        f32 v = std::fmaf(rows[k], bx, 0.0f);
        v = std::fmaf(rows[4 + k], by, v);
        v = std::fmaf(rows[8 + k], bz, v);
        c[k] = rows[12 + k] + v;  // vaddfp
    }
    be_f32 scales[3];
    DrawItem_GetScale(reinterpret_cast<const be_f32*>(item), scales);
    const f32 sx = scales[0], sy = scales[1], sz = scales[2];
    f32 m = sx - sy >= 0.0f ? sx : sy;  // fsubs + fsel
    m = m - sz >= 0.0f ? m : sz;
    const f32 radius = m * br;
    if (radius == global<be_f32>(l4::kItemRectZero)) {
        for (be_f32* p : {px0, py0, px1, py1}) *reinterpret_cast<be_u32*>(p) = 0u;
        return;
    }
    be_f32 sphere[4];
    sphere[3] = radius;
    sphere[1] = c[1];
    sphere[0] = c[0];
    sphere[2] = c[2];
    u8* view = *at<gp<u8>>(global<gp<u8>>(kCurrentSurface).get(), 672);
    View_ProjectSphere(sphere, host<be_f32>(kViewMatrix), at<be_f32>(view, 1008), px0, py0, px1, py1);
    const Config& cfg = config();
    const u32 w = cfg.width, h = cfg.height;
    const auto scale = [](be_f32* p, u32 n) { *p = *p * static_cast<f32>(n); };
    scale(px0, w);
    scale(py0, h);
    scale(px1, w);
    scale(py1, h);
}

}  // namespace sly4::gfx

NATIVE(00513B78, sly4::gfx::ZLayer_DrawBody)
NATIVE(00514200, sly4::gfx::ResolveLayer_Pass1)
NATIVE(00514838, sly4::gfx::ResolveLayer_Pass2)
NATIVE(00536D28, sly4::gfx::Stats_ResolveTexture)
NATIVE(00537CB8, sly4::gfx::Display_SelectTarget)
NATIVE(00536C3C, sly4::gfx::Draw_CopyRect)
NATIVE(00536D4C, sly4::gfx::Draw_CopyScreen)
NATIVE(005145C0, sly4::gfx::ResolveLayer_ItemRect)

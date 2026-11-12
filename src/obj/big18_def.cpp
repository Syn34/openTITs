// Game code, large-function sweep part 18: callees of part 17 (the def
// base constructor, the Havok body-record vector push, the physics object
// mode change).
#include "big18.h"

namespace sly4::obj {

namespace {
constexpr u32 DefBase_Init_000C26D8 = 0x000C26D8;   // (p, rec): the inner base constructor (name?)
constexpr u32 EffectDef_Find = 0x004BDEDC;          // (u32 hash, 0, 1) -> def or 0
constexpr u32 EffectDef_HasCommands = 0x004B95F0;   // (def) -> bool
constexpr u32 Hash_ToString = 0x0056A6E0;           // (u32 hash) -> const char*
constexpr u32 Hk_RemoveEntity = 0x0092FFC8;         // (u8* out, world, body)
constexpr u32 Hk_AddEntity = 0x0092F2A8;            // (world, body, 1)
constexpr u32 Volume_Notify112D8 = 0x000112D8;      // (volumes, Volume*)
constexpr u32 Shape_InitBody = 0x00011410;          // (Shape* out, volumes, body, const T* xf, shape, u32 mask)
constexpr u32 Volume_SetShape637D14 = 0x00637D14;   // (Volume*, const Shape*)
constexpr u32 Shape_Release638498 = 0x00638498;     // (Shape*)

constexpr u32 kDefVt = 0x00B11D98;                  // the def base vtable
constexpr u32 kMissingScript = 0x00AC6714;          // "missing lifetime script '%s' for def '%s'"
constexpr u32 kNoCommands = 0x00AC6744;             // "entity def '%s' lifetime script '%s' has no commands"
constexpr u32 kNoCommandsArg = 0x00B0F050;          // the string passed as its second '%s'
constexpr u32 kVolumes = 0x00F557EC;                // the volume manager
constexpr u32 kWorldPhys18 = 0x001281D4;            // World +0x1281D4: the physics holder (+4 hkpWorld*)

}  // namespace

// void f(BodyVector* v, const BodyRec* src) (no caller reads r3 or f1):
// grows to twice the capacity when full (PtrVector_Reserve), copies the
// 160-byte record's used words (+0..8, +16..24, +32, +36, +48..56,
// +64..88, +96..120, the top four bits of +124 one at a time over the old
// ones, +128..136, +144) to data + count * 160, bumps the count and the
// mark.
void BodyVector_Add_00745720(BodyVec160* v, const BodyRec160* s) {
    u32 n = v->count;
    const u32 cap = v->cap;
    if (n == cap) {
        call(fn::PtrVector_Reserve, v, (cap << 1) & 0x0001FFFEu);
        n = v->count;
    }
    BodyRec160& d = gm<BodyRec160>(v->data.ea + n * 160u);
    static constexpr u32 kW[] = {0, 1, 2, 4, 5, 6, 8, 9, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 24, 25, 26, 27, 28};
    for (const u32 k : kW) d.w[k] = s->w[k].get();
    const u32 w116 = s->w[29];
    u32 x = d.w[31];
    d.w[29] = w116;
    d.w[30] = s->w[30].get();
    x = (x & 0x7FFFFFFFu) | (s->w[31] & 0x80000000u);
    d.w[31] = x;
    for (u32 m = 0x40000000u; m >= 0x10000000u; m >>= 1) {
        x = (x & ~m) | (s->w[31] & m);
        d.w[31] = x;
    }
    d.w[32] = s->w[32].get();
    d.w[33] = s->w[33].get();
    d.w[34] = s->w[34].get();
    d.w[36] = s->w[36].get();
    const u32 n1 = v->count + 1;
    const u32 c = n1 & 0xFFFFu;
    const u32 hi = v->mark;
    v->count = static_cast<u16>(n1);
    v->mark = static_cast<u16>(static_cast<s32>(hi) > static_cast<s32>(c) ? hi : n1);
}

// void f(Def* p, const Record* rec) (no caller reads r3 or f1):
// DefBase_Init_000C26D8(p, rec), the vtable, +140 / +148 / +152 = rec +76
// / +80 / +84, +144 = 0, +176..+184 = rec +108..+116, bits 0xFFF0 of u16
// +188 from rec +120 (one bit at a time, then the nibble 0x00F0),
// +156..+172 = rec +88..+104, u16 +190 = rec +122; +144 =
// EffectDef_Find(rec +80, 0, 1) (the lifetime script). A missing script
// (hash set) is reported, and one without commands too.
void DefBase_Ctor_000C2E8C(Def18* p, const DefRec18* rec) {
    call(DefBase_Init_000C26D8, p, rec);
    p->w148 = rec->script.get();
    p->w140 = rec->w76.get();
    p->w152 = rec->w84.get();
    p->vt = kDefVt;
    p->script = 0u;
    p->w180 = rec->w112.get();
    p->w176 = rec->w108.get();
    u32 h = p->h188;
    p->w184 = rec->w116.get();
    for (u32 m = 0x8000u; m >= 0x0100u; m >>= 1) {
        h = (h & ~m) | (rec->h120 & m);
        p->h188 = static_cast<u16>(h);
    }
    h = (h & ~0x00F0u) | (rec->h120 & 0x00F0u);
    p->h188 = static_cast<u16>(h);
    u32 f[5];
    for (u32 k = 0; k < 5; ++k) f[k] = rec->w88[k];
    const u16 h122 = rec->h122;
    const u32 hash = rec->script;
    p->w156[0] = f[0];
    p->w156[1] = f[1];
    p->h190 = h122;
    p->w156[2] = f[2];
    p->w156[3] = f[3];
    p->w156[4] = f[4];
    u32 r = call<u32>(EffectDef_Find, hash, 0u, 1u);
    const u32 h80 = rec->script;
    p->script = r;
    if (h80 != 0 && r == 0) {
        const u32 s = call<u32>(Hash_ToString, h80);
        call(fn::String_Format, kMissingScript, s, rec->name.get());
        r = p->script;
    }
    if (r == 0) return;
    if (call<u32>(EffectDef_HasCommands, r) == 0) call(fn::String_Format, kNoCommands, &p->name[0], kNoCommandsArg);
}

// void f(PhysObj* o, s32 mode) (no caller reads r3 or f1): nothing when
// the mode (bits 0xF000 of u16 +432) is already `mode`. Else, by the new
// mode: 0 / 4: a body in a world (+8) is removed from it and the volume
// +436 notified; 3: +420 = 0; 1: vfunc 88(o, 1, 1, 1, 1), then a body gets
// its collision filter (+44: low 5 bits kept, high half = o +204) and,
// when not in a world, is added to the physics world; if vfunc 168(o) then
// the template's vfunc 12 transform (to the frame) shapes the volume +436
// (Shape_InitBody with mask 0xFFFF, or 0xFFFE without bit 0x01000000 of
// the template +300, and vfunc 76(o) as the shape); +420 = 0. The mode is
// stored.
void PhysObj_Finish_001F9F70(PhysObj18* o, s32 mode) {
    GuestFrame frame(208);
    u8* sp = frame.as<u8>();
    const u32 h = o->mode;
    PhysTmpl18* tmpl = o->tmpl;
    HkBody18* body = o->body;
    if (static_cast<s32>((h >> 12) & 0xFu) == mode) return;
    if (mode == 4 || mode == 0) {
        if (body != nullptr) {
            const u32 world = body->world;
            if (world != 0) {
                call(Hk_RemoveEntity, sp + 148, world, body);
                call(Volume_Notify112D8, global<be_u32>(kVolumes), &o->volume[0]);
            }
        }
    } else if (mode == 3) {
        o->w420 = 0u;
    } else if (mode == 1) {
        vcall(o, 88, 1u, 1u, 1u, 1u);
        HkBody18* b = o->body;
        if (b != nullptr) {
            const u32 x = (b->filter & 0x1Fu) | (o->layer << 16);
            const u32 world = b->world;
            b->filter = x;
            if (world == 0) {
                const u32 phys = gm<be_u32>(global<be_u32>(kWorld) + kWorldPhys18);
                call(Hk_AddEntity, gm<be_u32>(phys + 4).get(), b, 1u);
                if (vcall<u32>(o, 168) != 0) {
                    PhysTmpl18* t = o->tmpl;
                    call_ptr(vfn(t, 12), sp + 112, t);
                    const u32 flag = (tmpl->flags.w >> 24) & 1u;
                    HkBody18* b2 = o->body;
                    const u32 mask = flag != 0 ? 0xFFFFu : 0xFFFEu;
                    const u32 vols = global<be_u32>(kVolumes);
                    const u32 shape = vcall<u32>(o, 76);
                    call(Shape_InitBody, sp + 144, vols, b2, sp + 112, shape, mask);
                    call(Volume_SetShape637D14, &o->volume[0], sp + 144);
                    call(Shape_Release638498, sp + 144);
                }
            }
        }
        o->w420 = 0u;
    }
    o->mode = static_cast<u16>((o->mode & ~0xF000u) | ((static_cast<u32>(mode) << 12) & 0xF000u));
}

}  // namespace sly4::obj

NATIVE(00745720, sly4::obj::BodyVector_Add_00745720)
NATIVE(000C2E8C, sly4::obj::DefBase_Ctor_000C2E8C)
NATIVE(001F9F70, sly4::obj::PhysObj_Finish_001F9F70)

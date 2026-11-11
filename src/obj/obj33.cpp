// Game code, part 33: the small-function sweep continued from the last 61
// instruction functions (size order; obj32*.cpp did 57-61). Each was
// checked against the original in the PPC simulator (memory writes, calls
// and their arguments, r3 and f1).
#include "obj33.h"

using namespace sly4;
using namespace sly4::obj33;

// ---------------------------------------------------------------- sweep: 61 instructions
// T* f(a, b, src, s32 n, e): an animation stream (Stream_Init7CE648 on a
// local) and the skeleton h = 0x00804570(src, n, &out, {0, 0, 0} by value);
// for a skeleton, r = Scene_Load348F04(a, b, h, n, e) and
// AnimSkel_Release7D4EA8(h). Stream_Free7CEA58 either way. r3 = r (0 for
// no skeleton).
DECOMP(0034A108, Scene_LoadSkel_0034A108) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), src = arg<u32>(ctx, 2), n = arg<u32>(ctx, 3), e = arg<u32>(ctx, 4);
    Stack frame(ctx, 16);
    const u32 out = frame.addr(), strm = out + 4;  // sp+112 / sp+116
    store<u32>(out, 0);
    call(ctx, 0x007CE648, {strm});
    // The 12-byte zero struct goes in r6:r7 (written to the parameter save
    // area and reloaded with ld; the low half of r7 is padding).
    const u32 h = static_cast<u32>(call(ctx, 0x00804570, {src, Sx33(n), out, 0, 0}));
    if (h == 0) {
        call(ctx, 0x007CEA58, {strm});
        ctx.gpr[3] = 0;
        return;
    }
    const u32 r = static_cast<u32>(call(ctx, 0x00348F04, {a, b, h, n, e}));
    call(ctx, 0x007D4EA8, {h});
    call(ctx, 0x007CEA58, {strm});
    ctx.gpr[3] = r;
}

// void f(u8 ip[4]): the console's IP address (cellNetCtlGetInfo(16 =
// CELL_NET_CTL_INFO_IP_ADDRESS, &info) parsed with "%d.%d.%d.%d"), as four
// bytes in ip[0..3] (all 0 when unavailable or malformed), and the u16
// ip+4 = 14120. r3 = 14120.
DECOMP(00426CE0, Net_GetIpAddress_00426CE0) {
    const u32 ip = arg<u32>(ctx, 0);
    Stack frame(ctx, 288);
    const u32 b = frame.addr();  // sp+112: the bytes; +8..+20: the four ints; +24: the info union
    const u32 v = b + 8, info = b + 24;
    store<u32>(b, 0);
    // r5 = 0 is left over from the store above.
    if (static_cast<u32>(call(ctx, 0x00ABDE44, {16, info, 0})) == 0 &&
        static_cast<s32>(call(ctx, 0x0071EC78, {info, 0x00AE212Cu, v, v + 4, v + 8, v + 12})) == 4) {
        store<u16>(b + 4, 0);
        store<u8>(b, static_cast<u8>(load<u32>(v)));
        store<u8>(b + 1, static_cast<u8>(load<u32>(v + 4)));
        store<u8>(b + 2, static_cast<u8>(load<u32>(v + 8)));
        store<u8>(b + 3, static_cast<u8>(load<u32>(v + 12)));
    }
    store<u8>(ip, load<u8>(b));
    store<u8>(ip + 1, load<u8>(b + 1));
    store<u8>(ip + 2, load<u8>(b + 2));
    store<u16>(ip + 4, 14120);
    store<u8>(ip + 3, load<u8>(b + 3));
    ctx.gpr[3] = 14120;
}

// bool f(Reader* r): the current chunk read whole into the default memory
// stack, opened as a zip (Archive_OpenZip(buf, n, *0x00F65D30)) and loaded
// as XML (Xml_LoadFile_000D643C(z, 254)); then z->vt[8](z), the stack's
// vt[8](stack, buf) and *0x00F65D30 = 0. r3 = 1.
DECOMP(0048C294, Archive_LoadChunkZip_0048C294) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4, {r}));
    const u32 st = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260C, {st, n}));
    call(ctx, 0x00563768, {r, buf, n});
    const u32 z = static_cast<u32>(call(ctx, 0x00567034, {buf, n, load<u32>(0x00F65D30)}));
    call(ctx, 0x000D643C, {z, 254});
    VCall(ctx, z, 8, {z});
    const u32 st2 = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    VCall(ctx, st2, 8, {st2, buf});
    store<u32>(0x00F65D30, 0);
    ctx.gpr[3] = 1;
}

// void f(Vec3* out, const f32 xy[2]; f1 t): the screen point xy unprojected
// by the world's view v = *0x00F65CA0->+0x128168 (its +60 parameter set to
// the constant *0x004A58B8 for Call_0052296C(v, &p, xy), then restored),
// and out = lerp(v->+864, p, t). r3 = v + 864, f1 = out.x.
DECOMP(004A58C0, View_UnprojectLerp_004A58C0) {
    const u32 out = arg<u32>(ctx, 0), xy = arg<u32>(ctx, 1);
    const f64 t = ctx.fpr[1];
    const u32 v = load<u32>(load<u32>(0x00F65CA0) + 0x128168);
    Stack frame(ctx, 32);
    const u32 p = frame.addr(), q = p + 16;  // sp+112 / sp+128
    CopyW32(q, xy);
    CopyW32(q + 4, xy + 4);
    const f64 keep = Lfs32(v + 60);
    callf(ctx, 0x00520338, {v}, {Lfs32(0x004A58B8)});
    call(ctx, 0x0052296C, {v, p, q});
    callf(ctx, 0x00520338, {v}, {keep});
    const u32 c = v + 864;
    f64 r[3];
    for (u32 k = 0; k < 3; ++k) {
        const f64 b = Lfs32(c + 4 * k);
        const f64 d = static_cast<f32>(Lfs32(p + 4 * k) - b);
        r[k] = static_cast<f32>(std::fma(d, t, b));
    }
    for (u32 k = 0; k < 3; ++k) Stfs32(out + 4 * k, r[k]);
    ctx.gpr[3] = c;
    ctx.fpr[1] = r[0];
}

// bool f(Reader* r): the 64-byte name table 0x010C2989 (zeroed with its
// u8 0x010C2988 once, flag u8 0x010C2980): a byte (Iff_ReadU8, ignored)
// then 64 bytes read in; *0x00F65FF0 = the table; with the settings
// object g = *0x010C2C28, 0x004ECF08(g, 0x3D50438C, table). r3 = 1.
DECOMP(004BE75C, Settings_ReadNameTable_004BE75C) {
    const u32 r = arg<u32>(ctx, 0);
    if (load<u8>(0x010C2980) == 0) {
        call(ctx, 0x00726E54, {0x010C2988u, 0, 65});
        store<u8>(0x010C2980, 1);
    }
    Stack frame(ctx, 16);
    call(ctx, 0x005643C4, {r, frame.addr(), 0});  // sp+112
    constexpr u32 table = 0x010C2989u;
    for (u32 i = 0; i < 64; ++i) call(ctx, 0x0056998C, {r, table + i, 0});
    const u32 g = load<u32>(kGlobals10C2C28);
    store<u32>(0x00F65FF0, table);
    if (g != 0) call(ctx, 0x004ECF08, {g, 0x3D50438Cu, table});
    ctx.gpr[3] = 1;
}

// bool f(Xml* x): unless the settings object g (*0x010C2C28) exists with
// its u8 +584 set (then r3 = 1), the attribute "..." (0x00AF0138) hashed
// (String_HashNoCase) and appended to the u16-counted vector g+256 (data
// +0, capacity +4 doubled when full, count +8, high-water +10). r3 = 1, or
// 0 for no attribute.
DECOMP(004ED6A0, Settings_AddLevel_004ED6A0) {
    const u32 x = arg<u32>(ctx, 0);
    u32 g = load<u32>(kGlobals10C2C28);
    if (g != 0 && load<u8>(g + 584) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AF0138u}));
    if (a == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {a}));
    g = load<u32>(kGlobals10C2C28);
    const u32 v = g + 256;
    const u32 cap = load<u16>(v + 4);
    if (load<u16>(v + 8) == cap) call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
    store<u32>(load<u32>(v) + ((static_cast<u32>(load<u16>(v + 8)) << 2) & 0x3FFFCu), h);
    const u32 n = load<u16>(v + 8) + 1u;
    const u32 hw = load<u16>(v + 10);
    store<u16>(v + 8, static_cast<u16>(n));
    store<u16>(v + 10, static_cast<u16>(static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n));
    ctx.gpr[3] = 1;
}

// bool f(Xml* x): with the settings object g (*0x010C2C28; r3 = 0
// without), the bool 0x00AF0144 -> g+589 (default 0), the f32 0x00AF014C
// -> g+596 (default: current) and the f32 0x00AF015C -> g+592 (default
// K[0]), the last clamped with fsel to K[0]..K[1] (K = 0x004ED798; out of
// range values become the doubles K+16 / K+8). r3 = 1, f1 = g->+592.
DECOMP(004ED7B0, Settings_ReadStereo_004ED7B0) {
    const u32 x = arg<u32>(ctx, 0);
    const u32 g = load<u32>(kGlobals10C2C28);
    if (g == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    constexpr u32 K = 0x004ED798;
    call(ctx, o33fn::XmlReadBoolByName, {x, 0x00AF0144u, g + 589, 0});
    callf(ctx, o33fn::XmlReadF32ByName, {x, 0x00AF014Cu, g + 596}, {Lfs32(g + 596)});
    const f64 lo = Lfs32(K);
    callf(ctx, o33fn::XmlReadF32ByName, {x, 0x00AF015Cu, g + 592}, {lo});
    const f64 hi = Lfs32(K + 4), v = Lfs32(g + 592);
    const f64 dhi = std::bit_cast<f64>(load<u64>(K + 8)), dlo = std::bit_cast<f64>(load<u64>(K + 16));
    f64 r = static_cast<f32>(Fsel33(static_cast<f32>(hi - v), v, dhi));
    r = static_cast<f32>(Fsel33(static_cast<f32>(r - lo), r, dlo));
    Stfs32(g + 592, r);
    ctx.gpr[3] = 1;
    ctx.fpr[1] = r;
}

// ~T() (vt 0x00B11E88): SmallFree(o->+680, o->+696 * 16), the pointer
// vectors +620 / +600 / +580 / +556 / +536 cleared, vt 0x00B126B8, then
// VtB12550_Dtor_000DF934(o). r3 as that left it.
DECOMP(007440EC, Dtor_B11E88_007440EC) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B11E88);
    call(ctx, o32fn::SmallFree, {load<u32>(o + 680), load<u32>(o + 696) << 4});
    for (const u32 off : {620u, 600u, 580u, 556u, 536u}) PvClear32(ctx, o + off);
    store<u32>(o, 0x00B126B8);
    ctx.gpr[3] = call(ctx, 0x000DF934, {o});
}

// ~T() of a Havok array holder (o may be 0): count +12 = 0; unless flag
// 0x80000000 of +16 (don't deallocate) is set, vt 0x00B1B6F4 and the
// buffer +8 (capacity: +16 & 0x3FFFFFFF of 48-byte elements) freed by the
// allocator 0x011DF31C (vt[20]); then +8 = 0, vt 0x00B13974, +16 =
// 0x80000000 and the thread's memory router ->+44 vt[12](it, o, u16 +4)
// frees o itself. r3 as that call left it.
DECOMP(0075D4D0, HkArray48_Delete_0075D4D0) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 12, 0);
    const u32 w = load<u32>(o + 16);
    if ((w & 0x80000000u) == 0) {
        store<u32>(o, 0x00B1B6F4);
        const u32 bytes = static_cast<u32>(static_cast<u64>(w & 0x3FFFFFFFu) * 48);
        VCall(ctx, 0x011DF31Cu, 20, {0x011DF31Cu, load<u32>(o + 8), Sx33(bytes)});
    }
    store<u32>(o + 8, 0);
    store<u32>(o, 0x00B13974);
    const u32 router = HkRouter33(ctx);
    const u32 sz = load<u16>(o + 4);
    store<u32>(o + 16, 0x80000000u);
    const u32 rt = load<u32>(router + 44);
    ctx.gpr[3] = VCall(ctx, rt, 12, {rt, o, sz});
}

// s16 f(u16 a, u8** pp, u32* link): PushRecord33, the link target's u32
// cleared.
DECOMP(007737A4, Block_PushRecord_007737A4) { PushRecord33(ctx, true); }

// ---------------------------------------------------------------- sweep: 62 instructions
// void f(T* o, const Vec3* fwd): unless fwd's x and z both equal K[0]
// (K = 0x00015F88: 0, 1), the basis from fwd flattened to (x, K[0], z):
// o+144..+152 = the up vector 0x01164B60 (raw), o+176..+184 rewritten as
// is, o+160..+168 = the normalised forward and o+128..+136 =
// normalise(up x fwd). r3 = o, f1 = the right z (fwd.x when skipped).
DECOMP(00015F90, Basis_FromForward_00015F90) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1);
    constexpr u32 K = 0x00015F88, U = 0x01164B60;
    const f64 k0 = Lfs32(K);
    f64 x = Lfs32(v);
    ctx.fpr[1] = x;
    if (x == k0 && Lfs32(v + 8) == k0) return;
    f64 z = Lfs32(v + 8);
    f64 l = static_cast<f32>(k0 * k0);
    l = static_cast<f32>(std::fma(x, x, l));
    l = static_cast<f32>(std::fma(z, z, l));
    const f64 one = Lfs32(K + 4);
    const f64 ux = Lfs32(U), uz = Lfs32(U + 8), uy = Lfs32(U + 4);
    const u32 c0 = load<u32>(o + 176), c1 = load<u32>(o + 180), c2 = load<u32>(o + 184);
    l = static_cast<f32>(std::sqrt(l));
    CopyW32(o + 144, U);
    CopyW32(o + 148, U + 4);
    CopyW32(o + 152, U + 8);
    store<u32>(o + 176, c0);
    store<u32>(o + 180, c1);
    store<u32>(o + 184, c2);
    const f64 s = static_cast<f32>(one / l);
    z = static_cast<f32>(z * s);
    x = static_cast<f32>(x * s);
    const f64 y = static_cast<f32>(k0 * s);
    const f64 t3 = static_cast<f32>(ux * z), t10 = static_cast<f32>(uy * x), t9 = static_cast<f32>(uz * y);
    Stfs32(o + 160, x);
    const f64 ry = static_cast<f32>(std::fma(uz, x, -t3));
    const f64 rz = static_cast<f32>(std::fma(ux, y, -t10));
    const f64 rx = static_cast<f32>(std::fma(uy, z, -t9));
    Stfs32(o + 164, y);
    Stfs32(o + 168, z);
    f64 m = static_cast<f32>(ry * ry);
    m = static_cast<f32>(std::fma(rx, rx, m));
    m = static_cast<f32>(std::fma(rz, rz, m));
    m = static_cast<f32>(std::sqrt(m));
    const f64 q = static_cast<f32>(one / m);
    const f64 ox = static_cast<f32>(q * rx), oy = static_cast<f32>(q * ry), oz = static_cast<f32>(q * rz);
    Stfs32(o + 128, ox);
    Stfs32(o + 132, oy);
    Stfs32(o + 136, oz);
    ctx.fpr[1] = oz;
}

// T f(T* o): with the renderer flag (u8 0x00F9C01C+9) and o's +6260 (c),
// +6272, +6268 and +6184 all set: the mutex m = c->+672 locked
// (Call_00548508), and with the flag still set FrameBuffer_Reset_00523584(
// *0x010DDBB4, q->+8, q->+12) for q = 0x00F9C01C->+4 (when its +12 is set)
// and CallbackList_Invoke(*0x00F65C90->+12, *0x00F9C01C->+28) (when set);
// then Sync_00548550(m, 0). r3 = that, else o (0 when o->+6184 is 0).
DECOMP(00057178, Render_FlushLocked_00057178) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 G = 0x00F9C01C;
    ctx.gpr[3] = o;
    if (load<u8>(G + 9) == 0) return;
    const u32 c = load<u32>(o + 6260);
    if (c == 0 || load<u32>(o + 6272) == 0 || load<u32>(o + 6268) == 0) return;
    if (load<u32>(o + 6184) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 m = load<u32>(c + 672);
    call(ctx, 0x00548508, {m});
    if (load<u8>(G + 9) != 0) {
        const u32 fb = load<u32>(0x010DDBB4);
        const u32 q = load<u32>(G + 4);
        const u32 b = load<u32>(q + 12);
        if (b != 0) call(ctx, 0x00523584, {fb, load<u32>(q + 8), b});
        const u32 cb = load<u32>(load<u32>(G) + 28);
        const u32 l = load<u32>(load<u32>(0x00F65C90) + 12);
        if (cb != 0) call(ctx, 0x005651B0, {l, cb});
    }
    ctx.gpr[3] = call(ctx, 0x00548550, {m, 0});
}

// T f(T* o): the three attachments o->+348.. each detached
// (Obj_Detach0D72C0(o->+48, a)), told vt[40](a, 1) and cleared; o->+340
// (when set) gets vt[40](it, 0); then 0x00012928(o). r3 as that left it.
DECOMP(00061E98, Obj_DropAttachments_00061E98) {
    const u32 o = arg<u32>(ctx, 0);
    for (u32 i = 0; i < 3; ++i) {
        const u32 f = o + 348 + 4 * i;
        const u32 a = load<u32>(f);
        if (a == 0) continue;
        call(ctx, 0x000D72C0, {load<u32>(o + 48), a});
        const u32 b = load<u32>(f);
        VCall(ctx, b, 40, {b, 1});
        store<u32>(f, 0);
    }
    const u32 q = load<u32>(o + 340);
    if (q != 0) VCall(ctx, q, 40, {q, 0});
    ctx.gpr[3] = call(ctx, 0x00012928, {o});
}

// T* f(?, Xml* x): 24-byte node (vt 0x00B0FB58, +12/+16 = 0, f32 +20 =
// -1): hash 0x7FAAD4A0 -> +12, hash 0xFF457B34 -> +16, f32 0xFF478E71 ->
// +20.
DECOMP(0007D6D0, XmlNew_B0FB58_0007D6D0) {
    XmlGen32(ctx, 24, 0x00B0FB58, {{12, 'w'}, {16, 'w'}, {20, 'C', 0xBF800000u}, {0, 'v'}},
             {{'H', 0x7FAAD4A0u, 12, false}, {'H', 0xFF457B34u, 16, true}, {'F', 0xFF478E71u, 20, true}});
}

// T* f(?, Xml* x): 24-byte node (vt 0x00B10788, s32 +12 = -1, +16 = 0, u8
// +20 = 0): s32 0xA212C671 -> +12 (default -1), hash 0x79F40122 -> +16,
// bool 0x8C830DC3 -> +20.
DECOMP(0008D24C, XmlNew_B10788_0008D24C) {
    XmlGen32(ctx, 24, 0x00B10788, {{12, 'C', 0xFFFFFFFFu}, {0, 'v'}, {16, 'w'}, {20, 'b'}},
             {{'S', 0xA212C671u, 12, true}, {'H', 0x79F40122u, 16, false}, {'B', 0x8C830DC3u, 20, false}});
}

// T* f(?, Xml* x): 24-byte node (vt 0x00B11000, +12/+16 = 0, u8 +20 = 1):
// hash 0x7A30D7E3 -> +12, hash 0x8D47DA8F -> +16, bool 0x8488565B -> +20.
DECOMP(0009B7DC, XmlNew_B11000_0009B7DC) {
    XmlGen32(ctx, 24, 0x00B11000, {{12, 'w'}, {16, 'w'}, {20, 'B'}, {0, 'v'}},
             {{'H', 0x7A30D7E3u, 12, false}, {'H', 0x8D47DA8Fu, 16, true}, {'B', 0x8488565Bu, 20, true}});
}

// T f(?, CamRig* c, u32 msg): camera rig messages: 0x9535758E resets the
// two limit records c->+4 / c->+8 ({+4, +8} = {0, 1} / {1, 0}, +20/+24 =
// -60 / 140) after c->vt[24](c; f1 = *0x00090BFC) (r3 = 0);
// 0x86DF6221 calls CamRig_Reset (which reads no registers); 0x67430483 /
// 0x466CDEF5 give r3 = c; anything else leaves r3 = 0x86DF6221.
DECOMP(00090C00, CamRig_OnMessage_00090C00) {
    const u32 c = arg<u32>(ctx, 1), msg = arg<u32>(ctx, 2);
    if (msg == 0x67430483u || msg == 0x466CDEF5u) {
        ctx.gpr[3] = c;
        return;
    }
    if (msg == 0x9535758Eu) {
        const u32 a = load<u32>(c + 4), b = load<u32>(c + 8);
        ctx.fpr[1] = Lfs32(0x00090BFC);
        VCall(ctx, c, 24, {c});
        store<u32>(a + 4, 0);
        store<u32>(a + 8, 0x3F800000);
        store<u32>(b + 4, 0x3F800000);
        store<u32>(b + 8, 0);
        store<u32>(a + 20, 0xC2700000);
        store<u32>(a + 24, 0x430C0000);
        store<u32>(b + 20, 0xC2700000);
        store<u32>(b + 24, 0x430C0000);
        ctx.gpr[3] = 0;
        return;
    }
    if (msg == 0x86DF6221u) {
        ctx.gpr[3] = call(ctx, 0x0042A740, {});
        return;
    }
    ctx.gpr[3] = 0x86DF6221u;
}

// T f(T* o, Ent* e, s32* out): for an entry e of kind (s8 +24) 1 with a
// body e + (s8)e->+16: the first object p of o's vector +248 (u16 count
// +8) with p->+12 clear and Obj_GetBody_00252940(p) equal to it gives
// *out = -1 and r3 = o->+160; otherwise r3 = 0.
DECOMP(000DA85C, Obj_FindBodyOwner_000DA85C) {
    const u32 o = arg<u32>(ctx, 0), e = arg<u32>(ctx, 1), out = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (static_cast<s8>(load<u8>(e + 24)) != 1) return;
    const u32 t = e + static_cast<u32>(static_cast<s32>(static_cast<s8>(load<u8>(e + 16))));
    if (t == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 248);
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) {
            ctx.gpr[3] = 0;
            return;
        }
        const u32 p = load<u32>(load<u32>(v) + 4 * i);
        if (p == 0 || load<u32>(p + 12) != 0) continue;
        if (static_cast<u32>(call(ctx, 0x00252940, {p})) != t) continue;
        const u32 r = load<u32>(o + 160);
        store<u32>(out, 0xFFFFFFFFu);
        ctx.gpr[3] = r;
        return;
    }
}

// T f(T* o, Hit* m): for a hit passing Obj_TestE0BCC(o, m) and the tuning
// o->+216->+248 above K[1] (K = 0x000E84B8): the reaction o->vt[352](o, -,
// m->+4, 0x03C599D8; f1 = m->+60 * s * tuning) with s = (m->+48 - m->+96)
// / (m->+48 - m->+52) when m->+96 and m->+48 exceed m->+52, else K[0]
// (OPD in r4, entry in r7); bit 0x100 of the u16 m->+14 = !result. r3 =
// the cntlzw of the result, or o's vtable.
DECOMP(000E84C0, Obj_HitReactScaled_000E84C0) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    constexpr u32 K = 0x000E84B8;
    const u32 st = load<u32>(o + 216);
    const u32 r = static_cast<u32>(call(ctx, 0x000E0BCC, {o, m}));
    const u32 vt = load<u32>(o);
    ctx.gpr[3] = vt;
    if (r == 0) return;
    const f64 a = Lfs32(m + 96), b = Lfs32(m + 52);
    f64 s = Lfs32(K);
    if (a > b) {
        const f64 c = Lfs32(m + 48);
        if (c > b) s = static_cast<f32>(static_cast<f32>(c - a) / static_cast<f32>(c - b));
    }
    ctx.fpr[1] = s;
    const f64 tn = Lfs32(st + 248);
    if (!(tn > Lfs32(K + 4))) return;
    const u32 opd = load<u32>(vt + 352);
    ctx.fpr[1] = static_cast<f32>(static_cast<f32>(Lfs32(m + 60) * s) * tn);
    const u32 res = static_cast<u32>(call_ptr(ctx, opd, {o, opd, load<u32>(m + 4), 0x03C599D8u, load<u32>(opd)}));
    const u32 z = static_cast<u32>(std::countl_zero(res));
    store<u16>(m + 14, static_cast<u16>((load<u16>(m + 14) & ~0x100u) | ((z << 3) & 0x100u)));
    ctx.gpr[3] = z;
}

// T f(T* o, u32 bits): with the world's list w = *0x00F65CA0->+0x1281D4
// ->+4: each 48-byte entry of o's vector +388 (u16 count +8) with an
// object e at +28 gets e->+44 = (e->+44 & 0xFFFF0000) | bits, and when
// e->+8 is set 0x00930B70(w, e, 0, 1). r3 = o->+388 (o without a list).
DECOMP(000E9728, Obj_SetGroupBits_000E9728) {
    const u32 o = arg<u32>(ctx, 0), bits = arg<u32>(ctx, 1);
    const u32 w = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1281D4) + 4);
    ctx.gpr[3] = o;
    if (w == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 388);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(v) + 48 * i + 28);
        if (e == 0) continue;
        store<u32>(e + 44, (load<u32>(e + 44) & 0xFFFF0000u) | bits);
        if (load<u32>(e + 8) != 0) call(ctx, 0x00930B70, {w, e, 0, 1});
    }
}

// T* f(a, b, name, tmpl): a 240-byte spawn definition (SmallAlloc; vt
// 0x00B145C0, +236 = 0, flag 0x400 in +4) initialised by
// SpawnQuery_Init(p, name, tmpl, 0, 0x01164BD0, 0x01164B30, 1, 0, stack 0
// / - / 0; f1 = *0x0016CBDC), then XmlDef_ReadBase_000CBE10(a, b, p). r3 =
// p (0 for a failed allocation, still read).
DECOMP(0016CBE0, XmlDef_New_B145C0_0016CBE0) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), name = arg<u32>(ctx, 2), tmpl = arg<u32>(ctx, 3);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {240}));
    if (p != 0) {
        callf(ctx, 0x000CC4D4, {p, name, tmpl, 0, 0x01164BD0u, 0x01164B30u, 1, 0, 0, 0, 0}, {Lfs32(0x0016CBDC)});
        const u32 f = load<u32>(p + 4);
        store<u32>(p + 236, 0);
        store<u32>(p, 0x00B145C0);
        store<u32>(p + 4, f | 0x400u);
    }
    call(ctx, 0x000CBE10, {a, b, p});
    ctx.gpr[3] = p;
}

// bool f(?, u32 key, T* t): finds key in the binary tree of q = t->+4
// ->+16 (root q->+12->+0; node key +4, children +12 (node key above) /
// +16), pushing the visited nodes on q's stack (s16 depth +96, entries
// +16). Found: q->+100 = node, *0x011DE2AC = node->+8, r3 = (node->+8 !=
// 0). Not found: q->+100 = the last node, r3 = 0. An empty tree clears
// q->+100 and *0x011DE2AC.
DECOMP(0017A2C8, Tree_FindPath_0017A2C8) {
    const u32 key = arg<u32>(ctx, 1), t = arg<u32>(ctx, 2);
    ctx.gpr[3] = 0;
    if (t == 0) return;
    const u32 q = load<u32>(load<u32>(t + 4) + 16);
    if (q == 0) return;
    u32 n = load<u32>(load<u32>(q + 12));
    if (n == 0) {
        store<u32>(q + 100, 0);
        store<u32>(0x011DE2AC, 0);
        return;
    }
    store<u16>(q + 96, 0);
    for (;;) {
        const u32 k = load<u32>(n + 4);
        if (k == key) {
            store<u32>(q + 100, n);
            store<u32>(0x011DE2AC, load<u32>(n + 8));
            const u32 f = load<u32>(q + 100);
            if (f != 0 && load<u32>(f + 8) != 0) ctx.gpr[3] = 1;
            return;
        }
        const u32 c = k > key ? load<u32>(n + 12) : load<u32>(n + 16);
        if (c == 0) {
            store<u32>(q + 100, n);  // (n is never 0 here; the original's 0 case is dead)
            return;
        }
        const s32 d = static_cast<s16>(load<u16>(q + 96));
        store<u16>(q + 96, static_cast<u16>(d + 1));
        store<u32>(q + 16 + 4 * static_cast<u32>(d), n);
        n = c;
    }
}

// void f(T* o, u8 on): RagdollRelease33 with mode 6.
DECOMP(00184830, Ragdoll_Release6_00184830) { RagdollRelease33(ctx, 6); }

// void f(T* o, u8 on): RagdollRelease33 with mode 5.
DECOMP(0019211C, Ragdoll_Release5_0019211C) { RagdollRelease33(ctx, 5); }

// bool f(T* o; f1 a, f2 b): unless o->vt[32](o, 1), o's u8 +854 is clear,
// o->+788 is above K (*0x001CF708) or flag 0x20000000 of +552 is set
// (r3 = 1 for any of these): with b < K or a >= b, o->vt[420](o, 3, 0)
// and r3 = 0; else r3 = 1. f1 = K once loaded.
DECOMP(001CF710, Obj_TryState3_001CF710) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = ctx.fpr[1], b = ctx.fpr[2];
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    ctx.gpr[3] = 1;
    if (r != 0 || load<u8>(o + 854) == 0) return;
    const f64 k = Lfs32(0x001CF708);
    ctx.fpr[1] = k;
    if (!(Lfs32(o + 788) <= k)) return;
    if ((load<u32>(o + 552) & 0x20000000u) != 0) return;
    if (!(b < k) && !(a >= b)) return;
    VCall(ctx, o, 420, {o, 3, 0});
    ctx.gpr[3] = 0;
}

// bool f(T* o; f1 a, f2 b): unless o->vt[32](o, 1), o's u8 +854 is clear
// or flag 0x20000000 of +552 is set (r3 = 1): with b < K (*0x001CF808) or
// a >= b, o->vt[420](o, 4, 1), o->+788 = o->+764 and flag 0x80 set in the
// u8 +855 (r3 = 0, f1 = o->+764); else r3 = 1 (f1 = K).
DECOMP(001CF810, Obj_TryState4_001CF810) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = ctx.fpr[1], b = ctx.fpr[2];
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    ctx.gpr[3] = 1;
    if (r != 0 || load<u8>(o + 854) == 0) return;
    if ((load<u32>(o + 552) & 0x20000000u) != 0) return;
    const f64 k = Lfs32(0x001CF808);
    ctx.fpr[1] = k;
    if (!(b < k) && !(a >= b)) return;
    VCall(ctx, o, 420, {o, 4, 1});
    const u32 t = load<u8>(o + 855);
    ctx.fpr[1] = Lfs32(o + 764);
    CopyW32(o + 788, o + 764);
    store<u8>(o + 855, static_cast<u8>(t | 0x80u));
    ctx.gpr[3] = 0;
}

// bool f(T* o; f1 a, f2 b): unless o->vt[32](o, 1), o's u8 +854 is clear
// or flag 0x20000000 of +552 is set (r3 = 1): with b >= K (*0x001CF908)
// and a >= b, o->vt[420](o, 5, 0) and flag 0x80 cleared in the u8 +855
// (r3 = 0); else r3 = 1. f1 = K once loaded.
DECOMP(001CF910, Obj_TryState5_001CF910) {
    const u32 o = arg<u32>(ctx, 0);
    const f64 a = ctx.fpr[1], b = ctx.fpr[2];
    const u32 r = static_cast<u32>(VCall(ctx, o, 32, {o, 1}));
    ctx.gpr[3] = 1;
    if (r != 0 || load<u8>(o + 854) == 0) return;
    if ((load<u32>(o + 552) & 0x20000000u) != 0) return;
    const f64 k = Lfs32(0x001CF908);
    ctx.fpr[1] = k;
    if (!(b >= k) || !(a >= b)) return;
    VCall(ctx, o, 420, {o, 5, 0});
    store<u8>(o + 855, static_cast<u8>(load<u8>(o + 855) & 0x7Fu));
    ctx.gpr[3] = 0;
}

// T f(T* o, u8 on, u8 b, u8 c, u8 d): with on, d and the controller p =
// o->+420 set and p->+168 not K (*0x0020B9C8): p->+168 = 1.0 and flag
// 0x80 in its u8 +456; then 0x000D9F1C(o, on, b, c, d); with p (re-read),
// flag 0x80 of the u8 o->+444 clear (else r3 = 1) and on (else r3 = 0),
// p->vt[48](p, 0, 1). r3 as the last call left it; f1 = p->+168 when
// read.
DECOMP(0020B9D0, Obj_SetActive_0020B9D0) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2) & 0xFFu;
    const u32 c = arg<u32>(ctx, 3) & 0xFFu, d = arg<u32>(ctx, 4) & 0xFFu;
    const u32 p = load<u32>(o + 420);
    if (p != 0 && on != 0 && d != 0) {
        const f64 v = Lfs32(p + 168);
        ctx.fpr[1] = v;
        if (v != Lfs32(0x0020B9C8)) {
            const u32 f = load<u8>(p + 456);
            store<u32>(p + 168, 0x3F800000);
            store<u8>(p + 456, static_cast<u8>(f | 0x80u));
        }
    }
    ctx.gpr[3] = call(ctx, 0x000D9F1C, {o, on, b, c, d});
    const u32 q = load<u32>(o + 420);
    if (q == 0) return;
    if ((load<u8>(o + 444) & 0x80u) != 0) {
        ctx.gpr[3] = 1;
        return;
    }
    if (on == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    ctx.gpr[3] = VCall(ctx, q, 48, {q, 0, 1});
}

// ~T() (vt 0x00B1BC60, second base +388 0x00B1BE04): StateMachine_Shutdown
// (o), removed from the world list (World_ListRemove_0046B7A4(world, 0,
// 0x00F31548, o, o)), the physics body +416 taken out of the world
// (Hk_RemoveEntity92FFC8 when its +8 is set) and cleared, the skeleton
// +412 released and cleared, the member +1392 (vt 0x00B1BC30) destroyed
// (Dtor_B219D0_00464AD0), then vts 0x00B1200C / 0x00B126B8 and
// VtB12550_Dtor_000DF934(o). r3 as that left it.
DECOMP(00248118, Dtor_B1BC60_00248118) {
    const u32 o = arg<u32>(ctx, 0);
    store<u32>(o, 0x00B1BC60);
    store<u32>(o + 388, 0x00B1BE04);
    call(ctx, 0x00464B90, {o});
    call(ctx, 0x0046B7A4, {load<u32>(0x00F65CA0), 0, 0x00F31548u, o, o});
    const u32 a = load<u32>(o + 416);
    if (a != 0) {
        const u32 w = load<u32>(a + 8);
        if (w != 0) {
            Stack frame(ctx, 16);
            call(ctx, 0x0092FFC8, {frame.addr(), w, a});  // sp+112
        }
        store<u32>(o + 416, 0);
    }
    const u32 s = load<u32>(o + 412);
    if (s != 0) {
        call(ctx, 0x007D4EA8, {s});
        store<u32>(o + 412, 0);
    }
    store<u32>(o + 1392, 0x00B1BC30);
    call(ctx, 0x00464AD0, {o + 1392});
    store<u32>(o + 388, 0x00B1200C);
    store<u32>(o, 0x00B126B8);
    ctx.gpr[3] = call(ctx, 0x000DF934, {o});
}

// u32 f(T* o): each of the o->+200 28-byte slots (from o+44) with +44 = 0,
// +48 = -1 and +52 = 0 gets +52 = d->vt[280](d, o->+288 + 160 * i) for d
// = o->+8 (the count re-read after each). r3 = the count.
DECOMP(002972C0, Obj_ResolveSlots_002972C0) {
    const u32 o = arg<u32>(ctx, 0);
    u32 n = load<u32>(o + 200);
    for (u32 i = 0; i < n; ++i) {
        const u32 e = o + 28 * i;
        if (load<u32>(e + 44) != 0 || load<u32>(e + 48) != 0xFFFFFFFFu || load<u32>(e + 52) != 0) continue;
        const u32 x = load<u32>(o + 160 * i + 288);
        const u32 d = load<u32>(o + 8);
        store<u32>(e + 52, static_cast<u32>(VCall(ctx, d, 280, {d, x})));
        n = load<u32>(o + 200);
    }
    ctx.gpr[3] = n;
}

// u32 f(T* o, u32 k, PtrVec* v): pushes onto v each of o's o->+24 860-byte
// records (from o->+20) whose +744 equals k (the count re-read after each
// push). r3 = the count (o for k = 0).
DECOMP(002E2D7C, Obj_CollectByKey_002E2D7C) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1), v = arg<u32>(ctx, 2);
    ctx.gpr[3] = o;
    if (k == 0) return;
    u32 n = load<u32>(o + 24);
    for (u32 i = 0; i < n; ++i) {
        const u32 r = load<u32>(o + 20) + 860 * i;
        if (load<u32>(r + 744) != k) continue;
        PushU16Vec33(ctx, v, r);
        n = load<u32>(o + 24);
    }
    ctx.gpr[3] = n;
}

// u32 f(T* o): u8s +57 = 0 / +58 = 1, and each group of o's u16-counted
// list (+36 / +44) reset (+56/+60/+48 = 0, u8 +84 = 0, u8 +52 = u8 +53)
// with each member of its list (+64 / u16 +72) told vt[12](m, 0). r3 =
// the group count.
DECOMP(002FD1B0, Obj_ResetGroups_002FD1B0) {
    const u32 o = arg<u32>(ctx, 0);
    store<u8>(o + 57, 0);
    store<u8>(o + 58, 1);
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 44);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 g = load<u32>(load<u32>(o + 36) + 4 * i);
        store<u32>(g + 56, 0);
        store<u8>(g + 84, 0);
        const u32 t = load<u8>(g + 53);
        store<u32>(g + 60, 0);
        store<u32>(g + 48, 0);
        store<u8>(g + 52, static_cast<u8>(t));
        for (u32 j = 0; j < load<u16>(g + 72); ++j) {
            const u32 m = load<u32>(load<u32>(g + 64) + 4 * j);
            VCall(ctx, m, 12, {m, 0});
        }
    }
}

// T f(T* o): when o may show (Obj_CanShow2E2104(*0x00F64C20, o)) in state
// (u8 +18) 5 or 4: User_Start325B58(o, m) and Loader_Tick3250A0(o, m)
// with m = 3 / 4; then each object of the chain o->+2576 (next +2564)
// gets vt[32](it). r3 as the last call (or the state read) left it.
DECOMP(0032758C, Obj_TickChain_0032758C) {
    const u32 o = arg<u32>(ctx, 0);
    u64 r = call(ctx, 0x002E2104, {load<u32>(0x00F64C20), o});
    if (static_cast<u32>(r) != 0) {
        const u32 s = load<u8>(o + 18);
        r = s;
        if (s == 5 || s == 4) {
            const u32 m = s == 5 ? 3u : 4u;
            call(ctx, 0x00325B58, {o, m});
            r = call(ctx, 0x003250A0, {o, m});
        }
    }
    for (u32 p = load<u32>(o + 2576); p != 0; p = load<u32>(p + 2564)) r = VCall(ctx, p, 32, {p});
    ctx.gpr[3] = r;
}

// T f(T* o): for a flagged (+184 bit 0x80000000) object whose +160 >> 1
// is 0: o->+24 (when set) gets vt[40](it, 1, 1), +180 = 0, the effect
// o->+20 (when set) gets hash 0x02B81827 (Effect_SetHash4CF988), +160 =
// 2 | (+160 & 1), Speaker_SayId4A5C90(o->+28, o->+168, 0; f1 =
// *0x0036F0CC), then the effect (re-read) gets hash 0x04296E4C. r3 as the
// last call left it (0 / +160 >> 1 when skipped).
DECOMP(0036F0D0, Obj_StartTalk_0036F0D0) {
    const u32 o = arg<u32>(ctx, 0);
    if ((load<u32>(o + 184) & 0x80000000u) == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 h = load<u32>(o + 160) >> 1;
    ctx.gpr[3] = h;
    if (h != 0) return;
    const u32 a = load<u32>(o + 24);
    if (a != 0) VCall(ctx, a, 40, {a, 1, 1});
    const u32 e = load<u32>(o + 20);
    store<u32>(o + 180, 0);
    if (e != 0) call(ctx, 0x004CF988, {e, 0x02B81827u});
    const u32 w = load<u32>(o + 160);
    const u32 sp = load<u32>(o + 28);
    store<u32>(o + 160, 2u | (w & 1u));
    callf(ctx, 0x004A5C90, {sp, load<u32>(o + 168), 0}, {Lfs32(0x0036F0CC)});  // r3 stays as it returns
    const u32 e2 = load<u32>(o + 20);
    if (e2 != 0) call(ctx, 0x004CF988, {e2, 0x04296E4Cu});
}

// f32 f(Timer* t; f1 dt): while t->+8 is below t->+4, the length of the
// camera input (channels 18 / 19 / 20 of 0x010B1A90, Obj_GetChannel)
// above t->+0 restarts it (t->+8 = 0, u8 t->+12 = 1, r3 = 0); otherwise
// t->+8 += dt. f1 = the new t->+8 (the old one for a restart or when
// done); r3 as the last channel read left it.
DECOMP(003D360C, Timer_IdleInput_003D360C) {
    const u32 t = arg<u32>(ctx, 0);
    const f64 dt = ctx.fpr[1];
    constexpr u32 C = 0x010B1A90;
    const f64 a = callf(ctx, 0x0045B160, {C, 18}, {});
    const f64 b = callf(ctx, 0x0045B160, {C, 19}, {});
    const f64 c = callf(ctx, 0x0045B160, {C, 20}, {});
    const f64 v = Lfs32(t + 8);
    ctx.fpr[1] = v;
    if (!(v < Lfs32(t + 4))) return;
    f64 m = static_cast<f32>(std::fabs(b) * std::fabs(b));
    m = static_cast<f32>(std::fma(std::fabs(a), std::fabs(a), m));
    m = static_cast<f32>(std::fma(std::fabs(c), std::fabs(c), m));
    m = static_cast<f32>(std::sqrt(m));
    if (m > Lfs32(t)) {
        store<u32>(t + 8, 0);
        store<u8>(t + 12, 1);
        ctx.gpr[3] = 0;
        return;
    }
    const f64 n = static_cast<f32>(v + dt);
    Stfs32(t + 8, n);
    ctx.fpr[1] = n;
}

// T f(u32 key): the sound handle of key in the map 0x010AFEF8 (entry
// +8 + 12 * i + 4, or the default 0x010AFF08 when absent), stopped with a
// fade (Emitter_Stop(h, 1)) when it is live (see VoiceLive32). r3 = that,
// else the handle.
DECOMP(0044D1E8, Sound_StopByKey_0044D1E8) {
    const u32 key = arg<u32>(ctx, 0);
    constexpr u32 M = 0x010AFEF8;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);  // sp+112
    const s32 i = static_cast<s32>(call(ctx, 0x00784008, {M, frame.addr()}));
    const u32 at = i >= 0 ? load<u32>(M + 8) + 12 * static_cast<u32>(i) + 4 : M + 16;
    const u32 h = load<u32>(at);
    ctx.gpr[3] = h;
    if (VoiceLive32(h) == 0) return;
    ctx.gpr[3] = call(ctx, 0x00443424, {h, 1});
}

// T f(Node* n, u8 deep): nodes of kind ((+4 >> 10) & 63) 8 / 10 queued
// on the world's pair buffer (PairBuffer_Push(*0x00F65CA0->+0x1285E8->+8,
// 0x00F3C860 / 0x00F3C918, n)); with deep, recursively each child (first
// +272, next +268). r3 as the last call (or the kind) left it.
DECOMP(004A6CF8, Node_QueueTree_004A6CF8) {
    const u32 n = arg<u32>(ctx, 0), deep = arg<u32>(ctx, 1) & 0xFFu;
    ctx.gpr[3] = 0;
    if (n == 0) return;
    const u32 kind = (load<u32>(n + 4) >> 10) & 63u;
    ctx.gpr[3] = kind;
    if (kind == 8 || kind == 10) {
        const u32 pb = load<u32>(load<u32>(load<u32>(0x00F65CA0) + 0x1285E8) + 8);
        ctx.gpr[3] = call(ctx, 0x00564FF8, {pb, kind == 8 ? 0x00F3C860u : 0x00F3C918u, n});
    }
    if (deep == 0) return;
    for (u32 c = load<u32>(n + 272); c != 0; c = load<u32>(c + 268)) call(ctx, 0x004A6CF8, {c, deep});
}

// T* f(Src* a, Ref* b, Ref* c): for b->+4 = 1 and c->+4 = 0 (else r3 =
// 0): a 160-byte block from the thread's memory router (->+44 vt[8](it,
// 160)) with its u16 +4 = 160 (written even when the allocation failed),
// built by 0x004A9270(p, a->+28, *b->+0, a->+32, -, -, -, -, stack 0;
// f1 = a->+36, f2 = a->+40, f3 = f4 = *0x004AC710), and p->+16 = a->+16
// (at address 16 for a failed allocation, as the original). r3 = p.
DECOMP(004AC718, Hk_NewFromSrc_004AC718) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    if (load<u32>(b + 4) != 1 || load<u32>(c + 4) != 0) {
        ctx.gpr[3] = 0;
        return;
    }
    const u32 rt = load<u32>(HkRouter33(ctx) + 44);
    const u32 p = static_cast<u32>(VCall(ctx, rt, 8, {rt, 160}));
    store<u16>(p + 4, 160);
    if (p != 0) {
        const f64 k = Lfs32(0x004AC710);
        // r7..r10 are the float slots (not set by the original).
        callf(ctx, 0x004A9270, {p, load<u32>(a + 28), load<u32>(load<u32>(b)), load<u32>(a + 32), 0, 0, 0, 0, 0},
              {Lfs32(a + 36), Lfs32(a + 40), k, k});
    }
    store<u32>(p + 16, load<u32>(a + 16));
    ctx.gpr[3] = p;
}

// T* f(?, Xml* x): a 20-byte node (vt 0x00B22218, u16 +4 = 1, +12 = 0, u8
// +16 = 0): +12 = the hash of the attribute "occlPlane" (String_HashNoCase)
// or else the s32 0x3F4EC4AD; bool 0x72C27306 -> +16. r3 = p.
DECOMP(004C458C, XmlNew_OcclPlane_004C458C) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {20, 0, 16}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    store<u16>(p + 4, 1);
    store<u32>(p + 12, 0);
    store<u32>(p, 0x00B22218);
    store<u8>(p + 16, 0);
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AEF080u}));
    if (a != 0) store<u32>(p + 12, static_cast<u32>(call(ctx, 0x0056A710, {a})));
    else call(ctx, 0x004F99BC, {x, 0x3F4EC4ADu, p + 12, 0});
    call(ctx, 0x004F986C, {x, 0x72C27306u, p + 16, 0});
    ctx.gpr[3] = p;
}

// ~T() (vts 0x00B15308 + 8 / +436 / +476): Dtor3Vt33.
DECOMP(00751314, Dtor_B15308_00751314) { Dtor3Vt33(ctx, 0x00B15308); }

// ~T() (vts 0x00B18030 + 8 / +436 / +476): Dtor3Vt33.
DECOMP(00756CD4, Dtor_B18030_00756CD4) { Dtor3Vt33(ctx, 0x00B18030); }

// T* f(T* o, Obj* a, b, u16 h, u8 m, const Vec3* v; f1, f2, stack: u8 k
// (9th), f3, f4, u32 w (12th), const Vec3* p (13th)): vt 0x00B0F048, +4 =
// a, +8 = b, u16 +12 = h, u16 +14 flags (top bits 010, 0x0800 / 0x0400 /
// 0x0100 clear, 0x0200 = m & 1, 0x1000 = bit 0 of a's word +188; written
// once more before that bit when a is set), +16 = v, +32 = p, +48 / +52 /
// +60 / +64 = f1 / f2 / f3 / f4, u8 +56 = k, +80 = the up vector
// 0x01164B60, +96 = 0, +100 = w (vectors raw). r3 = o, f1 = up.z.
DECOMP(00757D20, Contact_Init_00757D20) {
    const u32 o = arg<u32>(ctx, 0), a = arg<u32>(ctx, 1), b = arg<u32>(ctx, 2), h = arg<u32>(ctx, 3);
    const u32 m = arg<u32>(ctx, 4), v = arg<u32>(ctx, 5);
    const u32 sp = static_cast<u32>(ctx.gpr[1]);
    const u32 k = load<u32>(sp + 116), w = load<u32>(sp + 140), p = load<u32>(sp + 148);
    u32 f = load<u16>(o + 14);
    store<u32>(o + 4, a);
    f = (f & ~0xE000u) | 0x4000u;
    store<u32>(o + 8, b);
    f &= ~0x0C00u;
    f = (f & ~0x0200u) | ((m << 9) & 0x0200u);
    store<u16>(o + 12, static_cast<u16>(h));
    f &= ~0x0100u;
    u32 bit = 0;
    if (a != 0) {
        store<u16>(o + 14, static_cast<u16>(f));
        bit = load<u32>(a + 188) & 1u;  // the low word of the doubleword +184
    }
    f = (f & ~0x1000u) | (bit << 12);
    store<u16>(o + 14, static_cast<u16>(f));
    CopyW32(o + 16, v);
    CopyW32(o + 20, v + 4);
    CopyW32(o + 24, v + 8);
    CopyW32(o + 32, p);
    const u32 p1 = load<u32>(p + 4);
    store<u32>(o, 0x00B0F048);
    store<u32>(o + 36, p1);
    const u32 p2 = load<u32>(p + 8);
    Stfs32(o + 48, ctx.fpr[1]);
    store<u32>(o + 40, p2);
    Stfs32(o + 52, ctx.fpr[2]);
    Stfs32(o + 60, ctx.fpr[3]);
    Stfs32(o + 64, ctx.fpr[4]);
    constexpr u32 U = 0x01164B60;
    const u32 u0 = load<u32>(U);
    store<u8>(o + 56, static_cast<u8>(k));
    store<u32>(o + 80, u0);
    const u32 u1 = load<u32>(U + 4);
    store<u32>(o + 100, w);
    store<u32>(o + 84, u1);
    const u32 u2 = load<u32>(U + 8);
    store<u32>(o + 96, 0);
    store<u32>(o + 88, u2);
    ctx.gpr[3] = o;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(u2));
}

// Deleting ~T() (o may be 0; vt 0x00B1F860): each of the o->+152 16-byte
// records of o->+148 frees its +4 buffer (Mem_FreeFrom, then 0); the
// array +148 freed and cleared; vt 0x00B1F668 and Mem_FreeFrom(o, 0). r3
// as that left it.
DECOMP(00772E78, Dtor_B1F860_00772E78) {
    const u32 o = arg<u32>(ctx, 0);
    ctx.gpr[3] = 0;
    if (o == 0) return;
    store<u32>(o, 0x00B1F860);
    u32 n = load<u32>(o + 152);
    for (u32 i = 0; i < n; ++i) {
        const u32 r = load<u32>(o + 148) + 16 * i;
        const u32 b = load<u32>(r + 4);
        if (b == 0) continue;
        call(ctx, o32fn::Mem_FreeFrom, {b, 0});
        store<u32>(r + 4, 0);
        n = load<u32>(o + 152);
    }
    FreeField32(ctx, o, 148);
    store<u32>(o, 0x00B1F668);
    ctx.gpr[3] = call(ctx, o32fn::Mem_FreeFrom, {o, 0});
}

// s16 f(u16 a, u8** pp, u32* link): PushRecord33, the link target's u16
// set to 0xFFFF.
DECOMP(00773998, Block_PushRecordFF_00773998) { PushRecord33(ctx, false); }

// Node* f(s16 id): NodeNewOn33("RTurnOnHighlight", binding "TurnOn").
DECOMP(00777FA8, NodeNew_RTurnOnHighlight_00777FA8) { NodeNewOn33(ctx, 0x00B1FE32, 0x00B1FE43); }

// Node* f(s16 id): NodeNewOn33("RFireWeapon", binding "on").
DECOMP(0077A858, NodeNew_RFireWeapon_0077A858) { NodeNewOn33(ctx, 0x00B20209, 0x00B20215); }

// ---------------------------------------------------------------- sweep: 63 instructions
// Deleting ~T() (vt 0x00B0FBA8): DeleteOwnedVec33 on the vector +24.
DECOMP(0007DD24, Delete_B0FBA8_0007DD24) { DeleteOwnedVec33(ctx, 0x00B0FBA8, 24); }

// Deleting ~T() (vt 0x00B0FBD0): DeleteOwnedVec33 on the vector +28.
DECOMP(0007E468, Delete_B0FBD0_0007E468) { DeleteOwnedVec33(ctx, 0x00B0FBD0, 28); }

// Deleting ~T() (vt 0x00B0FC20): DeleteOwnedVec33 on the vector +12.
DECOMP(0007F2F8, Delete_B0FC20_0007F2F8) { DeleteOwnedVec33(ctx, 0x00B0FC20, 12); }

// T* f(?, Xml* x): 20-byte node (vt 0x00B103C8, f32 +12 = -1, u8s +16 /
// +17 = 0): f32 0x00274ACD -> +12 (default *0x00089A08, also -1), bool
// 0xA8B76777 -> +16, bool 0x6D68B156 -> +17.
DECOMP(00089A10, XmlNew_B103C8_00089A10) {
    XmlGen32(ctx, 20, 0x00B103C8, {{0, 'v'}, {12, 'C', 0xBF800000u}, {16, 'b'}, {17, 'b'}},
             {{'F', 0x00274ACDu, 12, true}, {'B', 0xA8B76777u, 16, true}, {'B', 0x6D68B156u, 17, true}});
}

// T f(Cmd* c, Owner* x): the target t = Obj_FindById(c->+12), else the
// object of the effect Effect_Bind4BB7E0(x, c->+12, 0) (its +280 when its
// +276 is 0x02235C33); with u8 c->+17 set t->vt[176](t, u8 c->+16) (entry
// in r5, OPD in r6), else t->vt[216](t, u8 c->+16). r3 as that left it
// (0, or 0x02235C33 when an effect had no target).
DECOMP(0008A71C, Cmd_SetTargetFlag_0008A71C) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    u32 t = static_cast<u32>(call(ctx, 0x000D6A28, {load<u32>(c + 12)}));
    if (t == 0) {
        const u32 e = static_cast<u32>(call(ctx, 0x004BB7E0, {x, load<u32>(c + 12), 0}));
        if (e != 0) {
            ctx.gpr[3] = 0x02235C33u;
            if (load<u32>(e + 276) == 0x02235C33u) t = load<u32>(e + 280);
        }
    }
    if (t == 0) return;
    const u32 v = load<u8>(c + 16);
    if (load<u8>(c + 17) != 0) {
        const u32 opd = load<u32>(load<u32>(t) + 176);
        ctx.gpr[3] = call_ptr(ctx, opd, {t, v, load<u32>(opd), opd});
    } else {
        ctx.gpr[3] = VCall(ctx, t, 216, {t, v});
    }
}

// T f(Cmd* c, Owner* x): for the effect component Effect_FindComponent(x,
// c->+12) of kind 5 (vt[32]) with an object t (+28): the colour c+16
// (f1 = c->+20) applied by its target (t->+280 when t->+276 is
// 0x02235C33) as s->vt[120](s, c+16, -, 1) (OPD in r5, entry in r7), else
// by Obj_SetColourD6F90(t, c+16). r3 as the last call left it (the kind,
// or 5 for no object).
DECOMP(0008D070, Cmd_SetColour_0008D070) {
    const u32 c = arg<u32>(ctx, 0), x = arg<u32>(ctx, 1);
    const u32 k = static_cast<u32>(call(ctx, 0x004BB4B4, {x, load<u32>(c + 12)}));
    if (k == 0) return;
    if (static_cast<u32>(VCall(ctx, k, 32, {k})) != 5) return;
    const u32 t = load<u32>(k + 28);
    if (t == 0) return;
    const u32 s = load<u32>(t + 276) == 0x02235C33u ? load<u32>(t + 280) : 0u;
    if (s == 0) {
        callf(ctx, 0x000D6F90, {t, c + 16}, {Lfs32(c + 20)});
        return;
    }
    const u32 opd = load<u32>(load<u32>(s) + 120);
    ctx.fpr[1] = Lfs32(c + 20);
    ctx.gpr[3] = call_ptr(ctx, opd, {s, c + 16, opd, 1, load<u32>(opd)});
}

// void f(T* o): the default transform (rows +32 / +48 / +64 and +80..+88
// from the identity 0x01164BD0, raw), the two vectors +96 / +112 =
// 0x01164B40, and the fields +12..+24, +128..+136, +144..+168 = 0 and u8s
// +140..+143, +172, +224, +225 = 0, in the original's order. r3 = o, f1 =
// 0x01164B40's z.
DECOMP(0008DD48, Xform_Reset_0008DD48) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 A = 0x01164BD0, B = 0x01164B40;
    CopyW32(o + 32, A);
    CopyW32(o + 36, A + 4);
    CopyW32(o + 40, A + 8);
    CopyW32(o + 48, A + 16);
    CopyW32(o + 52, A + 20);
    CopyW32(o + 56, A + 24);
    CopyW32(o + 64, A + 32);
    CopyW32(o + 68, A + 36);
    CopyW32(o + 72, A + 40);
    CopyW32(o + 80, A + 48);
    const u32 a13 = load<u32>(A + 52);
    for (const u32 off : {12u, 16u, 20u}) store<u32>(o + off, 0);
    store<u32>(o + 84, a13);
    for (const u32 off : {24u, 144u, 156u}) store<u32>(o + off, 0);
    const u32 a14 = load<u32>(A + 56);
    for (const u32 off : {152u, 148u, 168u, 164u}) store<u32>(o + off, 0);
    store<u32>(o + 88, a14);
    store<u32>(o + 160, 0);
    CopyW32(o + 96, B);
    CopyW32(o + 100, B + 4);
    CopyW32(o + 104, B + 8);
    CopyW32(o + 112, B);
    const u32 b1 = load<u32>(B + 4);
    store<u32>(o + 128, 0);
    store<u32>(o + 116, b1);
    const u32 b2 = load<u32>(B + 8);
    store<u32>(o + 132, 0);
    store<u32>(o + 120, b2);
    store<u32>(o + 136, 0);
    for (const u32 off : {141u, 142u, 143u, 172u, 140u, 224u, 225u}) store<u8>(o + off, 0);
    ctx.gpr[3] = o;
    ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(b2));
}

// f32 f(Spring* s; f1 target, f2 dt): a critically damped follower (K =
// 0x000F9358: 0, 0.05, 0.05d, 1, 36, 9, 0d): for dt > 0, the target
// velocity (target - s->+12) / dt into +16 (+12 = target), the time
// constant +20 (clamped to 0.05 when above it, the rate 1 / it) driving
// the position +4 and velocity +8 toward the target (or toward the target
// velocity too with u8 +24 set; damping +28), and +20 -= dt (0 once
// spent). f1 = the new position (+4 unchanged for dt <= 0).
DECOMP(000F9380, Spring_Update_000F9380) {
    const u32 s = arg<u32>(ctx, 0);
    const f64 tg = ctx.fpr[1], dt = ctx.fpr[2];
    constexpr u32 K = 0x000F9358;
    const f64 k0 = Lfs32(K);
    if (dt <= k0) {
        ctx.fpr[1] = Lfs32(s + 4);
        return;
    }
    f64 v8 = static_cast<f32>(tg - Lfs32(s + 12));
    Stfs32(s + 12, tg);
    f64 f5 = k0;
    const f64 f4 = Lfs32(s + 20);
    const f64 one = Lfs32(K + 16);
    v8 = static_cast<f32>(v8 / dt);
    const f64 damp = Lfs32(s + 28);
    f64 f7 = static_cast<f32>(Fsel33(static_cast<f32>(Lfs32(K + 4) - f4), std::bit_cast<f64>(load<u64>(K + 8)), f4));
    f7 = static_cast<f32>(one / f7);
    Stfs32(s + 16, v8);
    if (load<u8>(s + 24) != 0) f5 = v8;
    const f64 w2 = static_cast<f32>(f7 * f7);
    f64 f6 = static_cast<f32>(damp * f7);
    const f64 a = static_cast<f32>(w2 * Lfs32(K + 20));
    f6 = static_cast<f32>(f6 * Lfs32(K + 24));
    const f64 rem = static_cast<f32>(f4 - dt);
    const f64 f8 = static_cast<f32>(dt * a);
    f64 f9 = static_cast<f32>(std::fma(dt, f6, one));
    f9 = static_cast<f32>(std::fma(f8, dt, f9));
    f6 = static_cast<f32>(f6 + f8);
    const f64 inv = static_cast<f32>(one / f9);
    const f64 vel = Lfs32(s + 8), pos = Lfs32(s + 4);
    f5 = static_cast<f32>(f5 - vel);
    const f64 d = static_cast<f32>(tg - pos);
    f6 = static_cast<f32>(f6 * inv);
    const f64 g = static_cast<f32>(a * inv);
    f5 = static_cast<f32>(f6 * f5);
    const f64 acc = static_cast<f32>(std::fma(g, d, f5));
    const f64 nv = static_cast<f32>(std::fma(acc, dt, vel));
    const f64 np = static_cast<f32>(std::fma(nv, dt, pos));
    Stfs32(s + 8, nv);
    Stfs32(s + 4, np);
    Stfs32(s + 20, static_cast<f32>(Fsel33(-rem, std::bit_cast<f64>(load<u64>(K + 32)), rem)));
    ctx.fpr[1] = np;
}

// void f(T* o, u8 on): RagdollRelease33 through vt[396](o, 4, 0).
DECOMP(001683F0, Ragdoll_Release4_001683F0) { RagdollRelease33(ctx, 4, 396); }

// void f(T* o, u8 on): RagdollRelease33 through vt[396](o, 9, 0).
DECOMP(0016EF50, Ragdoll_Release9_0016EF50) { RagdollRelease33(ctx, 9, 396); }

// T f(T* o, s32 i, -, u8 flag; f1 t): with the tuning s = o->+216 (r3 =
// 0x001AF468 without): while t exceeds the stage length (o+436 + 16 * i)
// * (s->+392 when flag 0x40000000 of +552 is set, else K[1]; K =
// 0x001AF468) and that is above K[0], t is reduced by it and i steps 1..6
// (7 wraps to 1; at stage 4 with a zero length t becomes K[0]); then
// o->vt[420](o, i, flag) and *o->+12 = t. r3 = o->+12, f1 = K[0].
DECOMP(001AF470, Obj_AdvanceStage_001AF470) {
    const u32 o = arg<u32>(ctx, 0), flag = arg<u32>(ctx, 3) & 0xFFu;
    u32 i = arg<u32>(ctx, 1);
    f64 t = ctx.fpr[1];
    constexpr u32 K = 0x001AF468;
    const u32 s = load<u32>(o + 216);
    ctx.gpr[3] = K;
    if (s == 0) return;
    const f64 k0 = Lfs32(K);
    ctx.fpr[1] = k0;
    if (t > k0) {
        const bool scaled = (load<u32>(o + 552) & 0x40000000u) != 0;
        for (;;) {
            f64 m = Lfs32(K + 4);
            const f64 len = Lfs32(o + (i << 4) + 436);
            if (scaled) m = Lfs32(s + 392);
            m = static_cast<f32>(len * m);
            if (m > t) break;
            if (m > k0) {
                t = static_cast<f32>(t - m);
            } else if (i == 4) {
                t = k0;
                break;
            }
            i = i + 1;
            if (i == 7) i = 1;
        }
    }
    VCall(ctx, o, 420, {o, i, flag});
    const u32 p = load<u32>(o + 12);
    Stfs32(p, t);
    ctx.gpr[3] = p;
}

// T f(T* o, x): 0x000E5EF0(o, x), then for each of the six slots i whose
// tuning byte (o->+216 + 517 + 80 * i) is set and whose object c = o->+588
// + 20 * i exists: 0x001C5738(&tmp, o, i) and c->vt[84](c, &tmp). r3 as
// the last step left it.
DECOMP(001C7C5C, Obj_RefreshSlots_001C7C5C) {
    const u32 o = arg<u32>(ctx, 0);
    const u32 st = load<u32>(o + 216);
    call(ctx, 0x000E5EF0, {o, ctx.gpr[4]});  // r4 passed through untouched (the callee reads it)
    Stack frame(ctx, 64);
    const u32 tmp = frame.addr();  // sp+112
    u64 r3 = 0;
    for (u32 i = 0; i < 6; ++i) {
        const u32 b = load<u8>(st + 517 + 80 * i);
        r3 = b;
        if (b == 0) continue;
        const u32 at = o + 588 + 20 * i;
        r3 = at;
        const u32 c = load<u32>(at);
        if (c == 0) continue;
        call(ctx, 0x001C5738, {tmp, o, i});
        r3 = VCall(ctx, c, 84, {c, tmp});
    }
    ctx.gpr[3] = r3;
}

// void f(T* o, u8 on): SoundOff33 with the effect +752.
DECOMP(001CC338, Obj_SoundOff752_001CC338) { SoundOff33(ctx, 752); }

// void f(T* o, u8 on): SoundOff33 with the effect +556.
DECOMP(001D46E8, Obj_SoundOff556_001D46E8) { SoundOff33(ctx, 556); }

// T* f(a, b, c): a 208-byte trigger (SmallAlloc; base 0x00746148(p, c, 0,
// 0, 1, 0, 1, 0, stack 0, 0), vt 0x00B1A448, +188 / +196 = 0, +192 = -1,
// the top five bits of +200 cleared, flag 0x10000000 in +4), then
// Trigger_ReadXml_0021C074(a, b, p). r3 = p (0 for a failed allocation,
// still read).
DECOMP(0021C380, Trigger_New_0021C380) {
    const u32 a = arg<u32>(ctx, 0), b = arg<u32>(ctx, 1), c = arg<u32>(ctx, 2);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {208}));
    if (p != 0) {
        call(ctx, 0x00746148, {p, c, 0, 0, 1, 0, 1, 0, 0, 0});
        const u32 w = load<u32>(p + 200);
        store<u32>(p + 188, 0);
        store<u32>(p, 0x00B1A448);
        store<u32>(p + 196, 0);
        store<u32>(p + 192, 0xFFFFFFFFu);
        const u32 f = load<u32>(p + 4);
        store<u32>(p + 200, w & 0x07FFFFFFu);
        store<u32>(p + 4, f | 0x10000000u);
    }
    call(ctx, 0x0021C074, {a, b, p});
    ctx.gpr[3] = p;
}

// ~T() (vt 0x00B1BE30): o's entry removed from the u16-counted pair list
// 0x01056F1C (count +2, 8-byte entries +8 keyed by +4; the later entries
// moved down), then List1013B5C_Remove(o). r3 as that left it.
DECOMP(002505A0, Dtor_B1BE30_002505A0) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 L = 0x01056F1C;
    store<u32>(o, 0x00B1BE30);
    const u32 n = load<u16>(L + 2);
    const u32 arr = load<u32>(L + 8);
    for (u32 i = 0; i < n; ++i) {
        if (load<u32>(arr + 8 * i + 4) != o) continue;
        u32 c = n;
        if (static_cast<s32>(n) >= 2 && static_cast<s32>(i) < static_cast<s32>(n - 1)) {
            do {
                const u32 e = arr + 8 * i;
                store<u32>(e, load<u32>(e + 8));
                const u32 v = load<u32>(e + 12);
                c = load<u16>(L + 2);
                store<u32>(e + 4, v);
                ++i;
            } while (static_cast<s32>(i) < static_cast<s32>(c - 1));
        }
        store<u16>(L + 2, static_cast<u16>(c - 1));
        break;
    }
    ctx.gpr[3] = call(ctx, 0x000D6940, {o});
}

// u32 f(T* o, u8 v): each component of type 15 (Obj_FindComponent(t, 15,
// prev)) of the +52 object t of each entry of o's u16-counted list (+28 /
// +36) gets vt[44](c, v). r3 = the count.
DECOMP(0026924C, Obj_NotifyType15_0026924C) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    for (u32 i = 0;; ++i) {
        const u32 n = load<u16>(o + 36);
        ctx.gpr[3] = n;
        if (!(i < n)) return;
        const u32 e = load<u32>(load<u32>(o + 28) + 4 * i);
        const u32 t = load<u32>(e + 52);
        if (t == 0) continue;
        for (u32 c = static_cast<u32>(call(ctx, 0x000D1288, {t, 15, 0})); c != 0;
             c = static_cast<u32>(call(ctx, 0x000D1288, {load<u32>(e + 52), 15, c})))
            VCall(ctx, c, 44, {c, v});
    }
}

// T f(T* o, const char* name, a, -, b, c, Src* src; f1 t): the previous
// source o->+420 released (Audio_Release446384(0x010ACC40, it)), +396 =
// t, +392 = a, +420 = src (its +108 use count incremented), +408 top bits
// 01, the name copied into +264 (127 chars, empty for none), +412 = b,
// +416 = c. r3 as String_CopyN left it, else 0.
DECOMP(002ABD48, Sound_SetSource_002ABD48) {
    const u32 o = arg<u32>(ctx, 0), name = arg<u32>(ctx, 1), a = arg<u32>(ctx, 2);
    const u32 b = arg<u32>(ctx, 4), c = arg<u32>(ctx, 5), src = arg<u32>(ctx, 6);
    const f64 t = ctx.fpr[1];
    const u32 old = load<u32>(o + 420);
    if (old != 0) call(ctx, 0x00446384, {0x010ACC40u, old});
    const u32 w = load<u32>(o + 408);
    Stfs32(o + 396, t);
    store<u32>(o + 392, a);
    store<u32>(o + 420, src);
    store<u32>(o + 408, (w & 0x7FFFFFFFu) | 0x40000000u);
    if (src != 0) store<u32>(src + 108, load<u32>(src + 108) + 1);
    store<u8>(o + 264, 0);
    ctx.gpr[3] = 0;
    if (name != 0 && load<u8>(name) != 0) call(ctx, 0x00553638, {o + 264, name, 127});
    store<u32>(o + 412, b);
    store<u32>(o + 416, c);
}

// T f(Col* o, u32 k, u32 id): o->+0 = k; the object Obj_FindById(id)
// becomes o->+4 when its +192 flag 4 is set (else 0; an unknown id keeps
// the old +4); the children of that object's +160 (first +272, next
// +268) are pushed onto the u16-counted vector o+8. r3 as the original
// leaves it (the last high-water value written, else the last value
// tested).
DECOMP(0030419C, Col_Gather_0030419C) {
    const u32 o = arg<u32>(ctx, 0), k = arg<u32>(ctx, 1), id = arg<u32>(ctx, 2);
    store<u32>(o, k);
    const u32 t = static_cast<u32>(call(ctx, 0x000D6A28, {id}));
    u32 q = load<u32>(o + 4);
    if (t != 0) {
        q = (load<u32>(t + 192) & 4u) != 0 ? t : 0u;
        store<u32>(o + 4, q);
        ctx.gpr[3] = t;
    }
    if (q == 0) return;
    const u32 h = load<u32>(q + 160);
    ctx.gpr[3] = h;
    const u32 v = o + 8;
    for (u32 c = load<u32>(h + 272); c != 0;) {
        const u32 cap = load<u16>(v + 4);
        if (load<u16>(v + 8) == cap) call(ctx, 0x00551718, {v, (cap << 1) & 0x1FFFEu});
        const u32 i = load<u16>(v + 8);
        const u32 d = load<u32>(v);
        const u32 n = load<u16>(v + 8) + 1u;
        const u32 hw = load<u16>(v + 10);
        store<u32>(d + ((i << 2) & 0x3FFFCu), c);
        store<u16>(v + 8, static_cast<u16>(n));
        c = load<u32>(c + 268);
        const u32 r = static_cast<s32>(hw) > static_cast<s32>(n & 0xFFFFu) ? hw : n;
        store<u16>(v + 10, static_cast<u16>(r));
        ctx.gpr[3] = r;
    }
}

// T f(Section* o, s32 m, u8 force): Section_SetModeBase_003260F0(o, m,
// force), first (unless unforced and already in mode m, u8 +18): for mode
// 4 the section becomes the world's current (*0x00F64C20->+80 = o, +2572
// = +2576, u8 +2581 = u8 +2580); for modes 5 / 6 with u8 +2582 set the
// handle *0x00F64CD0 (Handle_ResolveBit1) with flag 0x40000000 of +396
// gets Obj_Dispatch362(h, 1, 1, 0), and the world's current is cleared.
// r3 as the base left it (f1 = +2576 for mode 4).
DECOMP(00340038, Section_SetMode_00340038) {
    const u32 o = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1), f = arg<u32>(ctx, 2) & 0xFFu;
    if (f != 0 || load<u8>(o + 18) != m) {
        if (m == 5 || m == 6) {
            if (load<u8>(o + 2582) != 0) {
                const u32 h = static_cast<u32>(call(ctx, 0x003568E0, {load<u32>(0x00F64CD0)}));
                if (h != 0 && (load<u32>(h + 396) & 0x40000000u) != 0) call(ctx, 0x0036325C, {h, 1, 1, 0});
            }
            store<u32>(load<u32>(0x00F64C20) + 80, 0);
        } else if (m == 4) {
            store<u32>(load<u32>(0x00F64C20) + 80, o);
            const u32 x = load<u32>(o + 2576);
            ctx.fpr[1] = static_cast<f64>(std::bit_cast<f32>(x));
            const u32 y = load<u8>(o + 2580);
            store<u32>(o + 2572, x);
            store<u8>(o + 2581, static_cast<u8>(y));
        }
    }
    ctx.gpr[3] = call(ctx, 0x003260F0, {o, m, f});
}

// T f(Speaker* o, u32 key, u8 all, u8 x): the line of key in the map
// 0x0106BC14 (SortedMap_Find76D894) set on o (Speaker_SetLine34F690(o,
// line, x)) and, with all, on each node of o's vector +92
// (Node_Init350178(n, line, x)); an unknown key is only named
// (Hash_ToString(key)). r3 as the original leaves it.
DECOMP(00350764, Speaker_SetLineByKey_00350764) {
    const u32 o = arg<u32>(ctx, 0), key = arg<u32>(ctx, 1), all = arg<u32>(ctx, 2) & 0xFFu, x = arg<u32>(ctx, 3) & 0xFFu;
    Stack frame(ctx, 16);
    store<u32>(frame.addr(), key);  // sp+112
    const u32 p = static_cast<u32>(call(ctx, 0x0076D894, {0x0106BC14u, frame.addr()}));
    const u32 line = load<u32>(p);
    if (line == 0) {
        ctx.gpr[3] = call(ctx, 0x0056A6E0, {key});
        return;
    }
    call(ctx, 0x0034F690, {o, line, x});
    ctx.gpr[3] = all;
    if (all == 0) return;
    for (u32 i = 0;; ++i) {
        const u32 v = load<u32>(o + 92);
        ctx.gpr[3] = v;
        const u32 n = v != 0 ? load<u16>(v + 8) : 0u;
        if (!(i < n)) return;
        call(ctx, 0x00350178, {load<u32>(load<u32>(v) + 4 * i), line, x});
    }
}

// T f(T* o, u8 v): o->+24 (when set) gets vt[40](it, v, 1); o->+176,
// +180 and +184 (when set) get vt[88](it, v). r3 as the last step left it.
DECOMP(0036F9CC, Obj_SetVisible_0036F9CC) {
    const u32 o = arg<u32>(ctx, 0), v = arg<u32>(ctx, 1) & 0xFFu;
    const u32 a = load<u32>(o + 24);
    ctx.gpr[3] = a;
    if (a != 0) VCall(ctx, a, 40, {a, v, 1});
    for (const u32 off : {176u, 180u}) {
        const u32 b = load<u32>(o + off);
        ctx.gpr[3] = b;
        if (b != 0) VCall(ctx, b, 88, {b, v});
    }
    const u32 d = load<u32>(o + 184);
    if (d != 0) VCall(ctx, d, 88, {d, v});
}

// T f(T* o, u8 on, u32 b): for o with flag 0x80000000 and without flag
// 0x08000000 in +360: with on and +160 >> 1 = 0, +168 = 0 and
// 0x00371BA8(o, 0); without on and +160 >> 1 = 5, bit 0x40000000 of +360
// = b & 1, 0x00371360(o) and flag 0x01000000 set in +360. r3 as the
// original leaves it. (Its vt[12] branch for state 0 after the state-5
// test is dead.)
DECOMP(003735C8, Obj_SetTalkState_003735C8) {
    const u32 o = arg<u32>(ctx, 0), on = arg<u32>(ctx, 1) & 0xFFu, b = arg<u32>(ctx, 2);
    const u32 w = load<u32>(o + 360);
    if (on != 0) {
        ctx.gpr[3] = w;
        if ((w & 0x08000000u) != 0) return;
        ctx.gpr[3] = 0;
        if ((w & 0x80000000u) == 0) return;
        const u32 s = load<u32>(o + 160) >> 1;
        ctx.gpr[3] = s;
        if (s != 0) return;
        store<u32>(o + 168, 0);
        ctx.gpr[3] = call(ctx, 0x00371BA8, {o, 0});
        return;
    }
    ctx.gpr[3] = b;
    if ((w & 0x08000000u) != 0 || (w & 0x80000000u) == 0) return;
    if ((load<u32>(o + 160) >> 1) != 5) return;
    store<u32>(o + 360, (w & ~0x40000000u) | ((b & 1u) << 30));
    call(ctx, 0x00371360, {o});
    const u32 w2 = load<u32>(o + 360) | 0x01000000u;
    store<u32>(o + 360, w2);
    ctx.gpr[3] = w2;
}

// bool f(Reader* r): the current chunk read whole into the default memory
// stack, opened as a zip (Archive_OpenZip(buf, n, *0x00F65790)) and loaded
// by 0x0039129C(*0x00F6578C, z, 0); then z->vt[8](z), the stack's
// vt[8](stack, buf) and *0x00F65790 = 0. r3 = 1.
DECOMP(00391598, Archive_LoadChunkZip_00391598) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4, {r}));
    const u32 st = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260C, {st, n}));
    call(ctx, 0x00563768, {r, buf, n});
    const u32 z = static_cast<u32>(call(ctx, 0x00567034, {buf, n, load<u32>(0x00F65790)}));
    call(ctx, 0x0039129C, {load<u32>(0x00F6578C), z, 0});
    VCall(ctx, z, 8, {z});
    const u32 st2 = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    VCall(ctx, st2, 8, {st2, buf});
    store<u32>(0x00F65790, 0);
    ctx.gpr[3] = 1;
}

// T f(T* o): each object e of the list 0x01013B34 (u16 count +8) with
// flag 0x10 in +192 and a controller c (+244) for which c->vt[120](c) is
// set gets Obj_SceneEnd0DD02C(e, u8 o->+9, 0); then
// Flag_Set8_40000000(o). r3 as that left it.
DECOMP(00437C5C, Scene_EndAll_00437C5C) {
    const u32 o = arg<u32>(ctx, 0);
    constexpr u32 V = 0x01013B34;
    for (u32 i = 0; i < load<u16>(V + 8); ++i) {
        const u32 e = load<u32>(load<u32>(V) + 4 * i);
        if (e == 0 || (load<u32>(e + 192) & 0x10u) == 0) continue;
        const u32 c = load<u32>(e + 244);
        if (c == 0) continue;
        if (static_cast<u32>(VCall(ctx, c, 120, {c})) == 0) continue;
        call(ctx, 0x000DD02C, {e, load<u8>(o + 9), 0});
    }
    ctx.gpr[3] = call(ctx, 0x0042CA60, {o});
}

// bool f(Xml* x): when the settings name (*0x010C2C28, hashed with
// String_HashNoCase) is the world's (*0x00F65CA0->+0x128114), the
// attribute 0x00AE916C copied into the string pool (cursor *0x010B7838,
// base *0x010B7834, 0x8000 bytes) when it fits, the cursor advanced past
// it and *0x00F65C80 = the copy (unless the cursor was 0). r3 = 1.
DECOMP(004842A8, Settings_ReadPoolString_004842A8) {
    const u32 x = arg<u32>(ctx, 0);
    ctx.gpr[3] = 1;
    const u32 h = static_cast<u32>(call(ctx, 0x0056A710, {load<u32>(kGlobals10C2C28)}));
    ctx.gpr[3] = 1;
    if (h != load<u32>(load<u32>(0x00F65CA0) + 0x128114)) return;
    const u32 a = static_cast<u32>(call(ctx, 0x004F9628, {x, 0x00AE916Cu}));
    ctx.gpr[3] = 1;
    if (a == 0) return;
    const u32 len = static_cast<u32>(call(ctx, 0x00555FA4, {a}));
    ctx.gpr[3] = 1;
    const u32 cur = load<u32>(0x010B7838);
    if (!(cur + len + 1 < load<u32>(0x010B7834) + 0x8000u)) return;
    call(ctx, 0x00555F80, {cur, a});
    store<u32>(0x010B7838, load<u32>(0x010B7838) + len + 1);
    if (cur != 0) store<u32>(0x00F65C80, cur);
    ctx.gpr[3] = 1;
}

// bool f(Reader* r): the current chunk read whole into the default memory
// stack, opened as a zip (Archive_OpenZip(buf, n, 0x00AE849C)) and loaded
// by 0x004829F0(*0x00F65CA0, z); then z->vt[8](z), the stack's
// vt[8](stack, buf) and *0x00F65D34 = 0. r3 = 1.
DECOMP(0048C4C0, Archive_LoadWorldZip_0048C4C0) {
    const u32 r = arg<u32>(ctx, 0);
    const u32 n = static_cast<u32>(call(ctx, 0x005637D4, {r}));
    const u32 st = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    const u32 buf = static_cast<u32>(call(ctx, 0x0055260C, {st, n}));
    call(ctx, 0x00563768, {r, buf, n});
    const u32 z = static_cast<u32>(call(ctx, 0x00567034, {buf, n, 0x00AE849Cu}));
    call(ctx, 0x004829F0, {load<u32>(0x00F65CA0), z});
    VCall(ctx, z, 8, {z});
    const u32 st2 = static_cast<u32>(call(ctx, o33fn::Mem_GetDefaultStack, {}));
    VCall(ctx, st2, 8, {st2, buf});
    store<u32>(0x00F65D34, 0);
    ctx.gpr[3] = 1;
}

// T f(Owner* o): bindings 0x00AEE480 (kind 5) and 0x00AEE910 (kind 2) on
// o (Node_Bind(o, name, kind, 0, -1, 0)); the second gets one string
// slot (Binding_Reserve(b, 1)) holding a 100-byte copy of 0x00AEE9E4
// (Mem_Alloc, String_CopyN; count b->+0 below capacity +72, array +68);
// then Owner_AddName(o, 0x00AEE330, 1). r3 as that left it.
DECOMP(004B5B74, Owner_BindDefaults_004B5B74) {
    const u32 o = arg<u32>(ctx, 0);
    call(ctx, 0x004D3088, {o, 0x00AEE480u, 5, 0, ~0ull, 0});
    const u32 b = static_cast<u32>(call(ctx, 0x004D3088, {o, 0x00AEE910u, 2, 0, ~0ull, 0}));
    call(ctx, 0x004D2BB8, {b, 1});
    if (load<u32>(b) < load<u32>(b + 72)) {
        const u32 p = static_cast<u32>(call(ctx, 0x0054ED38, {100, 0, 16}));
        const u32 i = load<u32>(b);
        store<u32>(load<u32>(b + 68) + (i << 2), p);
        call(ctx, 0x00553638, {load<u32>(load<u32>(b + 68) + (i << 2)), 0x00AEE9E4u, 100});
        store<u32>(b, load<u32>(b) + 1);
    }
    ctx.gpr[3] = call(ctx, 0x004D3378, {o, 0x00AEE330u, 1});
}

// T* f(?, Xml* x): a 48-byte node (SmallAlloc; vt 0x00B21E20, u16 +4 =
// 1, +32 = 0x01164B40 raw) reading hash 0x0024728B -> +8 (default 0)
// and, when that is absent, the vec3s 0x00014B01 and 0x04DA9951 -> +32.
// r3 = p.
DECOMP(004BF554, XmlNew_B21E20_004BF554) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {48}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    constexpr u32 G = 0x01164B40;
    CopyW32(p + 32, G);
    store<u16>(p + 4, 1);
    const u32 g1 = load<u32>(G + 4);
    store<u32>(p, 0x00B21E20);
    store<u32>(p + 36, g1);
    CopyW32(p + 40, G + 8);
    ctx.fpr[1] = Lfs32(G + 8);
    if (static_cast<u32>(call(ctx, 0x004FA14C, {x, 0x0024728Bu, p + 8, 0})) == 0) {
        call(ctx, 0x004F9E0C, {x, 0x00014B01u, p + 32, p + 32});
        call(ctx, 0x004F9E0C, {x, 0x04DA9951u, p + 32, p + 32});
    }
    ctx.gpr[3] = p;
}

// T* f(?, Xml* x): a 96-byte node (SmallAlloc; vt 0x00B21E68, u16 +4 =
// 1, the transform +32..+88 = 0x01164BD0 rows raw) reading hash
// 0x0024728B -> +8 (default 0). r3 = p.
DECOMP(004BF7E0, XmlNew_B21E68_004BF7E0) {
    const u32 x = arg<u32>(ctx, 1);
    const u32 p = static_cast<u32>(call(ctx, 0x0055238C, {96}));
    ctx.gpr[3] = 0;
    if (p == 0) return;
    constexpr u32 A = 0x01164BD0;
    for (const u32 k : {0u, 4u, 8u, 16u, 20u}) CopyW32(p + 32 + k, A + k);
    const u32 a6 = load<u32>(A + 24);
    store<u16>(p + 4, 1);
    store<u32>(p + 56, a6);
    const u32 a8 = load<u32>(A + 32);
    store<u32>(p, 0x00B21E68);
    store<u32>(p + 64, a8);
    for (const u32 k : {36u, 40u, 48u, 52u, 56u}) CopyW32(p + 32 + k, A + k);
    ctx.fpr[1] = Lfs32(A + 56);
    call(ctx, 0x004FA14C, {x, 0x0024728Bu, p + 8, 0});
    ctx.gpr[3] = p;
}

// void f(f32 out[2], const T* o): out = o->+192/+196 when both are at
// least K (*0x004E59C4); else from o's image (o->+168->+20: u16 size
// +52 / +54) scaled by |o->+184| / |o->+188| when there is one, else the
// default 0x01164AFC (written first in any case; raw copies). f1 = the
// last value read or written (K when not copied).
DECOMP(004E59C8, Obj_GetSize2D_004E59C8) {
    const u32 out = arg<u32>(ctx, 0), o = arg<u32>(ctx, 1);
    constexpr u32 G = 0x01164AFC;
    CopyW32(out, G);
    CopyW32(out + 4, G + 4);
    const f64 k = Lfs32(0x004E59C4);
    ctx.fpr[1] = k;
    if (Lfs32(o + 192) >= k && Lfs32(o + 196) >= k) {
        CopyW32(out, o + 192);
        CopyW32(out + 4, o + 196);
        ctx.fpr[1] = Lfs32(o + 196);
        return;
    }
    const u32 m = load<u32>(o + 168);
    if (m == 0) return;
    const u32 n = load<u32>(m + 20);
    if (n == 0) return;
    const f64 w = static_cast<f32>(static_cast<f64>(load<u16>(n + 52)));
    const f64 h = static_cast<f32>(static_cast<f64>(load<u16>(n + 54)));
    const f64 x = static_cast<f32>(w * std::fabs(Lfs32(o + 184)));
    Stfs32(out, x);
    const f64 y = static_cast<f32>(h * std::fabs(Lfs32(o + 188)));
    Stfs32(out + 4, y);
    ctx.fpr[1] = y;
}

// void f(Blend* b, s32 mode): blend state by mode (word +0 gets bit
// 0x80000000 and the 16-bit field 0x7FFF8000 = factor << 15; u16 +4 =
// equation, u16 +6 = second factor): 4: 770 / 0x800B / 1; 3: 770 /
// 0x8006 / 1; 2: 770 / 0x8006 / 771; 1: 1 / 0x8006 / 0; 0: as 1 with
// bit 0x80000000 cleared. Other modes change nothing. r3 = b.
DECOMP(0074DFE0, Blend_SetMode_0074DFE0) {
    const u32 b = arg<u32>(ctx, 0), m = arg<u32>(ctx, 1);
    ctx.gpr[3] = b;
    u32 f, eq, f2;
    switch (m) {
    case 4: f = 770; eq = 0x800B; f2 = 1; break;
    case 3: f = 770; eq = 0x8006; f2 = 1; break;
    case 2: f = 770; eq = 0x8006; f2 = 771; break;
    case 1:
    case 0: f = 1; eq = 0x8006; f2 = 0; break;
    default: return;
    }
    u32 w = ((load<u32>(b) | 0x80000000u) & ~0x7FFF8000u) | ((f << 15) & 0x7FFF8000u);
    if (m == 0) w &= 0x7FFFFFFFu;
    store<u16>(b + 4, static_cast<u16>(eq));
    store<u16>(b + 6, static_cast<u16>(f2));
    store<u32>(b, w);
}

// ~T() of a Havok array holder (o may be 0; r3 = 0 then): count +20 = 0;
// unless flag 0x80000000 of +24 (don't deallocate) is set, vt 0x00F1DE58
// and the buffer +16 (capacity +24 & 0x3FFFFFFF of 48-byte elements)
// freed by the allocator 0x011DF31C (vt[20]); then +16 = 0, +24 =
// 0x80000000, vt 0x00B13C84 and the thread's memory router ->+44
// vt[12](it, o, 416) frees o itself. r3 as that call left it.
DECOMP(00765CD4, HkArray48_Delete_00765CD4) {
    const u32 o = arg<u32>(ctx, 0);
    if (o == 0) {
        ctx.gpr[3] = 0;
        return;
    }
    store<u32>(o + 20, 0);
    const u32 w = load<u32>(o + 24);
    if ((w & 0x80000000u) == 0) {
        const u32 buf = load<u32>(o + 16);
        store<u32>(o, 0x00F1DE58);
        const u32 bytes = static_cast<u32>(static_cast<u64>(w & 0x3FFFFFFFu) * 48);
        VCall(ctx, 0x011DF31Cu, 20, {0x011DF31Cu, buf, Sx33(bytes)});
    }
    store<u32>(o + 16, 0);
    store<u32>(o + 24, 0x80000000u);
    store<u32>(o, 0x00B13C84);
    const u32 rt = load<u32>(HkRouter33(ctx) + 44);
    ctx.gpr[3] = VCall(ctx, rt, 12, {rt, o, 416});
}

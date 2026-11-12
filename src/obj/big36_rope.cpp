// Game code, large-function sweep part 36: the mass-spring chain solver.
#include "big36.h"

namespace sly4::obj {

struct ChainParticle {
    Vec3 pos;       // +0
    pad<4> _12;
    Vec3 vel;       // +16
    pad<4> _28;
    Vec3 force;     // +32
    be_f32 mass;    // +44
    Vec3 dir;       // +48: unit direction to the next particle
    be_f32 dist;    // +60: distance to the next particle
};
static_assert(sizeof(ChainParticle) == 64 && offsetof(ChainParticle, mass) == 44);

struct Chain {
    gp<ChainParticle> parts;  // +0: n + 1 particles
    pad<4> _4;
    be_u32 n;                 // +8: segments
    be_u32 iters;             // +12
    u8 pin_first;             // +16
    u8 pin_last;              // +17
    pad<2> _18;
    be_f32 dt;                // +20
    be_f32 rest;              // +24: rest length
    be_f32 k_stretch;         // +28
    be_f32 k_compress;        // +32
    be_f32 damping;           // +36
    be_f32 drag;              // +40
    pad<4> _44;
    Vec3 gravity;             // +48 (scaled by the mass)
    pad<4> _60;
    Vec3 push;                // +64: a constant force
};
static_assert(offsetof(Chain, gravity) == 48 && offsetof(Chain, push) == 64);

namespace {
constexpr u32 kChainScratch = 0x01042B40;  // the particles are solved in this global buffer
constexpr u32 kOne = 0x001BC0E4;           // f32 1.0f
constexpr u32 kZero = 0x001BC0E8;          // f32 0.0f
constexpr u32 kMinLen = 0x001BC0EC;        // f32 1e-5f

void St(be_f32& d, f64 x) { d = static_cast<f32>(x); }
}  // namespace

// void Chain_Solve1BC0F0(Chain* c) (from 0x001BC5C8). The particles are
// copied to kChainScratch; each iteration: spring and damping forces along
// each segment's stored direction, gravity and the constant force, pinned
// ends get force = ZeroVec and mass word 1.0f, then vel += force / mass,
// vel -= vel * drag, force = 0, pos += vel * dt; then each segment's
// direction and length are remeasured (unnormalised below 1e-5). The
// particles are copied back.
void Chain_Solve1BC0F0(Chain* c) {
    ChainParticle* buf = host<ChainParticle>(kChainScratch);
    call(fn::Memmove, buf, c->parts.get(), (c->n << 6) + 64u);
    const u32 n = c->n;
    const u32 iters = c->iters;
    if (iters != 0) {
        const bool pinFirst = c->pin_first != 0;
        const bool pinLast = c->pin_last != 0;
        for (u32 it = 0; it < iters; ++it) {
            if (n != 0) {
                // springs: segment j between particles j and j + 1
                const f64 zero = lf(global<be_f32>(kZero));
                u32 j = 0;
                do {
                    ChainParticle* p = buf + j;
                    ChainParticle* q = p + 1;
                    const f64 dy = Subs(lf(q->pos.y), lf(p->pos.y));
                    const f64 dx = Subs(lf(q->pos.x), lf(p->pos.x));
                    const f64 dz = Subs(lf(q->pos.z), lf(p->pos.z));
                    const f64 ux = lf(p->dir.x), uy = lf(p->dir.y), uz = lf(p->dir.z);
                    f64 t = Muls(dy, uy);
                    t = Fmadds(dx, ux, t);
                    f64 ext = Fmadds(dz, uz, t);
                    ext = Subs(ext, lf(c->rest));
                    const f64 k = (ext > zero || ext == zero) ? lf(c->k_stretch) : lf(c->k_compress);
                    const f64 fs = Muls(ext, k);
                    const f64 dvy = Subs(lf(q->vel.y), lf(p->vel.y));
                    const f64 dvx = Subs(lf(q->vel.x), lf(p->vel.x));
                    const f64 dvz = Subs(lf(q->vel.z), lf(p->vel.z));
                    const f64 pfx = Fmadds(fs, ux, lf(p->force.x));
                    const f64 pfy = Fmadds(fs, uy, lf(p->force.y));
                    const f64 pfz = Fmadds(fs, uz, lf(p->force.z));
                    const f64 pm = lf(p->mass);
                    const f64 qfx = Fnmsubs(fs, ux, lf(q->force.x));
                    const f64 qfy = Fnmsubs(fs, uy, lf(q->force.y));
                    f64 rv = Muls(dvy, uy);
                    rv = Fmadds(dvx, ux, rv);
                    const f64 qfz = Fnmsubs(fs, uz, lf(q->force.z));
                    const u32 qmBits = bits(q->mass);
                    const f64 qm = lf(q->mass);
                    rv = Fmadds(dvz, uz, rv);
                    const f64 damp = lf(c->damping);
                    const f64 gx = Muls(Muls(ux, rv), damp);
                    const f64 gy = Muls(Muls(uy, rv), damp);
                    const f64 gz = Muls(Muls(uz, rv), damp);
                    St(p->force.x, Fmadds(gx, pm, pfx));
                    St(p->force.y, Fmadds(gy, pm, pfy));
                    St(p->force.z, Fmadds(gz, pm, pfz));
                    St(q->force.x, Fnmsubs(gx, qm, qfx));
                    St(q->force.y, Fnmsubs(gy, qm, qfy));
                    St(q->force.z, Fnmsubs(gz, qm, qfz));
                    set_bits(q->mass, qmBits);  // stfs of the lfs'd mass: the same word
                } while (++j < n);
            }
            // gravity (times the mass) and the constant force, for n + 1
            // particles (signed down-count, at least once)
            {
                ChainParticle* p = buf;
                s32 left = static_cast<s32>(n + 1u);
                do {
                    const f64 m = lf(p->mass);
                    --left;
                    const f64 fx = Fmadds(lf(c->gravity.x), m, lf(p->force.x));
                    const f64 fy = Fmadds(lf(c->gravity.y), m, lf(p->force.y));
                    const f64 fz = Fmadds(lf(c->gravity.z), m, lf(p->force.z));
                    St(p->force.x, Adds(fx, lf(c->push.x)));
                    St(p->force.y, Adds(fy, lf(c->push.y)));
                    St(p->force.z, Adds(fz, lf(c->push.z)));
                    ++p;
                } while (left > 0);
            }
            const Vec3& zv = global<Vec3>(kZeroVec);
            if (pinFirst) {
                buf->force = zv;  // raw words
                set_bits(buf->mass, 0x3F800000u);
            }
            if (pinLast) {
                ChainParticle& l = gm<ChainParticle>(kChainScratch + (n << 6));
                l.force = zv;
                set_bits(l.mass, 0x3F800000u);
            }
            // integrate n + 1 particles (signed down-count, at least once)
            {
                const f64 one = lf(global<be_f32>(kOne));
                ChainParticle* p = buf;
                s32 left = static_cast<s32>(n + 1u);
                do {
                    const f64 inv = Divs(one, lf(p->mass));
                    const f64 drag = lf(c->drag);
                    --left;
                    f64 vx = Fmadds(lf(p->force.x), inv, lf(p->vel.x));
                    f64 vy = Fmadds(lf(p->force.y), inv, lf(p->vel.y));
                    f64 vz = Fmadds(lf(p->force.z), inv, lf(p->vel.z));
                    const f64 px = lf(p->pos.x), py = lf(p->pos.y), pz = lf(p->pos.z);
                    vx = Fnmsubs(vx, drag, vx);
                    vy = Fnmsubs(vy, drag, vy);
                    vz = Fnmsubs(vz, drag, vz);
                    St(p->vel.x, vx);
                    St(p->vel.y, vy);
                    St(p->vel.z, vz);
                    const f64 dt = lf(c->dt);
                    set_bits(p->force.x, 0);
                    set_bits(p->force.y, 0);
                    set_bits(p->force.z, 0);
                    St(p->pos.x, Fmadds(vx, dt, px));
                    St(p->pos.y, Fmadds(vy, dt, py));
                    St(p->pos.z, Fmadds(vz, dt, pz));
                    ++p;
                } while (left > 0);
            }
            if (n != 0) {
                // remeasure each segment
                const f64 minLen = lf(global<be_f32>(kMinLen)), one = lf(global<be_f32>(kOne));
                u32 j = 0;
                do {
                    ChainParticle* p = buf + j;
                    ChainParticle* q = p + 1;
                    f64 dy = Subs(lf(q->pos.y), lf(p->pos.y));
                    f64 dx = Subs(lf(q->pos.x), lf(p->pos.x));
                    f64 dz = Subs(lf(q->pos.z), lf(p->pos.z));
                    f64 l2 = Muls(dy, dy);
                    l2 = Fmadds(dx, dx, l2);
                    l2 = Fmadds(dz, dz, l2);
                    const f64 len = Sqrts(l2);
                    if (len > minLen || len == minLen) {
                        const f64 s = Divs(one, len);
                        dx = Muls(dx, s);
                        dy = Muls(dy, s);
                        dz = Muls(dz, s);
                    }
                    St(p->dir.x, dx);
                    St(p->dir.y, dy);
                    St(p->dir.z, dz);
                    St(p->dist, len);
                } while (++j < n);
            }
        }
    }
    call(fn::Memmove, c->parts.get(), buf, (n << 6) + 64u);
}

}  // namespace sly4::obj

NATIVE(001BC0F0, sly4::obj::Chain_Solve1BC0F0)

// Native decompiled code: plain C++ functions with real types and signatures.
//
//     struct Camera { u8 pad0[48]; gp<Obj> split; /* +48 */ ... };
//
//     bool Camera_IsSplit(const Camera* cam) {
//         return cam->split && (cam->split->flags & 0x8000) != 0;
//     }
//     NATIVE(002A6044, Camera_IsSplit)
//
// The game's data still lives in PS3 memory (big endian, 32-bit pointers), so
// structures describe that layout with be<T> numbers and gp<T> pointers, and
// a T* here is a host pointer into guest memory. Native functions call each
// other directly. Only where native code meets code that is still
// recompiled is there a bridge:
//   - NATIVE(addr, fn) makes `fn` callable from recompiled code at its
//     original address (arguments and result moved between PPU registers and
//     the C++ signature, following the PPU ABI);
//   - call<R>(addr, args...) / vcall<R>(obj, slot, args...) call a guest
//     function that has no native version yet.
// The switches (--no-decomp, --decomp-off, coverage) work through NATIVE as
// they do through DECOMP.
#pragma once

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include "opentit/decomp.h"

namespace opentit::native {

using decomp::be;
using decomp::be_f32;
using decomp::be_f64;
using decomp::be_s16;
using decomp::be_s32;
using decomp::be_s64;
using decomp::be_u16;
using decomp::be_u32;
using decomp::be_u64;

// ---------------------------------------------------------------- guest <-> host addresses
template <typename T = u8>
T* host(u32 ea) {
    return ea ? reinterpret_cast<T*>(mem::ptr(ea)) : nullptr;
}
inline u32 guest(const void* p) {
    return p ? static_cast<u32>(static_cast<const u8*>(p) - mem::ptr(0)) : 0u;
}

// A 32-bit guest pointer stored in guest memory, typed.
template <typename T>
struct gp {
    be_u32 ea;
    T* get() const { return host<T>(ea.get()); }
    operator T*() const { return get(); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    explicit operator bool() const { return ea.get() != 0; }
    gp& operator=(T* p) {
        ea = guest(p);
        return *this;
    }
    gp& operator=(std::nullptr_t) {
        ea = 0u;
        return *this;
    }
};
static_assert(sizeof(gp<int>) == 4);

// A global variable at a fixed guest address.
template <typename T>
T& global(u32 ea) {
    return *host<T>(ea);
}

// Padding inside guest structures whose fields aren't known yet.
template <std::size_t N>
struct pad {
    u8 raw[N];
};

// ---------------------------------------------------------------- the current PPU context
// Set by the bridge while native code runs, used to call into guest code.
inline thread_local PPUContext* t_ctx = nullptr;
inline PPUContext& ctx() { return *t_ctx; }
struct CtxScope {
    PPUContext* prev;
    explicit CtxScope(PPUContext& c) : prev(t_ctx) { t_ctx = &c; }
    ~CtxScope() { t_ctx = prev; }
    CtxScope(const CtxScope&) = delete;
    CtxScope& operator=(const CtxScope&) = delete;
};

// Scratch memory on the guest stack, for out-parameters of guest calls:
//     GuestFrame f(sizeof(Vec3));  Vec3* out = f.as<Vec3>();
class GuestFrame {
public:
    explicit GuestFrame(u32 bytes) : s_(ctx(), bytes) {}
    u32 addr() const { return s_.addr(); }
    template <typename T>
    T* as(u32 offset = 0) const {
        return host<T>(s_.addr() + offset);
    }

private:
    decomp::Stack s_;
};

// ---------------------------------------------------------------- ABI value conversion
namespace detail {
template <typename T>
inline constexpr bool is_fp = std::is_floating_point_v<T>;

template <typename T>
u64 to_gpr(T v) {
    if constexpr (std::is_pointer_v<T>) return guest(v);
    else if constexpr (std::is_same_v<T, bool>) return v ? 1u : 0u;
    else if constexpr (std::is_enum_v<T>) return to_gpr(static_cast<std::underlying_type_t<T>>(v));
    else if constexpr (std::is_signed_v<T>) return static_cast<u64>(static_cast<s64>(v));
    else return static_cast<u64>(v);
}
template <typename T>
T from_gpr(u64 v) {
    if constexpr (std::is_pointer_v<T>) return host<std::remove_pointer_t<T>>(static_cast<u32>(v));
    else if constexpr (std::is_same_v<T, bool>) return static_cast<u8>(v) != 0;
    else if constexpr (std::is_enum_v<T>) return static_cast<T>(from_gpr<std::underlying_type_t<T>>(v));
    else return static_cast<T>(v);
}

// PPU ABI: parameter n uses GPR slot r3+n (or the caller's parameter save
// area at 48 + 8n past n = 7); floating-point parameters go in f1.. and still
// take their GPR slot.
template <typename... A>
struct Layout {
    static constexpr std::size_t n = sizeof...(A);
    static constexpr std::array<bool, n> fp = {is_fp<A>...};
    static constexpr std::array<int, n> fslot = [] {
        std::array<int, n> s{};
        int f = 1;
        for (std::size_t i = 0; i < n; ++i) s[i] = fp[i] ? f++ : 0;
        return s;
    }();
};

template <typename T>
T read_arg(PPUContext& c, std::size_t i, int fslot) {
    if constexpr (is_fp<T>) {
        return static_cast<T>(c.fpr[fslot]);
    } else {
        u64 raw;
        if (i < 8) raw = c.gpr[3 + i];
        else raw = mem::r64(static_cast<u32>(c.gpr[1]) + 48 + 8 * static_cast<u32>(i));
        return from_gpr<T>(raw);
    }
}

template <typename R>
void write_ret(PPUContext& c, R v) {
    if constexpr (is_fp<R>) c.fpr[1] = static_cast<f64>(v);
    else c.gpr[3] = to_gpr(v);
}
template <typename R>
R read_ret(PPUContext& c) {
    if constexpr (std::is_void_v<R>) return;
    else if constexpr (is_fp<R>) return static_cast<R>(c.fpr[1]);
    else return from_gpr<R>(c.gpr[3]);
}

template <typename R, typename... A, std::size_t... I>
void bridge_impl(R (*fn)(A...), PPUContext& c, std::index_sequence<I...>) {
    using L = Layout<A...>;
    std::tuple<A...> args{read_arg<A>(c, I, L::fslot[I])...};
    if constexpr (std::is_void_v<R>) std::apply(fn, args);
    else write_ret<R>(c, std::apply(fn, args));
}

template <typename T>
void put_arg(PPUContext& c, u64& gslot, int fslot, T v) {
    if constexpr (is_fp<T>) c.fpr[fslot] = static_cast<f64>(v);
    else gslot = to_gpr(v);
}

template <bool Ptr, typename R, typename... A, std::size_t... I>
R call_impl(u32 target, std::index_sequence<I...>, A... a) {
    using L = Layout<A...>;
    PPUContext& c = ctx();
    std::array<u64, sizeof...(A)> g{};
    (put_arg<A>(c, g[I], L::fslot[I], a), ...);
    if constexpr (Ptr) decomp::call_ptr(c, target, {g[I]...});
    else decomp::call(c, target, {g[I]...});
    return read_ret<R>(c);
}
}  // namespace detail

// Run a native function for a PPU caller.
template <auto Fn>
void bridge(PPUContext& c) {
    CtxScope scope(c);
    using F = decltype(Fn);
    [&]<typename R, typename... A>(R (*)(A...)) {
        detail::bridge_impl(Fn, c, std::index_sequence_for<A...>{});
    }(static_cast<F>(nullptr));
}

// Call a guest function (recompiled, or decompiled but not native yet).
template <typename R = void, typename... A>
R call(u32 addr, A... a) {
    return detail::call_impl<false, R>(addr, std::index_sequence_for<A...>{}, a...);
}
// Call through a function descriptor (a guest function pointer).
template <typename R = void, typename... A>
R call_ptr(u32 opd, A... a) {
    return detail::call_impl<true, R>(opd, std::index_sequence_for<A...>{}, a...);
}
// Virtual call: obj's vtable entry at byte offset `slot` holds a descriptor.
template <typename R = void, typename O, typename... A>
R vcall(O* obj, u32 slot, A... a) {
    const u32 vt = mem::r32(guest(obj));
    return call_ptr<R>(mem::r32(vt + slot), obj, a...);
}

}  // namespace opentit::native

// NATIVE(XXXXXXXX, fn) -- install native function `fn` (any qualified name) at
// guest address XXXXXXXX. Use it at namespace scope outside any namespace.
#define NATIVE(addr, fn) \
    DECOMP(addr, native_##addr) { ::opentit::native::bridge<&fn>(ctx); }

# Native decompiled code

Decompiled functions are plain C++: typed structures, real signatures, direct
calls. They do not use the PPU register context.

```cpp
#include "opentit/native.h"

namespace sly4::frame {
using namespace opentit::native;

struct SplitFlags { pad<164> _0; be_u32 flags; };   // +164
struct Camera     { pad<48> _0;  gp<SplitFlags> split; };  // +48

bool Camera_IsSplit(const Camera* camera) {
    const SplitFlags* obj = camera->split;
    return obj && (obj->flags & 0x8000) != 0;
}
}  // namespace sly4::frame

NATIVE(002A6044, sly4::frame::Camera_IsSplit)
```

## The pieces (engine/runtime/include/opentit/native.h)

- **Data layout.** The game's data is still the PS3's: big-endian numbers and
  32-bit pointers, in guest memory. Structures describe that layout:
  `be_u32`, `be_f32`, ... for numbers, `gp<T>` for pointers, `pad<N>` for
  bytes not understood yet, plain `u8`/`s8` for bytes. A `T*` in native code
  is a host pointer into guest memory.
- **Globals.** `global<T>(0x00F558D0)` is the variable at that guest address.
- **Calls between native functions** are ordinary C++ calls. Declare shared
  functions and structures in the area's header.
- **Calls into code that is not native yet:** `call<R>(addr, args...)`,
  `call_ptr<R>(opd, args...)` (a guest function pointer), and
  `vcall<R>(obj, slot, args...)` (obj's vtable entry at byte offset `slot`).
  Arguments follow the PPU ABI: integers and pointers in r3.., floats in f1..
  (each still taking its GPR slot), the 9th argument and later on the stack.
- **`NATIVE(XXXXXXXX, fn)`**, at the start of a line outside any namespace,
  installs `fn` at that guest address, so recompiled callers reach it. The
  recompiler leaves that address out, and `--no-decomp` / `--decomp-off` /
  `--decomp-coverage` work as for any decompiled function.
- **Guest-side scratch memory.** A local whose address is passed to guest code
  or stored in guest memory must live in guest memory: `GuestFrame f(bytes);`
  then `f.as<T>(offset)`. Locals only used by native code are ordinary C++
  locals.
- `host<T>(ea)` and `guest(p)` convert between guest addresses and pointers.

## Behaviour

The logic matches the original exactly, as before: memory writes, call order
and arguments, results. Integer widths, signedness, wrap-around and the
single-precision float rules (`(f32)(double op)`, `fmadds` = `(f32)fma`)
still apply.

What is not reproduced any more is register state that no caller uses:
leftover values in r3/f1 of void functions, junk in argument registers a
callee doesn't read, and the OPD/entry registers of virtual calls. Where a
caller that is still recompiled does read a register a function leaves behind
(see "Known dependencies" in DECOMP.md), set it before returning with
`ctx().gpr[n] = ...` and a comment; that goes away once the caller is native.

## Converting an existing DECOMP function

1. Describe the structures it touches (offsets are in the existing comments
   and `field<T>(x, off)` calls) in the area's header.
2. Give it its real signature and turn `arg<T>(ctx, n)` into parameters,
   `ret(ctx, v)` into `return v`, `field<T>(o, off)` into members,
   `call(ctx, addr, {...})` into a direct call to the native callee or
   `call<R>(addr, ...)`.
3. Drop code that only rebuilt leftover registers, unless a dependency above
   needs it.
4. Replace `DECOMP(ADDR, Name) {` with the native function and a
   `NATIVE(ADDR, ns::Name)` line.

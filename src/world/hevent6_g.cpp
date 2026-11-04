// World_HandleEvent callees, part 6: the effect component lookup (203
// instructions of inlined map searches).
#include "hevent6.h"

using namespace sly4;
using namespace sly4::hevent;
using namespace sly4::hevent6;

namespace sly4 {
namespace hevent6g {
// Component registries keyed by type hash: the def's (+4 def, +12 registry,
// map at +12) and the global one (0x010C2750, map at +12); the effect's own
// components are the map at +20.
constexpr u32 kGlobalComponents = 0x010C2750 + 12;
constexpr u32 kRegistryMap = 12;
constexpr u32 kEffectMap = 20;
constexpr u32 kLastComponent = 0x011DE490;   // u32 value found by the last lookup (name?)
}  // namespace hevent6g
}  // namespace sly4

using namespace sly4::hevent6g;

// Component* Effect_FindComponent(Effect* e, u32 type) (name?): the type's
// entry in the effect def's component registry (+4 -> +12), else (when the
// def has no registry or no entry) in the global one (searched twice, as the
// inlined find + get); a non-zero entry is returned. Otherwise the effect's
// own component map (+20) decides; 0 when absent. Each search records its
// path / node and the value at 0x011DE490 as the map code does.
DECOMP(004BB4B4, Effect_FindComponent) {
    const u32 e = arg<u32>(ctx, 0), type = arg<u32>(ctx, 1);
    u32 found = 0;
    const u32 def = load<u32>(e + 4);
    if (def != 0) {
        bool global = true;
        const u32 reg = load<u32>(def + 12);
        if (reg != 0) {
            const u32 m = reg + kRegistryMap;
            const u32 root = load<u32>(load<u32>(m));
            if (root == 0) {
                store<u32>(m + kMapNode, 0);
                store<u32>(kLastComponent, 0);
            } else if (const u32 n = map_walk(m, root, type); n != 0) {
                store<u32>(m + kMapNode, n);
                store<u32>(kLastComponent, load<u32>(n + kNodeValue));
                const u32 reg2 = load<u32>(def + 12);
                if (reg2 != 0) {
                    const u32 cur = load<u32>(reg2 + kRegistryMap + kMapNode);
                    found = cur != 0 ? load<u32>(cur + kNodeValue) : 0;
                }
                global = false;
            }
        }
        if (global) {
            const u32 m = kGlobalComponents;
            const u32 root = load<u32>(load<u32>(m));
            if (root == 0) {
                store<u32>(kLastComponent, 0);
                store<u32>(m + kMapNode, 0);
            } else if (const u32 n = map_walk(m, root, type); n != 0) {
                store<u32>(m + kMapNode, n);
                store<u32>(kLastComponent, load<u32>(n + kNodeValue));
                const u32 root2 = load<u32>(load<u32>(m));
                if (root2 == 0) {
                    store<u32>(kLastComponent, 0);
                    store<u32>(m + kMapNode, 0);
                } else if (const u32 n2 = map_walk(m, root2, type); n2 != 0) {
                    store<u32>(m + kMapNode, n2);
                    store<u32>(kLastComponent, load<u32>(n2 + kNodeValue));
                    found = load<u32>(n2 + kNodeValue);
                }
            }
        }
        if (found != 0) {
            ret(ctx, found);
            return;
        }
    }
    const u32 m = e + kEffectMap;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + kMapNode, 0);
        store<u32>(kLastComponent, 0);
        ret(ctx, 0u);
        return;
    }
    const u32 n = map_walk(m, root, type);
    if (n == 0) {
        ret(ctx, 0u);
        return;
    }
    store<u32>(m + kMapNode, n);
    store<u32>(kLastComponent, load<u32>(n + kNodeValue));
    const u32 cur = load<u32>(m + kMapNode);
    ret(ctx, cur != 0 ? load<u32>(cur + kNodeValue) : 0u);
}

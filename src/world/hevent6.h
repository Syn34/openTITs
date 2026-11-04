// Shared helpers for src/world/hevent6_*.cpp: the engine's inlined
// binary-tree map and the effect set lookup built on it.
#pragma once

#include "hevent.h"

namespace sly4 {
namespace hevent6 {

constexpr u32 kSetDefs = 16;             // Set +16: map of effect defs by name hash
constexpr u32 kLastDef = 0x011DE288;     // u32 def found by the last lookup (as in hevent5_c)

// The engine's map (inlined everywhere): a binary tree with an iterator.
//   +0 Node** root, +4 Node* path[20] (unchecked), +84 u16 path length,
//   +88 Node* current.  Node: +4 u32 key, +8 value, +12 left (smaller keys),
//   +16 right.
constexpr u32 kMapPath = 4, kMapDepth = 84, kMapNode = 88;
constexpr u32 kNodeKey = 4, kNodeValue = 8, kNodeLeft = 12, kNodeRight = 16;

// Pushes node on the map's path: length store first.
inline void map_push(u32 m, u32 node) {
    const s32 n = static_cast<s16>(load<u16>(m + kMapDepth));
    store<u16>(m + kMapDepth, static_cast<u16>(n + 1));
    store<u32>(m + kMapPath + static_cast<u32>(n) * 4, node);
}

// Binary search for key from root (non-zero), recording the path. Returns the
// node, or 0 after storing the last node visited as the current one.
inline u32 map_walk(u32 m, u32 root, u32 key) {
    store<u16>(m + kMapDepth, 0);
    u32 n = root;
    for (;;) {
        const u32 k = load<u32>(n + kNodeKey);
        if (k == key) return n;
        const u32 next = k > key ? load<u32>(n + kNodeLeft) : load<u32>(n + kNodeRight);
        if (next == 0) {
            store<u32>(m + kMapNode, n);
            return 0;
        }
        map_push(m, n);
        n = next;
    }
}

// Iterator begin: the leftmost node (or 0), stored as the current one.
inline u32 map_first(u32 m) {
    const u32 rootp = load<u32>(m);
    store<u16>(m + kMapDepth, 0);
    u32 n = load<u32>(rootp);
    if (n != 0) {
        for (u32 l; (l = load<u32>(n + kNodeLeft)) != 0; n = l) map_push(m, n);
    }
    store<u32>(m + kMapNode, n);
    return n;
}

// Iterator advance from node (the current one): the in-order successor, or 0.
inline u32 map_next(u32 m, u32 node) {
    u32 r = load<u32>(node + kNodeRight);
    if (r != 0) {
        map_push(m, node);
        while (load<u32>(r + kNodeLeft) != 0) {
            // This descent stores the path entry before the length.
            const s32 n = static_cast<s16>(load<u16>(m + kMapDepth));
            store<u32>(m + kMapPath + static_cast<u32>(n) * 4, r);
            r = load<u32>(r + kNodeLeft);
            store<u16>(m + kMapDepth, static_cast<u16>(n + 1));
        }
        store<u32>(m + kMapNode, r);
        return r;
    }
    u32 depth = load<u16>(m + kMapDepth);
    u32 child = node;
    for (;;) {
        if ((depth & 0xFFFF) == 0) {
            store<u32>(m + kMapNode, 0);
            return 0;
        }
        depth -= 1;
        store<u16>(m + kMapDepth, static_cast<u16>(depth));
        const u32 parent = load<u32>(m + kMapPath + static_cast<u32>(static_cast<s32>(static_cast<s16>(depth))) * 4);
        const bool from_left = load<u32>(parent + kNodeLeft) == child;
        store<u32>(m + kMapNode, parent);
        if (from_left) return parent;
        child = parent;
    }
}

// Looks hash up in a set's map: node at set +104 and its def at 0x011DE288
// (both 0 for an empty set; a miss leaves the last node visited and the
// global as they were). Returns the def or 0.
inline u32 set_lookup(u32 set, u32 hash) {
    const u32 m = set + kSetDefs;
    const u32 root = load<u32>(load<u32>(m));
    if (root == 0) {
        store<u32>(m + kMapNode, 0);
        store<u32>(kLastDef, 0);
        return 0;
    }
    const u32 n = map_walk(m, root, hash);
    if (n == 0) return 0;
    store<u32>(m + kMapNode, n);
    store<u32>(kLastDef, load<u32>(n + kNodeValue));
    const u32 cur = load<u32>(m + kMapNode);
    return cur != 0 ? load<u32>(cur + kNodeValue) : 0;
}
}  // namespace hevent6
}  // namespace sly4

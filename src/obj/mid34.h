// Game code, medium sweep part 34: helpers shared by mid34_*.cpp (more
// still-recompiled game functions of 120-400 instructions).
#pragma once

#include "native/objtypes.h"

namespace sly4::obj {

// ---------------------------------------------------------------- AVL

// The rebalance every AVL insert / delete-max instance inlines after its
// recursion or its leaf work (only when the tree's +8 flag is set): the
// node's height / balance fixed (AvlFix), a left rotation when the balance
// is > 1 and it has a right child (a right rotation of that child first
// when its balance is < 0), then a right rotation when the balance is < -1
// and it has a left child (a left rotation of that child first when its
// balance is > 0). `chk2`: *pp is re-checked for null before the second
// test (without it a null *pp is read at address 0, as the original does).
// rotl / rotr: Avl_Rotate(tree, Node** pp) of the instance.
template <typename N>
void AvlRebal34(AvlTree* t, gp<N>* pp, u32 rotl, u32 rotr, bool chk2) {
    if (t->balanced == 0) return;
    N* n = *pp;
    if (n == nullptr) return;
    AvlFix(n);
    n = *pp;
    if (n == nullptr) return;
    if (n->balance > 1) {
        if (N* r = n->right) {
            if (r->balance < 0) call(rotr, t, &n->right);
            call(rotl, t, pp);
        }
    }
    if (chk2 && !*pp) return;
    N& m = gm<N>(pp->ea);  // quirk: address 0 when null (no chk2)
    if (m.balance >= -1) return;
    N* l = m.left;
    if (l == nullptr) return;
    if (l->balance > 0) call(rotl, t, &m.left);
    call(rotr, t, pp);
}

}  // namespace sly4::obj

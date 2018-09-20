#ifndef BST_H
#define BST_H

#include <avlbst.h>

void proctree(struct bst *, void (*)(struct bst_node *),
              void (*)(struct bst *, struct bst_node *));

#endif

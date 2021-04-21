#ifndef NODE_H
#define NODE_H

#include <vector>

#include "maths.h"

struct Object;

struct Node {
    std::vector<Node*> children;
    V3 rotation;
    V3 translation;
    V3 scale;

    float model[16];
    bool flip;

    ~Node();
};

extern Node *create_node();
extern Node *descend_node(Node *node, unsigned int depth);

#endif
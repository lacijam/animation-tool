#include "node.h"


#include "object.h"

Node::~Node() 
{

}

Node *create_node()
{
    Node *node = new Node;

    node->rotation.E[0] = node->rotation.E[1] = node->rotation.E[2] = 0;
    node->translation.E[0] = node->translation.E[1] = node->translation.E[2] = 0;
    node->scale.E[0] = node->scale.E[1] = node->scale.E[2] = 0;
    node->flip = false;

    return node;
}

Node *descend_node(Node *node, unsigned int depth)
{
    if (depth == 0) {
        return node;
    }

    return descend_node(node->children[0], depth - 1);
}
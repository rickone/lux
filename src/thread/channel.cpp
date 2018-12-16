#include "channel.h"
#include <cstdlib> // malloc

ChanNode* ChanNode::create(size_t len) {
    ChanNode* node = (ChanNode*)malloc(sizeof(ChanNode) + len);
    node->next = nullptr;
    node->len = len;
    return node;
}

Channel::Channel() {
    _head = ChanNode::create(0);
    _tail = _head;
}

Channel::~Channel() {
    for (ChanNode* node = _head; node != nullptr; ) {
        ChanNode* next = node->next;
        free(node);
        node = next;
    }
    _head = nullptr;
    _tail = nullptr;
}

void Channel::push(ChanNode* new_node) {
    new_node->next = nullptr;
    ChanNode* old_node = nullptr;
    ChanNode* node = _tail.load();
    while (!node->next.compare_exchange_weak(old_node, new_node)) {
        node = old_node;
        old_node = nullptr;
    }
    _tail = new_node;
}

ChanNode* Channel::pop() {
    ChanNode* node = _head->next.load();
    while (node && !_head->next.compare_exchange_weak(node, node->next));
    return node;
}

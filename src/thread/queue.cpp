#include "queue.h"
#include <cstdlib> // malloc
#include <cstring> // memcpy

using namespace lux;

static Queue::Node* create_node(const char* data, size_t len) {
    Queue::Node* node = (Queue::Node*)malloc(sizeof(Queue::Node) + len);
    node->next = nullptr;
    node->len = len;
    if (data)
        memcpy(node->data, data, len);
    return node;
}

Queue::Queue() {
    auto dummy = create_node(nullptr, 0);
    _head = dummy;
    _tail = dummy;
}

Queue::~Queue() {
    // lock?
    for (Node* node = _head; node != nullptr; ) {
        Node* next = node->next;
        free(node);
        node = next;
    }
    _head = nullptr;
    _tail = nullptr;
}

void Queue::push(const char* data, size_t len) {
    Node* new_node = create_node(data, len);
    Node* node = _tail.load(std::memory_order_consume);

    while (!_tail.compare_exchange_weak(node, new_node, std::memory_order_release, std::memory_order_consume));

    node->next = new_node;
}

Queue::Node* Queue::pop() {
    Node* node = _head.load(std::memory_order_consume);
    Node* next = nullptr;
    do {
        next = node->next;
        if (next == nullptr)
            return nullptr;

    } while (!_head.compare_exchange_weak(node, next, std::memory_order_release, std::memory_order_consume));

    free(node);
    return next;
}

#pragma once

#include <atomic>

namespace lux {

class Queue {
public:
    struct Node {
        Node* next;
        size_t len;
        char data[0];
    };

    Queue();
    virtual ~Queue();

    void push(const char* data, size_t len);
    Node* pop();

private:
    std::atomic<Node*> _head;
    std::atomic<Node*> _tail;
};

} // lux

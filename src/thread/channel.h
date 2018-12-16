#pragma once

#include <atomic>

struct ChanNode {
    std::atomic<ChanNode*> next;
    size_t len;
    char data[0];

    static ChanNode* create(size_t len);
};

class Channel {
public:
    Channel();
    virtual ~Channel();

    void push(ChanNode* node);
    ChanNode* pop();

private:
    ChanNode* _head;
    std::atomic<ChanNode*> _tail;
};

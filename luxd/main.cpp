#include "lux_core.h"
#include "queue.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace lux;

void producer(Queue* q, int start, int num) {
    for (int i = 0; i < num; ++i) {
        int n = start + i;
        q->push((const char*)&n, sizeof(n));
    }
}

void consumer(Queue* q, std::atomic<bool>* notify_stop) {
    while (!notify_stop->load(std::memory_order_acquire)) {
        auto node = q->pop();
        if (node == nullptr)
            continue;

        printf("consume: %06d\n", *(int*)node->data);
    }
}

int main(int argc, char* argv[]) {
    /*
    const int producer_num = 20;
    const int consumer_num = 20;

    Queue q;
    std::atomic<bool> notify_stop(false);

    std::vector<std::thread> t1;
    for (int i = 0; i < producer_num; ++i)
        t1.emplace_back(producer, &q, 1 + i * 10000, 10000);

    std::vector<std::thread> t2;
    for (int i = 0; i < consumer_num; ++i)
        t2.emplace_back(consumer, &q, &notify_stop);

    for (int i = 0; i < producer_num; ++i)
        t1[i].join();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    notify_stop.store(true, std::memory_order_release);

    for (int i = 0; i < consumer_num; ++i)
        t2[i].join();

    return 0;
    */
    return lux::Core::main(argc, argv);
}

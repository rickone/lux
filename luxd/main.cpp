#include "lux_core.h"
#include "channel.h"
#include <iostream>
#include <thread>
#include <vector>

void producer(Channel* chan, int start, int num) {
    for (int i = 0; i < num; ++i) {
        int n = start + i;
        auto node = ChanNode::create(sizeof(n));
        memcpy(node->data, &n, sizeof(n));
        chan->push(node);
    }
}

void consumer(Channel* chan, std::atomic<bool>* notify_stop) {
    while (!notify_stop->load(std::memory_order_acquire)) {
        auto node = chan->pop();
        if (node == nullptr)
            continue;

        printf("consume: %d\n", *(int*)node->data);
        free(node);
    }
}

int main(int argc, char* argv[]) {
    /*
    const int producer_num = 10;
    const int consumer_num = 4;

    Channel chan;
    std::atomic<bool> notify_stop(false);

    std::vector<std::thread> t1;
    for (int i = 0; i < producer_num; ++i)
        t1.emplace_back(producer, &chan, i * 100, 100);

    std::vector<std::thread> t2;
    for (int i = 0; i < consumer_num; ++i)
        t2.emplace_back(consumer, &chan, &notify_stop);

    for (int i = 0; i < producer_num; ++i)
        t1[i].join();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    notify_stop.store(true, std::memory_order_release);

    for (int i = 0; i < consumer_num; ++i)
        t2[i].join();
    */
    return lux::Core::main(argc, argv);
    //return 0;
}

#ifndef ThreadPool_H
#define ThreadPool_H

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <condition_variable>

class ThreadPool {

    unsigned threadCount;
    std::vector<std::thread> threads;
    std::list<std::function<void(void)>> queue;

    std::atomic_bool stop;
    std::condition_variable wait_var;
    std::mutex queue_mutex;

    void Run() {
        while(!stop) {
            std::function<void(void)> run;
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (!queue.empty()) {
                run = queue.front();
                queue.pop_front();
                lock.unlock();
                run();
            }
            wait_var.notify_one();
        }
    }

public:
    ThreadPool(int c)
        : threadCount(c)
        , threads(threadCount)
        , stop(false)
    {
        for (unsigned i = 0; i < threadCount; ++i)
            threads[i] = std::move(std::thread([this]{ this->Run(); }));
    }

    ~ThreadPool() {
        JoinAll();
    }

    void AddTask(std::function<void(void)> job) {
        if (!stop) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue.emplace_back(job);
            wait_var.notify_one();
        }
    }

    void JoinAll() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        wait_var.wait(lock, [this]()->bool { return queue.empty(); });
        stop = true;
        lock.unlock();
        std::for_each(threads.begin(), threads.end(), [](auto &t){ if(t.joinable()) t.join(); });
    }

};

#endif /* ThreadPool_H */
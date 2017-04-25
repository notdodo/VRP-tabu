#ifndef ThreadPool_H
#define ThreadPool_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>

class ThreadPool {

private:
    // number of thread
    unsigned threadCount;
    std::list<std::thread> threads;
    // where tasks are storage
    std::list<std::function<void(void)> > queue;

    std::atomic_bool stop;
    std::condition_variable wait_var;
    std::mutex queue_mutex;

    // Body of every running thread.
    // The thread will run until deconstructor or `JoinAll` are called
    // and it fetch the top of the queue to find a task to exec.
    void Run() {
        while (!stop) {
            std::function<void(void)> run;
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (!queue.empty()) {
                run = queue.front();
                queue.pop_front();
                lock.unlock();
                // unlock befor `run` to ensure parallelism
                run();
            }
            // awake some sleeping threads
            wait_var.notify_one();
        }
    }

public:
    // Constructor
    ThreadPool(int c)
        : threadCount(c)
        , threads(threadCount)
        , stop(false)
    {
        // create the threads
        for (std::thread& t : threads)
            t = std::move(std::thread([this] { this->Run(); }));
    }

    // Deconstructor
    ~ThreadPool() {
        JoinAll();
    }

    // Add a task to queue
    // The function will add, at the end of the queue, a `void`
    // function only if no one is waiting for stop.
    void AddTask(std::function<void(void)> job) {
        if (!stop) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            queue.emplace_back(job);
            wait_var.notify_one();
        }
    }

    // Wait until all tasks ended.
    // If the queue is not empty wait the end of all tasks inserted
    // and terminate the threads.
    void JoinAll() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        wait_var.wait(lock, [this]() -> bool { return queue.empty(); });
        stop = true;
        lock.unlock();
        for (std::thread& t : threads) {
            if (t.joinable())
                t.join();
        }
    }
};

#endif /* ThreadPool_H */

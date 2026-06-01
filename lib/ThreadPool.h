#ifndef ThreadPool_H
#define ThreadPool_H

#include <condition_variable>
#include <exception>
#include <functional>
#include <list>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

/** @brief Small fixed-size thread pool for parallel route-neighborhood evaluation.
 *
 * Tasks are fire-and-forget closures. JoinAll waits until both queued and
 * active tasks finish, then shuts down workers.
 */
class ThreadPool {

  private:
    /** @brief Number of worker threads. */
    unsigned threadCount;
    std::list<std::thread> threads;

    /** @brief FIFO queue of submitted tasks. */
    std::list<std::function<void(void)>> queue;

    bool stop = false;
    unsigned activeTasks = 0;
    std::condition_variable wait_var;
    std::condition_variable complete_var;
    std::mutex queue_mutex;
    std::mutex join_mutex;

    static const ThreadPool*& CurrentPool() {
        static thread_local const ThreadPool* pool = nullptr;
        return pool;
    }

    /** @brief Worker loop used by every thread in the pool.
     *
     * Workers sleep until a task is available or shutdown starts. Active tasks
     * are counted separately from queued tasks so JoinAll can wait for both the
     * queue and currently executing work to finish before joining threads.
     */
    void Run() {
        const ThreadPool* previousPool = CurrentPool();
        CurrentPool() = this;

        while (true) {
            std::function<void(void)> run;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                wait_var.wait(lock, [this]() -> bool { return stop || !queue.empty(); });
                if (stop && queue.empty()) {
                    CurrentPool() = previousPool;
                    return;
                }
                run = std::move(queue.front());
                queue.pop_front();
                ++activeTasks;
            }
            // unlock before `run` to ensure parallelism
            try {
                run();
            } catch (...) {
                // keep the worker alive if a task wrapper throws
                (void)0;
            }
            {
                std::scoped_lock lock(queue_mutex);
                --activeTasks;
                if (queue.empty() && activeTasks == 0) {
                    complete_var.notify_all();
                }
            }
        }
    }

  public:
    /** @brief Start a fixed-size pool of worker threads.
     *
     * @param c Number of worker threads to create. A zero value is normalized
     * to one worker so callers can pass std::thread::hardware_concurrency()
     * safely on platforms where it returns zero.
     */
    explicit ThreadPool(unsigned c) : threadCount(c == 0 ? 1 : c), threads(threadCount) {
        // create the threads
        for (std::thread& t : threads)
            t = std::thread([this] { this->Run(); });
    }

    /** @brief Stop the pool after all queued work has completed. */
    ~ThreadPool() noexcept {
        try {
            JoinAll();
        } catch (...) {
            std::terminate();
        }
    }

    /** @brief Queue a task for asynchronous execution.
     *
     * Tasks submitted after shutdown starts are ignored. This keeps destructor
     * cleanup idempotent and avoids racing new work against JoinAll.
     */
    void AddTask(std::function<void(void)> job) {
        {
            std::scoped_lock lock(queue_mutex);
            if (stop) {
                return;
            }
            queue.emplace_back(std::move(job));
        }
        wait_var.notify_one();
    }

    /** @brief Wait for all queued and active tasks, then join workers.
     *
     * This method is safe to call explicitly and is also called by the
     * destructor. Once it starts, no additional work is accepted.
     */
    void JoinAll() {
        if (CurrentPool() == this) {
            throw std::logic_error("ThreadPool::JoinAll cannot be called from a worker thread");
        }

        std::scoped_lock join_lock(join_mutex);
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        wait_var.notify_all();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            complete_var.wait(lock, [this]() -> bool { return queue.empty() && activeTasks == 0; });
        }

        for (std::thread& t : threads) {
            if (t.joinable())
                t.join();
        }
    }
};

#endif /* ThreadPool_H */

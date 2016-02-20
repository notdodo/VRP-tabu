#ifndef CONCURRENT_THREADPOOL_H
#define CONCURRENT_THREADPOOL_H

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <condition_variable>

/**
 *  Simple ThreadPool that creates `threadCount` threads and pulls from a queue to get new jobs.
 *
 *  This class requires a number of c++11 features be present in your compiler.
 */
class ThreadPool {

    unsigned threadCount;
    std::vector<std::thread> threads;
    std::list<std::function<void(void)> > queue;

    std::atomic_int jobs_left;
    std::atomic_bool bailout;
    std::atomic_bool finished;
    std::condition_variable job_available_var;
    std::condition_variable wait_var;
    std::mutex wait_mutex;
    std::mutex queue_mutex;
    std::mutex mtx;

    /** @brief ###Take the next job in the queue and run it.
     *  Notify the main thread that a job has completed.
     */
    void Task() {
        while (!bailout) {
            next_job()();
            --jobs_left;
            wait_var.notify_one();
        }
    }

    /** @brief ###Get the next job.
     *
     *  Pop the first item in the queue,
     *  otherwise wait for a signal from the main thread.
     *  @return The lambda function to execute
     */
    std::function<void(void)> next_job() {
        std::function<void(void)> res;
        std::unique_lock<std::mutex> job_lock(queue_mutex);

        // Wait for a job if we don't have any.
        job_available_var.wait(job_lock, [this]()->bool { return queue.size() || bailout; });

        // Get job from the queue
        if (!bailout) {
            res = queue.front();
            queue.pop_front();
        }
        else {
            // If we're bailing out, 'inject' a job into the queue to keep jobs_left accurate.
            res = [] {};
            ++jobs_left;
        }
        return res;
    }

public:
    ThreadPool(int c)
        : threadCount(c)
        , threads(threadCount)
        , jobs_left(0)
        , bailout(false)
        , finished(false)
    {
        for (unsigned i = 0; i < threadCount; ++i)
            threads[i] = std::move(std::thread([this, i] { this->Task(); }));
    }

    /** @brief ###JoinAll on deconstruction */
    ~ThreadPool() {
        JoinAll();
    }

    /** @brief ###Add a new job to the pool.
     *  If there are no jobs in the queue,
     *  a thread is woken up to take the job. If all threads are busy,
     *  the job is added to the end of the queue.
     */
    void AddJob(std::function<void(void)> job) {
        std::lock_guard<std::mutex> guard(queue_mutex);
        queue.emplace_back(job);
        ++jobs_left;
        job_available_var.notify_one();
    }

    /** @brief ###Join with all threads.
     *
     *  Block until all threads have completed.
     *  The queue will be empty after this call, and the threads will
     *  be done. After invoking `ThreadPool::JoinAll`, the pool can no
     *  longer be used. If you need the pool to exist past completion
     *  of jobs, look to use `ThreadPool::WaitAll`.
     *  @param[in] WaitForAll If true, will wait for the queue to empty
     *                        before joining with threads. If false, will complete
     *                        current jobs, then inform the threads to exit.
     */
    void JoinAll(bool WaitForAll = true) {
        if (!finished) {
            if (WaitForAll) {
                WaitAll();
            }

            // note that we're done, and wake up any thread that's
            // waiting for a new job
            bailout = true;
            job_available_var.notify_all();

            for (auto& x : threads)
                if (x.joinable())
                    x.join();
            finished = true;
        }
    }

    /** @brief ###Wait for the pool to empty before continuing.
     *
     *  This does not call `std::thread::join`, it only waits until
     *  all jobs have finshed executing.
     */
    void WaitAll() {
        std::unique_lock<std::mutex> lk(wait_mutex);
        if (jobs_left > 0) {
            wait_var.wait(lk, [this] { return this->jobs_left == 0; });
        }
        lk.unlock();
    }
};

#endif //CONCURRENT_THREADPOOL_H
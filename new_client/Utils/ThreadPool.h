#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace SkyRAT {
namespace Utils {

    /**
     * @brief Thread pool implementation for asynchronous task execution
     * 
     * This class provides a pool of worker threads that can execute tasks
     * asynchronously. It's useful for handling multiple operations without
     * blocking the main thread.
     */
    class ThreadPool {
    public:
        /**
         * @brief Constructor
         * @param numThreads Number of worker threads to create
         */
        explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());

        /**
         * @brief Destructor - stops all threads and waits for completion
         */
        ~ThreadPool();

        // Non-copyable and non-movable
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        /**
         * @brief Enqueue a task for execution
         * @param f Function to execute
         * @param args Arguments to pass to function
         * @return Future that will contain the result
         */
        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) 
            -> std::future<typename std::result_of<F(Args...)>::type>;

        /**
         * @brief Get number of worker threads
         * @return Number of threads in the pool
         */
        size_t getThreadCount() const;

        /**
         * @brief Get number of pending tasks
         * @return Number of tasks waiting to be executed
         */
        size_t getPendingTaskCount() const;

        /**
         * @brief Check if thread pool is running
         * @return true if running, false if stopped
         */
        bool isRunning() const;

        /**
         * @brief Stop the thread pool and wait for all tasks to complete
         */
        void stop();

        /**
         * @brief Stop the thread pool immediately (may interrupt running tasks)
         */
        void stopNow();

    private:
        // Need to keep track of threads so we can join them
        std::vector<std::thread> m_workers;
        
        // The task queue
        std::queue<std::function<void()>> m_tasks;
        
        // Synchronization
        mutable std::mutex m_queueMutex;
        std::condition_variable m_condition;
        
        // Status
        bool m_stop;
        bool m_running;
        
        void workerThread();
    };

    // Template implementation must be in header
    template<class F, class... Args>
    auto ThreadPool::enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            // Don't allow enqueueing after stopping the pool
            if (m_stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            m_tasks.emplace([task](){ (*task)(); });
        }
        
        m_condition.notify_one();
        return result;
    }

} // namespace Utils
} // namespace SkyRAT

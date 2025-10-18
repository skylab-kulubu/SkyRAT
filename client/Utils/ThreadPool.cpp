#include "ThreadPool.h"
#include <iostream>

namespace SkyRAT {
namespace Utils {

ThreadPool::ThreadPool(size_t numThreads) 
    : m_stop(false), m_running(true) {
    
    if (numThreads == 0) {
        numThreads = 1; // Ensure at least one thread
    }
    
    std::cout << "[ThreadPool] Starting " << numThreads << " worker threads" << std::endl;
    
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::workerThread() {
    for (;;) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
            
            if (m_stop && m_tasks.empty()) {
                return;
            }

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "[ThreadPool] Exception in worker thread: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ThreadPool] Unknown exception in worker thread" << std::endl;
        }
    }
}

size_t ThreadPool::getThreadCount() const {
    return m_workers.size();
}

size_t ThreadPool::getPendingTaskCount() const {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    return m_tasks.size();
}

bool ThreadPool::isRunning() const {
    return m_running && !m_stop;
}

void ThreadPool::stop() {
    if (!m_running) {
        return;
    }
    
    std::cout << "[ThreadPool] Stopping thread pool..." << std::endl;
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    
    m_condition.notify_all();
    
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    m_running = false;
    std::cout << "[ThreadPool] Thread pool stopped" << std::endl;
}

void ThreadPool::stopNow() {
    if (!m_running) {
        return;
    }
    
    std::cout << "[ThreadPool] Force stopping thread pool..." << std::endl;
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
        // Clear pending tasks
        std::queue<std::function<void()>> empty;
        m_tasks.swap(empty);
    }
    
    m_condition.notify_all();
    
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    m_running = false;
    std::cout << "[ThreadPool] Thread pool force stopped" << std::endl;
}

} // namespace Utils
} // namespace SkyRAT

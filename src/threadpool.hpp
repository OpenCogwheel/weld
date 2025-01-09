#pragma once

#include <vector>
#include <future>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t num_threads)
        : m_ActiveThreads(0) {
        for (size_t i = 0; i < num_threads; ++i) {
            m_Workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(m_QueueMutex);
                        m_Condition.wait(lock, [this]() { return !m_Tasks.empty() || m_Stop; });

                        if (m_Stop && m_Tasks.empty()) return;

                        task = std::move(m_Tasks.front());
                        m_Tasks.pop();
                    }
                    
                    {
                        std::lock_guard<std::mutex> lock(m_ActiveThreadsMutex);
                        ++m_ActiveThreads;
                    }
                    
                    task();
                    
                    {
                        std::lock_guard<std::mutex> lock(m_ActiveThreadsMutex);
                        --m_ActiveThreads;
                    }

                    m_Condition.notify_all();
                }
            });
        }
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_Stop = true;
        }
        
        m_Condition.notify_all();
        
        for (std::thread& worker : m_Workers) {
            worker.join();
        }
    }

    template <class F, class... Args>
    std::future<void> enqueue(F&& f, Args&&... args) {
        auto task = std::make_shared<std::packaged_task<void()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<void> res = task->get_future();
        
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_Tasks.push([task]() {
                (*task)();
            });
        }
        
        m_Condition.notify_one();

        return res;
    }

    void get() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Condition.wait(lock, [this]() {
            return m_Tasks.empty() && m_ActiveThreads == 0;
        });
    }
private:
    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    bool m_Stop = false;

    std::atomic<int> m_ActiveThreads;
    std::mutex m_ActiveThreadsMutex;
};

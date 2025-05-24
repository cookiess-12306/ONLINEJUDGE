#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace ns_thread_pool
{
    class ThreadPool
    {
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

    public:
        ThreadPool(size_t threads) : stop(false)
        {
            for (size_t i = 0; i < threads; ++i)
                workers.emplace_back([this]
                                     {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                } });
        }

        void enqueue(std::function<void()> task)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace(std::move(task));
            }
            condition.notify_one();
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (std::thread &worker : workers)
                worker.join();
        }
    };
}

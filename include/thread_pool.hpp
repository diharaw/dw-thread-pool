#pragma once

#include "concurrent_queue.hpp"
#include "delegate.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>

#define TASK_SIZE_BYTES 48

namespace dw
{
    using WorkerFunction = Delegate<void(void*)>;
    
    struct Task
    {
        WorkerFunction _function;
        char						 _data[TASK_SIZE_BYTES];
    };
    
    template <typename T>
    inline T* task_data(Task& task)
    {
        return static_cast<T*>(&task._data[0]);
    }
    
    template <size_t WorkerCount = 4, size_t QueueSize = 1024>
    class ThreadPool
    {
        using TaskQueue = ConcurrentQueue<Task, QueueSize>;
        
    public:
        ThreadPool()
        {
            _shutdown = false;
            _pending_tasks = 0;
            
            // clear queue
            concurrent_queue::clear(_queue);
            
            // get number of logical threads on CPU
            _num_logical_threads = std::thread::hardware_concurrency();
            
            // spawn worker threads
            for (int i = 0; i < WorkerCount; i++)
                _worker_threads.push_back(std::thread(&ThreadPool::worker_function, this, i));
        }
        
        ~ThreadPool()
        {
            _mutex.lock();
            _shutdown = true;
            _condition.notify_all();
            _mutex.unlock();
            
            for (auto& thread : _worker_threads)
                thread.join();
        }
        
        inline void enqueue(Task& task)
        {
            concurrent_queue::push(_queue, task);
            _pending_tasks++;
            _condition.notify_one();
        }
        
        inline void wait()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condition.wait(lock, [this]
                            {
                                return (_pending_tasks == 0);
                            });
        }
        
        inline size_t get_num_logical_threads()
        {
            return _num_logical_threads;
        }
        
    private:
        inline void worker_function(int index)
        {
            while (true)
            {
                if (concurrent_queue::empty(_queue))
                {
                    // lock thread
                    std::unique_lock<std::mutex> lock(_mutex);
                    //
                    _condition.wait(lock, [this]
                                    {
                                        return (!concurrent_queue::empty(_queue) || _shutdown) ;
                                    });
                    
                    if (_shutdown)
                        return;
                }
                else
                {
                    // pop front item
                    Task task = concurrent_queue::pop(_queue);
                    
                    // execute task
                    task._function.Invoke(&task._data);
                    _pending_tasks--;
                    
                    _condition.notify_one();
                }
            }
        }
        
    private:
        bool									  _shutdown;
        uint16_t							  _num_logical_threads;
        TaskQueue						  _queue;
        std::vector<std::thread> _worker_threads;
        std::mutex						  _mutex;
        std::condition_variable   _condition;
        std::atomic_int				  _pending_tasks;
        
    };
}

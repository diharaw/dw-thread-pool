#pragma once

#include "delegate.hpp"
#include "semaphore.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>

#define MAX_TASKS 1024u
#define MASK MAX_TASKS - 1u

#define TASK_SIZE_BYTES 128

namespace dw
{
    using WorkerFunction = Delegate<void(void*)>;
    
    struct Task
    {
        WorkerFunction _function;
        char           _data[TASK_SIZE_BYTES];
        uint16_t       _num_completed;
    };
    
    template <typename T>
    inline T* task_data(Task& task)
    {
        return (T*)(&task._data[0]);
    }
    
    class WorkQueue
    {
    private:
        std::mutex _critical_section;
        Task       _task_pool[MAX_TASKS];
        Task*      _task_queue[MAX_TASKS];
        uint32_t   _front;
        uint32_t   _back;
        uint32_t   _num_tasks;
        uint32_t _num_pending_tasks;
        
    public:
        WorkQueue()
        {
            _num_tasks = 0;
            _num_pending_tasks = 0;
            _front = 0;
            _back = 0;
        }
        
        ~WorkQueue()
        {
            
        }
        
        Task* allocate()
        {
            uint32_t task_index = _num_tasks++;
            return &_task_pool[task_index & (MAX_TASKS - 1u)];
        }
        
        void push(Task* task)
        {
            std::lock_guard<std::mutex> lock(_critical_section);
            
            _task_queue[_back & MASK] = task;
            ++_back;
            _num_pending_tasks++;
        }
        
        Task* pop()
        {
            std::lock_guard<std::mutex> lock(_critical_section);
            
            const uint32_t job_count = _back - _front;
            if (job_count <= 0)
                return nullptr;
            
            --_back;
            return _task_queue[_back & MASK];
        }
        
        void finish_task(Task* task)
        {
            task->_function.Invoke(task->_data);
            
            if(task->_num_completed > 0)
                task->_num_completed--;
            
            if(_num_pending_tasks > 0)
                _num_pending_tasks--;
        }
        
        bool has_pending_tasks()
        {
            return (_num_pending_tasks != 0);
        }
        
        bool empty()
        {
            return (_front == 0 && _back == 0);
        }
    };
    
    class WorkerThread
    {
    private:
        Semaphore _wakeup;
        Semaphore _done;
        std::thread _thread;
        bool      _shutdown;
        
    public:
        WorkerThread()
        {
            _shutdown = false;
        }
        
        ~WorkerThread()
        {
        }
        
        void initialize(WorkQueue* queue)
        {
            _thread = std::thread(&WorkerThread::worker, this, queue);
        }
        
        void shutdown()
        {
            _shutdown = true;
            wakeup();
            _thread.join();
        }
        
        void wakeup()
        {
            _wakeup.notify();
        }
        
    private:
        void worker(WorkQueue* queue)
        {
            while(!_shutdown)
            {
                Task* task = queue->pop();
                
                if(!task)
                {
                    _done.notify();
                    _wakeup.wait();
                }
                else
                    queue->finish_task(task);
            }
        }
    };
    
    class ThreadPool
    {
    public:
        ThreadPool()
        {
            _shutdown = false;
            
            // get number of logical threads on CPU
            _num_logical_threads = std::thread::hardware_concurrency();
            
            _num_worker_threads = _num_logical_threads - 1;
            
            initialize();
        }
        
        ThreadPool(uint32_t workers)
        {
            _shutdown = false;
            
            // get number of logical threads on CPU
            _num_logical_threads = std::thread::hardware_concurrency();
            _num_worker_threads = workers;
            
            initialize();
        }
        
        ~ThreadPool()
        {
            for (uint32_t i = 0; i < _num_worker_threads; i++)
                _worker_threads[i].shutdown();
            
            delete[] _worker_threads;
        }
        
        inline Task* allocate()
        {
            Task* task_ptr = _queue.allocate();
            task_ptr->_num_completed = 1;
            return task_ptr;
        }
        
        inline void enqueue(Task* task)
        {
            if(task)
            {
                _queue.push(task);
                
                for(uint32_t i = 0; i < _num_worker_threads; i++)
                {
                    WorkerThread& thread = _worker_threads[i];
                    thread.wakeup();
                }
            }
        }
        
        inline void wait_for_all()
        {
            while(_queue.has_pending_tasks())
            {
                Task* task = _queue.pop();
                
                if(task)
                    _queue.finish_task(task);
            }
        }
        
        inline void wait_for_one(Task* pending_task)
        {
            while(pending_task->_num_completed > 0)
            {
                Task* task = _queue.pop();
                
                if(task)
                    _queue.finish_task(task);
            }
        }
        
        inline uint32_t get_num_logical_threads()
        {
            return _num_logical_threads;
        }
        
        inline uint32_t get_num_worker_threads()
        {
            return _num_worker_threads;
        }
        
    private:
        void initialize()
        {
            // spawn worker threads
            _worker_threads = new WorkerThread[_num_worker_threads];
            
            for (uint32_t i = 0; i < _num_worker_threads; i++)
                _worker_threads[i].initialize(&_queue);
        }
        
    private:
        bool					  _shutdown;
        uint32_t				  _num_logical_threads;
        WorkQueue                 _queue;
        WorkerThread*             _worker_threads;
        uint32_t                  _num_worker_threads;
    };

}

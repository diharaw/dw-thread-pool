#pragma once

#include "delegate.hpp"
#include "semaphore.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>

#define MAX_TASKS 1024u
#define MAX_CONTINUATIONS 16u
#define MASK MAX_TASKS - 1u
#define TASK_SIZE_BYTES 128

namespace dw
{
    using WorkerFunction = Delegate<void(void*)>;
    
    struct Task
    {
        WorkerFunction		  _function;
        char				  _data[TASK_SIZE_BYTES];
        std::atomic<uint16_t> _num_pending;
		Task*				  _parent;
		std::atomic<uint16_t> _num_continuations;
		Task*				  _continuations[MAX_CONTINUATIONS];
    };
    
    template <typename T>
    inline T* task_data(Task& task)
    {
        return (T*)(&task._data[0]);
    }
    
    struct WorkQueue
    {
        std::mutex			  _critical_section;
        Task				  _task_pool[MAX_TASKS];
        Task*				  _task_queue[MAX_TASKS];
        uint32_t			  _front;
        uint32_t			  _back;
        uint32_t			  _num_tasks;
		std::atomic<uint32_t> _num_pending_tasks;
        
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

        bool has_pending_tasks()
        {
            return (_num_pending_tasks != 0);
        }
        
        bool empty()
        {
            return (_front == 0 && _back == 0);
        }
    };
    
    struct WorkerThread
    {
		Semaphore	_wakeup;
        Semaphore	_done;
        std::thread _thread;

        void shutdown()
        {
            wakeup();
            _thread.join();
        }
        
        void wakeup()
        {
            _wakeup.notify();
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
            
			initialize_workers();
        }
        
        ThreadPool(uint32_t workers)
        {
            _shutdown = false;
            
            // get number of logical threads on CPU
            _num_logical_threads = std::thread::hardware_concurrency();
            _num_worker_threads = workers;
            
			initialize_workers();
        }
        
        ~ThreadPool()
        {
			_shutdown = true;

            for (uint32_t i = 0; i < _num_worker_threads; i++)
                _worker_threads[i].shutdown();
            
            delete[] _worker_threads;
        }
        
        inline Task* allocate()
        {
            Task* task_ptr = _queue.allocate();
            task_ptr->_num_pending = 1;
			task_ptr->_num_continuations = 0;
			task_ptr->_parent = nullptr;
            return task_ptr;
        }
        
		inline void add_as_child(Task* parent, Task* child)
		{
			if (parent && child)
			{
				child->_parent = parent;
				parent->_num_pending++;
			}
		}

		inline void add_as_continuation(Task* parent, Task* continuation)
		{
			if (parent && continuation)
			{
				if (parent->_num_continuations < MAX_CONTINUATIONS)
				{
					parent->_continuations[parent->_num_continuations] = continuation;
					parent->_num_continuations++;
				}
			}
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
                    run_task(task);
            }
        }
        
        inline void wait_for_one(Task* pending_task)
        {
            while(pending_task->_num_pending > 0)
            {
                Task* task = _queue.pop();
                
                if(task)
                    run_task(task);
            }
        }
        
        inline uint32_t num_logical_threads()
        {
            return _num_logical_threads;
        }
        
        inline uint32_t num_worker_threads()
        {
            return _num_worker_threads;
        }
        
    private:
        inline void initialize_workers()
        {
            // spawn worker threads
            _worker_threads = new WorkerThread[_num_worker_threads];
            
            for (uint32_t i = 0; i < _num_worker_threads; i++)
                _worker_threads[i]._thread = std::thread(&ThreadPool::worker, this, i);
        }

		inline void worker(uint32_t index)
		{
			WorkerThread& worker_thread = _worker_threads[index];

			while (!_shutdown)
			{
				Task* task = _queue.pop();

				if (!task)
				{
					worker_thread._done.notify();
					worker_thread._wakeup.wait();
				}
				else
					run_task(task);
			}
		}

		inline void run_task(Task* task)
		{
			wait_for_children(task);

			task->_function.Invoke(task->_data);

			if (task->_parent)
				task->_parent->_num_pending--;

			// Submit continuation tasks.
			for (uint32_t i = 0; i < task->_num_continuations; i++)
				_queue.push(task->_continuations[i]);

			if (task->_num_pending > 0)
				task->_num_pending--;

			if (_queue._num_pending_tasks > 0)
				_queue._num_pending_tasks--;
		}

		inline void wait_for_children(Task* task)
		{
			while (task->_num_pending > 1)
			{
				Task* wait_task = _queue.pop();

				if (wait_task)
					run_task(wait_task);
			}
		}
        
    private:
        bool					  _shutdown;
        uint32_t				  _num_logical_threads;
        WorkQueue                 _queue;
        WorkerThread*             _worker_threads;
        uint32_t                  _num_worker_threads;
    };

}

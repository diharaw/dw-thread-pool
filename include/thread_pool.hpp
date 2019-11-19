#pragma once

#include <functional>
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

// -----------------------------------------------------------------------------------------------------------------------------------

class Semaphore
{
public:
	Semaphore() : _signal(false)
	{

	}

	inline void notify()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_signal = true;
		_condition.notify_all();
	}

	inline void wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_condition.wait(lock, [&] { return _signal; });
		_signal = false;
	}

private:

	Semaphore(const Semaphore &);
	Semaphore & operator = (const Semaphore &);

	std::mutex              _mutex;
	std::condition_variable _condition;
	bool                    _signal;
};

// -----------------------------------------------------------------------------------------------------------------------------------

    struct Task
    {
        std::function<void(void*)> function;
        char				       data[TASK_SIZE_BYTES];
        std::atomic<uint16_t>      num_pending;
		Task*				       parent;
		std::atomic<uint16_t>      num_continuations;
		Task*				       continuations[MAX_CONTINUATIONS];
    };
    
// -----------------------------------------------------------------------------------------------------------------------------------
    
    template <typename T>
    inline T* task_data(Task* task)
    {
        return (T*)(&task->data[0]);
    }
    
// -----------------------------------------------------------------------------------------------------------------------------------
    
    struct WorkQueue
    {
        std::mutex			  m_critical_section;
        Task				  m_task_pool[MAX_TASKS];
        Task*				  m_task_queue[MAX_TASKS];
        uint32_t			  m_front;
        uint32_t			  m_back;
        uint32_t			  m_num_tasks;
		std::atomic<uint32_t> m_num_pending_tasks;
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        WorkQueue()
        {
            m_num_tasks = 0;
            m_num_pending_tasks = 0;
            m_front = 0;
            m_back = 0;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        ~WorkQueue() {}
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        Task* allocate()
        {
            uint32_t task_index = m_num_tasks++;
            return &m_task_pool[task_index & (MAX_TASKS - 1u)];
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------

        void push(Task* task)
        {
            std::lock_guard<std::mutex> lock(m_critical_section);
            
            m_task_queue[m_back & MASK] = task;
            ++m_back;
            m_num_pending_tasks++;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        Task* pop()
        {
            std::lock_guard<std::mutex> lock(m_critical_section);
            
            const uint32_t job_count = m_back - m_front;
            if (job_count <= 0)
                return nullptr;
            
            --m_back;
            return m_task_queue[m_back & MASK];
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------

        bool has_pending_tasks()
        {
            return (m_num_pending_tasks != 0);
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        bool empty()
        {
            return (m_front == 0 && m_back == 0);
        }
    };
    
// -----------------------------------------------------------------------------------------------------------------------------------
    
    struct WorkerThread
    {
		Semaphore	m_wakeup;
        Semaphore	m_done;
        std::thread m_thread;
        
// -----------------------------------------------------------------------------------------------------------------------------------

        void shutdown()
        {
            wakeup();
            m_thread.join();
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        void wakeup()
        {
            m_wakeup.notify();
        }
    };
    
// -----------------------------------------------------------------------------------------------------------------------------------
    
    class ThreadPool
    {
    public:
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        ThreadPool()
        {
            m_shutdown = false;
            
            // get number of logical threads on CPU
            m_num_logical_threads = std::thread::hardware_concurrency();
            
            m_num_worker_threads = m_num_logical_threads - 1;
            
			initialize_workers();
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        ThreadPool(uint32_t workers)
        {
            m_shutdown = false;
            
            // get number of logical threads on CPU
            m_num_logical_threads = std::thread::hardware_concurrency();
            m_num_worker_threads = workers;
            
			initialize_workers();
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        ~ThreadPool()
        {
			m_shutdown = true;

            for (uint32_t i = 0; i < m_num_worker_threads; i++)
                m_worker_threads[i].shutdown();
            
            delete[] m_worker_threads;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline Task* allocate()
        {
            Task* task_ptr = m_queue.allocate();
            task_ptr->num_pending = 1;
			task_ptr->num_continuations = 0;
			task_ptr->parent = nullptr;
            return task_ptr;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
		inline void add_as_child(Task* parent, Task* child)
		{
			if (parent && child)
			{
				child->parent = parent;
				parent->num_pending++;
			}
		}
        
// -----------------------------------------------------------------------------------------------------------------------------------

		inline void add_as_continuation(Task* parent, Task* continuation)
		{
			if (parent && continuation)
			{
				if (parent->num_continuations < MAX_CONTINUATIONS)
				{
					parent->continuations[parent->num_continuations] = continuation;
					parent->num_continuations++;
				}
			}
		}
        
// -----------------------------------------------------------------------------------------------------------------------------------

        inline void enqueue(Task* task)
        {
            if(task)
            {
                m_queue.push(task);
                
                for(uint32_t i = 0; i < m_num_worker_threads; i++)
                {
                    WorkerThread& thread = m_worker_threads[i];
                    thread.wakeup();
                }
            }
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline void wait_for_all()
        {
            while(m_queue.has_pending_tasks())
            {
                Task* task = m_queue.pop();
                
                if(task)
                    run_task(task);
            }
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline void wait_for_one(Task* pending_task)
        {
            while(pending_task->num_pending > 0)
            {
                Task* task = m_queue.pop();
                
                if(task)
                    run_task(task);
            }
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline uint32_t num_logical_threads()
        {
            return m_num_logical_threads;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline uint32_t num_worker_threads()
        {
            return m_num_worker_threads;
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
    private:
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
        inline void initialize_workers()
        {
            // spawn worker threads
            m_worker_threads = new WorkerThread[m_num_worker_threads];
            
            for (uint32_t i = 0; i < m_num_worker_threads; i++)
                m_worker_threads[i].m_thread = std::thread(&ThreadPool::worker, this, i);
        }
        
// -----------------------------------------------------------------------------------------------------------------------------------

		inline void worker(uint32_t index)
		{
			WorkerThread& worker_thread = m_worker_threads[index];

			while (!m_shutdown)
			{
				Task* task = m_queue.pop();

				if (!task)
				{
					worker_thread.m_done.notify();
					worker_thread.m_wakeup.wait();
				}
				else
					run_task(task);
			}
		}
        
// -----------------------------------------------------------------------------------------------------------------------------------

		inline void run_task(Task* task)
		{
			wait_for_children(task);

			task->function(task->data);

			if (task->parent)
				task->parent->num_pending--;

			// Submit continuation tasks.
			for (uint32_t i = 0; i < task->num_continuations; i++)
				m_queue.push(task->continuations[i]);

			if (task->num_pending > 0)
				task->num_pending--;

			if (m_queue.m_num_pending_tasks > 0)
				m_queue.m_num_pending_tasks--;
		}
        
// -----------------------------------------------------------------------------------------------------------------------------------

		inline void wait_for_children(Task* task)
		{
			while (task->num_pending > 1)
			{
				Task* wait_task = m_queue.pop();

				if (wait_task)
					run_task(wait_task);
			}
		}
        
// -----------------------------------------------------------------------------------------------------------------------------------
        
    private:
        bool					  m_shutdown;
        uint32_t				  m_num_logical_threads;
        WorkQueue                 m_queue;
        WorkerThread*             m_worker_threads;
        uint32_t                  m_num_worker_threads;
    };
} // namespace dw

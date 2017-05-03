#pragma once

#include <thread>
#include <condition_variable>
#include <deque>
#include <cassert>

class Semaphore
{
public:
    
    Semaphore(): _signal(false)
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

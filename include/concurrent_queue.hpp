#pragma once

#include <mutex>
#include <array>
#include <cassert>
#include <stdint.h>

namespace dw
{
    template<typename T, size_t N>
    struct ConcurrentQueue
    {
        uint16_t				 _front;
        uint16_t				 _back;
        uint16_t				 _size;
        std::mutex			 _mutex;
        std::array<T, N> _elements;
    };
    
    namespace concurrent_queue
    {
        template<typename T, size_t N>
        inline void clear(ConcurrentQueue<T, N>& queue)
        {
            std::lock_guard<std::mutex> scoped_lock(queue._mutex);
            queue._front = 0;
            queue._back = 0;
            queue._size = 0;
        }
        
        template<typename T, size_t N>
        inline bool empty(ConcurrentQueue<T, N>& queue)
        {
            std::lock_guard<std::mutex> scoped_lock(queue._mutex);
            return queue._size == 0;
        }
        
        template<typename T, size_t N>
        inline void push(ConcurrentQueue<T,N>& queue, T& element)
        {
            std::lock_guard<std::mutex> scoped_lock(queue._mutex);
            assert(queue._size < N);
            
            queue._elements[queue._front] = element;
            
            if ((queue._front == queue._size - 1) && (queue._back > 0))
                queue._front = 0;
            else
                queue._front++;
            
            queue._size++;
        }
        
        template<typename T, size_t N>
        inline T pop(ConcurrentQueue<T, N>& queue)
        {
            std::lock_guard<std::mutex> scoped_lock(queue._mutex);
            assert(queue._size != 0);
            
            uint16_t index = queue._back;
            
            if (queue._back == queue._size - 1)
                queue._back = 0;
            else
                queue._back++;
            
            queue._size--;
            return queue._elements[index];
        }
    }
}

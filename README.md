# dwThreadPool

A simple, dependency-free, C++ 11 based ThreadPool library.

## Features

* C++ 11
* Header only. Can be easily integrated into any codebase
* No external dependencies
* Implicit load balancing due to global task queue
* No dynamic allocations for the user
* Stefan Reinalter's [Delegates](https://blog.molecular-matters.com/2011/09/19/generic-type-safe-delegates-and-events-in-c/)
* Supports any C++ 11 compiler (Tested on AppleClang 8.0, MSVC 14.0)

### NOTE: This library is still in development. It is in no way production ready, nor is the design final.

## Tutorial

```cpp
#include <thread_pool.hpp>

// create simple struct containing task data.
struct my_task_data
{
  int   foo;
  float bar;
}

// task functions have to conform to this signature. can be free functions or methods.
void my_task(void* args)
{
  // do work here...
}

int main() 
{
    // create thread pool instance. 
    
    // Default constructor creates (N-1) number of workers, with N being the number of hardware threads.
    dw::ThreadPool thread_pool;
    
    // For a custom number of workers, pass in the numver of worker threads as a constructor parameter.
    dw::ThreadPool custom_worker_count = dw::ThreadPool(2);
    
    // create task on the stack. no dynamic allocations required.
    dw::Task task;
  
    // bind task function. uses Stefan Reinalter's Delegate implementation.
    task.Bind<&my_task>();
    
    // useful template function for casting task data to a pointer of your custom task data struct.
    my_task_data* data = dw::task_data<my_task_data>(task);
    
    // fill in task data.
    data->foo = 1;
    data->bar = 2.0f;
    
    // enqueue task into thread pool.
    thread_pool.enqueue(task);
    
    // wait till work is done.
    thread_pool.wait();
    
    return 0;
}
```

## Remotery Screenshot of Example

![alt text](https://github.com/diharaw/dwThreadPool/raw/master/doc/screenshot.png "Remotery Screenshot")

## Building the Example

The example project can be built using the [CMake](https://cmake.org/) build system generator. Plenty of tutorials around for that.

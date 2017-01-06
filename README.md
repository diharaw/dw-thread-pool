# dwThreadPool

## What is it?

A simple, header-only, dependency-free, C++ 11 based ThreadPool library.

## Features

## Sample

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
    // create thread pool instance with number of worker threads and queue size as template parameters.
    dw::ThreadPool<4, 100> thread_pool;
    
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

## Build Instructions



## License

```
Copyright (c) 2016 Dihara Wijetunga

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

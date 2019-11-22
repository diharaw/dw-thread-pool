[![License: MIT](https://img.shields.io/packagist/l/doctrine/orm.svg)](https://opensource.org/licenses/MIT)

# dwThreadPool
A simple, header-only, dependency-free, C++ 11 based ThreadPool library.

## Features

* C++ 11
* Minimal Source Code
* Header-only
* No external dependencies
* Task Grouping/Child Tasks
* Task Continuations
* No dynamic allocations for the user
* Fully cross-platform

## Compilers
* MSVC
* AppleClang

## Basics

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
    
    	// Allocate a new task from the thread pool. No dynamic allocations are done internally
    	dw::Task* task = thread_pool.allocate();
  
    	// Bind task function.
    	task->function = my_task;
    
    	// Useful template function for casting task data to a pointer of your custom task data struct.
    	my_task_data* data = dw::task_data<my_task_data>(task);
    
    	// Fill in task data.
    	data->foo = 1;
    	data->bar = 2.0f;
    
    	// enqueue task into thread pool.
    	thread_pool.enqueue(task);
    
    	// wait till work is done.
    	thread_pool.wait_for_all();
    
    	return 0;
}
```

## Waiting for a specified Task

```cpp
dw::Task* task = thread_pool.allocate();

thread_pool.enqueue(task);

// Busy waits on the calling thread until the specified function is done.
thread_pool.wait_for_one(task);

```

## Check whether a specified Task is done

```cpp
dw::Task* task = thread_pool.allocate();

thread_pool.enqueue(task);

// Returns true is task is done, false if not.
thread_pool.is_done(task);

```

## Task Continuations

```cpp
dw::Task* task1 = thread_pool.allocate();
dw::Task* task2 = thread_pool.allocate();

// Bind data and functions...

// First, add task2 as a continuation of task1.
thread_pool.add_as_continuation(task1, task2);

// Then enqueue the first task into the thread pool. The second task will automatically run.
thread_pool.enqueue(task1);

```

## Task Grouping/Child Tasks
NOTE: Child Tasks here refer to grouping a set of tasks to finish together. It is not meant to express dependencies between tasks. For that, use Task Continuations.

```cpp
dw::Task* parent_task;
dw::Task* child_tasks[10];

// Allocate and bind parent task.
parent_task = thread_pool.allocate();

for(uint32_t i = 0; i < 10; i++)
{
	// Allocate and bind Child tasks.
	child_tasks[i] = tp.allocate();

	// Add child as children of the parent task.
	thread_pool.add_as_child(parent_task, child_tasks[i]);

    	// Immediately enqueue each child task.
	thread_pool.enqueue(child_tasks[i]);
}

// Lastly enqueue parent task
thread_pool.enqueue(parent_task);

// Wait on the parent task to ensure the entire task group is completed
thread_pool.wait_for_one(parent_task);

```

## Remotery Screenshot of Example

![alt text](https://github.com/diharaw/dwThreadPool/raw/master/doc/screenshot.png "Remotery Screenshot")

## Building the Example

The example project can be built using the [CMake](https://cmake.org/) build system generator. Plenty of tutorials around for that.

## License
```
Copyright (c) 2019 Dihara Wijetunga

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

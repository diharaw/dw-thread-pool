#include <iostream>
#include <chrono>

#include <thread_pool.hpp>
#include <Remotery.h>

#define TEST_CASE_1_NUM_TASKS 37
#define TEST_CASE_2_NUM_TASKS 29
#define TEST_CASE_3_NUM_TASKS 1000

#define TEST_CASE_4_ITERATIONS 5
#define TEST_CASE_5_ITERATIONS 5


int num_completed = 0;
std::mutex global_mutex;

void task1_function(void* data)
{
	rmt_BeginCPUSample(task1_function, 0);

	std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::lock_guard<std::mutex> lock(global_mutex);
    num_completed++;
    std::cout << "Num Completed : " << num_completed << "/" << TEST_CASE_1_NUM_TASKS << std::endl;
    
    rmt_EndCPUSample();
}

void task2_function(void* data)
{
	rmt_ScopedCPUSample(task2_function, 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(rand()));
}

void task3_function(void* data)
{
	rmt_ScopedCPUSample(task3_function, 0);

	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test_case_4_t1_function(void* data)
{
    rmt_ScopedCPUSample(Task_1, 0);
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void test_case_4_t2_function(void* data)
{
    rmt_ScopedCPUSample(Task_2, 0);
    
    std::this_thread::sleep_for(std::chrono::seconds(4));
}

void test_case_4_t3_function(void* data)
{
    rmt_ScopedCPUSample(Task_3, 0);
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void test_case_4_t4_function(void* data)
{
    rmt_ScopedCPUSample(Task_4, 0);
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void test_case_1(dw::ThreadPool& tp)
{
    std::cout << "*****************************************" << std::endl;
    std::cout << "TEST CASE 1" << std::endl;
    std::cout << "*****************************************" << std::endl;
    
	dw::Task* task1 = tp.allocate();

	task1->_function.Bind<&task1_function>();

	for (int i = 0; i < TEST_CASE_1_NUM_TASKS; i++)
	{
		tp.enqueue(task1);
	}

	tp.wait_for_all();
}

void test_case_2(dw::ThreadPool& tp)
{
    std::cout << "*****************************************" << std::endl;
    std::cout << "TEST CASE 2" << std::endl;
    std::cout << "*****************************************" << std::endl;
    
	dw::Task* task2 = tp.allocate();

	task2->_function.Bind<&task2_function>();

	for (int i = 0; i < TEST_CASE_2_NUM_TASKS; i++)
	{
		tp.enqueue(task2);
	}

	tp.wait_for_all();
}

void test_case_3(dw::ThreadPool& tp)
{
	dw::Task* task = tp.allocate();

	task->_function.Bind<&task3_function>();

	for (int i = 0; i < TEST_CASE_3_NUM_TASKS; i++)
	{
		tp.enqueue(task);
	}

	tp.wait_for_all();
}

void test_case_4_continuations(dw::ThreadPool& tp)
{
	std::cout << "*****************************************" << std::endl;
	std::cout << "TEST CASE 4 - CONTINUATIONS" << std::endl;
	std::cout << "*****************************************" << std::endl;

    dw::Task* task1;
    dw::Task* task2;
    dw::Task* task3;
    dw::Task* task4;
    dw::Task* task2_cont_1;
	dw::Task* task2_cont_2;
	dw::Task* task2_cont_3;
    
    task1 = tp.allocate();
    task2 = tp.allocate();
    task3 = tp.allocate();
    task4 = tp.allocate();
	task2_cont_1 = tp.allocate();
	task2_cont_2 = tp.allocate();
	task2_cont_3 = tp.allocate();
    
    task1->_function.Bind<&test_case_4_t1_function>();
    task2->_function.Bind<&test_case_4_t2_function>();
    task3->_function.Bind<&test_case_4_t3_function>();
    task4->_function.Bind<&test_case_4_t4_function>();
	task2_cont_1->_function.Bind<&task3_function>();
	task2_cont_2->_function.Bind<&task3_function>();
	task2_cont_3->_function.Bind<&task3_function>();

	tp.add_as_continuation(task2, task2_cont_1);
	tp.add_as_continuation(task2_cont_1, task2_cont_2);
	tp.add_as_continuation(task2_cont_2, task2_cont_3);

    tp.enqueue(task1);
    tp.enqueue(task2);
    tp.enqueue(task3);
    tp.enqueue(task4);
   
    tp.wait_for_all();
}

void test_case_5_child_tasks(dw::ThreadPool& tp)
{
	std::cout << "*****************************************" << std::endl;
	std::cout << "TEST CASE 5 - CHILD TASKS (TASK GROUPING)" << std::endl;
	std::cout << "*****************************************" << std::endl;

	dw::Task* parent_task;
	dw::Task* child_tasks[10];

	parent_task = tp.allocate();
	parent_task->_function.Bind<&test_case_4_t1_function>();

	for(uint32_t i = 0; i < 10; i++)
	{
		child_tasks[i] = tp.allocate();
		child_tasks[i]->_function.Bind<&test_case_4_t2_function>();
		tp.add_as_child(parent_task, child_tasks[i]);
		tp.enqueue(child_tasks[i]);
	}

	tp.enqueue(parent_task);

	tp.wait_for_one(parent_task);
}

struct TestTaskData
{
	int a;
	float b;
};

int main()
{
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

	dw::ThreadPool thread_pool;
    
	for(int i = 0; i < TEST_CASE_4_ITERATIONS; i++)
		test_case_4_continuations(thread_pool);

	for (int i = 0; i < TEST_CASE_5_ITERATIONS; i++)
		test_case_5_child_tasks(thread_pool);

    std::cout << "*****************************************" << std::endl;
	std::cout << "DONE ALL TEST CASES" << std::endl;
    std::cout << "*****************************************" << std::endl;

	int a;
	std::cin >> a;
    
    rmt_DestroyGlobalInstance(rmt);

	return 0;
}

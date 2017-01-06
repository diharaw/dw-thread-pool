#include <iostream>
#include <chrono>

#include <thread_pool.hpp>
#include <Remotery.h>

#define TEST_CASE_1_NUM_TASKS 37
#define TEST_CASE_2_NUM_TASKS 29
#define TEST_CASE_3_NUM_TASKS 1000

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

	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void task3_function(void* data)
{
	rmt_ScopedCPUSample(task3_function, 0);

	std::this_thread::sleep_for(std::chrono::seconds(1 + rand()));
}

void test_case_1(dw::ThreadPool<>& tp)
{
    std::cout << "*****************************************" << std::endl;
    std::cout << "TEST CASE 1" << std::endl;
    std::cout << "*****************************************" << std::endl;
    
	dw::Task task1;

	task1._function.Bind<&task1_function>();

	for (int i = 0; i < TEST_CASE_1_NUM_TASKS; i++)
	{
		tp.enqueue(task1);
	}

	tp.wait();
}

void test_case_2(dw::ThreadPool<>& tp)
{
    std::cout << "*****************************************" << std::endl;
    std::cout << "TEST CASE 2" << std::endl;
    std::cout << "*****************************************" << std::endl;
    
	dw::Task task2;

	task2._function.Bind<&task2_function>();

	for (int i = 0; i < TEST_CASE_2_NUM_TASKS; i++)
	{
		tp.enqueue(task2);
	}

	tp.wait();
}

void test_case_3(dw::ThreadPool<>& tp)
{
	dw::Task task;

	task._function.Bind<&task3_function>();

	for (int i = 0; i < TEST_CASE_3_NUM_TASKS; i++)
	{
		tp.enqueue(task);
	}

	tp.wait();
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

	dw::ThreadPool<> thread_pool;

    test_case_2(thread_pool);
    test_case_1(thread_pool);

    std::cout << "*****************************************" << std::endl;
	std::cout << "DONE ALL TEST CASES" << std::endl;
    std::cout << "*****************************************" << std::endl;

	int a;
	std::cin >> a;
    
    rmt_DestroyGlobalInstance(rmt);

	return 0;
}

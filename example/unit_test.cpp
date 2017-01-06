#include <iostream>
#include <chrono>

#include <thread_pool.hpp>
#include <Remotery.h>

#define TEST_CASE_1_NUM_TASKS 30
#define TEST_CASE_2_NUM_TASKS 1000
#define TEST_CASE_3_NUM_TASKS 1000

void task1_function(void* data)
{
	rmt_ScopedCPUSample(task1_function, 0);

	std::this_thread::sleep_for(std::chrono::seconds(1));
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
	dw::Task task;

	task._function.Bind<&task2_function>();

	for (int i = 0; i < TEST_CASE_2_NUM_TASKS; i++)
	{
		tp.enqueue(task);
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

	test_case_1(thread_pool);

	rmt_DestroyGlobalInstance(rmt);

	std::cout << "DONE ALL TEST CASES" << std::endl;

	int a;
	std::cin >> a;

	return 0;
}

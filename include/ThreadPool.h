#pragma once

#include <atomic>
#include <vector>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <future>
#include <type_traits>
#include <memory>

#include <iostream>

namespace GGChess
{
	class TaskBase
	{
	public:
		virtual ~TaskBase() {};
		inline void operator () () { ExecuteTask(); }
	private:
		virtual void ExecuteTask() = 0;
	};

	template<typename Func, typename ReturnType>
	class Task : public TaskBase
	{
	public:
		Task(Func func) :
			task(std::move(func))
		{}

		std::future<ReturnType> GetFuture() {
			return task.get_future();
		}

		Task(Task&) = delete;
		Task& operator = (Task&) = delete;
	private:
		std::packaged_task<ReturnType()> task;

		void ExecuteTask() override {
			task();
		}
	};

	using TaskPtr = std::shared_ptr<TaskBase>;

	class TaskQueue
	{
	public:
		TaskQueue() : c(), q(), m() {}

		void push(TaskPtr task);
		TaskPtr pop();

	private:
		std::condition_variable c;
		std::queue<TaskPtr> q;
		std::mutex m;
	};

	class ThreadPool
	{
	public:
		ThreadPool();
		~ThreadPool();

		template<typename Func, typename ReturnType>
		std::future<ReturnType> submit(Func f) {
			std::shared_ptr<Task<Func, ReturnType>> taskPtr = std::make_shared<Task<Func, ReturnType>>(f);
			if(!taskPtr)
				std::cout << "task:" << taskPtr << '\n';
			work_queue.push(taskPtr);
			return taskPtr->GetFuture();
		}
	private:
		std::atomic_bool done;
		std::vector<std::thread> threads;
		TaskQueue work_queue;

		void worker_thread();
	};
}

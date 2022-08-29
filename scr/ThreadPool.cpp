#include "ThreadPool.h"

#include <iostream>

namespace GGChess
{
	void TaskQueue::push(TaskPtr task) {
		std::lock_guard<std::mutex> guard(m);
		q.push(task);
		c.notify_one();
	}

	TaskPtr TaskQueue::pop() {
		std::unique_lock<std::mutex> lock(m);

		while (q.empty())
			c.wait(lock);

		TaskPtr task = q.front();
		q.pop();
		return task;
	}


	ThreadPool::ThreadPool() :
		done(false), work_queue(), threads()
	{
		const int available = std::thread::hardware_concurrency();
		try {
			for (int i = 0; i < available / 2; i++)
				threads.push_back(std::thread(&ThreadPool::worker_thread, this));
		}
		catch (...) {
			done = true;
			throw;
		}
	}

	ThreadPool::~ThreadPool() {
		done = true;
	}

	void ThreadPool::worker_thread()
	{
		while (!done) {
			TaskPtr taskPtr = work_queue.pop();
			(*taskPtr)();
		}
	}
}

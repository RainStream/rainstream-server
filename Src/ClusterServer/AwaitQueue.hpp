#pragma once

#include <list>
#include <thread>
#include <cppcoro/task.hpp>
#include <cppcoro/async_auto_reset_event.hpp>
#include <cppcoro/async_mutex.hpp>
#include <cppcoro/sync_wait.hpp>

#ifdef _WIN32
#pragma comment(lib, "Synchronization.lib")
#endif // _WIN32


template <class T>
class AwaitQueue
{
public:
	AwaitQueue()
	{
		thread_ = std::thread([=]()
			{
				cppcoro::task<void> starter = start();
				cppcoro::sync_wait(starter);
			});
		
	}

	int size()
	{
		return this->pendingTasks.size();
	}

	void close()
	{
		if (this->closed)
			return;

		this->closed = true;

		/*for (const pendingTask of this->pendingTasks)
		{
			pendingTask.stopped = true;
			pendingTask.reject(new this->ClosedErrorClass("AwaitQueue closed"));
		}

		 Enpty the pending tasks array.
		this->pendingTasks.length = 0;*/
	}

	void push(cppcoro::task<T>&& task)
	{
		if (this->closed)
			//throw new this->ClosedErrorClass("AwaitQueue closed");
			return;

		this->pendingTasks.push_back(std::move(task));

		event.set();
	}

	void removeTask(int idx)
	{
		/*if (idx == = 0)
		{
			throw new TypeError("cannot remove task with index 0");
		}

		const pendingTask = this->pendingTasks[idx];

		if (!pendingTask)
			return;

		this->pendingTasks.splice(idx, 1);

		pendingTask.reject(
			new this->RemovedTaskErrorClass("task removed from the queue"));*/
	}

	void stop()
	{
		if (this->closed)
			return;

		this->event.set();

		//for (const pendingTask of this->pendingTasks)
		//{
		//	pendingTask.stopped = true;
		//	pendingTask.reject(new this->StoppedErrorClass("AwaitQueue stopped"));
		//}

		//// Enpty the pending tasks array.
		//this->pendingTasks.length = 0;
	}

	void dump()
	{
		
	}

protected:
	cppcoro::task<void> start()
	{
		auto startWaiter = [&]() -> cppcoro::task<>
		{
			while (!this->closed)
			{
				co_await event;

				if (this->closed)
					co_return;

				if (!this->pendingTasks.size())
					continue;

				cppcoro::task<T> task = std::move(this->pendingTasks.front());
				this->pendingTasks.pop_front();

				co_await task;
			}
		};

		co_await startWaiter();
	}

private:
	// Closed flag.
	bool closed = false;

	// Queue of pending tasks.
	std::list<cppcoro::task<T>> pendingTasks;

	cppcoro::async_auto_reset_event event;

	std::thread thread_;
};


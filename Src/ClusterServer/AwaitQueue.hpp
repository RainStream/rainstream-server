#pragma once

#include <list>

#include <Logger.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/single_consumer_event.hpp>
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
		_allWorks = start();
	}

	int size()
	{
		return this->_pendingTasks.size();
	}

	void close()
	{
		if (this->closed)
			return;

		this->closed = true;
	}

	void push(std::function<std::future<T>(void)>&& task)
	{
		if (this->closed)
			return;

		this->_pendingTasks.push_back(std::move(task));

		MSC_DEBUG("push task for size: [%d]", this->_pendingTasks.size());

		this->_event.set();
	}

	void stop()
	{
		if (this->closed)
			return;

		this->closed = true;

		this->_event.set();
	}

protected:
	std::future<void> start()
	{
		while (!this->closed)
		{
			co_await _event;

			if (this->closed)
				co_return;

			if (!this->_pendingTasks.size())
				continue;

			MSC_DEBUG("run task [%d] for size: [%d]", ++index, this->_pendingTasks.size());

			std::function<std::future<T>(void)> task = std::move(this->_pendingTasks.front());
			this->_pendingTasks.pop_front();

			co_await task();
		}

		co_return;
	}

private:
	int index = 0;
	// Closed flag.
	bool closed = false;

	// Queue of pending tasks.
	std::list<std::function<std::future<T>(void)>> _pendingTasks;

	cppcoro::single_consumer_event _event;

	std::future<void> _allWorks;
};


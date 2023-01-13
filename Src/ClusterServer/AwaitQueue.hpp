#pragma once

#include <list>
#include <Logger.h>
//#include <asyncpp/single_consumer_event.hpp>


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

	void push(std::function<async_simple::coro::Lazy<T>(void)>&& task)
	{
		if (this->closed)
			return;

		this->_pendingTasks.push_back(std::move(task));

		//this->_event.set();
	}

	void stop()
	{
		if (this->closed)
			return;

		this->closed = true;

		this->_event.set();
	}

protected:
	async_simple::coro::Lazy<void> start()
	{
		while (!this->closed)
		{
			//co_await _event;

			//_event.reset();

			if (this->closed)
				co_return;

			if (!this->_pendingTasks.size())
				continue;

			while (this->_pendingTasks.size())
			{
				auto task = std::move(this->_pendingTasks.front());
				this->_pendingTasks.pop_front();

				co_await task();
			}
		}

		co_return;
	}

private:
	// Closed flag.
	bool closed = false;

	// Queue of pending tasks.
	std::list<std::function<async_simple::coro::Lazy<T>(void)>> _pendingTasks;

	//cppcoro::single_consumer_event _event;

	async_simple::coro::Lazy<void> _allWorks;
};


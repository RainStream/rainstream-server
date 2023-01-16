#pragma once

#include <list>
#include <Logger.h>
#include <common.h>


template <class T>
class AwaitQueue
{
public:
	AwaitQueue()
	{
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

	void push(std::function<async_simple::coro::Lazy<T>(void)>&& pendingTask)
	{
		if (this->closed)
			return;

		this->_pendingTasks.push_back(std::move(pendingTask));

		// And execute it if this is the only task in the queue.
		if (this->_pendingTasks.size() == 1)
		{
			this->execute().start([=](async_simple::Try<void> Result) {
				if (this->_pendingTasks.size())
				{
					this->execute().start([](async_simple::Try<void> Result) {});
				}
			});
		}
	}

	void stop()
	{
		if (this->closed)
			return;

		this->closed = true;

		this->_event.set();
	}

protected:
	async_simple::coro::Lazy<void> execute()
	{
		if (this->closed)
			co_return;

		if (!this->_pendingTasks.size())
			co_return;

		auto task = std::move(this->_pendingTasks.front());

		co_await task();

		this->_pendingTasks.pop_front();

		co_return;
	}

private:
	// Closed flag.
	bool closed = false;

	// Queue of pending tasks.
	std::list<std::function<async_simple::coro::Lazy<T>(void)> > _pendingTasks;

	//cppcoro::single_consumer_event _event;

	//async_simple::coro::Lazy<void> _allWorks;
};


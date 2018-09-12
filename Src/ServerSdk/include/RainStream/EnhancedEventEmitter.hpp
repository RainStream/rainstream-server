#pragma once

namespace rs
{
	class Logger;

	class EventListener
	{
	public:
		virtual void onEvent(std::string event, Json data) = 0;
	};

	class EnhancedEventEmitter : public EventEmitter
	{
	public:
		EnhancedEventEmitter();

		template <typename ... Arg>
		void safeEmit(const std::string& event, Arg&& ... args)
		{
			try
			{
				this->emit(event, std::forward<Arg>(args)...);
			}
			catch (std::exception error)
			{
				this->_logger->error(
					"safeEmit() | event listener threw an error [event:%s]:%s",
					event.c_str(), error.what());
			}
		}

	protected:
		Logger * _logger = nullptr;
	};
}

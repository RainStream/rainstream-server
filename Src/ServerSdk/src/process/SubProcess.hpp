#pragma once

#include "StringUtils.hpp"

namespace rs
{
	class Socket;
	class Logger;

	class SubProcess : public EventEmitter
	{
	public:
		static SubProcess* spawn(std::string workerPath, AStringVector parameters);

		SubProcess();

		/* Callbacks fired by UV events. */
	public:
		void OnUvReqClosed(int64_t exit_status, int term_signal);

	public:
		void Close(std::string error = "");
		Socket* getSocket();

	private:
		// Allocated by this.
		uv_process_t req{ 0 };
		uv_process_options_t options{ 0 };

		// Others.
		bool closed{ false };

		Socket* socket{ nullptr };

		std::unique_ptr<Logger> logger;
	};

	inline Socket* SubProcess::getSocket()
	{
		return this->socket;
	}
}

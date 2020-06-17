#pragma once

#include <uv.h>
#include "EventEmitter.hpp"
#include "StringUtils.hpp"

namespace rs
{
	class Socket;

	class SubProcess : public EventEmitter
	{
	public:
		static SubProcess* spawn(std::string id, std::string workerPath, AStringVector parameters);

		SubProcess(std::string id);

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

		std::string _id;

		Socket* socket{ nullptr };
	};

	inline Socket* SubProcess::getSocket()
	{
		return this->socket;
	}
}

#ifndef PROTOO_WEBSOCKET_TRANSPORT_HPP
#define PROTOO_WEBSOCKET_TRANSPORT_HPP

#include "common.hpp"

namespace protoo
{
	class WebSocketTransport
	{
		friend class WebSocketServer;
	public:
		class Listener
		{
		public:
			virtual void onMessage(const std::string& message) = 0;
			virtual void onDisconnection(int code, const std::string& message) = 0;
		};
	public:
		explicit WebSocketTransport();
		virtual ~WebSocketTransport();

		void setListener(Listener* listener);

	public:
		void Close(int code = 1000, std::string message = std::string());
		bool closed();
		Defer send(const Json& data);

	protected:
		void setUserData(void* userData);
		void onMessage(const std::string& message);
		void onDisconnection(int code, const std::string& message);

	private:
		void* userData { nullptr };

		Listener* _listener{nullptr};
		// Closed flag.
		bool _closed = false;
	};
}

#endif


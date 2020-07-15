#ifndef PROTOO_WEBSOCKET_SERVER_HPP
#define PROTOO_WEBSOCKET_SERVER_HPP

#include "common.hpp"

namespace uS {
	struct Timer;
}

namespace protoo
{
	class WebSocketClient;

	class WebSocketServer
	{
	public:
		class Lisenter
		{
		public:
			virtual void OnConnected(WebSocketClient* transport) = 0;
		};
	public:
		explicit WebSocketServer(Lisenter* lisenter);
		WebSocketServer& operator=(const WebSocketServer&) = delete;
		WebSocketServer(const WebSocketServer&) = delete;
		virtual	~WebSocketServer();

	public:
		bool Connect(std::string url);

	protected:
		void doConnect();
		void onDisConnected();

	private:
		Lisenter * lisenter{ nullptr };
		// Closed flag.
		bool _closed = false;

		std::string url_;
		uS::Timer *timer_;
	};
}

#endif


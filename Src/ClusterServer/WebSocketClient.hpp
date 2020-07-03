#ifndef PROTOO_WEBSOCKET_TRANSPORT_HPP
#define PROTOO_WEBSOCKET_TRANSPORT_HPP

#include "common.hpp"

namespace protoo
{
	class WebSocketClient
	{
		friend class WebSocketServer;
	public:
		class Listener
		{
		public:
			virtual void onMessage(const std::string& message) = 0;
			virtual void onClosed(int code, const std::string& message) = 0;
		};
	public:
		explicit WebSocketClient(std::string url);
		virtual ~WebSocketClient();

		void setListener(Listener* listener);
		std::string url() const;
		std::string addresss() const;

	public:
		void Close(int code = 1000, std::string message = std::string());
		bool closed();
		void send(const json& data);

	protected:
		void setUserData(void* userData);
		void onMessage(const std::string& message);
		void onClosed(int code, const std::string& message);

	private:
		void* userData { nullptr };

		Listener* _listener{nullptr};
		std::string _url;
		std::string _address;
		// Closed flag.
		bool _closed = false;
	};
}

#endif


#ifndef PROTOO_WEBSOCKET_TRANSPORT_HPP
#define PROTOO_WEBSOCKET_TRANSPORT_HPP

#include "common.hpp"

namespace protoo
{
	class Request;

	class WebSocketClient
	{
		friend class WebSocketServer;
	public:
		class Listener
		{
		public:
			virtual std::future<void> OnRequest(WebSocketClient* transport, Request* request) = 0;
			virtual void OnClosed(int code, const std::string& message) = 0;
		};
	public:
		explicit WebSocketClient(std::string url);
		virtual ~WebSocketClient();

		void setListener(Listener* listener);
		std::string url() const;
		std::string addresss() const;

		void request(std::string method, const json& data = json::object());

	public:
		void Close(int code = 1000, std::string message = std::string());
		bool closed();
		void Send(const json& data);

	protected:
		void setUserData(void* userData);
		void onMessage(const std::string& message);
		void OnClosed(int code, const std::string& message);

	protected:
		void _handleRequest(json& jsonRequest);
		void _handleResponse(json& response);
		void _handleNotification(json& notification);

	private:
		void* userData { nullptr };

		Listener* _listener{nullptr};
		std::string _url;
		std::string _address;
		// Closed flag.
		bool _closed = false;

		uint32_t _nextId;
	};
}

#endif


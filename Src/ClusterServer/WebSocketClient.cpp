#define MS_CLASS "WebSocketClient"

#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <uWS/uWS.h>

namespace protoo
{
	WebSocketClient::WebSocketClient(std::string url)
		: _url(url)
	{
		
	}

	WebSocketClient::~WebSocketClient()
	{

	}

	void WebSocketClient::Close(int code, std::string message)
	{
		if (_closed)
		{
			return;
		}

		_closed = true;

		if (userData)
		{
			uWS::WebSocket<uWS::SERVER>* ws = static_cast <uWS::WebSocket<uWS::SERVER>*>(userData);
			ws->close(code, message.c_str());
		}
	}

	Defer WebSocketClient::send(const Json& data)
	{
		if (this->_closed)
			return promise::reject("transport closed");

		std::string message = data.dump();

		LOG(INFO) << "send Message to 0x%02x says : %s\n", userData, message.c_str();

		try
		{
			uWS::WebSocket<uWS::SERVER>* ws = static_cast <uWS::WebSocket<uWS::SERVER>*>(userData);
			ws->send(message.c_str());

			return promise::resolve();
		}
		catch (std::exception error)
		{
			return promise::reject(std::string(error.what()));
		}
	}

	bool WebSocketClient::closed()
	{
		return this->_closed;
	}

	void WebSocketClient::setListener(Listener* listener)
	{
		_listener = listener;
	}

	std::string WebSocketClient::url() const
	{
		return _url;
	}

	void WebSocketClient::setUserData(void* userData)
	{
		this->userData = userData;
	}

	void WebSocketClient::onMessage(const std::string& message)
	{
		LOG(INFO) << "recv Message from 0x%02x says : %s\n", userData, message.c_str();

		if (_listener)
		{
			_listener->onMessage(message);
		}
	}

	void WebSocketClient::onClosed(int code, const std::string& message)
	{
		if (this->_listener)
		{
			_listener->onClosed(code, message);
		}
	}
}

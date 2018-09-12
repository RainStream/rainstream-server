#define MS_CLASS "protoo/WebSocketTransport"

#include "protoo/WebSocketTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

#include <uWS/uWS.h>

namespace protoo
{
	WebSocketTransport::WebSocketTransport()
	{
		
	}

	WebSocketTransport::~WebSocketTransport()
	{

	}

	void WebSocketTransport::Close(int code, std::string message)
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

	Defer WebSocketTransport::send(const Json& data)
	{
		if (this->_closed)
			return promise::reject("transport closed");

		std::string message = data.dump();

		MS_INFO(RED,"send Message to 0x%02x says : %s\n", userData, message.c_str());

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

	bool WebSocketTransport::closed()
	{
		return this->_closed;
	}

	void WebSocketTransport::setListener(Listener* listener)
	{
		_listener = listener;
	}

	void WebSocketTransport::setUserData(void* userData)
	{
		this->userData = userData;
	}

	void WebSocketTransport::onMessage(const std::string& message)
	{
		MS_INFO(GREEN, "recv Message from 0x%02x says : %s\n", userData, message.c_str());

		if (_listener)
		{
			_listener->onMessage(message);
		}
	}

	void WebSocketTransport::onDisconnection(int code, const std::string& message)
	{
		if (this->_listener)
		{
			_listener->onDisconnection(code, message);
		}
	}
}

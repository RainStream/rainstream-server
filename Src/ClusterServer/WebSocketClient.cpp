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

		DLOG(INFO) << "send Message to " << userData << " says :" << message;

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

	std::string WebSocketClient::addresss() const
	{
		return _address;
	}

	void WebSocketClient::setUserData(void* userData)
	{
		this->userData = userData;

		uWS::WebSocket<uWS::SERVER>* ws = static_cast <uWS::WebSocket<uWS::SERVER>*>(userData);
		_address = ws->getAddress().address;

		LOG(INFO) << "WebSocketClient connected with [IP:" << _address << "]";
	}

	void WebSocketClient::onMessage(const std::string& message)
	{
		DLOG(INFO) << "recv Message from " << userData << " says :" << message;

		if (_listener)
		{
			_listener->onMessage(message);
		}
	}

	void WebSocketClient::onClosed(int code, const std::string& message)
	{
		LOG(INFO) << "WebSocketClient disconnected with [IP:" << _address << "]";
		if (this->_listener)
		{
			_listener->onClosed(code, message);
		}
	}
}

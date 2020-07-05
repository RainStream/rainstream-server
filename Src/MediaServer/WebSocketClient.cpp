#define MSC_CLASS "WebSocketClient"

#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <errors.hpp>

#include <uWS.h>

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

	void WebSocketClient::send(const json& data)
	{
		if (this->_closed)
			MSC_THROW_ERROR("transport closed");

		std::string message = data.dump();

		try
		{
			uWS::WebSocket<uWS::SERVER>* ws = static_cast <uWS::WebSocket<uWS::SERVER>*>(userData);
			ws->send(message.c_str());
		}
		catch (std::exception& error)
		{
			MSC_ERROR("send() failed:%s", error.what());

			throw error;
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

		//LOG(INFO) << "WebSocketClient connected with [IP:" << _address << "]";
	}

	void WebSocketClient::onMessage(const std::string& message)
	{
		//DLOG(INFO) << "recv Message from " << userData << " says :" << message;

		if (_listener)
		{
			_listener->onMessage(message);
		}
	}

	void WebSocketClient::onClosed(int code, const std::string& message)
	{
		//LOG(INFO) << "WebSocketClient disconnected with [IP:" << _address << "]";
		if (this->_listener)
		{
			_listener->onClosed(code, message);
		}
	}
}

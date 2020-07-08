#define MSC_CLASS "WebSocketClient"

#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <errors.hpp>
#include "Request.hpp"
#include <uWS.h>

namespace protoo
{
	WebSocketClient::WebSocketClient(std::string url)
		: _url(url)
		, _nextId(0)
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
			uWS::WebSocket<uWS::CLIENT>* ws = static_cast <uWS::WebSocket<uWS::CLIENT>*>(userData);
			ws->close(code, message.c_str());
		}
	}

	void WebSocketClient::Send(const json& data)
	{
		if (this->_closed)
			MSC_THROW_ERROR("transport closed");

		std::string message = data.dump();

		MSC_DEBUG("\r\n\r\nSend:%s\r\n\r\n", message.c_str());

		try
		{
			uWS::WebSocket<uWS::CLIENT>* ws = static_cast <uWS::WebSocket<uWS::CLIENT>*>(userData);
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

	void WebSocketClient::request(std::string method, const json& data)
	{
		this->_nextId < 4294967295 ? ++this->_nextId : (this->_nextId = 1);

		uint32_t id = this->_nextId;

		MSC_DEBUG("request() [method \"%s\", id: \"%d\"]", method.c_str(), id);

		if (this->_closed)
			throw new InvalidStateError("Channel closed");

		json request = {
			{ "request",true },
			{ "id",id },
			{ "method", method },
			{ "data", data }
		};

		this->Send(request);
	}

	void WebSocketClient::setUserData(void* userData)
	{
		this->userData = userData;

		uWS::WebSocket<uWS::CLIENT>* ws = static_cast <uWS::WebSocket<uWS::CLIENT>*>(userData);
		_address = ws->getAddress().address;
	}

	void WebSocketClient::onMessage(const std::string& message)
	{
		MSC_DEBUG("\r\n\r\nonMessage:%s\r\n\r\n", message.c_str());

		json data = json::parse(message);

		if (data.value("request", false))
		{
			this->_handleRequest(data);
		}
		else if (data.value("response", false))
		{
			this->_handleResponse(data);
		}
		else if (data.count("notification"))
		{
			this->_handleNotification(data);
		}
	}

	void WebSocketClient::OnClosed(int code, const std::string& message)
	{
		if (this->_listener)
		{
			_listener->OnClosed(code, message);
		}
	}

	void WebSocketClient::_handleRequest(json& jsonRequest)
	{
 		try
 		{
 			Request* request = new Request(this, jsonRequest);
 
 			this->_listener->OnRequest(this, request);
 		}
 		catch (const std::exception&)
 		{
 
 		}
	}

	void WebSocketClient::_handleResponse(json& response)
	{
		uint32_t id = response["id"].get<uint32_t>();

// 		if (!this->_sents.count(id))
// 		{
// 			MSC_ERROR("received response does not match any sent request [id:%d]", id);
// 
// 			return;
// 		}
// 
// 		std::promise<json> sent = std::move(this->_sents[id]);
// 		this->_sents.erase(id);
// 
// 		if (response.count("ok") && response["ok"].get<bool>())
// 		{
// 			sent.set_value(response["data"]);
// 		}
// 		else
// 		{
// 			sent.set_exception(std::make_exception_ptr(Error(response["errorReason"])));
// 		}
	}

	void WebSocketClient::_handleNotification(json& notification)
	{
		//this->listener->OnPeerNotify(this, notification);
	}
}

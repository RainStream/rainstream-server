#define MSC_CLASS "Peer"

#include "Peer.hpp"
#include <errors.hpp>
#include "Message.hpp"
#include "Request.hpp"
#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"


namespace protoo
{
	Peer::Peer(std::string peerName, protoo::WebSocketClient* transport, Listener* listener)
		: _peerName(peerName)
		, _transport(transport)
		, _listener(listener)
	{
		_handleTransport();
	}


	Peer::~Peer()
	{

	}

	std::string Peer::id()
	{
		return _peerName;
	}

	void Peer::close()
	{
		MSC_DEBUG("close()");

		if (this->_closed)
			return;

		this->_closed = true;

		// Close transport.
// 		this->_transport->close();

		// Close every pending request handler.
// 		for (auto handler : this->_requestHandlers)
// 		{
// 			handler.second.reject();
// 		}

		// Emit 'close' event.
		this->_listener->OnPeerClose(this);
	}

	void Peer::Send(const json& message)
	{
		try
		{
			this->_transport->send(message);
		}
		catch (const std::exception&)
		{

		}
	}

	void Peer::notify(std::string method, const json& data)
	{
		json notification = Message::createNotification(method, data);

		MSC_DEBUG("notify() [method:%s]", method.c_str());

		// This may throw.
		this->Send(notification);
	}

	task_t<json> Peer::request(std::string method, const json& data)
	{
		json request = Message::createRequest(method, data);

		uint32_t id = request["id"];

		MSC_DEBUG("request() [method:%s, id:%d]", method.c_str(), id);		

		promise_t<json> promise;
		this->_sents.insert(std::make_pair(id, std::move(promise)));

		this->Send(request);

		return this->_sents[id].get_return_object();
	}

	void Peer::onMessage(const std::string& message)
	{
		json data = json::parse(message);

		if (data.contains("request") && data["request"].is_boolean() && data.value("request", false))
		{
			this->_handleRequest(data);
		}
		else if (data.contains("response") && data["response"].is_boolean() && data.value("response", false))
		{
			this->_handleResponse(data);
		}
	}

	void Peer::onClosed(int code, const std::string& message)
	{
		this->close();
	}

	void Peer::_handleTransport()
	{
		if (this->_transport->closed())
		{
			this->_closed = true;
			this->close();

			return;
		}

		_transport->setListener(this);
	}

	void Peer::_handleRequest(json& jsonRequest)
	{
		try
		{
			Request* request = new Request(this, jsonRequest);

			this->_listener->OnPeerRequest(this, request);
		}
		catch (const std::exception&)
		{

		}
	}

	void Peer::_handleResponse(json& response)
	{
		uint32_t id = response["id"];

		if (!this->_sents.contains(id))
		{
			MSC_ERROR("received response does not match any sent request [id:%d]", id);

			return;
		}

		promise_t<json> sent = std::move(this->_sents[id]);
		this->_sents.erase(id);

		if (response.count("ok") && response["ok"].get<bool>())
		{
			sent.return_value(response["data"]);
		}
		else
		{
			//sent.set_exception(std::make_exception_ptr(Error(response["errorReason"])));
			sent.unhandled_exception();
		}
	}
}

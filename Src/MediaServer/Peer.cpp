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
		, listener(listener)
	{
		//_handleTransport();
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
		//logger.debug('close()');

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
		this->listener->OnPeerClose(this);
	}

// 	void Peer::Accept(uint32_t id, json& data)
// 	{
// 		auto it = _requests.find(id);
// 		if (it != _requests.end())
// 		{
// 			json request = it->second;
// 			json response = Message::createSuccessResponse(request, data);
// 
// 			this->_transport->send(response)
// 			.fail([=](std::string error)
// 			{
// 				// 				logger->warn(
// 				// 					"accept() failed, response could not be sent: %s", error.c_str());
// 			});
// 
// 			_requests.erase(it);
// 		}
//	}

// 	void Peer::Reject(uint32_t id, uint32_t code, const std::string& errorReason)
// 	{
// 		auto it = _requests.find(id);
// 		if (it != _requests.end())
// 		{
// 			json request = it->second;
// 
// 			json response = Message::createErrorResponse(request, code, errorReason);
// 
// 			this->_transport->send(response)
// 			.fail([=](std::string error)
// 			{
// 				// 				logger.warn(
// 				// 					"reject() failed, response could not be sent: %s", error.c_str());
// 			});
// 
// 			_requests.erase(it);
// 		}
//	}

// 	Defer Peer::send(std::string method, json data)
// 	{
// 		json request = Message::createRequest(method, data);
// 
// 		uint32_t id = request["id"].get<uint32_t>();
// 
// 		return this->_transport->send(request)
// 			.then([=]()
// 		{
// 			return newPromise([=](Defer d) {
// 				this->_requestHandlers.insert(std::make_pair(id, d));
// 			});
// 
// 			//  			return new Promise((pResolve, pReject) = >
// 			//  			{
// 			//  				const handler =
// 			//  				{
// 			//  				resolve: (data2) = >
// 			//  				{
// 			//  					if (!this->_requestHandlers.delete(request.id))
// 			//  						return;
// 			//  
// 			//  					clearTimeout(handler.timer);
// 			//  					pResolve(data2);
// 			//  				},
// 			//  
// 			//  					reject : (error) = >
// 			//  				{
// 			//  					if (!this->_requestHandlers.delete(request.id))
// 			//  						return;
// 			//  
// 			//  					clearTimeout(handler.timer);
// 			//  					pReject(error);
// 			//  				},
// 			//  
// 			//  					timer : setTimeout(() = >
// 			//  				{
// 			//  					if (!this->_requestHandlers.delete(request.id))
// 			//  						return;
// 			//  
// 			//  					pReject(Error("request timeout"));
// 			//  				}, REQUEST_TIMEOUT),
// 			//  
// 			//  					close : () = >
// 			//  				{
// 			//  					clearTimeout(handler.timer);
// 			//  					pReject(Error("peer closed"));
// 			//  				}
// 			//  				};
// 			//  
// 			//  				// Add handler stuff to the Map.
// 			//  				this->_requestHandlers.set(request.id, handler);
// 			// 			});
// 		});
// 	}
// 
// 	Defer Peer::notify(std::string method, json data)
// 	{
// 		json notification = Message::createNotification(method, data);
// 
// 		return this->_transport->send(notification);
// 	}

	void Peer::Send(const json& message)
	{
		try
		{
			this->_transport->Send(message);
		}
		catch (const std::exception&)
		{

		}
	}

	void Peer::notify(std::string method, json& data)
	{
		json notification = Message::createNotification(method, data);

		MSC_DEBUG("notify() [method:%s]", method.c_str());

		// This may throw.
		this->_transport->Send(notification);
	}

	std::future<json> Peer::request(std::string method, json& data)
	{
		json request = Message::createRequest(method, data);

		uint32_t id = request["id"];

		MSC_DEBUG("request() [method:%s, id:%d]", method.c_str(), id);

		this->Send(request);

		std::promise<json> promise;
		this->_sents.insert(std::make_pair(id, std::move(promise)));

		return this->_sents[id].get_future();
	}

	void Peer::onMessage(const std::string& message)
	{
		json data = json::parse(message);
// 
// 		if (data.value("request",false))
// 		{
// 			this->_handleRequest(data);
// 		}
// 		else if (data.value("response", false))
// 		{
// 			this->_handleResponse(data);
// 		}
// 		else if (data.count("notification"))
// 		{
// 			this->_handleNotification(data);
// 		}
	}

	void Peer::OnClosed(int code, const std::string& message)
	{
		this->close();
	}

// 	void Peer::_handleTransport()
// 	{
// 		if (this->_transport->closed())
// 		{
// 			this->_closed = true;
// 			this->close();
// 
// 			return;
// 		}
// 
// 		_transport->setListener(this);
// 
// 	}
// 
// 
// 	void Peer::_handleRequest(json& jsonRequest)
// 	{
// 		try
// 		{
// 			Request* request = new Request(this, jsonRequest);
// 
// 			this->listener->OnPeerRequest(this, request);
// 		}
// 		catch (const std::exception&)
// 		{
// 
// 		}
// 	}
// 
// 	void Peer::_handleResponse(json& response)
// 	{
// 		uint32_t id = response["id"].get<uint32_t>();
// 
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
// 			sent.set_exception(std::make_exception_ptr(Error(response["errorReason"].get<std::string>())));
// 		}
// 	}
// 
// 	void Peer::_handleNotification(json& notification)
// 	{
// 		this->listener->OnPeerNotify(this, notification);
// 	}
}

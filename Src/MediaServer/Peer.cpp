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
	Peer::Peer(std::string peerId, std::string roomId, protoo::WebSocketClient* transport, Listener* listener)
		: _peerId(peerId)
		, _roomId(roomId)
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
		return _peerId;
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

		delete this;
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

	void Peer::notify(std::string method, const json& data)
	{
		json notification = Message::createNotification(method, data);
		notification["roomId"] = _roomId;
		notification["peerId"] = _peerId;

		MSC_DEBUG("notify() [method:%s]", method.c_str());

		// This may throw.
		this->_transport->Send(notification);
	}

	std::future<json> Peer::request(std::string method, json& data)
	{
		return this->_transport->request(_peerId, _roomId, method, data);
	}

	void Peer::OnClosed(int code, const std::string& message)
	{
		this->close();
	}
}

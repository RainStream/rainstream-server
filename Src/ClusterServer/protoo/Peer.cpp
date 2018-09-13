#define MS_CLASS "protoo/Peer"

#include "protoo/Peer.hpp"
#include "protoo/Request.hpp"
#include "protoo/Message.hpp"
#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RainStreamError.hpp"

namespace protoo
{
	Peer::Peer(std::string peerName, protoo::WebSocketClient* transport, Listener* listener)
		: _peerName(peerName)
		, _transport(transport)
		, listener(listener)
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
		//logger.debug('close()');

		if (this->_closed)
			return;

		this->_closed = true;

		// Close transport.
// 		this->_transport->close();

		// Close every pending request handler.
		for (auto handler : this->_requestHandlers)
		{
			handler.second.reject();
		}

		// Emit 'close' event.
		this->listener->OnPeerClose(this);
	}

	rs::Peer* Peer::mediaPeer()
	{
		return _peer;
	}

	void Peer::setMediaPeer(rs::Peer* peer)
	{
		_peer = peer;
	}

	void Peer::Send(Json data)
	{

	}

	Defer Peer::send(std::string method, Json data)
	{
		Json request = Message::requestFactory(method, data);

		uint32_t id = request["id"].get<uint32_t>();

		return this->_transport->send(request)
			.then([=]()
		{
			return newPromise([=](Defer d) {
				this->_requestHandlers.insert(std::make_pair(id, d));
			});

			//  			return new Promise((pResolve, pReject) = >
			//  			{
			//  				const handler =
			//  				{
			//  				resolve: (data2) = >
			//  				{
			//  					if (!this->_requestHandlers.delete(request.id))
			//  						return;
			//  
			//  					clearTimeout(handler.timer);
			//  					pResolve(data2);
			//  				},
			//  
			//  					reject : (error) = >
			//  				{
			//  					if (!this->_requestHandlers.delete(request.id))
			//  						return;
			//  
			//  					clearTimeout(handler.timer);
			//  					pReject(error);
			//  				},
			//  
			//  					timer : setTimeout(() = >
			//  				{
			//  					if (!this->_requestHandlers.delete(request.id))
			//  						return;
			//  
			//  					pReject(Error("request timeout"));
			//  				}, REQUEST_TIMEOUT),
			//  
			//  					close : () = >
			//  				{
			//  					clearTimeout(handler.timer);
			//  					pReject(Error("peer closed"));
			//  				}
			//  				};
			//  
			//  				// Add handler stuff to the Map.
			//  				this->_requestHandlers.set(request.id, handler);
			// 			});
		});
	}

	Defer Peer::notify(std::string method, Json data)
	{
		Json notification = Message::notificationFactory(method, data);

		return this->_transport->send(notification);
	}

	void Peer::onMessage(const std::string& message)
	{
		Json data = Json::parse(message);

		if (data.value("request",false))
		{
			this->_handleRequest(data);
		}
		else if (data.count("response"))
		{
			this->_handleResponse(data);
		}
		else if (data.count("notification"))
		{
			this->_handleNotification(data);
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


	void Peer::_handleRequest(Json json)
	{
		auto accept = [=](Json data = Json::object())
		{
			Json response = Message::successResponseFactory(json, data);

			this->_transport->send(response)
				.fail([=](std::string error)
			{
				// 				logger->warn(
				// 					"accept() failed, response could not be sent: %s", error.c_str());
			});

		};

		auto reject = [=](int errorCode, std::string errorReason)
		{
			/*
			if (errorCode instanceof Error)
			{
				errorReason = errorCode.ToString().c_str();
				errorCode = 500;
			}
			else if (typeof errorCode == = "number" && errorReason instanceof Error)
			{
				errorReason = errorReason.ToString().c_str();
			}
			*/

			Json response = Message::errorResponseFactory(json, errorCode, errorReason);

			this->_transport->send(response)
				.fail([=](std::string error)
			{
				// 				logger.warn(
				// 					"reject() failed, response could not be sent: %s", error.c_str());
			});

		};

		Request* request = nullptr;

		try
		{
			request = new Request(this, json);
			request->accept = accept;
			request->reject = reject;
		}
		catch (const RainStreamError& error)
		{
			MS_ERROR_STD("discarding wrong Peer request %s", error.what());
		}

		if (request != nullptr)
		{
			// Notify the listener.
			this->listener->OnPeerRequest(this, request);

			// Delete the Request.
			delete request;
		}
	}

	void Peer::_handleResponse(Json response)
	{
		uint32_t id = response["id"].get<uint32_t>();

		if (!this->_requestHandlers.count(id))
		{
			//logger.error("received response does not match any sent request");

			return;
		}

		auto handler = this->_requestHandlers[id];


		if (response.count("ok") && response["ok"].get<bool>())
		{
			handler.resolve(response["data"]);
		}
		else
		{
			response["errorCode"];
			handler.reject(rs::Error(response["reason"].get<std::string>()));
		}
	}

	void Peer::_handleNotification(Json notification)
	{
		this->listener->OnNotification(this, notification);
	}
}
